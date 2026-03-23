# Aviora Network Service

## Overview

Network Service manages GSM and Ethernet interfaces. It initializes and starts all enabled interfaces based on customer configuration.

## Configuration

- **GSM**: PPP, device name (e.g. quectelUC2000), connection interface (huart1), resource path
- **Ethernet**: Device (e.g. ENC28J60), SPI interface, resource path

## API

- `AppNetworkService_Start()` — Initialize and start all network interfaces

## Architecture

- GSM: PPP over UART
- Ethernet: lwIP, ENC28J60 driver
- Config structs: `AppNetworkGsmConfig_t`, `AppNetworkEthConfig_t`
