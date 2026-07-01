# Express Config

A **Tesla / Ultrahand** overlay for the N Switch (custom firmware) that gives
you quick access to **screen brightness** and **Bluetooth audio** — without ever
opening the System Settings.

Put your AirPods (or any Bluetooth headphones / speaker) into pairing mode and
connect them straight from the overlay, and slide the screen brightness from
anywhere, even inside a game.

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
- **Language** is read once at startup via the `set` service
  (`setGetSystemLanguage`).

## Credits

- Built with **[libtesla](https://github.com/WerWolv/libtesla)** by WerWolv.
- Compatible with the **[Ultrahand Overlay](https://github.com/ppkantorski/Ultrahand-Overlay)** loader by ppkantorski.
- Toolchain by **[devkitPro](https://devkitpro.org/)**.

## License

Released under the **GPLv2** license, matching libtesla. See [LICENSE](LICENSE).

## Disclaimer

This is homebrew software for consoles running custom firmware. Use at your own
risk. Not affiliated with or endorsed by manufacturing company.
