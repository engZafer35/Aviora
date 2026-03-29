# Aviora ZMeter Gateway — Protocol Layer (Professional Guide)

> **JSON-over-TCP between device and head-end: push channel for outbound events, pull channel for inbound commands.**

This document describes the protocol stack under `Application/AppZMeterGw/Services/Protocol`: responsibilities, message types, state machines, and integration with meter jobs. Audience: firmware engineers, integration, and technical documentation.

---

## Executive Summary

| Topic | Summary |
|--------|---------|
| **Transport** | TCP; all payloads are JSON text |
| **Channels** | **Push** — device connects to server; **Pull** — server connects to device’s local port |
| **Core modules** | `AppTcpConnManager`, `AppMeterOperations`, `AppProtocolZD` |
| **Persistence** | Registration flag, optional server address override, meter/directive files on FS |
| **Test tool** | Python Tkinter server in `Protocol/server/server.py` |

---

## 1. Repository layout

```mermaid
flowchart LR
  subgraph inc["inc/"]
    H1[AppTcpConnManager.h]
    H2[AppMeterOperations.h]
    H3[AppProtocolZD.h]
  end
  subgraph src["src/"]
    C1[AppTcpConnManager.c]
    C2[AppMeterOperations.c]
    C3[AppProtocolZD.c]
  end
  subgraph tools["server/"]
    PY[server.py]
  end
  H1 --> C1
  H2 --> C2
  H3 --> C3
  C3 --> C1
  C3 --> C2
```

---

## 2. Logical architecture

```mermaid
flowchart TB
  subgraph HeadEnd["Head-end / test server"]
    SP[Listen: push port]
    PC[Client to device pull port]
  end

  subgraph Device["Gateway device"]
    PZD[AppProtocolZD state machine]
    TCP[AppTcpConnManager]
    MTR[AppMeterOperations worker]
    LOG[AppLogRecorder]
    SW[AppSWUpdate / FTP]
    FS[(File system)]

    PZD --> TCP
    PZD --> MTR
    PZD --> LOG
    PZD --> SW
    MTR --> FS
  end

  TCP -->|outbound JSON| SP
  SP -->|inbound JSON| TCP
  PC -->|commands JSON| TCP
  TCP -->|IncomingMsngCb_t| PZD
```

---

## 3. Push vs pull (conceptual)

```mermaid
flowchart LR
  subgraph Push["Push (device → server)"]
    D1[Device opens TCP] --> S1[Server accepts]
    D1 --> M1[ident / alive / data]
  end

  subgraph Pull["Pull (server → device)"]
    S2[Server opens TCP] --> D2[Device accept on local port]
    S2 --> M2[log / setting / requests]
  end
```

| Channel | TCP direction | Typical `function` values |
|---------|----------------|---------------------------|
| Push | Device is client | `ident`, `alive`, `readout`, `loadprofile` (data) |
| Pull | Device is server | `log`, `setting`, `fwUpdate`, `readout` (request), `loadprofile`, `directiveList`, `directiveAdd`, `directiveDelete` |

Every JSON message includes a **device header**:

```json
"device": { "flag": "AVI", "serialNumber": "0123456789ABCDE" }
```

---

## 4. AppTcpConnManager — connection lifecycle

The TCP thread multiplexes pull listener, pull client sockets, and push client with `select()`.

`appTcpConnManagerStart(serverIP, serverPort, pullPort, incomingMsngCb)` stores the callback **`IncomingMsngCb_t`**: `void (*IncomingMsngCb_t)(const char *channel, const char *data, unsigned int dataLength)`. On each receive, the thread calls `incomingMsngCb` with channel `"push"` or `"pull"` (`PUSH_TCP_SOCK_NAME` / `PULL_TCP_SOCK_NAME`). **AppProtocolZD** passes `appProtocolZDPutIncomingMessage`; any other protocol module can register its own handler the same way.

```mermaid
flowchart TD
  START["appTcpConnManagerStart(..., incomingMsngCb)"] --> THREAD[tcpConnectionThread loop]
  THREAD --> PULL_CREATE[taskPullSocCreat → listen on pullPort]
  THREAD --> PUSH_REQ[taskConnectPush → connect push socket]
  THREAD --> SEL[select on fds]
  SEL -->|pull listen readable| ACC[accept client]
  SEL -->|pull client readable| RECV_PULL["recv → incomingMsngCb pull"]
  SEL -->|push readable| RECV_PUSH["recv → incomingMsngCb push"]
  SEL -->|push writable| HANDSHAKE[complete non-blocking connect]
```

