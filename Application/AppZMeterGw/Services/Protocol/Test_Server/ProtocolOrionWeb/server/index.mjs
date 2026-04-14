/**
 * Orion TLV Web test server — TCP push + pull + WebSocket UI (parity with ProtocolOrionTLV_TestServer.py)
 */
import http from "http";
import net from "net";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

import express from "express";
import { WebSocketServer } from "ws";

import {
  PacketBuilder,
  parsePacket,
  extractPackets,
  getString,
  getUint8,
  getUint16,
  getUint32,
  getBool,
  getFunction,
  formatTlvFields,
  minifyJsonForDevice,
  TAG_TRANS_NUMBER,
  TAG_SERIAL_NUMBER,
  TAG_FUNCTION,
  TAG_REGISTERED,
  TAG_DEVICE_BRAND,
  TAG_DEVICE_MODEL,
  TAG_DEVICE_DATE,
  TAG_PULL_IP,
  TAG_PULL_PORT,
  TAG_REGISTER,
  TAG_PACKET_NUM,
  TAG_PACKET_STREAM,
  TAG_ACK_STATUS,
  TAG_SERVER_IP,
  TAG_SERVER_PORT,
  TAG_METER_OPERATION,
  TAG_METER_PROTOCOL,
  TAG_METER_TYPE,
  TAG_METER_BRAND,
  TAG_METER_SERIAL_NUM,
  TAG_METER_SERIAL_PORT,
  TAG_METER_INIT_BAUD,
  TAG_METER_FIX_BAUD,
  TAG_METER_FRAME,
  TAG_METER_INDEX,
  TAG_FW_ADDRESS,
  TAG_DIRECTIVE_NAME,
  TAG_START_DATE,
  TAG_END_DATE,
  TAG_DIRECTIVE_ID,
  TAG_DIRECTIVE_DATA,
  FUNC_IDENT,
  FUNC_ALIVE,
  FUNC_ACK,
  FUNC_NACK,
  FUNC_LOG,
  FUNC_SETTING,
  FUNC_FW_UPDATE,
  FUNC_READOUT,
  FUNC_LOADPROFILE,
  FUNC_DIRECTIVE_LIST,
  FUNC_DIRECTIVE_ADD,
  FUNC_DIRECTIVE_DEL,
  FUNC_NAMES,
} from "./protocol.mjs";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PULL_RESPONSE_TIMEOUT_SEC = 15;
const DEFAULT_PORT = 8723;
const WS_PORT = Number(process.env.PORT) || 8788;
const DEVICE_FLAG = "AVI";

const SCRIPT_DIR = __dirname;

function ts() {
  const d = new Date();
  return d.toISOString().replace("T", " ").slice(0, 23);
}

class OrionBridge {
  constructor(broadcast) {
    this.broadcast = broadcast;
    this.serverSock = null;
    this.deviceConn = null;
    this.deviceAddr = null;
    this.running = false;
    this.pushPort = DEFAULT_PORT;

    this.deviceSerial = "12345678";
    this.pullIp = "127.0.0.1";
    this.pullPort = 2622;
    this.deviceIdentified = false;

    this.autoIdent = true;
    this.autoAlive = true;
    this.autoDataAck = true;

    this._pullTrans = 1;

    const logName = `ba_server_log_${new Date().toISOString().slice(0, 19).replace(/[-:]/g, "").replace("T", "_")}.txt`;
    this.logFilePath = path.join(SCRIPT_DIR, logName);
    this.logFile = fs.createWriteStream(this.logFilePath, { flags: "a" });
  }

  emitStatus() {
    this.broadcast({
      type: "status",
      running: this.running,
      connected: !!this.deviceConn,
      deviceAddr: this.deviceAddr,
      deviceSerial: this.deviceSerial,
      pullIp: this.pullIp,
      pullPort: this.pullPort,
      pullButtonsEnabled: this.deviceIdentified,
      autoIdent: this.autoIdent,
      autoAlive: this.autoAlive,
      autoDataAck: this.autoDataAck,
    });
  }

  log(tag, text, channel = "") {
    const line = { type: "log", ts: ts(), tag, channel, text };
    this.broadcast(line);
    const prefix =
      tag === "sent"
        ? `[${line.ts}] [${channel}] → SENT`
        : tag === "recv"
          ? `[${line.ts}] [${channel}] ← RECV`
          : tag === "error"
            ? `[${line.ts}] [${channel}] ✖ ERROR`
            : `[${line.ts}] [${channel}] ℹ INFO`;
    try {
      this.logFile.write(`${prefix}\n${text}\n${"─".repeat(60)}\n`);
    } catch {
      /* ignore */
    }
  }

