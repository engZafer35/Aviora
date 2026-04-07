#!/usr/bin/env python3
"""
AVI OrionTLV Protocol Test Server
Binary TLV-over-TCP protocol test server for IoT meter gateway.

Usage:
    python ProtocolOrionTLV_TestServer.py [port]
    python ProtocolOrionTLV_TestServer.py 8723
"""

import sys
import os
import struct
import socket
import threading
import queue
import json
import time
import argparse
from datetime import datetime

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox

RECV_BUF = 4096
# Pull (sunucu → cihaz) yanıtı için toplam bekleme; tam TLV alınmadan EOF olursa bu süre dolana kadar beklenir (erken "kapandı" logu yok)
PULL_RESPONSE_TIMEOUT_SEC = 15.0
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_PORT = 8723

# ═══════════════════════════════════════════════════════════════
#                    TAG / Function definitions
# ═══════════════════════════════════════════════════════════════

PKT_START = ord('$')   # 0x24
PKT_END   = ord('#')   # 0x23

# Common
TAG_FLAG            = 0x0001
TAG_SERIAL_NUMBER   = 0x0002
TAG_FUNCTION        = 0x0003
TAG_TRANS_NUMBER    = 0x00FF  # session / correlation (big-endian uint16)

# Ident / Alive
TAG_REGISTERED      = 0x0101
TAG_DEVICE_BRAND    = 0x0102
TAG_DEVICE_MODEL    = 0x0103
TAG_DEVICE_DATE     = 0x0104
TAG_PULL_IP         = 0x0105
TAG_PULL_PORT       = 0x0106
TAG_REGISTER        = 0x0107

# Packet streaming
TAG_PACKET_NUM      = 0x0201
TAG_PACKET_STREAM   = 0x0202

# ACK / NACK
TAG_ACK_STATUS      = 0x0301

# Log
TAG_LOG_DATA        = 0x0401

# Meter
TAG_METER_OPERATION   = 0x0501
TAG_METER_PROTOCOL    = 0x0502
TAG_METER_TYPE        = 0x0503
TAG_METER_BRAND       = 0x0504
TAG_METER_SERIAL_NUM  = 0x0505
TAG_METER_SERIAL_PORT = 0x0506
TAG_METER_INIT_BAUD   = 0x0507
TAG_METER_FIX_BAUD    = 0x0508
TAG_METER_FRAME       = 0x0509
TAG_METER_CUSTOMER_NUM = 0x050A
TAG_METER_INDEX       = 0x050B

# Server config
TAG_SERVER_IP       = 0x0601
TAG_SERVER_PORT     = 0x0602

# Readout / Load profile
TAG_METER_ID        = 0x0701
TAG_READOUT_DATA    = 0x0702
TAG_DIRECTIVE_NAME  = 0x0703
TAG_START_DATE      = 0x0704
TAG_END_DATE        = 0x0705

# Directive
TAG_DIRECTIVE_ID    = 0x0801
TAG_DIRECTIVE_DATA  = 0x0802

# FW Update
TAG_FW_ADDRESS      = 0x0901

# Error
TAG_ERROR_CODE      = 0x0A01

# Function enum
FUNC_IDENT          = 0x01
FUNC_ALIVE          = 0x02
FUNC_ACK            = 0x03
FUNC_NACK           = 0x04
FUNC_LOG            = 0x05
FUNC_SETTING        = 0x06
FUNC_FW_UPDATE      = 0x07
FUNC_READOUT        = 0x08
FUNC_LOADPROFILE    = 0x09
FUNC_DIRECTIVE_LIST = 0x0A
FUNC_DIRECTIVE_ADD  = 0x0B
FUNC_DIRECTIVE_DEL  = 0x0C