Non-blocking connect is used for push until established, then blocking mode for send/receive.

---

## 5. AppMeterOperations — data paths

```mermaid
flowchart TB
  subgraph Registry["Meter registry"]
    ML[(meterList)]
    REG[(reg/serial)]
  end

  subgraph Directives["Directives"]
    DIR[(directive/*.json)]
    IDX[(directive.idx)]
  end

  subgraph Jobs["Async jobs"]
    Q[Job queue]
    W[Worker task]
    OUT[(meterDataOut/ task files)]
  end

  ML --> REG
  DIR --> IDX
  Q --> W --> OUT
```

Readout and load profile jobs write:

- `<taskId>_meterID.txt`
- `<taskId>_readout.txt` or `<taskId>_payload.txt`

Protocol reads these files after the callback and sends them on **push**, then deletes the files.

---

## 6. AppProtocolZD — registration and main loop (state view)

```mermaid
stateDiagram-v2
  [*] --> Init: Start task
  Init --> Unregistered: Not persisted OR not accepted
  Unregistered --> Unregistered: ident retry every ~30s
  Unregistered --> Registered: ident + register true
  Registered --> Registered: alive every 5 min
  Registered --> Registered: Handle pull JSON
  Registered --> Registered: Push meter data when job done
```

---

## 7. Sequence: first registration (ident)

```mermaid
sequenceDiagram
  participant D as Device
  participant S as Server
  Note over D,S: Push socket
  D->>S: ident + device info + pullIP/pullPort + registered false
  S->>D: ident + response.register true
  D->>D: persist registered flag
```

If the device was already registered (persisted), it may send `registered: true` in the ident payload per product rules.

---

## 8. Sequence: alive

```mermaid
sequenceDiagram
  participant D as Device
  participant S as Server
  D->>S: alive + deviceDate
  S->>D: ack (optional)
  Note over D: If no ack, next period still sends alive
```

---

## 9. Sequence: log request (pull, may stream)

```mermaid
sequenceDiagram
  participant S as Server
  participant D as Device pull socket
  S->>D: function log
  D->>S: log packet 1 + packetStream true
  D->>S: log packet N + packetStream false
  Note over D: Or single NO-LOG
```

Packets are capped around **1024 bytes** JSON total; application splits payload into chunks with `packetNum` / `packetStream`.

---

## 10. Sequence: setting (server + meters)

```mermaid
sequenceDiagram
  participant S as Server
  participant D as Device
  S->>D: setting + Server ip/port + meters[]
  D->>D: apply + optional TcpConn restart
  D->>S: ack or nack
```

---

## 11. Sequence: readout (pull request → push data)

```mermaid
sequenceDiagram
  participant S as Server
  participant Pull as Device pull
  participant MO as MeterOperations
  participant Push as Device push

  S->>Pull: readout + request
  Pull->>S: ack (or nack)
  Note over MO: Async job runs
  MO->>MO: write task files
  MO-->>Protocol: callback SUCCESS
  Push->>S: readout packets + data
  S->>Push: ack
  Note over Protocol: Delete task files regardless of ack
```

---

## 12. Sequence: firmware update

```mermaid
sequenceDiagram
  participant S as Server
  participant D as Device
  S->>D: fwUpdate + ftp URL
  D->>S: ack
  D->>D: AppSwUpdateInit → FTP download
```

---

## 13. Directive management (pull)

```mermaid
flowchart LR
  A[directiveList] --> B[Multiple responses with packetStream]
  C[directiveAdd] --> D[ack/nack]
  E[directiveDelete] --> F[ack/nack; * deletes all]
```

---

## 14. JSON framing on TCP

Multiple JSON objects may arrive in one `recv()`; the implementation buffers and parses complete objects (e.g. `JSONDecoder.raw_decode` on PC, equivalent logic on device).

```mermaid
flowchart LR
  BUF[Byte buffer] --> PARSE{Complete JSON?}
  PARSE -->|yes| MSG[Dispatch by function]
  PARSE -->|no| MORE[Read more bytes]
  MORE --> BUF
```

---

## 15. Python test server (developer tool)

```mermaid
flowchart TD
  UI[Tkinter UI] --> LISTEN[Listen push port]
  LISTEN --> DEV[Device connects]
  DEV --> IDENT[Parse ident → fill pull IP/port]
  IDENT --> BTN[Enable pull buttons]
  BTN --> CMD[User sends command]
  CMD --> TCP_OUT[TCP client to pullIP:pullPort]
  TCP_OUT --> LOG[Log panel + file]
```

