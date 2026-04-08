#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generate protocol headers from Customers/<customer>/Protocol/protocol_config.json:

- Customers/<customer>/Protocol/Cus_Protocol_Config.h  (macros, INIT_*, sensors)
- Customers/Protocol_Config.h                         (shim that includes the customer file)

Example:
  python Customers/generate_protocol_config.py --customer ZD_0101
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import os
from datetime import datetime
from pathlib import Path

def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def protocol_key_to_macro_prefix(protocol_key: str) -> str:
    """protocolRigelMq -> RIGEL_MQ, protocolOrionTlv -> ORION_TLV"""
    name = protocol_key[8:] if protocol_key.startswith("protocol") else protocol_key
    parts: list[str] = []
    for i, c in enumerate(name):
        if c.isupper() and i > 0 and name[i - 1].islower():
            parts.append("_")
        parts.append(c)
    return "".join(parts).upper()


def derive_sensor_init_from_read(read_func: str | None) -> str | None:
    if not read_func:
        return None
    if read_func.endswith("Read"):
        return read_func[:-4] + "Init"
    return read_func + "Init"


def c_include_line(path: str) -> str:
    return f'#include "{path}"'


def effective_protocol_settings(p: dict) -> dict:
    """
    Merge top-level protocol fields with nested ``connection`` for #define generation.
    Supports legacy JSON (no ``connection``) and new shape:

    - connection.type ``mqtt``: mqttBroker, deviceIpAddr, pullPort, optional mqtt topics
    - connection.type ``tcp``: serverIpAddr, pushPort, deviceIpAddr, pullPort
    """
    out = {k: v for k, v in p.items() if k != "connection"}
    conn = p.get("connection")
    if not conn or not isinstance(conn, dict):
        return out

    ctype = (conn.get("type") or "").strip().lower()
    if ctype == "mqtt":
        if "mqttBroker" in conn:
            out["mqttBroker"] = conn["mqttBroker"]
        if "deviceIpAddr" in conn:
            out["deviceIP"] = conn["deviceIpAddr"]
        elif "deviceIP" in conn:
            out["deviceIP"] = conn["deviceIP"]
        if "pullPort" in conn:
            out["pullPort"] = conn["pullPort"]
        for k in ("mqttRequestTopic", "mqttResponseTopic"):
            if k in conn:
                out[k] = conn[k]
    elif ctype == "tcp":
        if "serverIpAddr" in conn:
            out["serverIpAddr"] = conn["serverIpAddr"]
        if "pushPort" in conn:
            out["pushPort"] = conn["pushPort"]
        if "deviceIpAddr" in conn:
            out["deviceIP"] = conn["deviceIpAddr"]
        elif "deviceIP" in conn:
            out["deviceIP"] = conn["deviceIP"]
        if "pullPort" in conn:
            out["pullPort"] = conn["pullPort"]

    return out


def emit_rigel_defaults(p: dict) -> list[str]:
    lines: list[str] = []
    mb = p.get("mqttBroker") or {}
    ip = mb.get("ipAddr", "127.0.0.1")
    port = mb.get("port", 1883)
    user = mb.get("username", "")
    pwd = mb.get("password", "")
    dev_ip = p.get("deviceIP", "127.0.0.1")
    pull = p.get("pullPort", 2622)
    req_topic = p.get("mqttRequestTopic", "avi/request")
    resp_topic = p.get("mqttResponseTopic", "avi/response")
    lines.append(f'#define RG_RIGEL_DEFAULT_MQTT_BROKER_IP       "{ip}"')
    lines.append(f"#define RG_RIGEL_DEFAULT_MQTT_BROKER_PORT     ({port})")
    lines.append(f'#define RG_RIGEL_DEFAULT_MQTT_USER_NAME       "{user}"')
    lines.append(f'#define RG_RIGEL_DEFAULT_MQTT_PASSWORD        "{pwd}"')
    lines.append("")
    lines.append(f'#define RG_RIGEL_DEFAULT_DEVICE_IP            "{dev_ip}"')
    lines.append(f"#define RG_RIGEL_DEFAULT_PULL_PORT            ({pull})")
    lines.append("")
    lines.append(f'#define RG_RIGEL_DEFAULT_MQTT_REQUEST_TOPIC   "{req_topic}"')
    lines.append(f'#define RG_RIGEL_DEFAULT_MQTT_RESPONSE_TOPIC  "{resp_topic}"')
    return lines


