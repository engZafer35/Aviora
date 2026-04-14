import { useCallback, useEffect, useRef, useState } from "react";

type LogEntry = { ts: string; tag: string; channel: string; text: string; id: number };

type ServerStatus = {
  running: boolean;
  connected: boolean;
  deviceAddr: string | null;
  deviceSerial: string;
  pullIp: string;
  pullPort: number;
  pullButtonsEnabled: boolean;
  autoIdent: boolean;
  autoAlive: boolean;
  autoDataAck: boolean;
};

const DEFAULT_PORT = 8723;

const DEFAULT_DIRECTIVE_JSON = JSON.stringify(
  {
    id: "ReadoutDirective1",
    directive: [
      { operation: "setBaud", parameter: "300" },
      { operation: "setFraming", parameter: "7E1" },
      { operation: "sendData", parameter: "/?![0D][0A]" },
      { operation: "wait", parameter: "10" },
      { operation: "readData", parameter: "id" },
      { operation: "sendData", parameter: "[06]050[0D][0A]" },
      { operation: "setBaud", parameter: "9600" },
      { operation: "wait", parameter: "600" },
      { operation: "readData", parameter: "rawData" },
    ],
  },
  null,
  2
);

function wsUrl() {
  const proto = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${proto}//${window.location.host}/ws`;
}