  start(port) {
    if (this.running) return { ok: false, error: "Already running" };
    const p = Number(port);
    if (!Number.isFinite(p) || p < 1 || p > 65535) {
      return { ok: false, error: "Invalid port" };
    }
    try {
      this.serverSock = net.createServer();
      this.serverSock.on("connection", (conn) => this._onDeviceConnection(conn));
      this.serverSock.on("error", (e) => {
        this.log("error", `Sunucu hatası: ${e.message}`);
        this.running = false;
        this.emitStatus();
      });
      this.serverSock.listen(p, "0.0.0.0", () => {
        this.running = true;
        this.pushPort = p;
        this.deviceIdentified = true;
        this.log("info", `Sunucu başlatıldı – port ${p}`);
        this.emitStatus();
      });
    } catch (e) {
      return { ok: false, error: String(e.message || e) };
    }
    return { ok: true };
  }

  stop() {
    if (!this.running) return;
    this.running = false;
    if (this.deviceConn) {
      try {
        this.deviceConn.destroy();
      } catch {
        /* */
      }
      this.deviceConn = null;
      this.deviceAddr = null;
    }
    if (this.serverSock) {
      try {
        this.serverSock.close();
      } catch {
        /* */
      }
      this.serverSock = null;
    }
    this.deviceIdentified = false;
    this.log("info", "Sunucu durduruldu");
    this.emitStatus();
  }

  _onDeviceConnection(conn) {
    conn.setTimeout(0);
    if (this.deviceConn) {
      try {
        this.deviceConn.destroy();
      } catch {
        /* */
      }
    }
    this.deviceConn = conn;
    this.deviceAddr = conn.remoteAddress?.replace(/^::ffff:/, "") || "?";
    const port = conn.remotePort;
    this.log("info", `Cihaz bağlandı: ${this.deviceAddr}:${port}`);
    this.emitStatus();

    let buf = Buffer.alloc(0);
    conn.on("data", (data) => {
      buf = Buffer.concat([buf, data]);
      const { packets, rest } = extractPackets(buf);
      buf = rest;
      for (const pkt of packets) {
        this._handlePushPacket(pkt);
      }
    });
    conn.on("close", () => {
      this.log("info", "Cihaz bağlantısı kesildi");
      if (this.deviceConn === conn) {
        this.deviceConn = null;
        this.deviceAddr = null;
      }
      this.emitStatus();
    });
    conn.on("error", () => {
      if (this.deviceConn === conn) {
        this.deviceConn = null;
        this.deviceAddr = null;
      }
      this.emitStatus();
    });
  }

