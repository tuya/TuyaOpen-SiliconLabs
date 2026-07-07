# Getting Started with TuyaOpen for SiWx917 Platform

This guide explains how to set up and build TuyaOpen with SiWx917 support on Linux.

The SiWx917 platform is **not** a standalone project. It must live inside the TuyaOpen tree at:

```text
~/TuyaOpen/platform/SiWx917
```

## Table of Contents

- [Prerequisites](#prerequisites)
- [Project Structure](#project-structure)
- [Overview](#overview)
- [Step-by-Step Setup](#step-by-step-setup)
  - [Step 1: Clone TuyaOpen](#step-1-clone-tuyaopen)
  - [Step 2: Clone SiWx917 Platform](#step-2-clone-siwx917-platform)
  - [Step 3: Apply TuyaOpen Patch](#step-3-apply-tuyaopen-patch)
  - [Step 4: Set Up Build Environment](#step-4-set-up-build-environment)
  - [Step 5: Configure the Application](#step-5-configure-the-application)
  - [Step 6: Build](#step-6-build)
- [Quick Reference](#quick-reference)
- [Next Steps](#next-steps)
- [Notes](#notes)
- [Additional Resources](#additional-resources)

## Prerequisites

- **OS:** Ubuntu 22.04 LTS (recommended) or compatible Linux
- **Git**
- **Python 3.10+**
- **SSH key** (only if cloning the platform repository via SSH)
- Build tools are installed automatically by `export.sh` (CMake, Ninja, and Python packages)

Verify basics:

```bash
python3 --version
git --version
```

## Project Structure

```text
~/TuyaOpen/
├── .venv/
├── apps/tuya.ai/your_chat_bot/
│   └── config/SIWX917_AI_DEV_KIT.config
├── boards/SiWx917/                  # from patch
├── platform/
│   ├── platform_config.yaml
│   └── SiWx917/                     # platform repository
│       ├── tuyaos_adapter/
│       ├── mcu/
│       │   └── patch/RS9117_WC_SI.rps   # TA patch (flash once per device)
│       ├── slc/
│       ├── tools/
│       └── projects/tuya_open_patch/v1.6.0/01_TuyaOpen_v1.6.0.patch
└── export.sh
```

## Overview

Setup flow:

1. Clone [TuyaOpen](https://github.com/tuya/TuyaOpen) v1.6.0 into `~/TuyaOpen`
2. Clone the SiWx917 platform repository into `~/TuyaOpen/platform/SiWx917`
3. Apply the integration patch from the platform repo onto TuyaOpen
4. Configure and build the `your_chat_bot` example

## Step-by-Step Setup

### Step 1: Clone TuyaOpen

```bash
mkdir -p ~/TuyaOpen
cd ~/TuyaOpen
git clone https://github.com/tuya/TuyaOpen.git .
git checkout v1.6.0
```

**Details:**
- **URL:** https://github.com/tuya/TuyaOpen.git
- **Tag:** v1.6.0
- **Commit:** b07b9b2dfab59ed4f49cd635ac6f46ce9336268b

After checking out the tag, Git reports a detached HEAD state. That is expected.

### Step 2: Clone SiWx917 Platform

Clone the [TuyaOpen-SiliconLabs](https://github.com/tuya/TuyaOpen-SiliconLabs) repository **into** `platform/SiWx917` inside the TuyaOpen tree.

```bash
cd ~/TuyaOpen
mkdir -p platform
git clone https://github.com/tuya/TuyaOpen-SiliconLabs.git platform/SiWx917
cd platform/SiWx917
git checkout master
```

**Details:**
- **URL:** https://github.com/tuya/TuyaOpen-SiliconLabs
- **Location:** `~/TuyaOpen/platform/SiWx917` (required by the TuyaOpen build system)
- **Branch:** `master` (or the release branch documented in your version)

Update `platform/platform_config.yaml` if the pinned commit differs from your checkout:

```yaml
- name: SiWx917
  repo: https://github.com/tuya/TuyaOpen-SiliconLabs.git
  branch: master
  commit: <pinned-commit-sha>
```

### Step 3: Apply TuyaOpen Patch

The patch adds SiWx917 board support and required changes to the TuyaOpen tree (boards, Kconfig, example apps, and so on).

```bash
cd ~/TuyaOpen
git apply --ignore-whitespace --reject \
  platform/SiWx917/projects/tuya_open_patch/v1.6.0/01_TuyaOpen_v1.6.0.patch
```

**Patch file:**
- `platform/SiWx917/projects/tuya_open_patch/v1.6.0/01_TuyaOpen_v1.6.0.patch`

Trailing-whitespace warnings during `git apply` are informational and can be ignored.

If the patch is already applied, `git apply` will fail. Check for `boards/SiWx917/` in the TuyaOpen tree to confirm.

### Step 4: Set Up Build Environment

```bash
cd ~/TuyaOpen
source export.sh
```

This script:
- Creates `~/TuyaOpen/.venv`
- Installs Python dependencies from `requirements.txt`
- Prepares CMake, Ninja, and other build tools

Run `source export.sh` in every new terminal session before building.

### Step 5: Configure the Application

```bash
cd ~/TuyaOpen/apps/tuya.ai/your_chat_bot
source ~/TuyaOpen/export.sh
tos.py config choice
```

Select **`SIWX917_AI_DEV_KIT.config`** from the list. The menu index can change as new board configs are added, so always choose by **name**, not by a fixed number.

As of TuyaOpen v1.6.0, `SIWX917_AI_DEV_KIT.config` is typically option **6**:

```text
Choice config file: 6    # SIWX917_AI_DEV_KIT.config
```

Saved config path:

```text
~/TuyaOpen/apps/tuya.ai/your_chat_bot/config/SIWX917_AI_DEV_KIT.config
```

#### Advanced configuration (optional)

```bash
tos.py config menu
```

Useful options under **SiWx917**:

| Menu | Options |
|------|---------|
| Choice a board | `SIWX917_AI_DEV_KIT` (recommended), `BRD2605A` |
| Peripherals config | UART, I2S, GPIO, SPI/I2C |
| Flash Configuration → M4 Flash Size | `2040 KB` (default), `3008 KB` |

**M4 flash size change:** If you switch from 2040 KB to 3008 KB, update the device MBR after build. The build prints instructions and generates `mbr_config.json`. Example:

```bash
commander manufacturing write tambr --data mbr_config.json -d SiWG917M111MGTBA
commander manufacturing write m4mbrcf --data mbr_config.json -d SiWG917M111MGTBA
```

Commander is bundled at `platform/SiWx917/tools/commander/commander`.

### Step 6: Build

```bash
cd ~/TuyaOpen/apps/tuya.ai/your_chat_bot
source ~/TuyaOpen/export.sh
tos.py build
```

**Build flow:**
1. Detect `platform/SiWx917` (skip auto-download if already present)
2. Run platform `build_setup.py`
3. Configure via Kconfig
4. Compile with CMake and Ninja

Build output is under the project `build/` directory.

If the platform commit does not match `platform/platform_config.yaml`, `tos.py` may prompt to update. For local development you can keep your checked-out branch/commit.

## Quick Reference

```bash
# 1. Clone TuyaOpen
mkdir -p ~/TuyaOpen && cd ~/TuyaOpen
git clone https://github.com/tuya/TuyaOpen.git .
git checkout v1.6.0

# 2. Clone platform into platform/SiWx917
mkdir -p platform
git clone https://github.com/tuya/TuyaOpen-SiliconLabs.git platform/SiWx917

# 3. Apply patch
git apply --ignore-whitespace --reject \
  platform/SiWx917/projects/tuya_open_patch/v1.6.0/01_TuyaOpen_v1.6.0.patch

# 4. Environment
source export.sh

# 5. Configure (select SIWX917_AI_DEV_KIT.config)
cd apps/tuya.ai/your_chat_bot
tos.py config choice

# 6. Build
tos.py build
```

## Next Steps

After a successful build:

### 0. Flash TA firmware (required, once per device)

**Important:** Before flashing the M4 application firmware, you must flash the TA firmware patch **once** on each new SiWx917 board. Skip this step only if TA with the matching patch is already on the device.

**File:**

```text
platform/SiWx917/mcu/patch/RS9117_WC_SI.rps
```

**Tool:** Simplicity Commander (bundled in the platform repo):

```text
platform/SiWx917/tools/commander/commander
```

**Example:**

```bash
cd ~/TuyaOpen
platform/SiWx917/tools/commander/commander flash \
  platform/SiWx917/mcu/patch/RS9117_WC_SI.rps \
  --device SiWG917M111MGTBA
```

Adjust `--device` to match your target part number if it differs. This step is mandatory for Wi-Fi/BLE to work correctly with the TuyaOpen port.

### 1. Flash M4 application firmware

Use Simplicity Commander to flash the build output from the `your_chat_bot` project.

### 2. Provision and test

1. **Provision the device** over BLE (see [Notes](#notes))
2. **Run the chatbot example** and verify Wi-Fi and cloud connectivity
3. **Customize** the application for your product

## Notes

### Network provisioning

On SiWx917, only **BLE provisioning** is supported. Do not use `NETCFG_TUYA_WIFI_AP` or combined `NETCFG_TUYA_BLE | NETCFG_TUYA_WIFI_AP`.

The TuyaOpen v1.6.0 integration patch already sets this in `apps/tuya.ai/your_chat_bot/src/tuya_main.c`:

```c
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_NETCFG, &(netcfg_args_t){.type = NETCFG_TUYA_BLE});
#endif
```

No manual edit is needed after applying the patch.

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

Call `tkl_system_print_task_stats()` from your application (for example in `app_chat_bot.c`) to print task usage.

## Additional Resources

- **TuyaOpen:** https://github.com/tuya/TuyaOpen
- **TuyaOpen docs:** https://tuyaopen.ai/docs/about-tuyaopen
- **Environment setup:** https://tuyaopen.ai/docs/quick_start/enviroment-setup
- **Tuya AI Agent:** https://developer.tuya.com/en/docs/iot/ai-agent-management
- **SiWx917 platform:** https://github.com/tuya/TuyaOpen-SiliconLabs

## Version Information

| Component | Version |
|-----------|---------|
| TuyaOpen | v1.6.0 (`b07b9b2`) |
| Integration patch | `01_TuyaOpen_v1.6.0.patch` |
| Platform repo | [TuyaOpen-SiliconLabs](https://github.com/tuya/TuyaOpen-SiliconLabs) |
| Target board | `SIWX917_AI_DEV_KIT` |
