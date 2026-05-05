# Third-Party Licenses

This project includes or references third-party assets and libraries.

## Runtime / Build Dependencies

### LVGL

- **Used in**: UI framework (`lvgl.h`, `lv_conf.h`, UI code)
- **Version**: 9.3.0
- **Upstream**: [https://github.com/lvgl/lvgl](https://github.com/lvgl/lvgl)
- **License**: MIT

### WiFiManager

- **Used in**: Wi-Fi captive portal setup
- **Version**: 2.0.17
- **Upstream**: [https://github.com/tzapu/WiFiManager](https://github.com/tzapu/WiFiManager)
- **License**: MIT

### ArduinoJson

- **Used in**: JSON parsing
- **Version**: 7.4.2
- **Upstream**: [https://github.com/bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- **License**: MIT

### bb_captouch

- **Used in**: Capacitive touch controller support
- **Version**: 1.2.2
- **Upstream**: [https://github.com/bitbank2/bb_captouch](https://github.com/bitbank2/bb_captouch)
- **License**: Apache-2.0

### bb_spi_lcd

- **Used in**: Display driver backend
- **Version**: 2.7.1
- **Upstream**: [https://github.com/bitbank2/bb_spi_lcd](https://github.com/bitbank2/bb_spi_lcd)
- **License**: Apache-2.0

### incbin

- **Used in**: Header-only binary include helper
- **Version**: 0.1.2
- **Upstream**: [https://github.com/graphitemaster/incbin](https://github.com/graphitemaster/incbin)
- **License**: MIT

## Adapted Source Code

### Arduino ESP32 Core (`ESP_I2S.cpp`, `ESP_I2S.h`)

- **Used in**: Local audio driver implementation adapted from upstream
- **Upstream**: [https://github.com/espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)
- **License**: LGPL-2.1
- **Notes**:
  - Keep attribution and license references when redistributing adapted code.
  - If you distribute modified versions of LGPL-covered code, comply with LGPL obligations.

## Fonts and Icon Assets

### Montserrat

- **Assets**:
  - `assets/fonts/Montserrat-Regular.ttf` (optional local generator input)
  - `montserrat_*.c` (generated embedded fonts)
- **Source**: [Google Fonts / Montserrat](https://fonts.google.com/specimen/Montserrat)
- **License**: SIL Open Font License 1.1 (OFL-1.1)
- **License URL**: [https://openfontlicense.org/](https://openfontlicense.org/)

### Font Awesome Free (v7)

- **Assets**:
  - `assets/fonts/Font Awesome 7 Free-Solid-900.otf` (generator input)
  - `f1_symbols_28.c`
  - `weather_icons_12.c`
- **License model** (Font Awesome Free):
  - Icons: CC BY 4.0
  - Fonts: SIL OFL 1.1
  - Code: MIT
- **License URL**: [https://fontawesome.com/license/free](https://fontawesome.com/license/free)
- **Notes**:
  - Brand icons remain subject to trademark rights of their respective owners.

## License Texts in This Repository

Canonical license references and retrieval notes are stored under `licenses/`.

## Compliance Reminder

When distributing firmware, source, or archives containing third-party code/assets:

- Keep this file (or equivalent notices) with your distribution.
- Keep/ship relevant license texts and notices required by upstream licenses.
- Keep attribution where required.
- Review trademark restrictions when using brand logos/icons.