---

## 15a. Component dependency (C4-style)

```mermaid
flowchart TB
  subgraph Application["Application"]
    PZD[AppProtocolZD]
  end
  subgraph Services["Services"]
    TCP[AppTcpConnManager]
    MTR[AppMeterOperations]
    TIME[AppTimeService]
    LOG[AppLogRecorder]
    SWU[AppSWUpdate]
  end
  subgraph Lib["Libraries"]
    ZJ[ZDJson]
  end
  subgraph OS["OS / BSP"]
    FS[fs_port]
    NET[sockets]
  end

  PZD --> TCP
  PZD --> MTR
  PZD --> ZJ
  PZD --> TIME
  PZD --> LOG
  PZD --> SWU
  MTR --> ZJ
  MTR --> FS
  TCP --> NET
  MTR --> NET
```

---

## 15b. Packet fragmentation (readout / log / directives)

```mermaid
flowchart TD
  DATA[Large string or file] --> CHUNK[Split ~700 bytes payload]
  CHUNK --> P1[packetNum 1, packetStream true]
  CHUNK --> P2[packetNum 2, ...]
  CHUNK --> PN[packetNum N, packetStream false]
  P1 --> SEND[appTcpConnManagerSend push or pull]
  P2 --> SEND
  PN --> SEND
```

---

## 15c. Meter job internal phases

```mermaid
stateDiagram-v2
  [*] --> Queued: taskSlotAlloc
  Queued --> Running: worker picks job
  Running --> Finished: IEC flow done
  Finished --> Callback: meterTaskCallback
  Callback --> [*]: Protocol frees slot + sends data
```

---

## 15d. Test server — ident to pull unlock

```mermaid
flowchart LR
  I[Receive ident on push] --> U[Update pull IP/port fields]
  U --> E[Enable pull command buttons]
  E --> C[User clicks e.g. Log]
  C --> T[TCP connect to device pull]
```

---

## 16. Error and ACK model

| Response | Meaning |
|----------|---------|
| `function: "ack"` | Success |
| `function: "nack"` | Failure / invalid JSON / rejected operation |

Some flows (e.g. alive) tolerate missing server replies; readout data push still deletes local files even if no ack.

---

## 17. Configuration touchpoints (conceptual)

```mermaid
flowchart TB
  CONF[Default server IP/port in init] --> PZD[appProtocolZDInit]
  SET[setting message] --> OVERRIDE[Persist + TcpConn restart]
  OVERRIDE --> NEW[New push target]
```

---

## 18. Related documentation pages

| Page | Focus |
|------|--------|
| [Overview](index.html) | Short intro and push/pull table |
| [TCP Connection](tcp-conn.html) | Socket manager APIs, `IncomingMsngCb_t` at start |
| [Meter Operations](meter-operations.html) | Registry and jobs |
| [ProtocolZD](protocolzd.html) | Message dictionary with examples |
| [Test Server](test-server.html) | UI and scenarios |

---

## 19. Glossary

| Term | Meaning |
|------|---------|
| **Push** | Outbound connection from device to head-end |
| **Pull** | Inbound connection to device’s listening port |
| **Ident** | Device announces itself and pull endpoint |
| **Alive** | Periodic keep-alive |
| **Readout** | Meter reading result packaged as JSON |
| **Load profile** | Profile window read from `_payload.txt` |
| **IncomingMsngCb_t** | Callback registered with `appTcpConnManagerStart`; receives `channel`, `data`, `dataLength` for each TCP recv |

---

## 20. Mindmap — `function` field (all channels)

```mermaid
mindmap
  root((Protocol JSON))
    Push from device
      ident
      alive
      readout data
      loadprofile data
    Pull from server
      log
      setting
      fwUpdate
      readout request
      loadprofile request
      directiveList
      directiveAdd
      directiveDelete
    Common responses
      ack
      nack
```

---

## 21. Timeline — typical power-on to steady state

```mermaid
gantt
  title Device boot to steady state (conceptual)
  dateFormat YYYY-MM-DD
  axisFormat %d

  section Network
  TCP start           :a1, 2026-01-01, 1d
  Pull socket up      :a2, after a1, 1d

  section Registration
  Connect push        :b1, after a2, 1d
  Send ident          :b2, after b1, 1d
  Receive register    :b3, after b2, 1d

  section Steady
  Alive every 5 min   :c1, after b3, 3d
```
