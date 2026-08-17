#!/usr/bin/env python3
# coding=utf-8
"""
SiWx917 flashing hook for `tos.py flash`.

tools/cli_command/cli_flash.py looks for platform_flash_bridge.py in the
selected platform's root and, when present, calls platform_flash() instead of
tyutool. SiWx917 needs its own path because Simplicity Commander drives two
physically different channels, and using the wrong one corrupts the firmware:

    channel        file    command                    needs
    -------------  ------  -------------------------  -------------------------
    SWD / J-Link   .s37    commander flash            a debug probe
    serial / ISP   .rps    commander serial load      manual ISP+Reset keypress

`tos.py flash` with no arguments has to work here the same way it does on every
other platform, so the channel is detected rather than demanded:

    1. an explicit -p <port> always means serial;
    2. otherwise, probe SWD -- if a debug probe can reach the chip, use it,
       because that path needs no keypress at all;
    3. otherwise fall back to the single attached USB serial adapter.

The SWD probe costs about 1.5s when no probe is attached, which is cheap enough
to pay for not having to ask the user which cable they plugged in. It also
fails in exactly the cases where serial is the right answer -- no probe, board
held in ISP mode (SWO/GPIO_34 tied low disables the debug interface) -- so the
fallback lands correctly instead of guessing from USB IDs.

A .rps is never handed to `flash` and a .s37 is never handed to `serial load`;
see _guard_channel(). The NWP/TA (wireless coprocessor) firmware is only
reflashed when provisioning a fresh board or recovering a corrupted one, so it
is opt-in via SIWX917_FLASH_TA=1 rather than something this can do by accident.

This module is loaded by importlib.util.spec_from_file_location() under a
synthetic module name, so its package context is not set up and it cannot
import the platform's script.util helpers -- hence the local platform.system()
call instead of reusing get_system_name().
"""

import glob
import os
import platform
import subprocess

# -----------------------------------------------------------------------------
#                                  Constants
# -----------------------------------------------------------------------------

DEFAULT_DEVICE = "SiWG917M111MGTBA"

# Silicon Labs publishes a full build and a CLI-only build of Commander, and
# they carry different executable names. Both answer the same 28 subcommands
# (verified), and only CLI subcommands are used here, so either will do.
# script/bootstrap_silabs flattens whichever it installed into tools/commander/.
# Ordered by preference; the first hit wins.
COMMANDER_CANDIDATES = {
    "Linux": ("commander", "commander-cli"),
    "Darwin": ("commander", "commander-cli",
               "Commander.app/Contents/MacOS/commander"),
    "Windows": ("commander.exe", "commander-cli.exe"),
}

# Fallback globs for when pyserial is unavailable. Windows has no usable glob
# for COM ports, which is why it relies on pyserial or an explicit -p.
SERIAL_GLOBS = {
    "Linux": ("/dev/ttyUSB*", "/dev/ttyACM*"),
    "Darwin": ("/dev/cu.usbserial*", "/dev/cu.usbmodem*", "/dev/cu.SLAB_USBtoUART*"),
    "Windows": (),
}

TA_FIRMWARE_RELPATH = os.path.join("mcu", "patch", "RS9117_WC_SI.rps")

SWD_PROBE_TIMEOUT = 20

ISP_PROMPT = """
--------------------------------------------------------------------
 Serial (ISP) flashing -- this needs one manual step.

 The chip only listens on the ISP UART while it is held in ISP mode,
 and it does not stay there across commands. Right before the
 transfer starts:

   1. pull SWO / GPIO_34 low (ISP button, or a wire to GND)
   2. tap Reset

 Do not interrupt the transfer once it begins writing. A partial
 write can leave the device unbootable and recoverable only through
 a different procedure.
--------------------------------------------------------------------
"""

SWD_HINTS = """  SWD troubleshooting:
    - is the board powered?
    - SWCLK on GPIO_31, SWDIO on GPIO_33?
    - SWO / GPIO_34 tied low holds the chip in ISP mode, which
      disables the debug interface -- release it for SWD."""

SERIAL_HINTS = """  Serial/ISP troubleshooting:
    - chip must be in ISP mode: SWO / GPIO_34 low, then Reset
    - CH340 TX -> GPIO_8 (chip RX), CH340 RX -> GPIO_9 (chip TX)
    - GND must be common between adapter and board"""


