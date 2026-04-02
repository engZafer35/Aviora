#!/usr/bin/env python3
"""
AVI RigelMq Protocol Test Server
JSON-over-MQTT protocol test server for IoT meter gateway.

Topic directions (same as gateway firmware):
  - Gateway subscribes to:  avi/request   (commands from backend)
  - Gateway publishes to:   avi/response  (ident, alive, ack, data, …)

This process acts as the backend:
  - SUBSCRIBE avi/response  — receive gateway → server traffic
  - PUBLISH   avi/request   — send server → gateway traffic (ident reply, readout, ack, …)

Usage:
    python ProtocolRigelMq_TestServer.py [broker_ip] [broker_port]
    python ProtocolRigelMq_TestServer.py 127.0.0.1 1883
"""

import sys
import os
import json
import threading
import queue
import time
import argparse
from datetime import datetime

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("paho-mqtt kütüphanesi gerekli. 'pip install paho-mqtt' ile yükleyin.")
    sys.exit(1)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_BROKER_IP = "127.0.0.1"
DEFAULT_BROKER_PORT = 1883

# Gateway SUBscribes here (backend PUBLISHes commands / ident reply / ack)
REQUEST_TOPIC = "avi/request"
# Gateway PUBLISHes here (backend SUBscribes to see ident, alive, …)
RESPONSE_TOPIC = "avi/response"

# ═══════════════════════════════════════════════════════════════
#                    Protocol Test Server
# ═══════════════════════════════════════════════════════════════

