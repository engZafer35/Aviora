#!/usr/bin/env python3
"""
AVI Protocol Test Server
JSON-over-TCP protocol test server for IoT meter gateway.

Usage:
    python server.py [port]
    python server.py 8722
"""

import sys
import os
import json
import socket
import threading
import queue
import time
import argparse
from datetime import datetime

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox

RECV_BUF = 4096
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_PORT = 8722

# ═══════════════════════════════════════════════════════════════
#                    Protocol Test Server
# ═══════════════════════════════════════════════════════════════

class ProtocolTestServer:

    def __init__(self, root, push_port):
        self.root = root
        self.root.title("Protocol ZD Test Server")
        self.root.geometry("1200x750")
        self.root.minsize(1000, 600)

        self.push_port = push_port

        # ── state ──
        self.server_sock = None
        self.accept_th = None
        self.device_conn = None
        self.device_addr = None
        self.reader_th = None
        self.running = False
        self.lock = threading.Lock()

        # ── device info ──
        self.device_serial = tk.StringVar(value="")
        self.device_flag = "AVI"
        self.pull_ip = tk.StringVar(value="")
        self.pull_port_var = tk.StringVar(value="")
        self.device_identified = False

        # ── auto-response toggles ──
        self.auto_ident = tk.BooleanVar(value=True)
        self.auto_alive = tk.BooleanVar(value=True)
        self.auto_data_ack = tk.BooleanVar(value=True)

        # ── logging ──
        self.log_queue = queue.Queue()
        log_name = f"zd_server_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
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

        # --- Sunucu ---
        sf = ttk.LabelFrame(left, text=" Sunucu ", padding=6)
        sf.pack(fill="x", padx=4, pady=(0, 4))

        row = ttk.Frame(sf)
        row.pack(fill="x")
        ttk.Label(row, text="Push Port:").pack(side="left")
        self.port_entry = ttk.Entry(row, width=8)
        self.port_entry.insert(0, str(self.push_port))
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
        ttk.Checkbutton(af, text="Ident → register: true",
                         variable=self.auto_ident).pack(anchor="w")
        ttk.Checkbutton(af, text="Alive → ACK",
                         variable=self.auto_alive).pack(anchor="w")
        ttk.Checkbutton(af, text="Readout/LP Veri → ACK",
                         variable=self.auto_data_ack).pack(anchor="w")

        # --- Pull Komutları ---
        cf = ttk.LabelFrame(left, text=" Pull Komutları  (Sunucu → Cihaz) ", padding=6)
        cf.pack(fill="x", padx=4, pady=4)

        self.pull_info_lbl = ttk.Label(cf, text="⚠ Ident bekleniyor...",
                                        foreground="orange")
        self.pull_info_lbl.pack(anchor="w", pady=(0, 4))

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
        ]
        self.pull_buttons = []
        for text, cmd in cmds:
            btn = ttk.Button(cf, text=text, command=cmd, state="disabled")
            btn.pack(fill="x", pady=1)
            self.pull_buttons.append(btn)

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

    # ───────────────────── Server lifecycle ─────────────────────

    def start_server(self):
        if self.running:
            return
        try:
            port = int(self.port_entry.get())
        except ValueError:
            messagebox.showerror("Hata", "Geçerli bir port numarası girin.")
            return

        try:
            self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_sock.settimeout(1)
            self.server_sock.bind(("0.0.0.0", port))
            self.server_sock.listen(1)
        except OSError as e:
            messagebox.showerror("Hata", f"Port {port} açılamadı:\n{e}")
            return

        self.running = True
        self.push_port = port
        self.start_btn.config(state="disabled")
        self.stop_btn.config(state="normal")
        self.port_entry.config(state="disabled")
        self.status_lbl.config(text=f"● Çalışıyor  (port {port})", style="Green.TLabel")

        self.accept_th = threading.Thread(target=self._accept_loop, daemon=True)
        self.accept_th.start()
        self._log("info", f"Sunucu başlatıldı – port {port}")

    def stop_server(self):
        if not self.running:
            return
        self.running = False

        with self.lock:
            if self.device_conn:
                try:
                    self.device_conn.close()
                except OSError:
                    pass
                self.device_conn = None

        if self.server_sock:
            try:
                self.server_sock.close()
            except OSError:
                pass
            self.server_sock = None

        self.start_btn.config(state="normal")
        self.stop_btn.config(state="disabled")
        self.port_entry.config(state="normal")
        self.status_lbl.config(text="● Durduruldu", style="Red.TLabel")
        self.conn_lbl.config(text="Bağlı değil", foreground="gray")
        self._disable_pull_buttons()
        self._log("info", "Sunucu durduruldu")

    # ───────────────────── Accept / Read ─────────────────────

    def _accept_loop(self):
        while self.running:
            try:
                conn, addr = self.server_sock.accept()
                conn.settimeout(1)

                with self.lock:
                    if self.device_conn:
                        try:
                            self.device_conn.close()
                        except OSError:
                            pass
                    self.device_conn = conn
                    self.device_addr = addr

                self._log("info", f"Cihaz bağlandı: {addr[0]}:{addr[1]}")
                self.root.after(0, lambda: self.conn_lbl.config(
                    text=f"Bağlı: {addr[0]}:{addr[1]}", foreground="green"))

                self.reader_th = threading.Thread(target=self._push_reader,
                                                  args=(conn,), daemon=True)
                self.reader_th.start()

            except socket.timeout:
                continue
            except OSError:
                break

    def _push_reader(self, conn):
        buffer = ""
        decoder = json.JSONDecoder()

        while self.running:
            try:
                data = conn.recv(RECV_BUF)
                if not data:
                    break
                buffer += data.decode("utf-8", errors="replace")

                pos = 0
                while pos < len(buffer):
                    while pos < len(buffer) and buffer[pos] in " \t\r\n":
                        pos += 1
                    if pos >= len(buffer):
                        break
                    try:
                        obj, end = decoder.raw_decode(buffer, pos)
                        pos = end
                        self._handle_push_msg(obj)
                    except json.JSONDecodeError:
                        break
                buffer = buffer[pos:]

            except socket.timeout:
                continue
            except OSError:
                break

        self._log("info", "Cihaz bağlantısı kesildi")
        self.root.after(0, lambda: self.conn_lbl.config(
            text="Bağlı değil", foreground="gray"))
        self.root.after(0, self._disable_pull_buttons)

        with self.lock:
            if self.device_conn is conn:
                self.device_conn = None

    # ───────────────── Push message handling ─────────────────

    def _handle_push_msg(self, msg):
        pretty = json.dumps(msg, indent=2, ensure_ascii=False)
        self._log("recv", pretty, "PUSH")

        func = msg.get("function", "")

        # extract device info
        dev = msg.get("device", {})
        sn = dev.get("serialNumber", "")
        if sn:
            self.device_serial.set(sn)

        if func == "ident":
            resp = msg.get("response", {})
            pull_ip = resp.get("pullIP", "")
            pull_port = resp.get("pullPort", 0)

            # Use the actual TCP connection source IP instead of the
            # device-reported pullIP (which may be 127.0.0.1 / 0.0.0.0).
            conn_ip = ""
            with self.lock:
                if self.device_addr:
                    conn_ip = self.device_addr[0]

            if conn_ip and pull_ip in ("", "127.0.0.1", "0.0.0.0"):
                effective_ip = conn_ip
            else:
                effective_ip = pull_ip if pull_ip else conn_ip

            if effective_ip:
                self.pull_ip.set(effective_ip)
            if pull_port:
                self.pull_port_var.set(str(pull_port))

            self.device_identified = True

            self._log("info",
                      f"Ident alındı – SN:{sn}  registered:{resp.get('registered')}"
                      f"  pullIP:{effective_ip} (cihaz bildirdi:{pull_ip})"
                      f"  pullPort:{pull_port}")

            # Enable pull command buttons
            self.root.after(0, self._enable_pull_buttons)

            if self.auto_ident.get():
                ack = self._device_header()
                ack["function"] = "ident"
                ack["response"] = {"register": True}
                self._send_push(ack)

        elif func == "alive":
            self._log("info", "Alive alındı")
            if self.auto_alive.get():
                ack = self._device_header()
                ack["function"] = "ack"
                self._send_push(ack)

        elif func in ("readout", "loadprofile"):
            pnum = msg.get("packetNum", 0)
            pstream = msg.get("packetStream", False)
            self._log("info",
                      f"{func} verisi alındı – paket:{pnum}  stream:{pstream}")
            if not pstream and self.auto_data_ack.get():
                ack = self._device_header()
                ack["function"] = "ack"
                self._send_push(ack)

    def _enable_pull_buttons(self):
        for btn in self.pull_buttons:
            btn.config(state="normal")
        self.pull_info_lbl.config(
            text=f"✔ Pull: {self.pull_ip.get()}:{self.pull_port_var.get()}",
            foreground="green")

    def _disable_pull_buttons(self):
        for btn in self.pull_buttons:
            btn.config(state="disabled")
        self.pull_info_lbl.config(text="⚠ Ident bekleniyor...", foreground="orange")
        self.device_identified = False

    def _send_push(self, msg_dict):
        with self.lock:
            conn = self.device_conn
        if conn is None:
            self._log("error", "Push gönderim hatası: cihaz bağlı değil")
            return
        try:
            raw = json.dumps(msg_dict, ensure_ascii=False)
            conn.sendall(raw.encode("utf-8"))
            self._log("sent", json.dumps(msg_dict, indent=2, ensure_ascii=False), "PUSH")
        except OSError as e:
            self._log("error", f"Push gönderim hatası: {e}")

    # ──────────────── Pull command sender ────────────────

    def _send_pull(self, msg_dict, expect_stream=False):
        if not self.device_identified:
            self._log("error", "Pull gönderilemez: henüz ident alınmadı")
            return

        def worker():
            sock = None
            try:
                ip = self.pull_ip.get()
                port = int(self.pull_port_var.get())
                if not ip or not port:
                    self._log("error", "Pull IP veya Port bilgisi eksik")
                    return

                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(10)
                sock.connect((ip, port))

                raw = json.dumps(msg_dict, ensure_ascii=False)
                sock.sendall(raw.encode("utf-8"))
                self._log("sent", json.dumps(msg_dict, indent=2, ensure_ascii=False), "PULL")

                buffer = ""
                decoder = json.JSONDecoder()

                while True:
                    try:
                        chunk = sock.recv(RECV_BUF)
                        if not chunk:
                            break
                        buffer += chunk.decode("utf-8", errors="replace")

                        pos = 0
                        while pos < len(buffer):
                            while pos < len(buffer) and buffer[pos] in " \t\r\n":
                                pos += 1
                            if pos >= len(buffer):
                                break
                            try:
                                obj, end = decoder.raw_decode(buffer, pos)
                                pos = end
                                pretty = json.dumps(obj, indent=2, ensure_ascii=False)
                                self._log("recv", pretty, "PULL")

                                stream = obj.get("packetStream", False)
                                if not expect_stream or not stream:
                                    buffer = buffer[pos:]
                                    sock.close()
                                    return
                            except json.JSONDecodeError:
                                break
                        buffer = buffer[pos:]

                    except socket.timeout:
                        break

                sock.close()
            except Exception as e:
                self._log("error", f"Pull komutu hatası: {e}")
                if sock:
                    try:
                        sock.close()
                    except OSError:
                        pass

        threading.Thread(target=worker, daemon=True).start()

    # ──────────────── Helper builders ────────────────

    def _device_header(self):
        return {
            "device": {
                "flag": self.device_flag,
                "serialNumber": self.device_serial.get(),
            }
        }

    def _build_pull_msg(self, function, **extra):
        msg = self._device_header()
        msg["function"] = function
        msg.update(extra)
        return msg

    # ──────────────── Command handlers ────────────────

    def cmd_log_request(self):
        msg = self._build_pull_msg("log")
        self._send_pull(msg, expect_stream=True)

    def cmd_setting_server(self):
        vals = self._show_dialog("Sunucu Ayarla", [
            ("IP",   "192.168.1.100", "str"),
            ("Port", "8722",          "str"),
        ])
        if not vals:
            return
        msg = self._build_pull_msg("setting", request={
            "Server": {
                "ip": vals["IP"],
                "port": int(vals["Port"]),
            }
        })
        self._send_pull(msg)

    def cmd_add_meter(self):
        vals = self._show_dialog("Sayaç Ekle", [
            ("protocol",     "IEC62056",    "str"),
            ("type",         "electricity", "str"),
            ("brand",        "MKL",         "str"),
            ("serialNumber", "12345678",    "str"),
            ("serialPort",   "port-1",      "str"),
            ("initBaud",     "300",         "str"),
            ("fixBaud",      False,         "bool"),
            ("frame",        "7E1",         "str"),
        ])
        if not vals:
            return
        meter = {
            "protocol":     vals["protocol"],
            "type":         vals["type"],
            "brand":        vals["brand"],
            "serialNumber": vals["serialNumber"],
            "serialPort":   vals["serialPort"],
            "initBaud":     int(vals["initBaud"]),
            "fixBaud":      vals["fixBaud"],
            "frame":        vals["frame"],
        }
        msg = self._build_pull_msg("setting", request={
            "meters": [{"operation": "add", "meter": meter}]
        })
        self._send_pull(msg)

    def cmd_remove_meter(self):
        vals = self._show_dialog("Sayaç Sil", [
            ("brand",        "MKL",      "str"),
            ("serialNumber", "12345678", "str"),
        ])
        if not vals:
            return
        msg = self._build_pull_msg("setting", request={
            "meters": [{
                "operation": "remove",
                "meter": {
                    "brand": vals["brand"],
                    "serialNumber": vals["serialNumber"],
                }
            }]
        })
        self._send_pull(msg)

    def cmd_fw_update(self):
        vals = self._show_dialog("Firmware Güncelle", [
            ("FTP Adresi", "ftp://user:pass@192.168.1.1:21/firmware.bin", "str"),
        ])
        if not vals:
            return
        msg = self._build_pull_msg("fwUpdate", request={
            "address": vals["FTP Adresi"]
        })
        self._send_pull(msg)

    def cmd_readout(self):
        vals = self._show_dialog("Readout İste", [
            ("Directive",         "ReadoutDirective1", "str"),
            ("Meter Serial No",   "12345678",          "str"),
        ])
        if not vals:
            return
        msg = self._build_pull_msg("readout", request={
            "directive": vals["Directive"],
            "parameters": {
                "meterSerialNumber": vals["Meter Serial No"],
            }
        })
        self._send_pull(msg)

    def cmd_loadprofile(self):
        vals = self._show_dialog("Load Profile İste", [
            ("Directive",       "ProfileDirective1",     "str"),
            ("Meter Serial No", "12345678",              "str"),
            ("Başlangıç",      "2021-06-22 00:00:00",   "str"),
            ("Bitiş",          "2021-06-22 12:05:00",   "str"),
        ])
        if not vals:
            return
        msg = self._build_pull_msg("loadprofile", request={
            "directive":         vals["Directive"],
            "METERSERIALNUMBER": vals["Meter Serial No"],
            "startDate":         vals["Başlangıç"],
            "endDate":           vals["Bitiş"],
        })
        self._send_pull(msg)

    def cmd_directive_list(self):
        vals = self._show_dialog("Directive Listele", [
            ("Filtre ID (* = tümü)", "*", "str"),
        ])
        if not vals:
            return
        msg = self._build_pull_msg("directiveList", request={
            "filter": {"id": vals["Filtre ID (* = tümü)"]}
        })
        self._send_pull(msg, expect_stream=True)

    def cmd_directive_add(self):
        default_dir = json.dumps({
            "id": "ReadoutDirective1",
            "directive": [
                {"operation": "setBaud",    "parameter": "300"},
                {"operation": "setFraming", "parameter": "7E1"},
                {"operation": "sendData",   "parameter": "/?![0D][0A]"},
                {"operation": "wait",       "parameter": "10"},
                {"operation": "readData",   "parameter": "id"},
                {"operation": "sendData",   "parameter": "[06]050[0D][0A]"},
                {"operation": "setBaud",    "parameter": "9600"},
                {"operation": "wait",       "parameter": "600"},
                {"operation": "readData",   "parameter": "rawData"},
            ]
        }, indent=2, ensure_ascii=False)

        vals = self._show_dialog("Directive Ekle", [
            ("Directive (JSON)", default_dir, "text"),
        ])
        if not vals:
            return
        try:
            dir_obj = json.loads(vals["Directive (JSON)"])
        except json.JSONDecodeError as e:
            messagebox.showerror("JSON Hatası", f"Geçersiz JSON:\n{e}")
            return

        msg = self._build_pull_msg("directiveAdd", request={
            "directive": dir_obj
        })
        self._send_pull(msg)

    def cmd_directive_delete(self):
        vals = self._show_dialog("Directive Sil", [
            ("Directive ID (* = tümü)", "ReadoutDirective1", "str"),
        ])
        if not vals:
            return
        msg = self._build_pull_msg("directiveDelete", request={
            "filter": {"id": vals["Directive ID (* = tümü)"]}
        })
        self._send_pull(msg)

    # ──────────────── Input dialog ────────────────

    def _show_dialog(self, title, fields):
        """
        fields: list of (label, default_value, type)
        type: 'str', 'bool', 'text'
        Returns dict {label: value} or None.
        """
        dlg = tk.Toplevel(self.root)
        dlg.title(title)
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.resizable(False, False)

        result = {"ok": False, "values": {}}
        entries = {}

        frame = ttk.Frame(dlg, padding=12)
        frame.pack(fill="both", expand=True)

        for i, (label, default, ftype) in enumerate(fields):
            ttk.Label(frame, text=label + ":").grid(
                row=i, column=0, sticky="nw", padx=(0, 8), pady=3)

            if ftype == "bool":
                var = tk.BooleanVar(value=default)
                w = ttk.Checkbutton(frame, variable=var)
                entries[label] = ("bool", var)
            elif ftype == "text":
                w = tk.Text(frame, width=55, height=12,
                            font=("Consolas", 9))
                w.insert("1.0", str(default))
                entries[label] = ("text", w)
            else:
                w = ttk.Entry(frame, width=45)
                w.insert(0, str(default))
                entries[label] = ("entry", w)

            w.grid(row=i, column=1, sticky="ew", pady=3)

        frame.columnconfigure(1, weight=1)

        def on_ok(_=None):
            result["ok"] = True
            for lbl, (ft, widget) in entries.items():
                if ft == "bool":
                    result["values"][lbl] = widget.get()
                elif ft == "text":
                    result["values"][lbl] = widget.get("1.0", "end-1c")
                else:
                    result["values"][lbl] = widget.get()
            dlg.destroy()

        def on_cancel(_=None):
            dlg.destroy()

        btn_frame = ttk.Frame(frame)
        btn_frame.grid(row=len(fields), column=0, columnspan=2, pady=(12, 0))
        ttk.Button(btn_frame, text="  Gönder  ", command=on_ok).pack(side="left", padx=4)
        ttk.Button(btn_frame, text="  İptal  ", command=on_cancel).pack(side="left", padx=4)

        dlg.bind("<Return>", on_ok)
        dlg.bind("<Escape>", on_cancel)

        # center on parent
        dlg.update_idletasks()
        pw = self.root.winfo_width()
        ph = self.root.winfo_height()
        px = self.root.winfo_x()
        py = self.root.winfo_y()
        dw = dlg.winfo_width()
        dh = dlg.winfo_height()
        dlg.geometry(f"+{px + (pw - dw) // 2}+{py + (ph - dh) // 2}")

        dlg.wait_window()
        return result["values"] if result["ok"] else None

    # ──────────────── Logging ────────────────

    def _log(self, tag, text, channel=""):
        ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
        self.log_queue.put((ts, tag, channel, text))

    def _poll_log(self):
        while not self.log_queue.empty():
            ts, tag, channel, text = self.log_queue.get_nowait()

            ch_str = f"[{channel}] " if channel else ""

            if tag == "sent":
                prefix = f"[{ts}] {ch_str}→ SENT"
                color = "sent"
            elif tag == "recv":
                prefix = f"[{ts}] {ch_str}← RECV"
                color = "recv"
            elif tag == "error":
                prefix = f"[{ts}] {ch_str}✖ ERROR"
                color = "error"
            else:
                prefix = f"[{ts}] {ch_str}ℹ INFO"
                color = "info"

            line = f"{prefix}\n{text}\n{'─' * 60}\n"

            self.log_text.config(state="normal")
            self.log_text.insert("end", f"{prefix}\n", color)
            self.log_text.insert("end", f"{text}\n")
            self.log_text.insert("end", f"{'─' * 60}\n", "info")
            self.log_text.see("end")
            self.log_text.config(state="disabled")

            # file
            try:
                self.log_file.write(line)
                self.log_file.flush()
            except Exception:
                pass

        self.root.after(100, self._poll_log)

    def clear_log(self):
        self.log_text.config(state="normal")
        self.log_text.delete("1.0", "end")
        self.log_text.config(state="disabled")

    # ──────────────── Cleanup ────────────────

    def on_close(self):
        self.stop_server()
        try:
            self.log_file.close()
        except Exception:
            pass
        self.root.destroy()


# ═══════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(
        description="Protocol ZD Test Server")
    parser.add_argument(
        "port", type=int, nargs="?", default=DEFAULT_PORT,
        help=f"Push sunucu portu (varsayılan: {DEFAULT_PORT})")
    args = parser.parse_args()

    root = tk.Tk()
    app = ProtocolTestServer(root, args.port)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