# -----------------------------------------------------------------------------
#                              Internal helpers
# -----------------------------------------------------------------------------

def _platform_root() -> str:
    return os.path.dirname(os.path.abspath(__file__))


def _find_commander(logger) -> str:
    """Locate the Simplicity Commander executable for this host OS."""
    host = platform.system()
    candidates = COMMANDER_CANDIDATES.get(host)
    if candidates is None:
        logger.error(f"Unsupported host OS for SiWx917 flashing: {host}")
        return ""

    base = os.path.join(_platform_root(), "tools", "commander")
    for rel in candidates:
        path = os.path.join(base, rel)
        if os.path.isfile(path) and (host == "Windows" or os.access(path, os.X_OK)):
            logger.debug(f"Using commander: {path}")
            return path

    logger.error(f"Simplicity Commander not found under {base}")
    logger.error("Run ./script/bootstrap_silabs in the platform directory to "
                 "install it.")
    return ""


def _device(using_data) -> str:
    return using_data.get("CONFIG_CHIP_CHOICE", "") or DEFAULT_DEVICE


def _artifact_paths(using_data, binfile: str):
    """
    Derive the app images from the QIO bin path cli_flash.py hands us.

      binfile   <app>/.build/bin/<project>_QIO_<ver>.bin
      s37       <app>/.build/bin/<project>_<ver>.s37
      rps       <app>/.build/<project>/<project>.rps
    """
    bin_dir = os.path.dirname(binfile)
    build_dir = os.path.dirname(bin_dir)
    project = using_data.get("CONFIG_PROJECT_NAME", "")
    version = using_data.get("CONFIG_PROJECT_VERSION", "")

    s37 = os.path.join(bin_dir, f"{project}_{version}.s37")
    rps = os.path.join(build_dir, project, f"{project}.rps")
    return s37, rps


def _usb_serial_ports() -> list:
    """
    USB serial adapters only.

    pyserial lists every legacy /dev/ttyS* alongside the real adapters, and
    only USB devices carry a vendor ID, so that is the discriminator.
    """
    try:
        from serial.tools import list_ports
        return sorted(p.device for p in list_ports.comports()
                      if p.vid is not None)
    except ImportError:
        found = []
        for pattern in SERIAL_GLOBS.get(platform.system(), ()):
            found.extend(glob.glob(pattern))
        return sorted(found)


def _swd_reachable(commander: str, device: str, logger) -> bool:
    """Can a debug probe reach the chip right now? Quiet, read-only."""
    logger.debug("Probing for a debug probe (commander device info)...")
    try:
        done = subprocess.run([commander, "device", "info", "-d", device],
                              stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL,
                              timeout=SWD_PROBE_TIMEOUT)
        return done.returncode == 0
    except (OSError, subprocess.TimeoutExpired):
        return False


def _guard_channel(image: str, serial: bool, logger) -> bool:
    """
    Refuse the file/channel combinations that damage the device.

    Commander will happily accept a .rps over SWD and leave the NWP firmware
    in a broken state, so the check has to happen here rather than relying on
    the tool to reject it.
    """
    ext = os.path.splitext(image)[1].lower()
    if serial and ext != ".rps":
        logger.error(f"Serial (ISP) flashing needs a .rps image, got: {image}")
        return False
    if not serial and ext != ".s37":
        logger.error(f"SWD flashing needs a .s37 image, got: {image}")
        return False
    return True


def _run(argv, logger) -> bool:
    """Run commander with its output going straight through to the terminal."""
    logger.debug("exec: " + " ".join(argv))
    try:
        return subprocess.run(argv).returncode == 0
    except OSError as e:
        logger.error(f"Failed to launch commander: {e}")
        return False


def _check_image(image: str, logger) -> bool:
    if os.path.isfile(image):
        return True
    logger.error(f"Image not found: {image}")
    logger.error("Build first with [tos.py build].")
    return False


def _flash_serial(commander, image, port, device, fixedspeed, logger) -> bool:
    if not _guard_channel(image, serial=True, logger=logger):
        return False
    if not _check_image(image, logger):
        return False

    logger.info(ISP_PROMPT)
    argv = [commander, "serial", "load", image,
            "--serialport", port, "--showprogress", "-d", device]
    if fixedspeed:
        argv.append("--fixedspeed")

    if _run(argv, logger):
        return True
    logger.error(SERIAL_HINTS)
    return False


