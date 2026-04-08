#!/usr/bin/env python3
"""
TimeService Configuration Code Generator

Purpose:
- Read customer time_config.json from Customers/<customer>/Time/
- Generate:
  - Application/AppZMeterGw/Customers/TimeService_Config.h
  - Application/AppZMeterGw/Services/TimeService/src/AppTimeService_Autogen.c

Usage:
  python generate_time_service_code.py --customer ZD_0101

Design:
- Avoid customer-specific #if/#else in core code.
- Select pre-written, testable modules by generating a small wiring/include file.
- linuxLocalTime: when timeService.use and extRTC/intRTC are off and linuxLocalTime.use is
  true, backend uses LINUX_LOCAL_TIME_* macros from driverPath (e.g. Linux_DateTime.h);
  otherwise soft tick is used when no RTC is selected.
"""

import argparse
import json
import os
import re
from datetime import datetime
from pathlib import Path


def parse_timezone_to_minutes(tz: str) -> int:
    """
    Parse strings like:
      UTC+3, UTC-5, UTC+3:30, UTC-4:00
    Return offset minutes from UTC to local time.
    """
    if not tz:
        return 0
    s = tz.strip().upper()
    if s == "UTC":
        return 0

    m = re.fullmatch(r"UTC([+-])(\d{1,2})(?::(\d{1,2}))?", s)
    if not m:
        raise ValueError(f"Unsupported timeZone format: {tz!r}")

    sign = 1 if m.group(1) == "+" else -1
    hours = int(m.group(2))
    minutes = int(m.group(3) or "0")

    if hours > 14 or minutes >= 60:
        raise ValueError(f"Invalid timeZone value: {tz!r}")

    return sign * (hours * 60 + minutes)


def _extract_driver_macros(driver_path: Path, block_marker: str) -> list[str]:
    """
    Extract #define lines from driver file between
    /** block_marker */ and /** end of block_marker */.
    """
    if not driver_path.exists():
        return []
    text = driver_path.read_text(encoding="utf-8")
    start = f"/** {block_marker} */"
    end = f"/** end of {block_marker} */"
    i = text.find(start)
    if i < 0:
        return []
    i += len(start)
    j = text.find(end, i)
    if j < 0:
        return []
    block = text[i:j].strip()
    lines = [ln.strip() for ln in block.splitlines() if ln.strip().startswith("#define")]
    return lines