  _handlePushPacket(raw) {
    const fields = parsePacket(raw);
    if (!fields) {
      this.log("error", `Geçersiz paket: ${raw.toString("hex")}`);
      return;
    }
    const hexDump = raw.toString("hex").toUpperCase();
    const pretty = formatTlvFields(fields);
    this.log("recv", `HEX: ${hexDump}\n${pretty}`, "PUSH");

    const func = getFunction(fields);
    const sn = getString(fields, TAG_SERIAL_NUMBER);
    if (sn) this.deviceSerial = sn;

    if (func === FUNC_IDENT) {
      const pullIpStr = getString(fields, TAG_PULL_IP);
      const pullPortVal = getUint16(fields, TAG_PULL_PORT);
      const registered = getBool(fields, TAG_REGISTERED);
      let connIp = this.deviceAddr || "";
      let effectiveIp = pullIpStr;
      if (connIp && (!pullIpStr || pullIpStr === "127.0.0.1" || pullIpStr === "0.0.0.0")) {
        effectiveIp = connIp;
      } else if (!effectiveIp) effectiveIp = connIp;
      if (effectiveIp) this.pullIp = effectiveIp;
      if (pullPortVal) this.pullPort = pullPortVal;

      this.deviceIdentified = true;
      const brand = getString(fields, TAG_DEVICE_BRAND);
      const model = getString(fields, TAG_DEVICE_MODEL);
      const date = getString(fields, TAG_DEVICE_DATE);
      this.log(
        "info",
        `Ident alındı – SN:${sn}  registered:${registered}  brand:${brand}  model:${model}  date:${date}  pullIP:${effectiveIp} (cihaz:${pullIpStr})  pullPort:${pullPortVal}`
      );
      this.emitStatus();

      if (this.autoIdent) {
        const trans = getUint16(fields, TAG_TRANS_NUMBER);
        const pb = new PacketBuilder();
        pb.addUint16(TAG_TRANS_NUMBER, trans);
        pb.addDeviceHeader(DEVICE_FLAG, sn);
        pb.addUint8(TAG_FUNCTION, FUNC_IDENT);
        pb.addBool(TAG_REGISTER, true);
        this._sendPushRaw(pb.finish());
      }
    } else if (func === FUNC_ALIVE) {
      const date = getString(fields, TAG_DEVICE_DATE);
      this.log("info", `Alive alındı – date:${date}`);
      if (this.autoAlive) {
        const trans = getUint16(fields, TAG_TRANS_NUMBER);
        const pb = new PacketBuilder();
        pb.addUint16(TAG_TRANS_NUMBER, trans);
        pb.addDeviceHeader(DEVICE_FLAG, this.deviceSerial);
        pb.addUint8(TAG_FUNCTION, FUNC_ACK);
        pb.addBool(TAG_ACK_STATUS, true);
        this._sendPushRaw(pb.finish());
      }
    } else if (func === FUNC_READOUT || func === FUNC_LOADPROFILE) {
      const pnum = getUint16(fields, TAG_PACKET_NUM);
      const pstream = getBool(fields, TAG_PACKET_STREAM);
      const fname = FUNC_NAMES.get(func) ?? "?";
      this.log("info", `${fname} verisi alındı – paket:${pnum}  stream:${pstream}`);
      if (!pstream && this.autoDataAck) {
        const trans = getUint16(fields, TAG_TRANS_NUMBER);
        const pb = new PacketBuilder();
        pb.addUint16(TAG_TRANS_NUMBER, trans);
        pb.addDeviceHeader(DEVICE_FLAG, this.deviceSerial);
        pb.addUint8(TAG_FUNCTION, FUNC_ACK);
        pb.addBool(TAG_ACK_STATUS, true);
        this._sendPushRaw(pb.finish());
      }
    } else if (func === FUNC_ACK) {
      this.log("info", "ACK alındı (push)");
    } else if (func === FUNC_NACK) {
      this.log("info", "NACK alındı (push)");
    }
  }

  _sendPushRaw(packetBytes) {
    const conn = this.deviceConn;
    if (!conn) {
      this.log("error", "Push gönderim hatası: cihaz bağlı değil");
      return;
    }
    try {
      conn.write(packetBytes);
      const fields = parsePacket(packetBytes);
      const hexDump = packetBytes.toString("hex").toUpperCase();
      const pretty = fields ? formatTlvFields(fields) : "";
      this.log("sent", `HEX: ${hexDump}\n${pretty}`, "PUSH");
    } catch (e) {
      this.log("error", `Push gönderim hatası: ${e.message}`);
    }
  }

  _nextPullTrans() {
    const t = this._pullTrans;
    this._pullTrans = (this._pullTrans % 0xffff) + 1;
    if (this._pullTrans === 0) this._pullTrans = 1;
    return t;
  }

  _buildPullPacket(funcId, extraFn) {
    const pb = new PacketBuilder();
    pb.addUint16(TAG_TRANS_NUMBER, this._nextPullTrans());
    pb.addDeviceHeader(DEVICE_FLAG, this.deviceSerial);
    pb.addUint8(TAG_FUNCTION, funcId);
    if (extraFn) extraFn(pb);
    return pb.finish();
  }

