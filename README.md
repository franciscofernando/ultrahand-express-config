# Express Config

A **Tesla / Ultrahand** overlay for the N Switch (custom firmware) that gives
you quick access to **screen brightness**, **Bluetooth audio**, **Wi‑Fi** and the
**system clock** — without ever opening the System Settings.

Put your AirPods (or any Bluetooth headphones / speaker) into pairing mode and
connect them straight from the overlay, toggle Wi‑Fi, fix the clock, and slide the
screen brightness from anywhere, even inside a game.

## Features

- **Brightness**
  - Slider from 0–100 %
  - Quick presets: 10 / 25 / 50 / 75 / 100 %
  - Auto-brightness toggle (adjusting manually turns auto off)
- **Bluetooth audio**
  - Turn the Bluetooth radio on/off
  - See paired devices with a live "Connected" status
  - Connect / disconnect a paired device (**A**) or forget it (**Y**)
  - Scan for and pair **new** audio devices (AirPods, headphones, speakers…)
- **Wi‑Fi**
  - Turn wireless communication on/off (disconnect / reconnect)
  - Live status: current network name, connection state and signal strength (0–3 bars)
- **Time & date**
  - Edit year, month, day, hour and minute (Left/Right to adjust, with proper
    month lengths and leap years) and save it to the system clock
- **Automatic language**: the UI is shown in **Spanish** if the console language is
  Spanish (Spain or Latin America), and in **English** for every other language.

## Requirements

- A Switch running **Atmosphère** (or compatible CFW)
- The **Tesla overlay loader** installed: `nx-ovlloader` + **[Ultrahand](https://github.com/ppkantorski/Ultrahand-Overlay)** or **[Tesla-Menu](https://github.com/WerWolv/Tesla-Menu)**
- **Firmware 13.0.0 or newer** for the Bluetooth audio features.
  On older firmware the Bluetooth section shows as *Not available*, but brightness
  still works.

## Installation

1. Download `express-config.ovl` (from the [Releases](../../releases) page, or build it yourself — see below).
2. Copy it to your SD card at:
   ```
   /switch/.overlays/express-config.ovl
   ```
3. On the console, open the overlay menu (default: **L + D-Pad Down + Right Stick**)
   and select **Express Config**.

## Usage

- **Brightness** – move the slider with Left/Right, or tap a preset. Toggle
  auto-brightness under *Options*.
- **Bluetooth**
  - **A** on a paired device → connect / disconnect
  - **Y** on a paired device → forget (remove pairing)
  - *Search headphones / speakers…* → put the device in pairing mode
    (for AirPods, hold the case button until the light blinks white) and press
    **A** on it once it appears.
- **Wi‑Fi** – toggle *WiFi / wireless* to disconnect or reconnect. The status
  section shows the current network and signal strength.
- **Time & date** – highlight a field (Year / Month / Day / Hour / Minute), press
  **Left/Right** to change it, then select *Save time & date*.

## Building from source

Requires [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the
**switch-dev** package group (`devkitA64` + `libnx`).

```bash
export DEVKITPRO=/opt/devkitpro
make
```

This produces `express-config.ovl`.

### Project layout

```
express-config/
├── Makefile                      # based on the official libtesla template
├── source/main.cpp               # all overlay logic
└── lib/libtesla/include/
    ├── tesla.hpp                 # libtesla (WerWolv)
    └── stb_truetype.h
```

## How it works

- **Brightness** uses the `lbl` system service (`lblSetCurrentBrightnessSetting`,
  `lblEnable/DisableAutoBrightnessControl`, …).
- **Bluetooth** uses the `btm:sys` system service
  (`btmsysStartAudioDeviceDiscovery`, `btmsysConnectAudioDevice`,
  `btmsysGetPairedAudioDevices`, …), which exposes audio device management on
  firmware 13.0.0+.
- **Wi‑Fi** uses the `nifm` service. Toggling wireless
  (`nifmSetWirelessCommunicationEnabled`) requires the `nifm:a`/`nifm:s` session,
  so the overlay initializes Admin → System → User and falls back to read‑only if
  it only gets a `nifm:u` session.
- **Time & date** uses the `time` service. Writing the clock
  (`timeSetCurrentTime`) requires the `time:s` session, so the overlay requests it
  via `__nx_time_service_type` and falls back to `time:u` (read‑only) otherwise.
- **Language** is read once at startup via the `set` service
  (`setGetSystemLanguage`).

> Some of these actions need elevated system services (`nifm:a`, `time:s`). If your
> overlay loader doesn't grant them, those controls show as *read‑only* / *no
> permission* instead of failing silently.

## Credits

- Built with **[libtesla](https://github.com/WerWolv/libtesla)** by WerWolv.
- Compatible with the **[Ultrahand Overlay](https://github.com/ppkantorski/Ultrahand-Overlay)** loader by ppkantorski.
- Toolchain by **[devkitPro](https://devkitpro.org/)**.

## License

Released under the **GPLv2** license, matching libtesla. See [LICENSE](LICENSE).

## Disclaimer

This is homebrew software for consoles running custom firmware. Use at your own
risk. Not affiliated with or endorsed by manufacturing company.