def generate_cus_config_h(cfg: dict, repo_root: Path) -> str:
    ts = cfg.get("timeService", {})

    use = bool(ts.get("use", True))
    tz_str = ts.get("timeZone", "UTC")
    tz_min = parse_timezone_to_minutes(tz_str)

    ntp = ts.get("ntp", {})
    ntp_use = use and bool(ntp.get("use", False))
    ntp_host = str(ntp.get("ipAddr", "pool.ntp.org"))
    ntp_port = int(ntp.get("port", 123))
    ntp_period = int(ntp.get("updatePeriodMin", 10))

    int_rtc = ts.get("intRTC", {})
    ext_rtc = ts.get("extRTC", {})
    int_use = use and bool(int_rtc.get("use", False))
    ext_use = use and bool(ext_rtc.get("use", False))

    linux_lt = ts.get("linuxLocalTime", {})
    linux_lt_dev = linux_lt.get("deviceConfig", linux_lt)
    linux_lt_driver_path = str(linux_lt_dev.get("driverPath", "") or "")
    # Linux host clock: only when no hardware RTC backend is selected
    linux_local_use = use and (not int_use) and (not ext_use) and bool(linux_lt.get("use", False))

    ext_dev = ext_rtc.get("deviceConfig", ext_rtc)
    int_dev = int_rtc.get("deviceConfig", int_rtc)

    ext_driver_path = ext_dev.get("driverPath", "")
    int_driver_path = int_dev.get("driverPath", "")
    ext_i2c_addr = ext_dev.get("i2cAddress", "0")
    ext_i2c_line = ext_dev.get("i2cBusName", "I2C_LINE_UNUSED")

    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    def b(x: bool) -> str:
        return "1" if x else "0"

    lines = [
        "/**",
        " * @author: Zafer Satilmis",
        " * @hype-man: Judas Priest with Tim Ripper Owens - Metal Gods", 
        " * @file TimeService_Config.h",
        " * @brief AUTO-GENERATED configuration for TimeService",
        " */",
        "#ifndef __CUS_TIME_SERVICE_CONFIG_H__",
        "#define __CUS_TIME_SERVICE_CONFIG_H__",
        "",
        f"/* generated on: {now} */",
        "",
        f"#define APP_TIME_SERVICE_USE              ({b(use)})",
        f"#define APP_TIME_SERVICE_USE_NTP          ({b(ntp_use)})",
        f"#define APP_TIME_SERVICE_USE_INT_RTC      ({b(int_use)})",
        f"#define APP_TIME_SERVICE_USE_EXT_RTC      ({b(ext_use)})",
        f"#define APP_TIME_SERVICE_USE_LINUX_LOCAL_TIME ({b(linux_local_use)})",
        "",
        f"/* timeZone: {tz_str} => offset minutes */",
        f"#define APP_TIME_SERVICE_TZ_OFFSET_MINUTES ({tz_min})",
        "",
        f'#define APP_TIME_SERVICE_DEFAULT_NTP_HOST  "{ntp_host}"',
        f"#define APP_TIME_SERVICE_DEFAULT_NTP_PORT  ({ntp_port})",
        f"#define APP_TIME_SERVICE_NTP_UPDATE_PERIOD_MIN ({ntp_period})",
        "",
    ]

    # Include path: from Customers/ to repo root is ../
    inc_prefix = "../"

    # --- extRTC: driver include (contains EXT_RTC_* macros), I2C macros ---
    if ext_use and ext_driver_path:
        lines.append(f'#include "{inc_prefix}{ext_driver_path}"')
        lines.append("")
        lines.append(f"#define EXT_RTC_I2C_ADDR  ({ext_i2c_addr})")
        lines.append(f"#define EXT_RTC_I2C_LINE  ({ext_i2c_line})")
    else:
        lines.append("#define EXT_RTC_I2C_ADDR          (0)")
        lines.append("#define EXT_RTC_I2C_LINE          (I2C_LINE_UNUSED)")
        lines.append("#define EXT_RTC_INIT_FUNC(x)      (FAILURE)")
        lines.append("#define EXT_RTC_GET_TIME_FUNC(t)  (FAILURE)")
        lines.append("#define EXT_RTC_SET_TIME_FUNC(t)  (FAILURE)")

    lines.append("")

    # --- intRTC: driver include (contains INT_RTC_* macros) or FAILURE macros ---
    if int_use and int_driver_path:
        lines.append(f'#include "{inc_prefix}{int_driver_path}"')
    else:
        lines.append("#define INT_RTC_INIT_FUNC(x)      (FAILURE)")
        lines.append("#define INT_RTC_GET_TIME_FUNC(t)  (FAILURE)")
        lines.append("#define INT_RTC_SET_TIME_FUNC(t)  (FAILURE)")

    lines.append("")

    # --- linuxLocalTime: driver header defines LINUX_LOCAL_TIME_* macros ---
    if linux_local_use and linux_lt_driver_path:
        lines.append(f'#include "{inc_prefix}{linux_lt_driver_path}"')
        lines.append("")

    lines.append("")
    lines.append("#endif /* __CUS_TIME_SERVICE_CONFIG_H__ */")
    lines.append("")

    return "\n".join(lines)

def generate_config_h(cus_pat: Path) -> str:
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    lines = [
        "/**",
        " * @file TimeService_Config.h",
        " * @brief AUTO-GENERATED configuration for TimeService",
        " *        AUTO-GENERATED FILE - DO NOT EDIT MANUALLY",
        " */",
        "#ifndef __APP_TIME_SERVICE_CONFIG_H__",
        "#define __APP_TIME_SERVICE_CONFIG_H__",
        "",
        f"/* generated on: {now} */",
        f"/* customer name: {cus_pat.parent.parent.name} */",
        "",
        f'#include "{cus_pat.parent.parent.name}/Time/Cus_TimeService_Config.h"',
        "",
    ]


    lines.append("")
    lines.append("#endif /* __APP_TIME_SERVICE_CONFIG_H__ */")
    lines.append("")

    return "\n".join(lines)