  _sendPull(packetBytes, expectStream = false) {
    if (!this.deviceIdentified) {
      this.log("error", "Pull gönderilemez: henüz ident alınmadı");
      return;
    }
    const ip = this.pullIp;
    const port = this.pullPort;
    if (!ip || !port) {
      this.log("error", "Pull IP veya Port bilgisi eksik");
      return;
    }

    const sock = new net.Socket();
    sock.setNoDelay(true);
    let buf = Buffer.alloc(0);
    let gotCompleteResponse = false;
    let finished = false;
    let hardTimer;

    const done = () => {
      if (finished) return;
      finished = true;
      if (hardTimer) clearTimeout(hardTimer);
      try {
        sock.destroy();
      } catch {
        /* */
      }
    };

    const tryParse = () => {
      const { packets, rest } = extractPackets(buf);
      buf = rest;
      for (const pkt of packets) {
        gotCompleteResponse = true;
        const pktFields = parsePacket(pkt);
        if (pktFields) {
          const hexD = pkt.toString("hex").toUpperCase();
          const pr = formatTlvFields(pktFields);
          this.log("recv", `HEX: ${hexD}\n${pr}`, "PULL");
        }
        const stream = pktFields ? getBool(pktFields, TAG_PACKET_STREAM) : false;
        if (!expectStream || !stream) {
          done();
          return true;
        }
      }
      return false;
    };

    sock.on("data", (chunk) => {
      buf = Buffer.concat([buf, chunk]);
      tryParse();
    });

    sock.on("close", () => {
      if (hardTimer) clearTimeout(hardTimer);
      if (finished) return;
      finished = true;
      if (!gotCompleteResponse) {
        this.log(
          "error",
          "Pull: yanıt alınamadı (EOF, süre veya cihaz yanıt göndermeden kapattı)"
        );
      }
    });

    sock.on("error", (e) => {
      if (hardTimer) clearTimeout(hardTimer);
      this.log("error", `Pull komutu hatası: ${e.message}`);
      done();
    });

    sock.connect(port, ip, () => {
      try {
        sock.write(packetBytes);
        const fields = parsePacket(packetBytes);
        const hexDump = packetBytes.toString("hex").toUpperCase();
        const pretty = fields ? formatTlvFields(fields) : "";
        this.log("sent", `HEX: ${hexDump}\n${pretty}`, "PULL");
      } catch (e) {
        this.log("error", `Pull yazma: ${e.message}`);
        done();
      }
    });

    hardTimer = setTimeout(() => {
      if (!gotCompleteResponse && !finished) {
        this.log("error", `Pull: ${PULL_RESPONSE_TIMEOUT_SEC} sn içinde yanıt alınamadı`);
        done();
      }
    }, PULL_RESPONSE_TIMEOUT_SEC * 1000);
  }

  handlePull(cmd, dialog) {
    switch (cmd) {
      case "log_request":
        this._sendPull(this._buildPullPacket(FUNC_LOG), true);
        break;
      case "setting_server": {
        const ip = dialog?.IP ?? "127.0.0.1";
        const portStr = dialog?.Port ?? "8723";
        this._sendPull(
          this._buildPullPacket(FUNC_SETTING, (pb) => {
            pb.addString(TAG_SERVER_IP, ip);
            pb.addUint16(TAG_SERVER_PORT, Number(portStr));
          })
        );
        break;
      }
      case "add_meter":
        this._sendPull(
          this._buildPullPacket(FUNC_SETTING, (pb) => {
            pb.addUint8(TAG_METER_INDEX, 0);
            pb.addString(TAG_METER_OPERATION, "add");
            pb.addString(TAG_METER_PROTOCOL, dialog?.protocol ?? "IEC62056");
            pb.addString(TAG_METER_TYPE, dialog?.type ?? "electricity");
            pb.addString(TAG_METER_BRAND, dialog?.brand ?? "MKL");
            pb.addString(TAG_METER_SERIAL_NUM, dialog?.serialNumber ?? "12345678");
            pb.addString(TAG_METER_SERIAL_PORT, dialog?.serialPort ?? "port-1");
            pb.addUint32(TAG_METER_INIT_BAUD, Number(dialog?.initBaud ?? 300));
            pb.addBool(TAG_METER_FIX_BAUD, !!dialog?.fixBaud);
            pb.addString(TAG_METER_FRAME, dialog?.frame ?? "7E1");
          })
        );
        break;
      case "remove_meter":
        this._sendPull(
          this._buildPullPacket(FUNC_SETTING, (pb) => {
            pb.addUint8(TAG_METER_INDEX, 0);
            pb.addString(TAG_METER_OPERATION, "remove");
            pb.addString(TAG_METER_BRAND, dialog?.brand ?? "MKL");
            pb.addString(TAG_METER_SERIAL_NUM, dialog?.serialNumber ?? "12345678");
          })
        );
        break;
      case "fw_update":
        this._sendPull(
          this._buildPullPacket(FUNC_FW_UPDATE, (pb) => {
            pb.addString(TAG_FW_ADDRESS, dialog?.ftp ?? dialog?.["FTP Adresi"] ?? "");
          })
        );
        break;
      case "readout":
        this._sendPull(
          this._buildPullPacket(FUNC_READOUT, (pb) => {
            pb.addString(TAG_DIRECTIVE_NAME, dialog?.Directive ?? "");
            pb.addString(TAG_METER_SERIAL_NUM, dialog?.["Meter Serial No"] ?? "");
          })
        );
        break;
      case "loadprofile":
        this._sendPull(
          this._buildPullPacket(FUNC_LOADPROFILE, (pb) => {
            pb.addString(TAG_DIRECTIVE_NAME, dialog?.Directive ?? "");
            pb.addString(TAG_METER_SERIAL_NUM, dialog?.["Meter Serial No"] ?? "");
            pb.addString(TAG_START_DATE, dialog?.Başlangıç ?? dialog?.start ?? "");
            pb.addString(TAG_END_DATE, dialog?.Bitiş ?? dialog?.end ?? "");
          })
        );
        break;
      case "directive_list":
        this._sendPull(
          this._buildPullPacket(FUNC_DIRECTIVE_LIST, (pb) => {
            pb.addString(TAG_DIRECTIVE_ID, dialog?.filter ?? "*");
          }),
          true
        );
        break;
      case "directive_add": {
        let wire;
        try {
          wire = minifyJsonForDevice(dialog?.json ?? dialog?.directiveJson ?? "{}");
        } catch (e) {
          this.log("error", `JSON: ${e.message}`);
          return;
        }
        this._sendPull(
          this._buildPullPacket(FUNC_DIRECTIVE_ADD, (pb) => {
            pb.addString(TAG_DIRECTIVE_DATA, wire);
          })
        );
        break;
      }
      case "directive_delete":
        this._sendPull(
          this._buildPullPacket(FUNC_DIRECTIVE_DEL, (pb) => {
            pb.addString(TAG_DIRECTIVE_ID, dialog?.id ?? "");
          })
        );
        break;
      default:
        this.log("error", `Unknown pull cmd: ${cmd}`);
    }
  }
}

