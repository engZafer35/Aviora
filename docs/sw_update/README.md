# Aviora SW Update

## Overview

Software Update module supports OTA and Ethernet-based firmware updates via FTP. It downloads a remote file and saves it locally.

## API

```c
RETURN_STATUS AppSwUpdateInit(
  const char *serverIp,
  U16 serverPort,
  const char *username,
  const char *password,
  const char *remoteFilePath,
  const char *localFilePath
);
```

## Flow

- Starts FTP download task
- Result shared via data bus: `EN_SW_UPDATE_RESULT_IDLE`, `SUCCESS`, `FAILED`, `INVALID_PARAM`
- Used for OTA and Ethernet updates