def emit_orion_tlv_defaults(p: dict) -> list[str]:
    srv = p.get("serverIpAddr", "127.0.0.1")
    push = p.get("pushPort", 1883)
    dev = p.get("deviceIP", "127.0.0.1")
    pull = p.get("pullPort", 2622)
    return [
        f'#define ORION_TLV_DEFAULT_SERVER_IP       "{srv}"',
        f"#define ORION_TLV_DEFAULT_PUSH_PORT       ({push})",
        f'#define ORION_TLV_DEFAULT_DEVICE_IP       "{dev}"',
        f"#define ORION_TLV_DEFAULT_PULL_PORT       ({pull})",
    ]


def emit_zd_defaults(p: dict) -> list[str]:
    """ZD and Metallix TLV sources use ZD_DEFAULT_* in AppProtocolZD.h / Metallix .c"""
    srv = p.get("serverIpAddr", "127.0.0.1")
    push = p.get("pushPort", 1883)
    dev = p.get("deviceIP", "127.0.0.1")
    pull = p.get("pullPort", 2622)
    return [
        f'#define ZD_DEFAULT_SERVER_IP   "{srv}"',
        f"#define ZD_DEFAULT_PUSH_PORT   ({push})",
        f'#define ZD_DEFAULT_DEVICE_IP   "{dev}"',
        f"#define ZD_DEFAULT_PULL_PORT   ({pull})",
    ]


def emit_protocol_default_block(protocol_key: str, p: dict, zd_defaults_emitted: list[bool]) -> list[str]:
    """
    zd_defaults_emitted: single-element list used as mutable bool so ZD_DEFAULT_* are
    emitted once when both protocolZD and protocolMetallix are active (same macro names).
    """
    settings = effective_protocol_settings(p)
    if protocol_key == "protocolRigelMq":
        return emit_rigel_defaults(settings)
    if protocol_key == "protocolOrionTlv":
        return emit_orion_tlv_defaults(settings)
    if protocol_key == "protocolZD":
        zd_defaults_emitted[0] = True
        return emit_zd_defaults(settings)
    if protocol_key == "protocolMetallix":
        if zd_defaults_emitted[0]:
            return [
                "/* Metallix TLV reuses ZD_DEFAULT_* (see AppProtocolMetallixTLV.c); "
                "already defined above for protocolZD. */",
            ]
        zd_defaults_emitted[0] = True
        return emit_zd_defaults(settings)
    return [
        f"/* No generated default #defines for {protocol_key}; add manually if needed. */",
    ]


def sensor_macro_suffix(sensor_name: str) -> str:
    return sensor_name.upper()


# Meter sensörü: MeterComm arayüzü + appMeterOperationsStart — her ikisi de include edilir
METER_OPERATIONS_INCLUDE = "AppZMeterGw/Services/Protocol/inc/AppMeterOperations.h"


