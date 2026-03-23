# Aviora Display Manager

## Overview

Display Manager drives LED or LCD displays. Customer-specific drivers are selected via `display_config.json`. Code generation produces `AppDisplayManager_Autogen.c`.

## Configuration

`display_config.json`:

```json
{
  "displayConfig": {
    "type": "led",
    "customerPath": "./Customers/ZD_0101/Display/",
    "customFileName": "DisplayCustom_LED.c"
  }
}
```

## API

- `AppDisplayInit()` — Initialize display
- `AppDisplayStart()` — Start display task

## Backends

- LED displays
- LCD displays (customer-specific `DisplayCustom_*.c`)