TAG_NAMES = {
    TAG_FLAG: "FLAG", TAG_SERIAL_NUMBER: "SERIAL_NUMBER", TAG_FUNCTION: "FUNCTION",
    TAG_TRANS_NUMBER: "TRANS_NUMBER",
    TAG_REGISTERED: "REGISTERED", TAG_DEVICE_BRAND: "DEVICE_BRAND",
    TAG_DEVICE_MODEL: "DEVICE_MODEL", TAG_DEVICE_DATE: "DEVICE_DATE",
    TAG_PULL_IP: "PULL_IP", TAG_PULL_PORT: "PULL_PORT", TAG_REGISTER: "REGISTER",
    TAG_PACKET_NUM: "PACKET_NUM", TAG_PACKET_STREAM: "PACKET_STREAM",
    TAG_ACK_STATUS: "ACK_STATUS", TAG_LOG_DATA: "LOG_DATA",
    TAG_METER_OPERATION: "METER_OPERATION", TAG_METER_PROTOCOL: "METER_PROTOCOL",
    TAG_METER_TYPE: "METER_TYPE", TAG_METER_BRAND: "METER_BRAND",
    TAG_METER_SERIAL_NUM: "METER_SERIAL_NUM", TAG_METER_SERIAL_PORT: "METER_SERIAL_PORT",
    TAG_METER_INIT_BAUD: "METER_INIT_BAUD", TAG_METER_FIX_BAUD: "METER_FIX_BAUD",
    TAG_METER_FRAME: "METER_FRAME", TAG_METER_CUSTOMER_NUM: "METER_CUSTOMER_NUM",
    TAG_METER_INDEX: "METER_INDEX",
    TAG_SERVER_IP: "SERVER_IP", TAG_SERVER_PORT: "SERVER_PORT",
    TAG_METER_ID: "METER_ID", TAG_READOUT_DATA: "READOUT_DATA",
    TAG_DIRECTIVE_NAME: "DIRECTIVE_NAME", TAG_START_DATE: "START_DATE",
    TAG_END_DATE: "END_DATE",
    TAG_DIRECTIVE_ID: "DIRECTIVE_ID", TAG_DIRECTIVE_DATA: "DIRECTIVE_DATA",
    TAG_FW_ADDRESS: "FW_ADDRESS", TAG_ERROR_CODE: "ERROR_CODE",
}

FUNC_NAMES = {
    FUNC_IDENT: "IDENT", FUNC_ALIVE: "ALIVE", FUNC_ACK: "ACK", FUNC_NACK: "NACK",
    FUNC_LOG: "LOG", FUNC_SETTING: "SETTING", FUNC_FW_UPDATE: "FW_UPDATE",
    FUNC_READOUT: "READOUT", FUNC_LOADPROFILE: "LOADPROFILE",
    FUNC_DIRECTIVE_LIST: "DIRECTIVE_LIST", FUNC_DIRECTIVE_ADD: "DIRECTIVE_ADD",
    FUNC_DIRECTIVE_DEL: "DIRECTIVE_DEL",
}


# ═══════════════════════════════════════════════════════════════
#                  TLV Builder / Parser helpers
# ═══════════════════════════════════════════════════════════════

class PacketBuilder:
    """Build a $...# TLV packet."""

    def __init__(self):
        self._buf = bytearray()
        self._buf.append(PKT_START)

    def add_string(self, tag, val):
        data = val.encode("ascii", errors="replace") if val else b""
        self._add_tlv(tag, data)

    def add_bool(self, tag, val):
        self._add_tlv(tag, bytes([0x01 if val else 0x00]))

    def add_uint8(self, tag, val):
        self._add_tlv(tag, bytes([val & 0xFF]))

    def add_uint16(self, tag, val):
        self._add_tlv(tag, struct.pack("!H", val & 0xFFFF))

    def add_uint32(self, tag, val):
        self._add_tlv(tag, struct.pack("!I", val & 0xFFFFFFFF))

    def add_int16(self, tag, val):
        self._add_tlv(tag, struct.pack("!h", val))

    def add_raw(self, tag, data):
        self._add_tlv(tag, data)

    def add_device_header(self, flag, serial):
        self.add_string(TAG_FLAG, flag)
        self.add_string(TAG_SERIAL_NUMBER, serial)

    def finish(self):
        self._buf.append(PKT_END)
        return bytes(self._buf)

    def _add_tlv(self, tag, data):
        self._buf += struct.pack("!HH", tag, len(data))
        self._buf += data


def parse_packet(raw):
    """
    Parse a $...# packet into list of (tag, value_bytes) tuples.
    Returns None if invalid.
    """
    if not raw or raw[0] != PKT_START or raw[-1] != PKT_END:
        return None

    payload = raw[1:-1]
    fields = []
    pos = 0
    while pos + 4 <= len(payload):
        tag, length = struct.unpack_from("!HH", payload, pos)
        pos += 4
        if pos + length > len(payload):
            break
        val = payload[pos:pos + length]
        fields.append((tag, val))
        pos += length
    return fields