def build_sensor_init_macro(sensor: dict) -> tuple[str, list[str]]:
    """Returns (macro_block, include_paths)."""
    name = sensor["name"]
    suffix = sensor_macro_suffix(name)
    macro_name = f"SENSOR_{suffix}_INITIALIZE_FUNC"
    stype = sensor.get("type", "")
    includes: list[str] = []
    drv = sensor.get("drvSrcPath")
    if drv:
        includes.append(drv)

    if stype == "meter":
        init_f = sensor.get("initFunc")
        write_f = sensor.get("writeFunc")
        read_f = sensor.get("readFunc")
        baud_f = sensor.get("setBaudrateFunc")
        if not all([init_f, write_f, read_f, baud_f]):
            raise ValueError(f"Sensor '{name}' (meter): initFunc, writeFunc, readFunc, setBaudrateFunc required")
        block = f"""#define {macro_name}(setErrorFlag)   \\
                                    do{{ \\
                                        /* Sensor initialization code here */ \\
                                        MeterCommInterface_t meterComm = {{ \\
                                            .meterCommInit        = {init_f}, \\
                                            .meterCommSend        = {write_f}, \\
                                            .meterCommReceive     = {read_f}, \\
                                            .meterCommSetBaudrate = {baud_f}, \\
                                        }}; \\
                                        setErrorFlag |= appMeterOperationsStart(&meterComm); \\
                                    }} while(0)"""
        if METER_OPERATIONS_INCLUDE not in includes:
            includes.append(METER_OPERATIONS_INCLUDE)
        return block, includes

    if stype in ("temperature", "humidity"):
        struct = "SensorTemperature_t" if stype == "temperature" else "SensorHumidity_t"
        init_f = sensor.get("initFunc") or derive_sensor_init_from_read(sensor.get("readFunc"))
        read_f = sensor.get("readFunc")
        write_f = sensor.get("writeFunc")
        erase_f = sensor.get("eraseFunc")
        sync_f = sensor.get("syncFunc")
        if not all([init_f, read_f, write_f, erase_f, sync_f]):
            raise ValueError(
                f"Sensor '{name}' ({stype}): need initFunc or derivable readFunc, "
                "readFunc, writeFunc, eraseFunc, syncFunc"
            )
        var = re.sub(r"[^a-zA-Z0-9_]", "_", name)
        block = f"""#define {macro_name}(setErrorFlag)   \\
                                    do{{ \\
                                        {struct} {var} = {{ \\
                                            .sensorInit        = {init_f}, \\
                                            .sensorRead        = {read_f}, \\
                                            .sensorWrite       = {write_f}, \\
                                            .sensorErase       = {erase_f}, \\
                                            .sensorSync        = {sync_f}, \\
                                        }}; \\
                                        setErrorFlag |= appSensorOperationsStart(&{var}); \\
                                    }} while(0)"""
        return block, includes

    raise ValueError(f"Unsupported sensor type '{stype}' for sensor '{name}'")