const app = express();
const __root = path.join(__dirname, "..");
const distPath = path.join(__root, "dist");
if (fs.existsSync(distPath)) {
  app.use(express.static(distPath));
  app.get("*", (_req, res) => {
    res.sendFile(path.join(distPath, "index.html"));
  });
}

const server = http.createServer(app);
const wss = new WebSocketServer({ server, path: "/ws" });

/** @type {Set<import('ws').WebSocket>} */
const clients = new Set();

function broadcast(obj) {
  const s = JSON.stringify(obj);
  for (const c of clients) {
    if (c.readyState === 1) c.send(s);
  }
}

const bridge = new OrionBridge(broadcast);

wss.on("connection", (ws) => {
  clients.add(ws);
  bridge.emitStatus();
  ws.on("message", (raw) => {
    let msg;
    try {
      msg = JSON.parse(raw.toString());
    } catch {
      return;
    }
    switch (msg.type) {
      case "start":
        bridge.start(msg.port ?? DEFAULT_PORT);
        break;
      case "stop":
        bridge.stop();
        break;
      case "clearLog":
        broadcast({ type: "clearLog" });
        break;
      case "setAuto":
        bridge.autoIdent = !!msg.autoIdent;
        bridge.autoAlive = !!msg.autoAlive;
        bridge.autoDataAck = !!msg.autoDataAck;
        bridge.emitStatus();
        break;
      case "setDevice":
        if (msg.serial != null) bridge.deviceSerial = String(msg.serial);
        if (msg.pullIp != null) bridge.pullIp = String(msg.pullIp);
        if (msg.pullPort != null) bridge.pullPort = Number(msg.pullPort);
        bridge.emitStatus();
        break;
      case "pull":
        bridge.handlePull(msg.cmd, msg.dialog ?? {});
        break;
      default:
        break;
    }
  });
  ws.on("close", () => clients.delete(ws));
});

server.listen(WS_PORT, "0.0.0.0", () => {
  console.log(`OrionTLV Web bridge: http://127.0.0.1:${WS_PORT}/  (WS /ws)`);
  console.log(`  (tüm arayüzler: aynı port — örn. http://192.168.56.1:${WS_PORT}/ )`);
  if (fs.existsSync(distPath)) {
    console.log("Serving static from dist/");
  } else {
    console.log("No dist/ — run Vite on :5173 for UI, or npm run build");
  }
});
