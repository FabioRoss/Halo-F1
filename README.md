# Halo F1

**A Formula 1 desktop companion for the JC4827W543 (ESP32-S3 + 4.3" TFT)**

Halo F1 is a small, always-on display that shows everything you need about the current F1 season — next race session times in your local timezone, the full drivers' and constructors' championship standings, the latest session results, and F1 news headlines. No apps, no browser tabs, no ads. Just a glance at your desk.

The pre-compiled firmware is free to install from the [project website](https://halof1.com/). This repository contains the full source code for reference and personal use.

---

## Features

- **Session Times** — FP1, FP2, FP3, Sprint Qualifying, Sprint Race, Qualifying and Race start times for the upcoming weekend, automatically converted to your local timezone
- **Drivers' Championship** — Full standings table updated throughout the season; includes a pre-season fallback that populates the driver and constructor roster before the first race
- **Session Results** — Qualifying (Q1/Q2/Q3) and race results fetched from the live OpenF1 API, including gap to leader
- **No Spoiler Mode** — No unwanted spoilers when watching live sessions isn't an option!
- **Latest News** — F1 headlines pulled from selectable RSS feeds (The Race (EN), Formula1.com (EN), Motorsport-Total (DE), RacingNews365 (NL), FormulaPassion (IT))
- **Night Mode** — Configurable dimming window; set start/stop times and a separate night brightness level
- **9 Languages** — English, Italian, Spanish, French, Dutch, German, Portuguese, Norwegian, Polish
- **Captive-portal Wi-Fi setup** — On first boot the device broadcasts an access point (`Halo-F1`); connect from any phone or laptop to enter your home network credentials. No app or computer required after initial flashing
- **Update notifications** — The device checks for new firmware versions on startup and shows an in-app notification if an update is available

---

## Hardware

| Component    | Detail                                |
| ------------ | ------------------------------------- |
| Board        | **JC4827W543**                        |
| SoC          | ESP32-S3                              |
| Display      | 4.3" TFT, 480 × 272 px                |
| Touch        | Capacitive (recommended) or Resistive |
| Connectivity | Wi-Fi (2.4 GHz), Bluetooth            |

The JC4827W543 is available [on Aliexpress at this link](https://s.click.aliexpress.com/e/_c2xQCrDH), get the capacitive touch version for a nicer look. A snap-fit 3D-printable case (no screws, no glue, prints in PLA on any FDM printer) is available free on [MakerWorld](https://makerworld.com/it/models/2492192-halo-your-f1-desktop-companion).

---

## Flashing

The easiest way to install the firmware is via the project website, directly from Chrome or Edge — no software needed:

**[halof1.com](https://halof1.com/)**

If you want to compile from source, see the [Building from Source](#building-from-source) section below.

**Important:** Make sure to install the ipex antenna on the back of the board, this board won't be able to maintain a stable WiFi connection without it.

---

## Building from Source

### Requirements

- [Arduino IDE](https://www.arduino.cc/en/software) 2.x
- ESP32 Arduino core (`esp32` by Espressif, tested on **3.x**)

### Arduino Libraries

Install the following through the Library Manager (Sketch → Include Library → Manage Libraries).
**IMPORTANT:** This project has been developed over an extended period of time, libraries used in it might have introduced breaking changes in latest updates. To ensure full compatibility please make sure to download the correct versions where indicated

| Library             | Author          |
| ------------------- | --------------- |
| `LVGL v9.3`         | LVGL            |
| `WiFiManager`       | tzapu           |
| `ArduinoJson`       | Benoit Blanchon |
| `bb_spi_lcd v2.7.1` | Larry Bank      |

> **Note:** `lv_conf.h` must be configured for the JC4827W543 display before compiling. The `lv_conf.h` included in this repository is already set up correctly.

### Pin Configuration

The following defines are set at the top of `Halo-F1.ino` and match the JC4827W543 board:

```cpp
#define DISPLAY_TYPE    DISPLAY_CYD_543
#define TOUCH_CAPACITIVE
#define TOUCH_SDA        8
#define TOUCH_SCL        4
#define TOUCH_INT        3
#define TOUCH_RST       -1
#define TOUCH_MOSI      11
#define TOUCH_MISO      13
#define TOUCH_CLK       12
#define TOUCH_CS        38
#define SCREEN_WIDTH   272
#define SCREEN_HEIGHT  480
```

### Compiling and Uploading

1. Clone or download this repository
2. Open `Halo-F1.ino` in Arduino IDE
3. If your board is Capacitive Touch, make sure `#define TOUCH_CAPACITIVE` is set, otherwise comment it out to compile for Resistive Touch
4. Select **ESP32S3 Dev Module** as the target board
5. Set **Flash Size** to **4MB**, **PSRAM** to **OPI PSRAM** (8 MB), and **Partition Scheme** to `Huge APP (3MB No OTA/1MB SPIFFS)`. Set **USB CDC On Boot** to **enabled** to get Serial feed via USB
6. Connect the board via a USB-A to USB-C data cable
7. If the port does not appear, hold `BOOT`, press `RST`, then release `BOOT` to enter flash mode
8. Click **Upload**

---

## Data Sources

Halo F1 fetches all data over HTTPS. No account or API key is required.

| Data                          | Source                                       |
| ----------------------------- | -------------------------------------------- |
| Driver standings              | [Jolpica / Ergast F1 API](https://jolpi.ca/) |
| Race calendar & session times | [Jolpica / Ergast F1 API](https://jolpi.ca/) |
| Live session results          | [OpenF1 API](https://openf1.org/)            |
| Timezone offset from IP       | [IP API](https://ipapi.co/)                  |
| Weather Forecast              | [Open Meteo](https://open-meteo.com/)        |
| News headlines                | Selectable RSS feeds (The Race (EN), Formula1.com (EN), Motorsport-Total (DE), RacingNews365 (NL), FormulaPassion (IT)) |

### Anonymous Statistics

On startup, and periodically, the device sends a minimal anonymous ping to a server at `we-race.it`. This request carries:

- A randomly generated device UUID (generated once, stored in flash)
- The selected display language
- The configured UTC offset
- The current firmware version

No personal data, Wi-Fi credentials, location or network information is ever transmitted. This telemetry is used solely to count active installs, check for firmware updates and deliver in-app notifications such as release announcements. You can verify this behaviour yourself in the `sendStatisticData()` function inside `wifi_handler.h`.

---

## Project Structure

```
Halo-F1.ino           — Main sketch: hardware initialisation, setup(), loop()
ui.h                  — LVGL UI construction and all event handlers
wifi_handler.h        — Wi-Fi setup, all API fetch functions, statistics ping
weather.h             — Weather API calls and utils
localized_strings.h   — Translated string tables for all 9 supported languages
settings.h            — Persistent settings save/load (Preferences/NVS)
utils.h               — Utility functions (UUID generation, time helpers, etc.)
notifications.h       — In-app notification queue and scheduler
audio.h               — I²S notification sound playback
touchscreen.h         — Capacitive touch driver wrapper (GT911)
ESP_I2S.cpp / .h      — I²S audio driver (adapted from the Arduino ESP32 core)
lv_bb_spi_lcd.cpp/.h  — LVGL bridge for bb_spi_lcd display backend
lv_conf.h             — LVGL configuration tuned for the JC4827W543 display
montserrat_*.c        — Embedded font assets used by the UI
f1_symbols_28.c       — Icon font symbols for tab/navigation glyphs
weather_icons_12.c    — Weather icon glyph font used in race sessions
```

---

## Future Developments

- [x] No spoiler mode
- [x] Weather forecast for each session
- [ ] Audio notifications when a new article is fetched
  - [x] Library inclusion and set up
  - [ ] Exclude sound during night times
  - [ ] Add Menu config option
  - [ ] 3D Case rework to fit speaker
- [x] Add constructors standings, let user choose if to display drivers, constructors or both standings
- [x] Add timezone override in menu
- [x] Add more RSS feeds

---

## Copyright & Terms of Use

Copyright © 2026 Fabio Rossato. All rights reserved.

This source code is made publicly available for **personal, non-commercial use only**.

**You are free to:**

- Read, study and learn from the code
- Build and flash the firmware for personal use on your own device
- Fork the repository and make private modifications for personal use

**You are not permitted to:**

- Use this code, in whole or in part, in any commercial product, service or paid project
- Redistribute or republish the source code, compiled firmware or any derivative under a different name or project without explicit written permission
- Remove or alter copyright notices in any file

No open-source license is granted. The absence of a license means this code is **not** open source — all rights not explicitly listed above are reserved by the author. If you are unsure whether your intended use is permitted, open an issue or get in touch.

---

## Acknowledgements

- [LVGL](https://lvgl.io/) — Embedded graphics library
- [WiFiManager](https://github.com/tzapu/WiFiManager) — Captive-portal Wi-Fi configuration by tzapu
- [ArduinoJson](https://arduinojson.org/) — JSON parsing by Benoit Blanchon
- [bb_spi_lcd](https://github.com/bitbank2/bb_spi_lcd) — SPI display driver by Larry Bank
- [Jolpica / Ergast F1 API](https://jolpi.ca/) — Race calendar and standings data
- [OpenF1](https://openf1.org/) — Live session result data
- [Open Meteo](https://open-meteo.com/) — Weather forecast data
- [IP API](https://ipapi.co/) — Timezone offset via IP

---

## Links

- 🌐 **Website & firmware installer** — [halof1.com](https://halof1.com/)
- 🖨 **3D-printable case** — [MakerWorld](https://makerworld.com/it/models/2492192-halo-your-f1-desktop-companion)
- 📸 **Instagram** — [@\fabiotechgarage](https://instagram.com/fabiotechgarage)
- 👾 **Discord Server** - [Invite link](https://discord.gg/fMa5KDeFUV)
- ☕ **Support the project** — [paypal.me/rossatof](https://paypal.me/rossatof)
