# RAW UI

[![Build](https://github.com/bitplugg/rawui/actions/workflows/build.yml/badge.svg)](https://github.com/bitplugg/rawui/actions/workflows/build.yml)
[![Latest Release](https://img.shields.io/github/v/release/bitplugg/rawui?include_prereleases&label=release)](https://github.com/bitplugg/rawui/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![PS4](https://img.shields.io/badge/PS4-fw%2013.52-lightgrey)](#)

Material Design 3 Expressive UI shell for PlayStation 4 (GoldHEN). Pure C++17, SDL2, custom MD3 software renderer.

## Features

- 🎨 Material Design 3 Expressive design language with dynamic color theming
- 🚀 Smooth spring animations and geometric focus navigation
- 📦 PKG installer with real-time progress tracking
- 📁 File manager for mounted volumes (/data, /mnt, /usb, /user)
- 🛡️ GoldHEN status detection and system info
- ⚙️ Settings with UI scale, motion, contrast, and theme customization
- 🎮 Optimized for DualShock 4

## Download

- **[Latest Release](https://github.com/bitplugg/rawui/releases/latest)** - Download `RAWUI0001.pkg`
- **[GitHub Actions](https://github.com/bitplugg/rawui/actions)** - Download build artifacts
- **[Web Installer](https://bitplugg.github.io/rawui/)** - GitHub Pages landing

## Installation

1. Download `RAWUI0001.pkg` from the latest release
2. Transfer to your PS4 (USB/FTP)
3. Install via Package Installer (GoldHEN required)
4. Launch from your homebrew menu

## Controls

| Input | Action |
|---|---|
| D-Pad / Left Stick | Navigate focus |
| Cross (X) | Confirm |
| Circle (O) | Back |
| Square (□) | Secondary action |
| Triangle (△) | Screen FAB |
| L1 / R1 | Switch screens |
| Options | Toggle dark/light theme |
| Left/Right | Adjust sliders/segmented controls |

## Building

### Local (Linux)

```bash
# Requires OpenOrbis PS4 Toolchain v0.5.4
curl -L -o tc.tar.gz https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases/download/v0.5.4/toolchain-llvm-18.tar.gz
tar -xzf tc.tar.gz
export OO_PS4_TOOLCHAIN=$PWD/OpenOrbis/PS4Toolchain
export PATH=/tmp/dotnet31:$PATH  # for PkgTool.Core on newer distros
bash tools/build.sh
```

Built `.pkg` will be in `dist/`.

### Docker

```bash
bash tools/docker-build.sh
```

### CI

GitHub Actions automatically builds on push to `main` and publishes artifacts.

## Technical Stack

- **Language**: C++17
- **Graphics**: SDL2 + custom software rasterizer
- **Fonts**: Roboto Flex (variable) + Material Symbols
- **Toolchain**: OpenOrbis PS4 Toolchain
- **Packaging**: PkgTool.Core

## License

MIT License - see [LICENSE](LICENSE) for details.

## Credits

Inspired by the PS4 homebrew scene. Built for use with [GoldHEN](https://github.com/GoldHEN/GoldHEN).