def generate_autogen_c(cfg: dict) -> str:
    ts = cfg.get("timeService", {})

    use = bool(ts.get("use", True))
    ntp = ts.get("ntp", {})
    ntp_use = (use) and bool(ntp.get("use", False))
    int_use = (use) and bool(ts.get("intRTC", {}).get("use", False))
    ext_use = (use) and bool(ts.get("extRTC", {}).get("use", False))
    linux_lt = ts.get("linuxLocalTime", {})
    linux_local_use = (use) and (not int_use) and (not ext_use) and bool(linux_lt.get("use", False))
    soft_use = (use) and (not int_use) and (not ext_use) and (not linux_local_use)
    now = datetime.now().strftime("%d %b %Y - %H:%M:%S")

    print(f"\033[93mTimeService use: {use}\033[0m")
    print(f"\033[93mNTP use: {ntp_use}\033[0m")
    print(f"\033[93mInt RTC use: {int_use}\033[0m")
    print(f"\033[93mExt RTC use: {ext_use}\033[0m")
    print(f"\033[93mLinux local time use: {linux_local_use}\033[0m")
    print(f"\033[93mSoft tick use: {soft_use}\033[0m")

    lines = [
        "/******************************************************************************",
        "* #Author       : Auto-generated",
        f"* #Date         : {now}",
        "* #File Name    : AppTimeService_Autogen.c",
        "*******************************************************************************/",
        "/******************************************************************************",
        "* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY",
        f"* Customer Name: {cfg.get('customer', 'Unknown')}",
        "* Generated from customer time_config.json. Only used units are included.",
        "*******************************************************************************/",
        "",
        '#include "Project_Conf.h"',
        '#include "../../../Customers/TimeService_Config.h"',
        "",
    ]    

    for mod in ["time_modules/TimeSync_Ntp.h", "../Middleware/MiddZModem/inc/MiddRTC.h", "time_modules/TimeBackend_SoftTick.h"]:
        use_mod = (
            (mod == "time_modules/TimeSync_Ntp.h" and ntp_use)
            or (mod == "../Middleware/MiddZModem/inc/MiddRTC.h" and (int_use or ext_use))
            or (mod == "time_modules/TimeBackend_SoftTick.h" and soft_use)
        )
        if use_mod:
            lines.append(f'#include "{mod}"')
            
    lines.append("")
    # appTimeServiceAutogenInit(const char *ntpHost, U16 ntpPort)
    lines += [
        "static RETURN_STATUS appTimeServiceAutogenInit(const char *ntpHost, U16 ntpPort)",
        "{",
    ]
    if not use:
        lines += ["    (void)ntpHost;", "    (void)ntpPort;", "    return FAILURE;", "}", ""]
    else:
        if ntp_use:
            lines += [   
                "    if (SUCCESS != appTimeNtpSetServer(ntpHost, ntpPort))", "    {", "        return FAILURE;","    }", ""]
        if int_use:
            lines += ["    if (SUCCESS != middRtcIntInit())", "    {", "        return FAILURE;", "    }", ""]
        if ext_use:
            lines += ["    if (SUCCESS != middRtcExtInit())", "    {", "        return FAILURE;", "    }", ""]
        if linux_local_use:
            lines += ["    // linux local time is used"]
        if soft_use:
            lines += ["    if (SUCCESS != appTimeSoftTickInit())", "    {", "        return FAILURE;", "    }", ""]
        lines += ["    return SUCCESS;", "}", ""]

    # appTimeServiceAutogenGetEpochFromPreferredSource - returns epoch (U32), 0 on error
    lines += ["static U32 appTimeServiceAutogenGetEpochFromPreferredSource(void)", "{"]
    
    if int_use:
        lines += [
            "    MiddRtcStr_t r;",
            "    U32 e = 0;",
            "    if (SUCCESS == middRtcIntGetTime(&r))",
            "    {",
            "        //dont need to check return value here since 0 is an invalid epoch and indicates failure", 
            "        appTimeServiceRtcStrToEpoch(&r, &e);",
            "    }",
            "    return e;",
        ]
    elif ext_use:
        lines += [
            "    MiddRtcStr_t r;",
            "    U32 e = 0;",
            "    if (SUCCESS == middRtcExtGetTime(&r))",
            "    {",
            "        //dont need to check return value here since 0 is an invalid epoch and indicates failure",
            "        appTimeServiceRtcStrToEpoch(&r, &e);",
            "    }",
            "    return e;",
        ]
    elif linux_local_use:
        lines += ["    return getCurrentUnixTime();"]
    elif soft_use:
        lines += [
            "    return appTimeSoftTickGetEpoch();",
        ]
    else:
        lines += ["    return 0;"]
    lines += ["}", ""]

    # appTimeServiceAutogenUpdateRtcsFromEpoch
    # - If no RTC is used (soft-tick only), don't generate RTC conversion code.
    # - If one/both RTC are used, convert once to RTC str and then update the active RTC(s).
    lines += [
        "static RETURN_STATUS appTimeServiceAutogenUpdateRtcsFromEpoch(U32 epoch)",
        "{",
    ]
    if not use:
        lines += ["    RETURN_STATUS retVal = FAILURE;"]
    else:
        lines += ["    RETURN_STATUS retVal = SUCCESS;"]
        if int_use or ext_use:
            lines += [
                "    MiddRtcStr_t r;",
                "",
                "    appTimeServiceEpochToRtcStr(epoch, &r);",
                "",
            ]
            if int_use:
                lines += ["    if (SUCCESS != middRtcIntSetTime(&r))", "    {", "        retVal = FAILURE;", "    }"]
            if ext_use:
                lines += ["    if (SUCCESS != middRtcExtSetTime(&r))", "    {", "        retVal = FAILURE;", "    }"]

        if linux_local_use:
            lines += [
                "    if (0 != linuxLocalTimeSvcSetEpoch(epoch))",
                "    {",
                "        retVal = FAILURE;",
                "    }",
            ]
        if soft_use:
            lines += ["    retVal = appTimeSoftTickSetEpoch(epoch);"]

    lines += ["    return retVal;", "}", ""]

    # appTimeServiceAutogenGetNtpEpoch
    lines += ["static U32 appTimeServiceAutogenGetNtpEpoch(void)", "{"]
    if ntp_use:
        lines += [
            "    return appTimeNtpGetEpoch();",
        ]
    else:
        lines += ["    return 0;"]
    lines += ["}", ""]

    # appTimeServiceAutogenSetNtpServer
    lines += [
        "static RETURN_STATUS appTimeServiceAutogenSetNtpServer(const char *host, U16 port)",
        "{",
    ]
    if ntp_use:
        lines += [
            "    return appTimeNtpSetServer(host, port);",
        ]
    else:
        lines += ["    (void)host;", "    (void)port;", "    return FAILURE;"]
    lines += ["}", ""]

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Generate TimeService config and autogen from customer time_config.json"
    )
    parser.add_argument(
        "--customer",
        required=True,
        help="Customer name (e.g. ZD_0101). Config is read from Customers/<customer>/Time/time_config.json",
    )
    args = parser.parse_args()

    here = Path(__file__).resolve().parent
    json_path = here / args.customer / "Time" / "time_config.json"
    if not json_path.exists():
        raise SystemExit(f"\033[91mConfig not found: {json_path}\033[0m")

    cfg = json.loads(json_path.read_text(encoding="utf-8"))
    print(f"\033[95mProcessing customer: {args.customer}\033[0m")

    repo_root = here.parent    
    service_dir = repo_root / "Application" / "AppZMeterGw" / "Services" / "TimeService"
    out_h = here / "TimeService_Config.h"
    cusTsConfout_h = here / args.customer / "Time" / "Cus_TimeService_Config.h"
    out_c = service_dir / "src" / "AppTimeService_Autogen.c"

    os.makedirs(out_h.parent, exist_ok=True)
    os.makedirs(out_c.parent, exist_ok=True)
    os.makedirs(cusTsConfout_h.parent, exist_ok=True)

    cusTsConfout_h.write_text(generate_cus_config_h(cfg, repo_root), encoding="utf-8")
    out_c.write_text(generate_autogen_c(cfg), encoding="utf-8")
    out_h.write_text(generate_config_h(json_path), encoding="utf-8")

    print(f"Generated: {cusTsConfout_h}")
    print(f"Generated: {out_h}")
    print(f"Generated: {out_c}")


if __name__ == "__main__":
    main()