export default function App() {
  const wsRef = useRef<WebSocket | null>(null);
  const logEndRef = useRef<HTMLDivElement>(null);
  const [connectedWs, setConnectedWs] = useState(false);
  const [logId, setLogId] = useState(0);
  const [logs, setLogs] = useState<LogEntry[]>([]);
  const [status, setStatus] = useState<ServerStatus>({
    running: false,
    connected: false,
    deviceAddr: null,
    deviceSerial: "12345678",
    pullIp: "127.0.0.1",
    pullPort: 2622,
    pullButtonsEnabled: false,
    autoIdent: true,
    autoAlive: true,
    autoDataAck: true,
  });

  const [port, setPort] = useState(String(DEFAULT_PORT));
  const [serial, setSerial] = useState("12345678");
  const [pullIp, setPullIp] = useState("127.0.0.1");
  const [pullPortStr, setPullPortStr] = useState("2622");

  const [autoIdent, setAutoIdent] = useState(true);
  const [autoAlive, setAutoAlive] = useState(true);
  const [autoDataAck, setAutoDataAck] = useState(true);

  const [modal, setModal] = useState<{
    title: string;
    cmd: string;
    expectStream?: boolean;
    fields: { key: string; label: string; type: "str" | "bool" | "text"; default: string | boolean }[];
  } | null>(null);
  const [modalValues, setModalValues] = useState<Record<string, string | boolean>>({});

  const send = useCallback((obj: object) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(obj));
    }
  }, []);

  useEffect(() => {
    const ws = new WebSocket(wsUrl());
    wsRef.current = ws;
    ws.onopen = () => setConnectedWs(true);
    ws.onclose = () => setConnectedWs(false);
    ws.onmessage = (ev) => {
      try {
        const msg = JSON.parse(ev.data as string);
        if (msg.type === "log") {
          setLogId((n) => {
            const id = n + 1;
            setLogs((prev) => {
              const next = [...prev, { ...msg, id }];
              return next.length > 2000 ? next.slice(-1500) : next;
            });
            return id;
          });
        } else if (msg.type === "clearLog") {
          setLogs([]);
        } else if (msg.type === "status") {
          setStatus((s) => ({
            ...s,
            running: msg.running,
            connected: msg.connected,
            deviceAddr: msg.deviceAddr ?? null,
            deviceSerial: msg.deviceSerial ?? s.deviceSerial,
            pullIp: msg.pullIp ?? s.pullIp,
            pullPort: msg.pullPort ?? s.pullPort,
            pullButtonsEnabled: msg.pullButtonsEnabled,
            autoIdent: msg.autoIdent,
            autoAlive: msg.autoAlive,
            autoDataAck: msg.autoDataAck,
          }));
          if (msg.deviceSerial != null) setSerial(msg.deviceSerial);
          if (msg.pullIp != null) setPullIp(msg.pullIp);
          if (msg.pullPort != null) setPullPortStr(String(msg.pullPort));
          if (msg.autoIdent != null) setAutoIdent(msg.autoIdent);
          if (msg.autoAlive != null) setAutoAlive(msg.autoAlive);
          if (msg.autoDataAck != null) setAutoDataAck(msg.autoDataAck);
        }
      } catch {
        /* ignore */
      }
    };
    return () => ws.close();
  }, []);

  useEffect(() => {
    logEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [logs.length]);

  useEffect(() => {
    send({ type: "setAuto", autoIdent, autoAlive, autoDataAck });
  }, [autoIdent, autoAlive, autoDataAck, send]);

  useEffect(() => {
    send({
      type: "setDevice",
      serial,
      pullIp,
      pullPort: Number(pullPortStr) || 0,
    });
  }, [serial, pullIp, pullPortStr, send]);

  const openModal = (
    title: string,
    cmd: string,
    fields: { key: string; label: string; type: "str" | "bool" | "text"; default: string | boolean }[],
    expectStream?: boolean
  ) => {
    const init: Record<string, string | boolean> = {};
    for (const f of fields) init[f.key] = f.default;
    setModalValues(init);
    setModal({ title, cmd, fields, expectStream });
  };

  const submitModal = () => {
    if (!modal) return;
    send({ type: "pull", cmd: modal.cmd, dialog: modalValues });
    setModal(null);
  };

  const pullCommands: { label: string; icon: string; cmd: string; onClick: () => void }[] = [
    { label: "Log İste", icon: "📋", cmd: "log_request", onClick: () => send({ type: "pull", cmd: "log_request" }) },
    {
      label: "Sunucu Ayarla",
      icon: "🖧",
      cmd: "setting_server",
      onClick: () =>
        openModal("Sunucu Ayarla", "setting_server", [
          { key: "IP", label: "IP", type: "str", default: "127.0.0.1" },
          { key: "Port", label: "Port", type: "str", default: "8723" },
        ]),
    },
    {
      label: "Sayaç Ekle",
      icon: "➕",
      cmd: "add_meter",
      onClick: () =>
        openModal("Sayaç Ekle", "add_meter", [
          { key: "protocol", label: "protocol", type: "str", default: "IEC62056" },
          { key: "type", label: "type", type: "str", default: "electricity" },
          { key: "brand", label: "brand", type: "str", default: "MKL" },
          { key: "serialNumber", label: "serialNumber", type: "str", default: "12345678" },
          { key: "serialPort", label: "serialPort", type: "str", default: "port-1" },
          { key: "initBaud", label: "initBaud", type: "str", default: "300" },
          { key: "fixBaud", label: "fixBaud", type: "bool", default: false },
          { key: "frame", label: "frame", type: "str", default: "7E1" },
        ]),
    },
    {
      label: "Sayaç Sil",
      icon: "➖",
      cmd: "remove_meter",
      onClick: () =>
        openModal("Sayaç Sil", "remove_meter", [
          { key: "brand", label: "brand", type: "str", default: "MKL" },
          { key: "serialNumber", label: "serialNumber", type: "str", default: "12345678" },
        ]),
    },
    {
      label: "FW Güncelle",
      icon: "⬆",
      cmd: "fw_update",
      onClick: () =>
        openModal("Firmware Güncelle", "fw_update", [
          {
            key: "ftp",
            label: "FTP Adresi",
            type: "str",
            default: "ftp://user:pass@192.168.1.1:21/firmware.bin",
          },
        ]),
    },
    {
      label: "Readout İste",
      icon: "📖",
      cmd: "readout",
      onClick: () =>
        openModal("Readout İste", "readout", [
          { key: "Directive", label: "Directive", type: "str", default: "ReadoutDirective1" },
          { key: "Meter Serial No", label: "Meter Serial No", type: "str", default: "12345678" },
        ]),
    },
    {
      label: "Load Profile İste",
      icon: "📊",
      cmd: "loadprofile",
      onClick: () =>
        openModal("Load Profile İste", "loadprofile", [
          { key: "Directive", label: "Directive", type: "str", default: "ProfileDirective1" },
          { key: "Meter Serial No", label: "Meter Serial No", type: "str", default: "12345678" },
          { key: "Başlangıç", label: "Başlangıç", type: "str", default: "2026-04-07 00:00:00" },
          { key: "Bitiş", label: "Bitiş", type: "str", default: "2026-04-07 18:05:00" },
        ]),
    },
    {
      label: "Directive Listele",
      icon: "📑",
      cmd: "directive_list",
      onClick: () =>
        openModal("Directive Listele", "directive_list", [
          { key: "filter", label: 'Filtre ID (* = tümü)', type: "str", default: "*" },
        ]),
    },
    {
      label: "Directive Ekle",
      icon: "📝",
      cmd: "directive_add",
      onClick: () =>
        openModal("Directive Ekle", "directive_add", [
          { key: "json", label: "Directive (JSON blob)", type: "text", default: DEFAULT_DIRECTIVE_JSON },
        ]),
    },
    {
      label: "Directive Sil",
      icon: "🗑",
      cmd: "directive_delete",
      onClick: () =>
        openModal("Directive Sil", "directive_delete", [
          { key: "id", label: "Directive ID (* = tümü)", type: "str", default: "ReadoutDirective1" },
        ]),
    },
  ];

  const prefixClass = (tag: string) => {
    if (tag === "sent") return "log-line-sent";
    if (tag === "recv") return "log-line-recv";
    if (tag === "error") return "log-line-error";
    return "log-line-info";
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-950 via-slate-900 to-slate-950">
      <header className="border-b border-slate-800/80 bg-slate-900/40 backdrop-blur-md animate-fade-in">
        <div className="mx-auto flex max-w-[1600px] items-center justify-between px-4 py-3">
          <div>
            <h1 className="bg-gradient-to-r from-teal-300 to-sky-400 bg-clip-text text-xl font-bold text-transparent">
              Protocol OrionTLV — Web Test Server
            </h1>
            <p className="text-xs text-slate-500">Binary TLV over TCP · WebSocket köprüsü (Node)</p>
          </div>
          <div
            className={`flex items-center gap-2 rounded-full px-3 py-1 text-xs font-medium transition-all ${
              connectedWs ? "bg-emerald-500/15 text-emerald-400" : "bg-rose-500/15 text-rose-400"
            }`}
          >
            <span
              className={`h-2 w-2 rounded-full ${connectedWs ? "animate-pulse-soft bg-emerald-400" : "bg-rose-400"}`}
            />
            UI {connectedWs ? "canlı" : "bağlantı yok"}
          </div>
        </div>
      </header>

      <div className="mx-auto grid max-w-[1600px] gap-4 p-4 lg:grid-cols-[340px_1fr]">
        <aside className="space-y-4 animate-fade-in">
          <section className="rounded-xl border border-slate-800 bg-slate-900/60 p-4 shadow-lg shadow-black/20 transition hover:border-slate-700">
            <h2 className="mb-3 text-sm font-semibold text-slate-300">Sunucu (OrionTLV)</h2>
            <label className="mb-2 block text-xs text-slate-500">Push Port</label>
            <div className="flex gap-2">
              <input
                type="text"
                value={port}
                disabled={status.running}
                onChange={(e) => setPort(e.target.value)}
                className="w-24 rounded-lg border border-slate-700 bg-slate-950 px-2 py-1.5 font-mono text-sm text-teal-300 outline-none focus:border-teal-600 disabled:opacity-50"
              />
              <button
                type="button"
                disabled={status.running}
                onClick={() => send({ type: "start", port: Number(port) || DEFAULT_PORT })}
                className="flex-1 rounded-lg bg-emerald-600 px-3 py-1.5 text-sm font-medium text-white shadow transition hover:bg-emerald-500 disabled:opacity-40"
              >
                ▶ Başlat
              </button>
              <button
                type="button"
                disabled={!status.running}
                onClick={() => send({ type: "stop" })}
                className="flex-1 rounded-lg bg-rose-600 px-3 py-1.5 text-sm font-medium text-white shadow transition hover:bg-rose-500 disabled:opacity-40"
              >
                ■ Durdur
              </button>
            </div>
            <p
              className={`mt-3 flex items-center gap-2 text-sm ${
                status.running ? "text-emerald-400" : "text-rose-400"
              }`}
            >
              <span className="inline-block h-2 w-2 rounded-full bg-current" />
              {status.running ? `Çalışıyor (port ${port})` : "Durduruldu"}
            </p>
          </section>

          <section className="rounded-xl border border-slate-800 bg-slate-900/60 p-4 shadow-lg transition hover:border-slate-700">
            <h2 className="mb-3 text-sm font-semibold text-slate-300">Cihaz bilgisi</h2>
            <div className="space-y-2 text-sm">
              <div>
                <label className="text-xs text-slate-500">Seri No</label>
                <input
                  value={serial}
                  onChange={(e) => setSerial(e.target.value)}
                  className="mt-0.5 w-full rounded-lg border border-slate-700 bg-slate-950 px-2 py-1.5 font-mono text-xs outline-none focus:border-teal-600"
                />
              </div>
              <div>
                <label className="text-xs text-slate-500">Pull IP</label>
                <input
                  value={pullIp}
                  onChange={(e) => setPullIp(e.target.value)}
                  className="mt-0.5 w-full rounded-lg border border-slate-700 bg-slate-950 px-2 py-1.5 font-mono text-xs outline-none focus:border-teal-600"
                />
              </div>
              <div>
                <label className="text-xs text-slate-500">Pull Port</label>
                <input
                  value={pullPortStr}
                  onChange={(e) => setPullPortStr(e.target.value)}
                  className="mt-0.5 w-full rounded-lg border border-slate-700 bg-slate-950 px-2 py-1.5 font-mono text-xs outline-none focus:border-teal-600"
                />
              </div>
              <p className="text-xs text-slate-500">
                Bağlantı:{" "}
                {status.connected && status.deviceAddr ? (
                  <span className="text-emerald-400">{status.deviceAddr}</span>
                ) : (
                  <span className="text-slate-600">yok</span>
                )}
              </p>
            </div>
          </section>

          <section className="rounded-xl border border-slate-800 bg-slate-900/60 p-4">
            <h2 className="mb-3 text-sm font-semibold text-slate-300">Otomatik cevap</h2>
            <label className="mb-2 flex cursor-pointer items-center gap-2 text-sm text-slate-400">
              <input type="checkbox" checked={autoIdent} onChange={(e) => setAutoIdent(e.target.checked)} />
              Ident → register: true
            </label>
            <label className="mb-2 flex cursor-pointer items-center gap-2 text-sm text-slate-400">
              <input type="checkbox" checked={autoAlive} onChange={(e) => setAutoAlive(e.target.checked)} />
              Alive → ACK
            </label>
            <label className="flex cursor-pointer items-center gap-2 text-sm text-slate-400">
              <input type="checkbox" checked={autoDataAck} onChange={(e) => setAutoDataAck(e.target.checked)} />
              Readout/LP veri → ACK
            </label>
          </section>

          <section className="rounded-xl border border-slate-800 bg-slate-900/60 p-4">
            <h2 className="mb-2 text-sm font-semibold text-slate-300">Pull komutları</h2>
            <p
              className={`mb-3 text-xs ${
                status.pullButtonsEnabled ? "text-emerald-400" : "text-amber-500"
              }`}
            >
              {status.pullButtonsEnabled
                ? `✔ Pull: ${pullIp}:${pullPortStr}`
                : "⚠ Sunucu başlatıldığında komutlar etkin olur.)"}
            </p>
            <div className="flex flex-col gap-1.5">
              {pullCommands.map((c) => (
                <button
                  key={c.cmd}
                  type="button"
                  disabled={!status.pullButtonsEnabled}
                  onClick={c.onClick}
                  className="rounded-lg border border-slate-700 bg-slate-800/80 px-3 py-2 text-left text-sm text-slate-200 transition hover:border-teal-600/50 hover:bg-slate-800 disabled:cursor-not-allowed disabled:opacity-35"
                >
                  <span className="mr-2">{c.icon}</span>
                  {c.label}
                </button>
              ))}
            </div>
          </section>

          <button
            type="button"
            onClick={() => send({ type: "clearLog" })}
            className="w-full rounded-lg border border-slate-700 py-2 text-sm text-slate-400 transition hover:bg-slate-800"
          >
            Logları temizle
          </button>
        </aside>

        <main className="flex min-h-[520px] flex-col rounded-xl border border-slate-800 bg-[#12141a] shadow-xl animate-fade-in">
          <div className="border-b border-slate-800 px-4 py-2">
            <h2 className="text-sm font-semibold text-slate-300">Mesaj logları (Binary TLV)</h2>
          </div>
          <div className="custom-scrollbar flex-1 overflow-y-auto p-4 font-mono text-[11px] leading-relaxed">
            {logs.map((line) => (
              <div key={line.id} className="mb-4 animate-fade-in border-b border-slate-800/50 pb-3">
                <div className={`mb-1 font-semibold ${prefixClass(line.tag)}`}>
                  [{line.ts}] {line.channel ? `[${line.channel}] ` : ""}
                  {line.tag === "sent" ? "→ SENT" : line.tag === "recv" ? "← RECV" : line.tag === "error" ? "✖ ERROR" : "ℹ INFO"}
                </div>
                <pre className="whitespace-pre-wrap break-all text-slate-300">{line.text}</pre>
                <div className="mt-2 text-slate-600">{"─".repeat(48)}</div>
              </div>
            ))}
            <div ref={logEndRef} />
          </div>
        </main>
      </div>

      {modal && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 p-4 backdrop-blur-sm animate-fade-in">
          <div
            className="max-h-[90vh] w-full max-w-lg overflow-y-auto rounded-2xl border border-slate-700 bg-slate-900 p-6 shadow-2xl animate-fade-in"
            role="dialog"
          >
            <h3 className="mb-4 text-lg font-semibold text-white">{modal.title}</h3>
            <div className="space-y-3">
              {modal.fields.map((f) => (
                <div key={f.key}>
                  <label className="mb-1 block text-xs text-slate-500">{f.label}</label>
                  {f.type === "bool" ? (
                    <input
                      type="checkbox"
                      checked={!!modalValues[f.key]}
                      onChange={(e) => setModalValues((v) => ({ ...v, [f.key]: e.target.checked }))}
                    />
                  ) : f.type === "text" ? (
                    <textarea
                      value={String(modalValues[f.key] ?? "")}
                      onChange={(e) => setModalValues((v) => ({ ...v, [f.key]: e.target.value }))}
                      rows={12}
                      className="w-full rounded-lg border border-slate-700 bg-slate-950 p-2 font-mono text-xs text-teal-100"
                    />
                  ) : (
                    <input
                      type="text"
                      value={String(modalValues[f.key] ?? "")}
                      onChange={(e) => setModalValues((v) => ({ ...v, [f.key]: e.target.value }))}
                      className="w-full rounded-lg border border-slate-700 bg-slate-950 px-2 py-1.5 font-mono text-xs"
                    />
                  )}
                </div>
              ))}
            </div>
            <div className="mt-6 flex justify-end gap-2">
              <button
                type="button"
                onClick={() => setModal(null)}
                className="rounded-lg px-4 py-2 text-sm text-slate-400 hover:bg-slate-800"
              >
                İptal
              </button>
              <button
                type="button"
                onClick={submitModal}
                className="rounded-lg bg-teal-600 px-4 py-2 text-sm font-medium text-white hover:bg-teal-500"
              >
                Gönder
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
