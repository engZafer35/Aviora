#!/usr/bin/env python3
"""
TimeService Configuration Code Generator

Purpose:
- Read customer time_config.json
- Generate:
  - Application/AppZMeterGw/Services/TimeService/inc/AppTimeService_Config.h
  - Application/AppZMeterGw/Services/TimeService/src/AppTimeService_Autogen.c

Design:
- Avoid customer-specific #if/#else in core code.
- Select pre-written, testable modules by generating a small wiring/include file.
"""

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


def generate_config_h(cfg: dict) -> str:
    ts = cfg.get("timeService", {})

    use = bool(ts.get("use", True))
    tz_str = ts.get("timeZone", "UTC")
    tz_min = parse_timezone_to_minutes(tz_str)

    ntp = ts.get("ntp", {})
    ntp_use = bool(ntp.get("use", False))
    ntp_host = str(ntp.get("ipAddr", "pool.ntp.org"))
    ntp_port = int(ntp.get("port", 123))
    ntp_period = int(ntp.get("updatePeriodMin", 10))

    int_rtc = ts.get("intRTC", {})
    ext_rtc = ts.get("extRTC", {})
    int_use = bool(int_rtc.get("use", False))
    ext_use = bool(ext_rtc.get("use", False))

    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    def b(x: bool) -> str:
        return "1" if x else "0"

    return "\n".join(
        [
            "/**",
            " * @file AppTimeService_Config.h",
            " * @brief AUTO-GENERATED configuration for TimeService",
            " */",
            "#ifndef __APP_TIME_SERVICE_CONFIG_H__",
            "#define __APP_TIME_SERVICE_CONFIG_H__",
            "",
            f"/* generated on: {now} */",
            "",
            f"#define APP_TIME_SERVICE_USE              ({b(use)})",
            f"#define APP_TIME_SERVICE_USE_NTP          ({b(ntp_use)})",
            f"#define APP_TIME_SERVICE_USE_INT_RTC      ({b(int_use)})",
            f"#define APP_TIME_SERVICE_USE_EXT_RTC      ({b(ext_use)})",
            "",
            f"/* timeZone: {tz_str} => offset minutes */",
            f"#define APP_TIME_SERVICE_TZ_OFFSET_MINUTES ({tz_min})",
            "",
            f'#define APP_TIME_SERVICE_DEFAULT_NTP_HOST  "{ntp_host}"',
            f"#define APP_TIME_SERVICE_DEFAULT_NTP_PORT  ({ntp_port})",
            f"#define APP_TIME_SERVICE_NTP_UPDATE_PERIOD_MIN ({ntp_period})",
            "",
            "#endif /* __APP_TIME_SERVICE_CONFIG_H__ */",
            "",
        ]
    )


def generate_autogen_c(cfg: dict) -> str:
    ts = cfg.get("timeService", {})

    use = bool(ts.get("use", True))
    ntp = ts.get("ntp", {})
    ntp_use = bool(ntp.get("use", False))
    int_use = bool(ts.get("intRTC", {}).get("use", False))
    ext_use = bool(ts.get("extRTC", {}).get("use", False))
    soft_use = (use) and (not int_use) and (not ext_use)

    now = datetime.now().strftime("%d %b %Y - %H:%M:%S")

    lines = [
        "/******************************************************************************",
        "* #Author       : Auto-generated",
        f"* #Date         : {now}",
        "* #File Name    : AppTimeService_Autogen.c",
        "*******************************************************************************/",
        "/******************************************************************************",
        "* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY",
        "* Generated from customer time_config.json. Only used units are included.",
        "*******************************************************************************/",
        "",
        '#include "Project_Conf.h"',
        '#include "AppTimeService_Config.h"',
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
        if soft_use:
            lines += ["    if (SUCCESS != appTimeSoftTickInit())", "    {", "        return FAILURE;", "    }", ""]
        lines += ["    return SUCCESS;", "}", ""]

    # appTimeServiceAutogenGetEpochUtcFromPreferredSource - returns epoch (U32), 0 on error
    lines += ["static U32 appTimeServiceAutogenGetEpochUtcFromPreferredSource(void)", "{"]
    
    if int_use:
        lines += [
            "    MiddRtcStr_t r;",
            "    U32 e = 0;",
            "    if (SUCCESS == middRtcIntGetTime(&r))",
            "    {",
            "        //dont need to check return value here since 0 is an invalid epoch and indicates failure", 
            "        appTimeServiceRtcStrToEpochUtc(&r, &e);",
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
            "        appTimeServiceRtcStrToEpochUtc(&r, &e);",
            "    }",
            "    return e;",
        ]
    elif soft_use:
        lines += [
            "    return appTimeSoftTickGetEpochUtc();",
        ]
    else:
        lines += ["    return 0;"]
    lines += ["}", ""]

    # appTimeServiceAutogenUpdateRtcsFromEpochUtc
    # - If no RTC is used (soft-tick only), don't generate RTC conversion code.
    # - If one/both RTC are used, convert once to RTC str and then update the active RTC(s).
    lines += [
        "static RETURN_STATUS appTimeServiceAutogenUpdateRtcsFromEpochUtc(U32 epochUtc)",
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
                "    appTimeServiceEpochUtcToRtcStr(epochUtc, &r);",
                "",
            ]
            if int_use:
                lines += ["    if (SUCCESS != middRtcIntSetTime(&r))", "    {", "        retVal = FAILURE;", "    }"]
            if ext_use:
                lines += ["    if (SUCCESS != middRtcExtSetTime(&r))", "    {", "        retVal = FAILURE;", "    }"]

        if soft_use:
            lines += ["    retVal = appTimeSoftTickSetEpochUtc(epochUtc);"]

    lines += ["    return retVal;", "}", ""]

    # appTimeServiceAutogenGetNtpEpochUtc
    lines += ["static U32 appTimeServiceAutogenGetNtpEpochUtc(void)", "{"]
    if ntp_use:
        lines += [
            "    return appTimeNtpGetEpochUtc();",
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
    here = Path(__file__).resolve().parent
    json_path = here / "time_config.json"
    if not json_path.exists():
        raise SystemExit(f"Config not found: {json_path}")

    cfg = json.loads(json_path.read_text(encoding="utf-8"))

    # Repo root assumed: Customers/<cust>/Time/ -> repo root is 3 parents up (..../Aviora)
    repo_root = here.parents[2]
    service_dir = repo_root / "Application" / "AppZMeterGw" / "Services" / "TimeService"
    out_h = service_dir / "inc" / "AppTimeService_Config.h"
    out_c = service_dir / "src" / "AppTimeService_Autogen.c"

    os.makedirs(out_h.parent, exist_ok=True)
    os.makedirs(out_c.parent, exist_ok=True)

    out_h.write_text(generate_config_h(cfg), encoding="utf-8")
    out_c.write_text(generate_autogen_c(cfg), encoding="utf-8")

    print(f"Generated: {out_h}")
    print(f"Generated: {out_c}")


if __name__ == "__main__":
    main()