def _flash_swd(commander, image, device, logger) -> bool:
    if not _guard_channel(image, serial=False, logger=logger):
        return False
    if not _check_image(image, logger):
        return False

    logger.info(f"SWD flashing via debug probe: {os.path.basename(image)}")
    if _run([commander, "flash", image, "-d", device], logger):
        return True
    logger.error(SWD_HINTS)
    return False


# -----------------------------------------------------------------------------
#                                 Entry point
# -----------------------------------------------------------------------------

def platform_flash(using_data=None,
                   binfile="",
                   port="",
                   baud=0,
                   boards_root="",
                   logger=None,
                   **kwargs) -> dict:
    """
    Flash a SiWx917 target. Returns {"success": bool, "message": str}.

    Channel is auto-detected (see module docstring); -p <port> forces serial.
    Set SIWX917_FLASH_TA=1 to flash the NWP/TA firmware instead of the app
    (serial only, and only needed on a fresh or bricked device).
    """
    using_data = using_data or {}

    commander = _find_commander(logger)
    if not commander:
        return {"success": False, "message": "Simplicity Commander not available"}

    device = _device(using_data)
    s37, rps = _artifact_paths(using_data, binfile)

    # commander negotiates 921600 baud before transferring and aborts the whole
    # load when that handshake fails -- which is what a CH340-class adapter
    # typically does. --fixedspeed skips the handshake and transfers at the
    # default rate, so it is the default here: a slower flash beats one that
    # never completes. Set SIWX917_HIGHSPEED=1 for an adapter that can take it.
    fixedspeed = os.environ.get("SIWX917_HIGHSPEED") != "1"
    if baud:
        logger.info(f"-b {baud} is not used: commander offers only its own "
                    "negotiated rate or --fixedspeed, not an arbitrary value.")

    if os.environ.get("SIWX917_FLASH_TA") == "1":
        ta_port = port or _sole_port(_usb_serial_ports(), logger)
        if not ta_port:
            return {"success": False,
                    "message": "TA firmware needs serial/ISP: pass -p <port>"}
        ta_image = os.path.join(_platform_root(), TA_FIRMWARE_RELPATH)
        logger.info("SIWX917_FLASH_TA=1 -- flashing NWP/TA firmware, not the app.")
        ok = _flash_serial(commander, ta_image, ta_port, device, fixedspeed, logger)
        return {"success": ok,
                "message": "" if ok else "TA firmware flash failed"}

    if port:
        logger.info(f"Port given ({port}) -- using serial/ISP.")
        ok = _flash_serial(commander, rps, port, device, fixedspeed, logger)
        return {"success": ok, "message": "" if ok else "serial/ISP flash failed"}

    # No port: pick the channel ourselves so a bare `tos.py flash` works.
    if _swd_reachable(commander, device, logger):
        logger.info("Debug probe reached the chip -- using SWD (no keypress "
                    "needed). Pass -p <port> to force serial/ISP.")
        ok = _flash_swd(commander, s37, device, logger)
        return {"success": ok, "message": "" if ok else "SWD flash failed"}

    ports = _usb_serial_ports()
    chosen = _sole_port(ports, logger)
    if not chosen:
        logger.error("No way to reach the device: no debug probe responded and "
                     f"{'no USB serial adapter was found' if not ports else 'the port is ambiguous'}.")
        logger.error(SWD_HINTS)
        logger.error(SERIAL_HINTS)
        return {"success": False, "message": "no usable flash channel found"}

    logger.info(f"No debug probe responded -- using serial/ISP on {chosen}.")
    ok = _flash_serial(commander, rps, chosen, device, fixedspeed, logger)
    return {"success": ok, "message": "" if ok else "serial/ISP flash failed"}


def _sole_port(ports: list, logger) -> str:
    """One adapter is unambiguous; several need the user to say which."""
    if len(ports) == 1:
        return ports[0]
    if len(ports) > 1:
        logger.error(f"Several USB serial adapters found: {', '.join(ports)}")
        logger.error("Pick one with: tos.py flash -p <port>")
    return ""
