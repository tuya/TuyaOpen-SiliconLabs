# Getting Started with TuyaOpen for SiWx917 Platform

This guide explains how to set up and build TuyaOpen with SiWx917 support on Linux.

The SiWx917 platform is **not** a standalone project. It must live inside the TuyaOpen tree at:

```text
~/TuyaOpen/platform/SiWx917
```

> **Note:** SiWx917 is a first-class platform in current TuyaOpen. Board support lives in
> `boards/SiWx917/` in the main repository, so no integration patch has to be applied.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Project Structure](#project-structure)
- [Overview](#overview)
- [Step-by-Step Setup](#step-by-step-setup)
  - [Step 1: Clone TuyaOpen](#step-1-clone-tuyaopen)
  - [Step 2: Prepare SiWx917 Platform](#step-2-prepare-siwx917-platform)
  - [Step 3: Set Up Build Environment](#step-3-set-up-build-environment)
  - [Step 4: Configure the Application](#step-4-configure-the-application)
  - [Step 5: Build](#step-5-build)
- [Quick Reference](#quick-reference)
- [Next Steps](#next-steps)
- [Notes](#notes)
- [Additional Resources](#additional-resources)

## Prerequisites

- **OS:** Ubuntu 22.04 LTS (recommended) or compatible Linux
- **Git**
- **Python 3.12** (managed by `export.sh` / `uv`)
- Build tools are installed automatically by `export.sh` (CMake, Ninja, and Python packages)
- Silicon Labs SLC / Simplicity tools (pulled by `platform_prepare.py` as needed)

Verify basics:

```bash
python3 --version
git --version
```

## Project Structure

```text
~/TuyaOpen/
├── .venv/
├── apps/
│   ├── tuya.ai/your_chat_bot/config/SIWX917_AI_DEV_KIT.config
│   └── tuya_cloud/switch_demo/config/SIWX917_AI_DEV_KIT.config
├── boards/SiWx917/                  # first-class board support in main repo
├── platform/
│   ├── platform_config.yaml         # includes SiWx917 entry
│   └── SiWx917/                     # this platform repository
│       ├── tuyaos_adapter/
│       ├── mcu/
│       │   └── patch/RS9117_WC_SI.rps   # TA patch (flash once per device)
│       ├── slc/
│       └── tools/
└── export.sh
```

## Overview

Setup flow:

1. Clone a TuyaOpen tree that already contains `boards/SiWx917/` and a `SiWx917` entry in `platform/platform_config.yaml`
2. Ensure `platform/SiWx917` points at this repository (`dev` branch recommended while porting)
3. Configure and build `switch_demo` first, then `your_chat_bot`

## Step-by-Step Setup

### Step 1: Clone TuyaOpen

```bash
mkdir -p ~/TuyaOpen
cd ~/TuyaOpen
git clone https://github.com/tuya/TuyaOpen.git .
```

Use a tree that already integrates SiWx917 (boards + `platform_config.yaml`). If you are developing against a local fork, keep that commit pinned.

### Step 2: Prepare SiWx917 Platform

`tos.py` / platform prepare downloads platforms listed in `platform/platform_config.yaml`.

For local development you can also place this repo directly:

```bash
cd ~/TuyaOpen
mkdir -p platform
git clone https://github.com/tuya/TuyaOpen-SiliconLabs.git platform/SiWx917
cd platform/SiWx917
git checkout dev
```

Or symlink a working copy:

```bash
ln -sfn /path/to/TuyaOpen-SiliconLabs ~/TuyaOpen/platform/SiWx917
```

Confirm `platform/platform_config.yaml` contains:

```yaml
- name: SiWx917
  repo: https://github.com/tuya/TuyaOpen-SiliconLabs
  branch: dev
  commit: <pinned-commit-sha>
```

### Step 3: Set Up Build Environment

```bash
cd ~/TuyaOpen
source export.sh
```

This script:

- Creates `~/TuyaOpen/.venv`
- Syncs Python dependencies
- Prepares host build tools

Run `source export.sh` in every new terminal session before building.

### Step 4: Configure the Application

Select the board config with `tos.py config choice` and pick **`SIWX917_AI_DEV_KIT.config`**
by name — the menu index shifts as new board configs are added.

```bash
cd ~/TuyaOpen/apps/tuya_cloud/switch_demo   # smoke test first
source ~/TuyaOpen/export.sh
tos.py config choice
```

```bash
cd ~/TuyaOpen/apps/tuya.ai/your_chat_bot    # then the AI chat bot
tos.py config choice
```

`tos.py config choice` copies the selected file over `app_default.config` — that
is how the tool records the active selection — and then regenerates the build
cache. Treat `app_default.config` as tool-managed scratch: keep durable settings
in `config/<BOARD>.config`, and do **not** commit the churn `config choice` leaves
behind. `app_default.config` is the app's default for every platform, so
committing a SiWx917 copy of it makes an unconfigured build target SiWx917 for
everyone else.

#### Advanced configuration (optional)

```bash
tos.py config menu
```

Useful options under **SiWx917**:

| Menu | Options |
|------|---------|
| Choice a board | `SIWX917_AI_DEV_KIT` (recommended), `BRD2605A` |
| Peripherals config | UART, I2S, GPIO, SPI/I2C |
| Core M4 Flash Size | `2040 KB` (default), `3008 KB` |

**Changing M4 flash size requires an MBR update on the device.** After building
with `3008 KB`, the build prints instructions and generates `mbr_config.json`:

```bash
commander manufacturing write tambr  --data mbr_config.json -d SiWG917M111MGTBA
commander manufacturing write m4mbrcf --data mbr_config.json -d SiWG917M111MGTBA
```

Commander is bundled at `platform/SiWx917/tools/commander/commander`.

### Step 5: Build

```bash
tos.py build
```

Firmware artifacts are under the app `dist/` / `.build/` directories.

## Quick Reference

| Item | Value |
|------|-------|
| Platform name | `SiWx917` |
| Board (AI kit) | `SIWX917_AI_DEV_KIT` |
| Platform repo | `platform/SiWx917` |
| Chat bot config | `apps/tuya.ai/your_chat_bot/config/SIWX917_AI_DEV_KIT.config` |
| Switch demo config | `apps/tuya_cloud/switch_demo/config/SIWX917_AI_DEV_KIT.config` |

## Next Steps

### 0. Flash TA firmware (required once per device)

Before flashing the M4 application firmware, flash the TA firmware patch **once**
on each new SiWx917 board. Skip only if TA with the matching patch is already on
the device. Wi-Fi/BLE will not work correctly with this port until it is done.

```bash
cd ~/TuyaOpen
platform/SiWx917/tools/commander/commander flash \
  platform/SiWx917/mcu/patch/RS9117_WC_SI.rps \
  --device SiWG917M111MGTBA
```

Adjust `--device` if your target part number differs.

### 1. Flash M4 application firmware

Use Simplicity Commander to flash the build output from the app's `dist/` directory.

### 2. Provision and test

1. Provision the device over BLE (see [Notes](#notes))
2. Run the example and verify Wi-Fi and cloud connectivity
3. Keep Release builds for voice apps; Debug can hurt audio real-time performance

## Notes

### Network provisioning

On SiWx917, only **BLE provisioning** works: SoftAP and BLE cannot coexist on this
chip. The adapter enforces this itself — `tkl_wifi_start_ap()` and
`tkl_wifi_set_work_mode(WWM_SOFTAP / WWM_STATIONAP)` return `OPRT_NOT_SUPPORTED`
in STA-only mode, so an app requesting `NETCFG_TUYA_BLE | NETCFG_TUYA_WIFI_AP`
still provisions over BLE. No application edit is required.

### Debug task status

To enable FreeRTOS run-time stats, add to `.build/slc/autogen/sl_event_handler.c`:

```c
__attribute__((weak)) uint32_t ulGetRunTimeCounterValue(void)
{
  return osKernelGetTickCount();
}
```

And in `.build/slc/config/FreeRTOSConfig.h`:

```c
extern uint32_t ulGetRunTimeCounterValue(void);
void vConfigureTimerForRunTimeStats(void);
#define configGENERATE_RUN_TIME_STATS 1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#define portGET_RUN_TIME_COUNTER_VALUE() ulGetRunTimeCounterValue()
```

Then call `tkl_system_print_task_stats()` from your application.

### Build and porting notes

- Chip vendor SDKs (Simplicity / Wiseconnect) are downloaded by `platform_prepare.py` and pinned in `platform_libsdepend`
- `build_setup.py` expects the app build directory as the 5th CLI argument (passed by current `tos.py` / `cli_build.py`)
- AES-GCM uses platform AES via `ENABLE_PLATFORM_AES` + `tal_aes_gcm_*`, with the scratch buffer allocated per call so concurrent TLS sessions cannot corrupt each other; any hardware failure falls back to software mbedtls
- No-codec playback volume is applied as digital gain in the board's `tdd_audio_no_codec`
- MP3 playback needs `MP3_DECODER_STATIC_BUF` (selected by the SiWx917 boards). Without it the minimp3 scratch comes from PSRAM via `MP3_MALLOC`, and the per-frame access latency stutters audibly. It trades ~24KB of internal RAM for that and makes decoding single-stream only
- Large static buffers can use `TUYA_MEM_SECTION_RAM` / `TUYA_MEM_SECTION_PSRAM` (`tuya_mem_section.h`). The section names carry **no** leading dot and the linker scripts must match exactly, or the input section is silently orphaned
- **Anti-stutter (latest TuyaOpen):** keep large `AI_*_RINGBUF_SIZE` in board config; enable both `ENABLE_EXT_RAM` (AI path) and `CONFIG_SPIRAM` (adapter). Latest defaults (20KB) are too small for voice.

## Additional Resources

- [TuyaOpen](https://github.com/tuya/TuyaOpen)
- [TuyaOpen-SiliconLabs](https://github.com/tuya/TuyaOpen-SiliconLabs)