def generate_header(data: dict) -> str:
    proto_section = data.get("protocol") or {}
    if not proto_section.get("use", True):
        protocols: dict = {}
        active_protocol_items: list[tuple[str, dict]] = []
    else:
        protocols = (proto_section.get("protocols") or {})
        active_protocol_items = [(k, v) for k, v in protocols.items() if v.get("use")]

    sensors_list = proto_section.get("sensors") or []
    sensors_by_name = {s["name"]: s for s in sensors_list if "name" in s}

    linked_order: list[str] = []
    for _key, p in active_protocol_items:
        ln = p.get("linkedSensorName")
        if ln and ln not in linked_order:
            linked_order.append(ln)

    active_sensor_defs: list[dict] = []
    for sn in linked_order:
        if sn not in sensors_by_name:
            raise ValueError(f"linkedSensorName '{sn}' not found in protocol.sensors[]")
        active_sensor_defs.append(sensors_by_name[sn])

    lines: list[str] = []
    lines.append("/******************************************************************************")
    lines.append("* designed by  : Zafer Satilmis")
    lines.append("* hype-man     : Pentagram/Mezarkabul - Secret Missile")
    lines.append(f"* customer     : { data.get('customer') }")
    lines.append(f"* version      : { data.get('version') }")
    lines.append(f"* releaseDate  : { data.get('releaseDate') }")
    lines.append(f"* generated by : { os.path.basename(__file__) }")
    lines.append(f"* generated on : { datetime.now().strftime("%Y-%m-%d %H:%M:%S") }")


    lines.append("*******************************************************************************/")
    lines.append("")
    lines.append("/******************************************************************************")
    lines.append("* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY")
    lines.append(f"* Customer Name: { data.get('customer') }")
    lines.append(f"* Generated from customer protocol_config.json. Only used units are included.")
    lines.append("******************************************************************************/")
    lines.append("/****************************** IFNDEF & DEFINE ******************************/")
    lines.append("#ifndef __CUS_PROTOCOL_CONFIG_H__")
    lines.append("#define __CUS_PROTOCOL_CONFIG_H__")
    lines.append("/********************************* INCLUDES **********************************/")
    lines.append('#include "Project_Conf.h"')
    lines.append("/***************************** MACRO DEFINITIONS *****************************/")
    lines.append("")
    lines.append(f"#define ACTIVE_PROTOCOL_NUMBER ({len(active_protocol_items)})")
    lines.append("")

    zd_defaults_emitted: list[bool] = [False]
    for protocol_key, p in active_protocol_items:
        pf = p.get("protocolFunc") or {}
        src = pf.get("sourcePath")
        if not src:
            raise ValueError(f"protocol '{protocol_key}': protocolFunc.sourcePath missing")
        prefix = protocol_key_to_macro_prefix(protocol_key)
        init_fn = pf.get("initFunc")
        start_fn = pf.get("startFunc")
        stop_fn = pf.get("stopFunc")
        put_fn = pf.get("putIncomingFunc")
        if not all([init_fn, start_fn, stop_fn, put_fn]):
            raise ValueError(f"protocol '{protocol_key}': protocolFunc must define init/start/stop/putIncoming")
        lines.append(f"/****** PROTOCOL - {prefix} *****************************/")
        lines.append(c_include_line(src))
        lines.append(f"#define APP_{prefix}_PROTOCOL_INIT_FUNC(serialNumber)   {init_fn}(serialNumber)")
        lines.append(f"#define APP_{prefix}_PROTOCOL_START_FUNC()              {start_fn}()")
        lines.append(f"#define APP_{prefix}_PROTOCOL_STOP_FUNC()               {stop_fn}()")
        lines.append(
            f"#define APP_{prefix}_PROTOCOL_PUT_INCOMING_FUNC(channel, data, dataLength) "
            f"{put_fn}(channel, data, dataLength)"
        )
        lines.append("")
        for dline in emit_protocol_default_block(protocol_key, p, zd_defaults_emitted):
            lines.append(dline)
        lines.append("")

    # INIT_PROTOCOLS
    lines.append("/******* PROTOCOL - INIT *****************************/")
    init_inner: list[str] = ["setErrorFlag = 0;\\"]
    for _key, _p in active_protocol_items:
        prefix = protocol_key_to_macro_prefix(_key)
        init_inner.append(
            f"if(!setErrorFlag) {{setErrorFlag |= APP_{prefix}_PROTOCOL_INIT_FUNC(serialNum); zosDelayTask(1000); }} \\"
        )
        init_inner.append(
            f"if(!setErrorFlag) {{setErrorFlag |= APP_{prefix}_PROTOCOL_START_FUNC(); zosDelayTask(1000); }} \\"
        )
    init_body = "\n                                        ".join(init_inner)
    lines.append("#define INIT_PROTOCOLS(setErrorFlag, serialNum)   \\")
    lines.append("                                    do{ \\")
    lines.append(f"                                        {init_body}")
    lines.append("                                    } while(0)")
    lines.append("")
    lines.append("")
    lines.append("/********************************* SENSOR DEFINITIONS *************************/")
    lines.append(f"#define ACTIVE_SENSOR_NUMBER ({len(active_sensor_defs)})")
    lines.append("")

    sensor_includes_seen: set[str] = set()
    sensor_macros: list[str] = []
    for sensor in active_sensor_defs:
        macro, incs = build_sensor_init_macro(sensor)
        for inc in incs:
            if inc not in sensor_includes_seen:
                lines.append(c_include_line(inc))
                sensor_includes_seen.add(inc)
        sensor_macros.append(macro)
        lines.append(macro)
        lines.append("")

    init_sensor_lines: list[str] = ["setErrorFlag = 0;\\"]
    for sensor in active_sensor_defs:
        suffix = sensor_macro_suffix(sensor["name"])
        init_sensor_lines.append(f"if(!setErrorFlag) SENSOR_{suffix}_INITIALIZE_FUNC(setErrorFlag); \\")
    init_sensors_body = "\n                                        ".join(init_sensor_lines)

    lines.append("#define INIT_SENSORS(setErrorFlag)   \\")
    lines.append("                                    do{ \\")
    lines.append(f"                                        {init_sensors_body}")
    lines.append("                                    } while(0)")
    lines.append("")
    lines.append("")
    lines.append("/******************************* TYPE DEFINITIONS *****************************/")
    lines.append("")
    lines.append("/************************* GLOBAL VARIBALE REFERENCES *************************/")
    lines.append("")
    lines.append("/************************* GLOBAL FUNCTION DEFINITIONS ************************/")
    lines.append("")
    lines.append("#endif /* __CUS_PROTOCOL_CONFIG_H__ */")
    lines.append("")
    lines.append("/********************************* End Of File ********************************/")
    lines.append("")
    return "\n".join(lines)