def find_tag(fields, tag):
    """Return the first value bytes for the given tag, or None."""
    for t, v in fields:
        if t == tag:
            return v
    return None


def get_string(fields, tag, default=""):
    v = find_tag(fields, tag)
    return v.decode("ascii", errors="replace") if v else default


def get_uint8(fields, tag, default=0):
    v = find_tag(fields, tag)
    return v[0] if v and len(v) >= 1 else default


def get_uint16(fields, tag, default=0):
    v = find_tag(fields, tag)
    return struct.unpack("!H", v)[0] if v and len(v) >= 2 else default


def get_uint32(fields, tag, default=0):
    v = find_tag(fields, tag)
    return struct.unpack("!I", v)[0] if v and len(v) >= 4 else default


def get_bool(fields, tag, default=False):
    v = find_tag(fields, tag)
    return (v[0] != 0) if v and len(v) >= 1 else default


def get_function(fields):
    return get_uint8(fields, TAG_FUNCTION, 0)


def minify_json_for_device(text: str) -> str:
    """
    Dialog / düzenlenebilir JSON metnini ağ üzerinde tek satır, gereksiz boşluksuz
    göndermek için sıkıştırır (json.loads → json.dumps compact).
    """
    obj = json.loads(text.strip())
    return json.dumps(obj, separators=(",", ":"), ensure_ascii=False)


def format_json_for_log_display(text: str):
    """
    Logda göstermek için JSON'u girintili metne çevirir; parse edilemezse None.
    """
    try:
        obj = json.loads(text.strip())
    except (json.JSONDecodeError, TypeError, ValueError):
        return None
    return json.dumps(obj, indent=2, ensure_ascii=False)


def format_tlv_fields(fields):
    """Pretty-print TLV fields for the log panel."""
    lines = []
    for tag, val in fields:
        tag_name = TAG_NAMES.get(tag, f"0x{tag:04X}")

        if tag == TAG_FUNCTION:
            fname = FUNC_NAMES.get(val[0], f"0x{val[0]:02X}") if len(val) == 1 else val.hex()
            lines.append(f"  {tag_name:22s}  = {fname}")
        elif tag in (TAG_REGISTERED, TAG_REGISTER, TAG_PACKET_STREAM,
                     TAG_ACK_STATUS, TAG_METER_FIX_BAUD):
            bval = (val[0] != 0) if val else False
            lines.append(f"  {tag_name:22s}  = {bval}")
        elif tag in (TAG_TRANS_NUMBER, TAG_PULL_PORT, TAG_SERVER_PORT, TAG_PACKET_NUM):
            num = struct.unpack("!H", val)[0] if len(val) == 2 else int.from_bytes(val, "big")
            lines.append(f"  {tag_name:22s}  = {num}")
        elif tag == TAG_METER_INIT_BAUD:
            num = struct.unpack("!I", val)[0] if len(val) == 4 else int.from_bytes(val, "big")
            lines.append(f"  {tag_name:22s}  = {num}")
        elif tag == TAG_METER_INDEX:
            lines.append(f"  {tag_name:22s}  = {val[0] if val else '?'}")
        elif tag == TAG_ERROR_CODE:
            num = struct.unpack("!h", val)[0] if len(val) == 2 else 0
            lines.append(f"  {tag_name:22s}  = {num}")
        elif tag == TAG_DIRECTIVE_DATA:
            s = val.decode("utf-8", errors="replace")
            pj = format_json_for_log_display(s)
            if pj:
                lines.append(f"  {tag_name:22s}  =")
                for pl in pj.splitlines():
                    lines.append(f"      {pl}")
            else:
                disp = s if len(s) <= 400 else s[:397] + "..."
                lines.append(f"  {tag_name:22s}  = {disp!r}")
        else:
            try:
                s = val.decode("utf-8", errors="replace")
                # Diğer alanlarda JSON benzeri içerik (cihaz düz JSON gönderir)
                if tag in (TAG_LOG_DATA, TAG_READOUT_DATA) or (
                    len(s) > 2 and s.lstrip()[:1] in "{["
                ):
                    pj = format_json_for_log_display(s)
                    if pj:
                        lines.append(f"  {tag_name:22s}  =")
                        for pl in pj.splitlines():
                            lines.append(f"      {pl}")
                        continue
                #if len(s) > 80:
                  #  s = s[:77] + "..."
                lines.append(f"  {tag_name:22s}  = \"{s}\"")
            except UnicodeDecodeError:
                lines.append(f"  {tag_name:22s}  = [{val.hex()}]")
    return "\n".join(lines)