class ProtocolTestServer:

    def __init__(self, root, broker_ip, broker_port):
        self.root = root
        self.root.title("Protocol RigelMq Test Server")
        self.root.geometry("1200x900")
        #self.root.minsize(1200, 800)

        self.broker_ip = broker_ip
        self.broker_port = broker_port

        # ── state ──
        self.mqtt_client = None
        self.running = False
        self.lock = threading.Lock()

        # ── device info ──
        self.device_serial = tk.StringVar(value="")
        self.device_flag = "AVI"
        self.pull_ip = tk.StringVar(value="")
        self.pull_port_var = tk.StringVar(value="")
        self.device_identified = False
        self.last_ident_trans_number = 0

        # ── auto-response toggles ──
        self.auto_ident = tk.BooleanVar(value=True)
        self.auto_alive = tk.BooleanVar(value=True)
        self.auto_data_ack = tk.BooleanVar(value=True)

        # directiveList & directiveDelete: same request.filter.id (exact id, or "*" = all)
        self.directive_filter_id = tk.StringVar(value="ReadoutDirective1")

        # ── logging ──
        self.log_queue = queue.Queue()
        log_name = f"rigel_mq_server_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        self.log_file_path = os.path.join(SCRIPT_DIR, log_name)
        self.log_file = open(self.log_file_path, "a", encoding="utf-8")

        self._build_ui()
        self._poll_log()

    # ───────────────────────── UI ─────────────────────────

    def _build_ui(self):
        style = ttk.Style()
        for theme in ("vista", "clam", "default"):
            try:
                style.theme_use(theme)
                break
            except tk.TclError:
                continue

        style.configure("Green.TLabel", foreground="green")
        style.configure("Red.TLabel", foreground="red")

        paned = ttk.PanedWindow(self.root, orient="horizontal")
        paned.pack(fill="both", expand=True, padx=6, pady=6)

        # ── LEFT PANEL ──
        left = ttk.Frame(paned, width=300)
        paned.add(left, weight=0)

        # --- Broker ---
        sf = ttk.LabelFrame(left, text=" MQTT Broker ", padding=6)
        sf.pack(fill="x", padx=4, pady=(0, 4))

        row = ttk.Frame(sf)
        row.pack(fill="x")
        ttk.Label(row, text="IP:").pack(side="left")
        self.ip_entry = ttk.Entry(row, width=15)
        self.ip_entry.insert(0, self.broker_ip)
        self.ip_entry.pack(side="left", padx=4)

        row2 = ttk.Frame(sf)
        row2.pack(fill="x")
        ttk.Label(row2, text="Port:").pack(side="left")
        self.port_entry = ttk.Entry(row2, width=8)
        self.port_entry.insert(0, str(self.broker_port))
        self.port_entry.pack(side="left", padx=4)

        btn_row = ttk.Frame(sf)
        btn_row.pack(fill="x", pady=4)
        self.start_btn = ttk.Button(btn_row, text="▶  Başlat", command=self.start_server)
        self.start_btn.pack(side="left", expand=True, fill="x", padx=2)
        self.stop_btn = ttk.Button(btn_row, text="■  Durdur", command=self.stop_server,
                                   state="disabled")
        self.stop_btn.pack(side="left", expand=True, fill="x", padx=2)

        self.status_lbl = ttk.Label(sf, text="● Durduruldu", style="Red.TLabel")
        self.status_lbl.pack(anchor="w")

        # --- Cihaz Bilgisi ---
        df = ttk.LabelFrame(left, text=" Cihaz Bilgisi ", padding=6)
        df.pack(fill="x", padx=4, pady=4)

        for r, (label, var) in enumerate([
            ("Seri No:", self.device_serial),
            ("Pull IP:", self.pull_ip),
            ("Pull Port:", self.pull_port_var),
        ]):
            ttk.Label(df, text=label).grid(row=r, column=0, sticky="w", pady=1)
            ttk.Entry(df, textvariable=var, width=20).grid(row=r, column=1, sticky="ew", padx=4, pady=1)
        df.columnconfigure(1, weight=1)

        self.conn_lbl = ttk.Label(df, text="Bağlı değil", foreground="gray")
        self.conn_lbl.grid(row=3, column=0, columnspan=2, sticky="w", pady=2)

        # --- Otomatik Cevap ---
        af = ttk.LabelFrame(left, text=" Otomatik Cevap ", padding=6)
        af.pack(fill="x", padx=4, pady=4)
        ttk.Checkbutton(af, text='Ident → response.registered: true',
                         variable=self.auto_ident).pack(anchor="w")
        ttk.Checkbutton(af, text="Alive → ACK",
                         variable=self.auto_alive).pack(anchor="w")
        ttk.Checkbutton(af, text="Readout/LP Veri → ACK",
                         variable=self.auto_data_ack).pack(anchor="w")

        # --- directiveList / directiveDelete: request.filter.id (AppProtocolRigelMq) ---
        dlf = ttk.LabelFrame(left, text=" directiveList / directiveDelete — filter id ", padding=6)
        dlf.pack(fill="x", padx=4, pady=4)
        ttk.Label(
            dlf,
            text="id, or * ",
            wraplength=260,
        ).pack(anchor="w")
        ttk.Entry(dlf, textvariable=self.directive_filter_id, width=24).pack(fill="x", pady=(4, 2))

        # --- Push Komutları ---
        cf = ttk.LabelFrame(left, text=" Push Komutları  (Sunucu → Cihaz) ", padding=6)
        cf.pack(fill="x", padx=4, pady=4)

        self.push_info_lbl = ttk.Label(cf, text="⚠ Ident bekleniyor...",
                                        foreground="orange")
        self.push_info_lbl.pack(anchor="w", pady=(0, 4))

        cmds = [
            ("📋  Log İste",             self.cmd_log_request),
            ("🖧  Sunucu Ayarla",        self.cmd_setting_server),
            ("➕  Sayaç Ekle",           self.cmd_add_meter),
            ("➖  Sayaç Sil",            self.cmd_remove_meter),
            ("⬆  FW Güncelle",          self.cmd_fw_update),
            ("📖  Readout İste",         self.cmd_readout),
            ("📊  Load Profile İste",    self.cmd_loadprofile),
            ("📑  Directive Listele",    self.cmd_directive_list),
            ("📝  Directive Ekle",       self.cmd_directive_add),
            ("🗑  Directive Sil",        self.cmd_directive_delete),
            ("🔄  Ident reply (registered)", self.cmd_register),
        ]
        self.push_buttons = []
        for text, cmd in cmds:
            btn = ttk.Button(cf, text=text, command=cmd, state="disabled")
            btn.pack(fill="x", pady=1)
            self.push_buttons.append(btn)

        # --- Alt kontroller ---
        ttk.Button(left, text="Logları Temizle", command=self.clear_log).pack(
            fill="x", padx=4, pady=6)

        # ── RIGHT PANEL (log) ──
        right = ttk.Frame(paned)
        paned.add(right, weight=1)

        ttk.Label(right, text="Mesaj Logları",
                  font=("Segoe UI", 11, "bold")).pack(anchor="w", padx=6, pady=(0, 2))

        log_frame = ttk.Frame(right)
        log_frame.pack(fill="both", expand=True, padx=4, pady=2)

        self.log_text = tk.Text(
            log_frame, wrap="word",
            font=("Consolas", 9),
            bg="#1e1e1e", fg="#d4d4d4",
            insertbackground="white",
            state="disabled", relief="flat", bd=0,
        )
        sb = ttk.Scrollbar(log_frame, command=self.log_text.yview)
        self.log_text.configure(yscrollcommand=sb.set)
        sb.pack(side="right", fill="y")
        self.log_text.pack(side="left", fill="both", expand=True)

        self.log_text.tag_configure("ts",    foreground="#6A9955")
        self.log_text.tag_configure("sent",  foreground="#4EC9B0")
        self.log_text.tag_configure("recv",  foreground="#569CD6")
        self.log_text.tag_configure("info",  foreground="#808080")
        self.log_text.tag_configure("error", foreground="#F44747")

    # ───────────────────── MQTT lifecycle ─────────────────────

    def start_server(self):
        if self.running:
            return
        try:
            ip = self.ip_entry.get().strip()
            port = int(self.port_entry.get())
        except ValueError:
            messagebox.showerror("Hata", "Geçerli bir IP ve port numarası girin.")
            return

        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self._on_connect
        self.mqtt_client.on_message = self._on_message
        self.mqtt_client.on_disconnect = self._on_disconnect

        try:
            self.mqtt_client.connect(ip, port, 60)
        except Exception as e:
            messagebox.showerror("Hata", f"MQTT bağlantısı başarısız:\n{e}")
            return

        self.mqtt_client.loop_start()
        self.running = True
        self.broker_ip = ip
        self.broker_port = port
        self.start_btn.config(state="disabled")
        self.stop_btn.config(state="normal")
        self.ip_entry.config(state="disabled")
        self.port_entry.config(state="disabled")
        self.status_lbl.config(text=f"● Bağlanıyor... ({ip}:{port})", style="Red.TLabel")
        self._log("info", f"MQTT client başlatıldı – {ip}:{port}")

    def stop_server(self):
        if not self.running:
            return
        self.running = False

        if self.mqtt_client:
            self.mqtt_client.loop_stop()
            self.mqtt_client.disconnect()
            self.mqtt_client = None

        self.start_btn.config(state="normal")
        self.stop_btn.config(state="disabled")
        self.ip_entry.config(state="normal")
        self.port_entry.config(state="normal")
        self.status_lbl.config(text="● Durduruldu", style="Red.TLabel")
        self.conn_lbl.config(text="Bağlı değil", foreground="gray")
        for btn in self.push_buttons:
            btn.config(state="disabled")
        self._log("info", "MQTT client durduruldu")

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.status_lbl.config(text=f"● Bağlı ({self.broker_ip}:{self.broker_port})", style="Green.TLabel")
            self.conn_lbl.config(text="MQTT Bağlı", foreground="green")
            # Receive gateway publishes (ident, alive, log chunks, …)
            client.subscribe(RESPONSE_TOPIC)
            self._log("info", f"Subscribed to {RESPONSE_TOPIC} (gateway → server)")
        else:
            self.status_lbl.config(text="● Bağlantı hatası", style="Red.TLabel")
            self._log("error", f"MQTT connect failed: {rc}")

    def _on_disconnect(self, client, userdata, rc):
        self.status_lbl.config(text="● Bağlantı kesildi", style="Red.TLabel")
        self.conn_lbl.config(text="Bağlı değil", foreground="gray")
        for btn in self.push_buttons:
            btn.config(state="disabled")

    def _on_message(self, client, userdata, msg):
        try:
            payload = msg.payload.decode('utf-8')
            self._log("recv", f"[{msg.topic}] {payload}")
            if msg.topic != RESPONSE_TOPIC:
                self._log("info", f"(ignored topic {msg.topic})")
                return
            self._handle_message(payload)
        except Exception as e:
            self._log("error", f"Message parse error: {e}")

    # ───────────────────── Message handling ─────────────────────

    def _handle_message(self, payload):
        try:
            data = json.loads(payload)
        except json.JSONDecodeError:
            self._log("error", "Invalid JSON received")
            return

        func = data.get("function", "")
        trans_num = data.get("transNumber", 0)

        if func == "ident":
            self._handle_ident(data)
        elif func == "alive":
            self._handle_alive(data)
        elif func == "log":
            self._handle_log(data)
        elif func == "readout":
            self._handle_readout(data)
        elif func == "loadprofile":
            self._handle_loadprofile(data)
        elif func == "directiveList":
            self._handle_directive_list(data)
        elif func in ("ack", "nack"):
            self._log("info", f"{func.upper()} transNumber={trans_num}")
        elif func == "register":
            self._handle_register(data)
        else:
            self._log("error", f"Unknown function: {func}")

    def _handle_ident(self, data):
        device = data.get("device", {})
        self.device_serial.set(device.get("serialNumber", ""))
        self.device_flag = device.get("flag", "AVI")
        response = data.get("response", {})
        self.pull_ip.set(str(response.get("pullIP", "")))
        self.pull_port_var.set(str(response.get("pullPort", "")))
        self.last_ident_trans_number = int(data.get("transNumber", 0))
        self.device_identified = True
        self.push_info_lbl.config(text="✓ Ident alındı", foreground="green")
        for btn in self.push_buttons:
            btn.config(state="normal")
        if self.auto_ident.get():
            self.send_ident_reply(registered=True)

    def _handle_alive(self, data):
        if self.auto_alive.get():
            self._send_ack(data.get("transNumber", 0))

    def _handle_log(self, data):
        if self.auto_data_ack.get():
            self._send_ack(data.get("transNumber", 0))

    def _handle_readout(self, data):
        if self.auto_data_ack.get():
            self._send_ack(data.get("transNumber", 0))

    def _handle_loadprofile(self, data):
        if self.auto_data_ack.get():
            self._send_ack(data.get("transNumber", 0))

    def _handle_directive_list(self, data):
        """
        Gateway → backend on avi/response: directive payload (not ack/nack).
        Firmware builds JSON with function directiveList, transNumber, response.directive { id, steps }.
        """
        trans_num = data.get("transNumber", 0)
        resp = data.get("response", {})
        directive = resp.get("directive", {})
        did = directive.get("id", "")
        steps = directive.get("steps")
        n_steps = len(steps) if isinstance(steps, list) else 0
        self._log(
            "info",
            f"directiveList response: transNumber={trans_num}, directive.id={did!r}, steps={n_steps}",
        )
        try:
            self._log("info", json.dumps(data, indent=2))
        except (TypeError, ValueError):
            pass

    def _handle_register(self, data):
        # Legacy/unused: firmware dispatches "register" only if added to protocol
        pass

    def send_ident_reply(self, registered=True):
        """Publish ident reply on avi/request — must reuse the same transNumber as the gateway ident."""
        if self.last_ident_trans_number <= 0:
            self._log("error", "send_ident_reply: no ident transNumber yet (wait for gateway ident on avi/response)")
            return
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "ident",
            "transNumber": self.last_ident_trans_number,
            "response": {"registered": bool(registered)},
        }
        self._publish_to_gateway(msg)

    def _send_ack(self, trans_num):
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "ack",
            "transNumber": trans_num
        }
        self._publish_to_gateway(msg)

    def _send_nack(self, trans_num):
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "nack",
            "transNumber": trans_num
        }
        self._publish_to_gateway(msg)

    def _publish_to_gateway(self, msg):
        """Server → gateway: firmware only listens on avi/request."""
        payload = json.dumps(msg, indent=2)
        self.mqtt_client.publish(REQUEST_TOPIC, payload)
        self._log("sent", f"[{REQUEST_TOPIC}] {payload}")

    def _publish(self, msg):
        """Alias: all backend commands go to the gateway subscription topic."""
        self._publish_to_gateway(msg)

    # ───────────────────── Commands ─────────────────────

    def cmd_log_request(self):
        trans_num = int(time.time() * 1000) % 65536
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "log",
            "transNumber": trans_num
        }
        self._publish(msg)

    def cmd_setting_server(self):
        trans_num = int(time.time() * 1000) % 65536
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "setting",
            "transNumber": trans_num,
            "request": {
                "mqtt": {
                    "broker": self.broker_ip,
                    "port": int(self.broker_port),
                    "userName": "",
                    "password": "",
                    "requestTopic": "avi/request",
                    "responseTopic": "avi/response",
                }
            },
        }
        self._publish(msg)

    def cmd_add_meter(self):
        trans_num = int(time.time() * 1000) % 65536
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "addMeter",
            "transNumber": trans_num,
            "response": {
                "serialNumber": "12345678",
                "protocol": "IEC62056",
                "type": "electricity"
            }
        }
        self._publish(msg)

    def cmd_remove_meter(self):
        trans_num = int(time.time() * 1000) % 65536
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "removeMeter",
            "transNumber": trans_num,
            "response": {
                "serialNumber": "12345678"
            }
        }
        self._publish(msg)

    def cmd_fw_update(self):
        trans_num = int(time.time() * 1000) % 65536
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "fwUpdate",
            "transNumber": trans_num,
            "response": {
                "url": "ftp://user:pass@192.168.1.100/firmware.bin"
            }
        }
        self._publish(msg)

    def cmd_readout(self):
        trans_num = int(time.time() * 1000) % 65536
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "readout",
            "transNumber": trans_num,
            "directive": "ReadoutDirective1",
            "meterId": "12345678"
        }
        self._publish(msg)

    def cmd_loadprofile(self):
        trans_num = int(time.time() * 1000) % 65536
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "loadprofile",
            "transNumber": trans_num,
            "directive": "ProfileDirective1",
            "meterId": "12345678",
            "startDate": "2021-06-22 00:00:00",
            "endDate": "2021-06-22 12:00:00"
        }
        self._publish(msg)

    def cmd_directive_list(self):
        """
        Publish directiveList request on avi/request (gateway subscribes).
        request.filter.id: specific directive id, or '*' to receive every directive in separate responses.
        """
        fid = (self.directive_filter_id.get() or "").strip()
        if not fid:
            messagebox.showwarning(
                "directiveList",
                "Enter request.filter.id (e.g. ReadoutDirective1) or * to list all.",
            )
            return
        trans_num = int(time.time() * 1000) % 65536
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "directiveList",
            "transNumber": trans_num,
            "request": {
                "filter": {
                    "id": fid,
                }
            },
        }
        self._publish(msg)

    def cmd_directive_add(self):
        trans_num = int(time.time() * 1000) % 65536
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "directiveAdd",
            "transNumber": trans_num,
            "response": {
                "id": "TestDirective",
                "directive": {
                    "operation": "sendData",
                    "parameter": "/?!\r\n"
                }
            }
        }
        self._publish(msg)

    def cmd_directive_delete(self):
        """
        Same request.filter.id as directiveList. id '*' deletes all directives (firmware: appMeterOperationsDeleteDirective).
        """
        fid = (self.directive_filter_id.get() or "").strip()
        if not fid:
            messagebox.showwarning(
                "directiveDelete",
                "Enter request.filter.id (e.g. TestDirective) or * to delete all.",
            )
            return
        trans_num = int(time.time() * 1000) % 65536
        msg = {
            "device": {"flag": self.device_flag, "serialNumber": self.device_serial.get()},
            "function": "directiveDelete",
            "transNumber": trans_num,
            "request": {
                "filter": {
                    "id": fid,
                }
            },
        }
        self._publish(msg)

    def cmd_register(self):
        """Send ident reply with response.registered (same transNumber as last gateway ident)."""
        self.send_ident_reply(registered=True)

    # ───────────────────── Logging ─────────────────────

    def _log(self, tag, msg):
        ts = datetime.now().strftime("%H:%M:%S")
        line = f"[{ts}] {msg}\n"
        self.log_queue.put((tag, line))
        self.log_file.write(line)
        self.log_file.flush()

    def _poll_log(self):
        try:
            while True:
                tag, line = self.log_queue.get_nowait()
                self.log_text.config(state="normal")
                self.log_text.insert("end", line, tag)
                self.log_text.see("end")
                self.log_text.config(state="disabled")
        except queue.Empty:
            pass
        self.root.after(100, self._poll_log)

    def clear_log(self):
        self.log_text.config(state="normal")
        self.log_text.delete(1.0, "end")
        self.log_text.config(state="disabled")

# ═══════════════════════════════════════════════════════════════
#                        Main
# ═══════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(description="RigelMq Protocol Test Server")
    parser.add_argument("broker_ip", nargs="?", default=DEFAULT_BROKER_IP,
                        help=f"MQTT broker IP (default: {DEFAULT_BROKER_IP})")
    parser.add_argument("broker_port", nargs="?", type=int, default=DEFAULT_BROKER_PORT,
                        help=f"MQTT broker port (default: {DEFAULT_BROKER_PORT})")

    args = parser.parse_args()

    root = tk.Tk()
    app = ProtocolTestServer(root, args.broker_ip, args.broker_port)
    root.mainloop()

if __name__ == "__main__":
    main()