def generate_protocol_config_shim(customer: str) -> str:
    """Customers/Protocol_Config.h — includes the selected customer's Cus_Protocol_Config.h."""
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    lines = [
        "/**",
        " * @file Protocol_Config.h",
        " * @brief AUTO-GENERATED configuration for Protocol",
        " *        AUTO-GENERATED FILE - DO NOT EDIT MANUALLY",
        " */",
        "#ifndef __APP_PROTOCOL_CONFIG_H__",
        "#define __APP_PROTOCOL_CONFIG_H__",
        "",
        f"/* generated on: {ts} */",
        f"/* customer name: {customer} */",
        "",
        f'#include "{customer}/Protocol/Cus_Protocol_Config.h"',
        "",
        "",
        "#endif /* __APP_PROTOCOL_CONFIG_H__ */",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    # Prefer UTF-8 on Windows so paths and help print reliably
    out = sys.stdout
    if hasattr(out, "reconfigure"):
        try:
            out.reconfigure(encoding="utf-8", errors="replace")
        except (OSError, ValueError, AttributeError):
            pass

    parser = argparse.ArgumentParser(
        description=(
            "Generate Cus_Protocol_Config.h from the customer's Protocol/protocol_config.json "
            "and Customers/Protocol_Config.h shim that includes that customer."
        ),
        epilog=(
            "Example:\n"
            "  python Customers/generate_protocol_config.py --customer ZD_0101\n"
            "\n"
            "Input : Customers/ZD_0101/Protocol/protocol_config.json\n"
            "Output: Customers/ZD_0101/Protocol/Cus_Protocol_Config.h\n"
            "        Customers/Protocol_Config.h"
        ),
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument(
        "--customer",
        required=True,
        metavar="NAME",
        help="Customer folder name under Customers/ (e.g. ZD_0101)",
    )
    args = parser.parse_args()

    root = repo_root()
    protocol_dir = root / "Customers" / args.customer / "Protocol"
    cfg_path = protocol_dir / "protocol_config.json"
    out_path = protocol_dir / "Cus_Protocol_Config.h"
    if not cfg_path.is_file():
        print(f"Error: config not found: {cfg_path}", file=sys.stderr)
        return 1

    with open(cfg_path, encoding="utf-8") as f:
        data = json.load(f)

    text = generate_header(data)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print(f"Wrote {out_path}")

    shim_path = root / "Customers" / "Protocol_Config.h"
    shim_text = generate_protocol_config_shim(args.customer)
    with open(shim_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(shim_text)
    print(f"Wrote {shim_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