def extract_packets(buf):
    """
    Extract all complete $...# packets from buffer.
    Returns (list_of_packets, remaining_buffer).
    """
    packets = []
    while True:
        start = buf.find(bytes([PKT_START]))
        if start < 0:
            buf = b""
            break
        end = buf.find(bytes([PKT_END]), start + 1)
        if end < 0:
            buf = buf[start:]
            break
        packets.append(buf[start:end + 1])
        buf = buf[end + 1:]
    return packets, buf


# ═══════════════════════════════════════════════════════════════
#                    Protocol Test Server
# ═══════════════════════════════════════════════════════════════

class OrionTLVTestServer:

    def __init__(self, root, push_port):
        self.root = root
        self.root.title("Protocol OrionTLV Test Server")
        self.root.geometry("1200x780")
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
        self.device_serial = tk.StringVar(value="12345678")
        self.device_flag = "AVI"
        self.pull_ip = tk.StringVar(value="127.0.0.1")
        self.pull_port_var = tk.StringVar(value="2622")
        self.device_identified = False

        # ── auto-response toggles ──
        self.auto_ident = tk.BooleanVar(value=True)
        self.auto_alive = tk.BooleanVar(value=True)
        self.auto_data_ack = tk.BooleanVar(value=True)

        self._pull_trans = 1  # 1..0xFFFF for pull commands (matches firmware convention)

        # ── logging ──
        self.log_queue = queue.Queue()
        log_name = f"ba_server_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
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

        # ── LEFT PANEL (dikey kaydırma — tüm kontroller dar ekranda sığsın) ──
        left = ttk.Frame(paned, width=300)
        paned.add(left, weight=0)

        left_canvas = tk.Canvas(left, highlightthickness=0, borderwidth=0)
        left_scroll = ttk.Scrollbar(left, orient="vertical", command=left_canvas.yview)
        left_canvas.configure(yscrollcommand=left_scroll.set)

        scroll_inner = ttk.Frame(left_canvas)
        left_win = left_canvas.create_window((0, 0), window=scroll_inner, anchor="nw")

        def _left_scroll_region(_event=None):
            left_canvas.configure(scrollregion=left_canvas.bbox("all"))

        def _left_canvas_width(event):
            left_canvas.itemconfigure(left_win, width=event.width)

        scroll_inner.bind("<Configure>", _left_scroll_region)
        left_canvas.bind("<Configure>", _left_canvas_width)

        def _on_left_panel_mousewheel(event):
            """Sol panel (veya altındaki widget) üzerindeyken dikey kaydır."""
            try:
                w = self.root.winfo_containing(event.x_root, event.y_root)
            except tk.TclError:
                return
            p = w
            while p:
                if p is left:
                    if getattr(event, "delta", 0):
                        left_canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
                    elif getattr(event, "num", None) == 4:
                        left_canvas.yview_scroll(-3, "units")
                    elif getattr(event, "num", None) == 5:
                        left_canvas.yview_scroll(3, "units")
                    return "break"
                p = p.master

        self.root.bind_all("<MouseWheel>", _on_left_panel_mousewheel, add="+")
        self.root.bind_all("<Button-4>", _on_left_panel_mousewheel, add="+")
        self.root.bind_all("<Button-5>", _on_left_panel_mousewheel, add="+")

        left_scroll.pack(side="right", fill="y")
        left_canvas.pack(side="left", fill="both", expand=True)

        # --- Sunucu ---
        sf = ttk.LabelFrame(scroll_inner, text=" Sunucu (OrionTLV) ", padding=6)
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
        df = ttk.LabelFrame(scroll_inner, text=" Cihaz Bilgisi ", padding=6)
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
        af = ttk.LabelFrame(scroll_inner, text=" Otomatik Cevap ", padding=6)
        af.pack(fill="x", padx=4, pady=4)
        ttk.Checkbutton(af, text="Ident → register: true",
                         variable=self.auto_ident).pack(anchor="w")
        ttk.Checkbutton(af, text="Alive → ACK",
                         variable=self.auto_alive).pack(anchor="w")
        ttk.Checkbutton(af, text="Readout/LP Veri → ACK",
                         variable=self.auto_data_ack).pack(anchor="w")

        # --- Pull Komutları ---
        cf = ttk.LabelFrame(scroll_inner, text=" Pull Komutları  (Sunucu → Cihaz) ", padding=6)
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
        ttk.Button(scroll_inner, text="Logları Temizle", command=self.clear_log).pack(
            fill="x", padx=4, pady=6)

        # ── RIGHT PANEL (log) ──
        right = ttk.Frame(paned)
        paned.add(right, weight=1)

        ttk.Label(right, text="Mesaj Logları (Binary TLV)",
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
        self.log_text.tag_configure("hex",   foreground="#CE9178")

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
        self.device_identified = True
        self._enable_pull_buttons()

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
        buf = b""

        while self.running:
            try:
                data = conn.recv(RECV_BUF)
                if not data:
                    break
                buf += data

                packets, buf = extract_packets(buf)
                for pkt in packets:
                    self._handle_push_packet(pkt)

            except socket.timeout:
                continue
            except OSError:
                break

        self._log("info", "Cihaz bağlantısı kesildi")
        self.root.after(0, lambda: self.conn_lbl.config(
            text="Bağlı değil", foreground="gray"))
        #self.root.after(0, self._disable_pull_buttons)

        with self.lock:
            if self.device_conn is conn:
                self.device_conn = None

    # ───────────────── Push message handling ─────────────────

    def _handle_push_packet(self, raw):
        fields = parse_packet(raw)
        if fields is None:
            self._log("error", f"Geçersiz paket: {raw.hex()}")
            return

        hex_dump = raw.hex().upper()
        pretty = format_tlv_fields(fields)
        self._log("recv", f"HEX: {hex_dump}\n{pretty}", "PUSH")

        func = get_function(fields)
        sn = get_string(fields, TAG_SERIAL_NUMBER)
        if sn:
            self.device_serial.set(sn)

        if func == FUNC_IDENT:
            pull_ip = get_string(fields, TAG_PULL_IP)
            pull_port = get_uint16(fields, TAG_PULL_PORT)
            registered = get_bool(fields, TAG_REGISTERED)

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

            brand = get_string(fields, TAG_DEVICE_BRAND)
            model = get_string(fields, TAG_DEVICE_MODEL)
            date = get_string(fields, TAG_DEVICE_DATE)

            self._log("info",
                      f"Ident alındı – SN:{sn}  registered:{registered}"
                      f"  brand:{brand}  model:{model}  date:{date}"
                      f"  pullIP:{effective_ip} (cihaz:{pull_ip})"
                      f"  pullPort:{pull_port}")

            self.root.after(0, self._enable_pull_buttons)

            if self.auto_ident.get():
                trans = get_uint16(fields, TAG_TRANS_NUMBER)
                pb = PacketBuilder()
                pb.add_uint16(TAG_TRANS_NUMBER, trans)
                pb.add_device_header(self.device_flag, sn)
                pb.add_uint8(TAG_FUNCTION, FUNC_IDENT)
                pb.add_bool(TAG_REGISTER, True)
                self._send_push_raw(pb.finish())

        elif func == FUNC_ALIVE:
            date = get_string(fields, TAG_DEVICE_DATE)
            self._log("info", f"Alive alındı – date:{date}")
            if self.auto_alive.get():
                trans = get_uint16(fields, TAG_TRANS_NUMBER)
                pb = PacketBuilder()
                pb.add_uint16(TAG_TRANS_NUMBER, trans)
                pb.add_device_header(self.device_flag, self.device_serial.get())
                pb.add_uint8(TAG_FUNCTION, FUNC_ACK)
                pb.add_bool(TAG_ACK_STATUS, True)
                self._send_push_raw(pb.finish())

        elif func in (FUNC_READOUT, FUNC_LOADPROFILE):
            pnum = get_uint16(fields, TAG_PACKET_NUM)
            pstream = get_bool(fields, TAG_PACKET_STREAM)
            fname = FUNC_NAMES.get(func, "?")
            self._log("info", f"{fname} verisi alındı – paket:{pnum}  stream:{pstream}")
            if not pstream and self.auto_data_ack.get():
                trans = get_uint16(fields, TAG_TRANS_NUMBER)
                pb = PacketBuilder()
                pb.add_uint16(TAG_TRANS_NUMBER, trans)
                pb.add_device_header(self.device_flag, self.device_serial.get())
                pb.add_uint8(TAG_FUNCTION, FUNC_ACK)
                pb.add_bool(TAG_ACK_STATUS, True)
                self._send_push_raw(pb.finish())

        elif func == FUNC_ACK:
            self._log("info", "ACK alındı (push)")
        elif func == FUNC_NACK:
            self._log("info", "NACK alındı (push)")

    def _enable_pull_buttons(self):
        for btn in self.pull_buttons:
            btn.config(state="enable")
        self.pull_info_lbl.config(
            text=f"✔ Pull: {self.pull_ip.get()}:{self.pull_port_var.get()}",
            foreground="green")

    def _disable_pull_buttons(self):
        for btn in self.pull_buttons:
            btn.config(state="disabled")
        self.pull_info_lbl.config(text="⚠ Ident bekleniyor...", foreground="orange")
        self.device_identified = False

    def _send_push_raw(self, packet_bytes):
        with self.lock:
            conn = self.device_conn
        if conn is None:
            self._log("error", "Push gönderim hatası: cihaz bağlı değil")
            return
        try:
            conn.sendall(packet_bytes)
            fields = parse_packet(packet_bytes)
            hex_dump = packet_bytes.hex().upper()
            pretty = format_tlv_fields(fields) if fields else ""
            self._log("sent", f"HEX: {hex_dump}\n{pretty}", "PUSH")
        except OSError as e:
            self._log("error", f"Push gönderim hatası: {e}")

    # ──────────────── Pull command sender ────────────────

    def _send_pull(self, packet_bytes, expect_stream=False):
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
                try:
                    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                except OSError:
                    pass
                sock.settimeout(PULL_RESPONSE_TIMEOUT_SEC)
                sock.connect((ip, port))

                sock.sendall(packet_bytes)

                fields = parse_packet(packet_bytes)
                hex_dump = packet_bytes.hex().upper()
                pretty = format_tlv_fields(fields) if fields else ""
                self._log("sent", f"HEX: {hex_dump}\n{pretty}", "PULL")

                # Cevap gelene kadar (veya toplam süre dolana kadar) bekle
                deadline = time.monotonic() + PULL_RESPONSE_TIMEOUT_SEC
                buf = b""
                got_complete_response = False

                while True:
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        self._log(
                            "error",
                            f"Pull: {PULL_RESPONSE_TIMEOUT_SEC:.0f} sn içinde yanıt alınamadı",
                        )
                        return

                    remaining = max(remaining, 0.001)
                    sock.settimeout(remaining)
                    try:
                        chunk = sock.recv(RECV_BUF)
                    except socket.timeout:
                        self._log(
                            "error",
                            f"Pull: {PULL_RESPONSE_TIMEOUT_SEC:.0f} sn içinde yanıt alınamadı",
                        )
                        return

                    if not chunk:
                        # TCP EOF: Henüz tam bir TLV yoksa bu logu hemen gösterme — bazı cihazlar
                        # yanıtı geciktirir veya kısa süre sonra gönderir; süre dolana kadar bekle.
                        if not got_complete_response:
                            wr = deadline - time.monotonic()
                            if wr > 0:
                                time.sleep(wr)
                            self._log(
                                "error",
                                "Pull: yanıt alınamadı (süre doldu veya cihaz yanıt göndermeden bağlantıyı kapattı)",
                            )
                            return
                        self._log(
                            "info",
                            "Pull: karşı uç bağlantıyı kapattı (yanıt alındıktan sonra)",
                        )
                        return

                    buf += chunk
                    packets, buf = extract_packets(buf)
                    for pkt in packets:
                        got_complete_response = True
                        pkt_fields = parse_packet(pkt)
                        if pkt_fields:
                            hex_d = pkt.hex().upper()
                            pr = format_tlv_fields(pkt_fields)
                            self._log("recv", f"HEX: {hex_d}\n{pr}", "PULL")

                        stream = (
                            get_bool(pkt_fields, TAG_PACKET_STREAM)
                            if pkt_fields
                            else False
                        )
                        # Tek yanıt (ACK/NACK vb.): ilk tam pakette biter. Akış: stream=True iken okumaya devam
                        if not expect_stream or not stream:
                            return

            except Exception as e:
                self._log("error", f"Pull komutu hatası: {e}")
            finally:
                if sock:
                    try:
                        sock.close()
                    except OSError:
                        pass

        threading.Thread(target=worker, daemon=True).start()

    # ──────────────── Helper builders ────────────────

    def _next_pull_trans(self):
        t = self._pull_trans
        self._pull_trans = (self._pull_trans % 0xFFFF) + 1
        if self._pull_trans == 0:
            self._pull_trans = 1
        return t

    def _build_pull_packet(self, func_id, extra_fn=None):
        pb = PacketBuilder()
        pb.add_uint16(TAG_TRANS_NUMBER, self._next_pull_trans())
        pb.add_device_header(self.device_flag, self.device_serial.get())
        pb.add_uint8(TAG_FUNCTION, func_id)
        if extra_fn:
            extra_fn(pb)
        return pb.finish()

    # ──────────────── Command handlers ────────────────

    def cmd_log_request(self):
        pkt = self._build_pull_packet(FUNC_LOG)
        self._send_pull(pkt, expect_stream=True)

    def cmd_setting_server(self):
        vals = self._show_dialog("Sunucu Ayarla", [
            ("IP",   "127.0.0.1", "str"),
            ("Port", "8723",          "str"),
        ])
        if not vals:
            return

        def extra(pb):
            pb.add_string(TAG_SERVER_IP, vals["IP"])
            pb.add_uint16(TAG_SERVER_PORT, int(vals["Port"]))

        pkt = self._build_pull_packet(FUNC_SETTING, extra)
        self._send_pull(pkt)

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

        def extra(pb):
            pb.add_uint8(TAG_METER_INDEX, 0)
            pb.add_string(TAG_METER_OPERATION, "add")
            pb.add_string(TAG_METER_PROTOCOL, vals["protocol"])
            pb.add_string(TAG_METER_TYPE, vals["type"])
            pb.add_string(TAG_METER_BRAND, vals["brand"])
            pb.add_string(TAG_METER_SERIAL_NUM, vals["serialNumber"])
            pb.add_string(TAG_METER_SERIAL_PORT, vals["serialPort"])
            pb.add_uint32(TAG_METER_INIT_BAUD, int(vals["initBaud"]))
            pb.add_bool(TAG_METER_FIX_BAUD, vals["fixBaud"])
            pb.add_string(TAG_METER_FRAME, vals["frame"])

        pkt = self._build_pull_packet(FUNC_SETTING, extra)
        self._send_pull(pkt)

    def cmd_remove_meter(self):
        vals = self._show_dialog("Sayaç Sil", [
            ("brand",        "MKL",      "str"),
            ("serialNumber", "12345678", "str"),
        ])
        if not vals:
            return

        def extra(pb):
            pb.add_uint8(TAG_METER_INDEX, 0)
            pb.add_string(TAG_METER_OPERATION, "remove")
            pb.add_string(TAG_METER_BRAND, vals["brand"])
            pb.add_string(TAG_METER_SERIAL_NUM, vals["serialNumber"])

        pkt = self._build_pull_packet(FUNC_SETTING, extra)
        self._send_pull(pkt)

    def cmd_fw_update(self):
        vals = self._show_dialog("Firmware Güncelle", [
            ("FTP Adresi", "ftp://user:pass@192.168.1.1:21/firmware.bin", "str"),
        ])
        if not vals:
            return

        def extra(pb):
            pb.add_string(TAG_FW_ADDRESS, vals["FTP Adresi"])

        pkt = self._build_pull_packet(FUNC_FW_UPDATE, extra)
        self._send_pull(pkt)

    def cmd_readout(self):
        vals = self._show_dialog("Readout İste", [
            ("Directive",         "ReadoutDirective1", "str"),
            ("Meter Serial No",   "12345678",          "str"),
        ])
        if not vals:
            return

        def extra(pb):
            pb.add_string(TAG_DIRECTIVE_NAME, vals["Directive"])
            pb.add_string(TAG_METER_SERIAL_NUM, vals["Meter Serial No"])

        pkt = self._build_pull_packet(FUNC_READOUT, extra)
        self._send_pull(pkt)

    def cmd_loadprofile(self):
        vals = self._show_dialog("Load Profile İste", [
            ("Directive",       "ProfileDirective1",     "str"),
            ("Meter Serial No", "12345678",              "str"),
            ("Başlangıç",      "2026-04-07 00:00:00",   "str"),
            ("Bitiş",          "2026-04-07 18:05:00",   "str"),
        ])
        if not vals:
            return

        def extra(pb):
            pb.add_string(TAG_DIRECTIVE_NAME, vals["Directive"])
            pb.add_string(TAG_METER_SERIAL_NUM, vals["Meter Serial No"])
            pb.add_string(TAG_START_DATE, vals["Başlangıç"])
            pb.add_string(TAG_END_DATE, vals["Bitiş"])

        pkt = self._build_pull_packet(FUNC_LOADPROFILE, extra)
        self._send_pull(pkt)

    def cmd_directive_list(self):
        vals = self._show_dialog("Directive Listele", [
            ("Filtre ID (* = tümü)", "*", "str"),
        ])
        if not vals:
            return

        def extra(pb):
            pb.add_string(TAG_DIRECTIVE_ID, vals["Filtre ID (* = tümü)"])

        pkt = self._build_pull_packet(FUNC_DIRECTIVE_LIST, extra)
        self._send_pull(pkt, expect_stream=True)

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
            ("Directive (JSON blob)", default_dir, "text", (62, 22)),
        ])
        if not vals:
            return

        raw_json = vals["Directive (JSON blob)"]
        try:
            wire_payload = minify_json_for_device(raw_json)
        except json.JSONDecodeError as e:
            messagebox.showerror(
                "JSON hatası",
                f"Geçerli JSON değil; düzeltip tekrar deneyin.\n\n{e}",
            )
            return

        def extra(pb):
            pb.add_string(TAG_DIRECTIVE_DATA, wire_payload)

        pkt = self._build_pull_packet(FUNC_DIRECTIVE_ADD, extra)
        self._send_pull(pkt)

    def cmd_directive_delete(self):
        vals = self._show_dialog("Directive Sil", [
            ("Directive ID (* = tümü)", "ReadoutDirective1", "str"),
        ])
        if not vals:
            return

        def extra(pb):
            pb.add_string(TAG_DIRECTIVE_ID, vals["Directive ID (* = tümü)"])

        pkt = self._build_pull_packet(FUNC_DIRECTIVE_DEL, extra)
        self._send_pull(pkt)

    # ──────────────── Input dialog ────────────────

    def _show_dialog(self, title, fields):
        dlg = tk.Toplevel(self.root)
        dlg.title(title)
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.resizable(False, False)

        result = {"ok": False, "values": {}}
        entries = {}

        frame = ttk.Frame(dlg, padding=12)
        frame.pack(fill="both", expand=True)

        for i, row in enumerate(fields):
            if len(row) >= 4:
                label, default, ftype, text_size = row[0], row[1], row[2], row[3]
            else:
                label, default, ftype = row
                text_size = (55, 12)

            ttk.Label(frame, text=label + ":").grid(
                row=i, column=0, sticky="nw", padx=(0, 8), pady=3)

            if ftype == "bool":
                var = tk.BooleanVar(value=default)
                w = ttk.Checkbutton(frame, variable=var)
                entries[label] = ("bool", var)
            elif ftype == "text":
                tw, th = text_size
                w = tk.Text(
                    frame, width=tw, height=th, font=("Consolas", 9), wrap="none"
                )
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
        description="Protocol OrionTLV Test Server (Binary TLV)")
    parser.add_argument(
        "port", type=int, nargs="?", default=DEFAULT_PORT,
        help=f"Push sunucu portu (varsayılan: {DEFAULT_PORT})")
    args = parser.parse_args()

    root = tk.Tk()
    app = OrionTLVTestServer(root, args.port)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
