# Aviora TimeService

## Overview

TimeService provides current time to other application modules. It supports multiple backends: NTP, internal RTC, external RTC, and software tick. Configuration is JSON-driven; code is auto-generated to avoid `#if/#else` branching.

## Configuration

Each customer has `Customers/<customer>/Time/time_config.json`:

```json
{
  "timeService": {
    "use": true,
    "timeZone": "UTC",
    "ntp": {
      "use": true,
      "ipAddr": "pool.ntp.org",
      "port": 123,
      "updatePeriodMin": 60
    },
    "intRTC": { "use": true, ... },
    "extRTC": { "use": false, ... }
  }
}
```

## Generator

```bash
python Customers/generate_time_service_code.py --customer ZD_2622
```

Outputs:
- `Customers/TimeService_Config.h`
- `Application/.../AppTimeService_Autogen.c`

## API

- `appTimeServiceInit(ntpHost, ntpPort)` — Initialize, optionally start NTP sync
- `appTimeServiceSetTime(epoch)` — Set current time
- `appTimeServiceGetTime(tm*)` — Get local time as struct tm
- `appTimeServiceGetEpoch(outEpoch)` — Get epoch seconds
- `appTimeServiceFormatNow(buf, size, fmt)` — Format time string
- `appTimeServiceSetNtpServer(host, port)` — Runtime NTP server change

## Backends

- **NTP** — Network Time Protocol
- **intRTC** — MCU internal RTC
- **extRTC** — External RTC (e.g. M41T11)
- **SoftTick** — Software tick for bare-metal
