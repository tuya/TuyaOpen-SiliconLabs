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
import json
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
MFG_INFO_TIMEOUT = 30

# Where the version sits inside an RPS header, and what has to hold for the
# header to be one at all.
#
# Reverse-engineered, not documented: commander offers no way to read a version
# out of an .rps (rps has only convert/create/load/sign), so the layout was
# derived by matching the bundled TA image against what `mfg917 info` reported
# for a board carrying it -- all eight fields agreed, across two separate
# offsets. Treat it as a strong guess rather than a specification: check the
# structure first and report the version as unknown when anything is off,
# because the only thing acting on a wrong answer could do is overwrite a good
# NWP firmware.
RPS_HEADER_LEN = 64
RPS_MAGIC_OFFSET = 4          # uint16 LE, 0x900D
RPS_MAGIC = 0x900D
RPS_IMAGE_SIZE_OFFSET = 8     # uint32 LE, file length minus the header
RPS_VER_A_OFFSET = 0x0C       # build, security, minor, major
RPS_VER_B_OFFSET = 0x2C       # patch, customer, rom_id, chip_id

# Set to app / ta / both to answer the flash menu without a prompt, for CI and
# production programming.
FLASH_ACTION_ENV = "SIWX917_FLASH"
FLASH_ACTIONS = ("app", "ta", "both")

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


def _flash_ta(commander, device, serial, port, fixedspeed, logger) -> bool:
    """
    Write the bundled NWP/TA firmware.

    Two channels, two commands. Over serial this is the same `serial load` the
    application takes, because both are .rps images going through the ISP
    bootloader. Over SWD it is `rps load`, which is what Commander provides for
    putting an NWP image on a device -- not plain `flash`, which is a raw flash
    writer and knows nothing about the image it is handed.
    """
    image = os.path.join(_platform_root(), TA_FIRMWARE_RELPATH)
    if not _check_image(image, logger):
        return False

    if serial:
        return _flash_serial(commander, image, port, device, fixedspeed, logger)

    logger.info(f"Loading NWP/TA firmware over SWD: {os.path.basename(image)}")
    if _run([commander, "rps", "load", image, "-d", device], logger):
        return True
    logger.error(SWD_HINTS)
    return False


# -----------------------------------------------------------------------------
#                          NWP/TA firmware inspection
# -----------------------------------------------------------------------------

def _bundled_ta_version(logger) -> str:
    """Version of the TA image shipped here, or "" when it cannot be read."""
    image = os.path.join(_platform_root(), TA_FIRMWARE_RELPATH)
    try:
        size = os.path.getsize(image)
        with open(image, "rb") as f:
            head = f.read(RPS_HEADER_LEN)
    except OSError as e:
        logger.debug(f"Cannot read {image}: {e}")
        return ""

    if len(head) < RPS_HEADER_LEN:
        logger.debug(f"{image} is shorter than an RPS header")
        return ""

    magic = int.from_bytes(
        head[RPS_MAGIC_OFFSET:RPS_MAGIC_OFFSET + 2], "little")
    image_size = int.from_bytes(
        head[RPS_IMAGE_SIZE_OFFSET:RPS_IMAGE_SIZE_OFFSET + 4], "little")
    if magic != RPS_MAGIC or image_size != size - RPS_HEADER_LEN:
        logger.debug(f"{image} does not look like the RPS layout this reads "
                     f"(magic 0x{magic:04x}, image_size {image_size}, "
                     f"file {size})")
        return ""

    build, security, minor, major = head[RPS_VER_A_OFFSET:RPS_VER_A_OFFSET + 4]
    patch, customer, rom_id, chip_id = \
        head[RPS_VER_B_OFFSET:RPS_VER_B_OFFSET + 4]
    # Same field order and formatting the running firmware prints at boot, so
    # the two can be compared by eye as well as by string.
    return (f"{chip_id:x}{rom_id:x}.{major}.{minor}.{security}"
            f".{patch}.{customer}.{build}")


def _version_is_blank(version: str) -> bool:
    """A version of nothing but zeros and separators means no firmware."""
    return not [c for c in version if c not in "0.: \t"]


def _device_nwp_version(commander, device, serial, port, logger):
    """
    Ask the device what NWP/TA firmware it runs.

    return: (answered, version)

    answered says the device replied, whatever it replied. That is the
    distinction the caller needs: "this board reports no NWP firmware" is a
    fact to act on, while "nothing could be asked" -- no probe, a serial
    channel that does not carry this command, a Commander too old to report it
    -- is not, and must never be mistaken for the first.
    """
    argv = [commander, "mfg917", "info", "-d", device, "--json"]
    if serial and port:
        argv += ["--serialinterface", "--serialport", port]

    logger.debug("Reading NWP firmware version: " + " ".join(argv))
    try:
        done = subprocess.run(argv, capture_output=True, text=True,
                              timeout=MFG_INFO_TIMEOUT)
    except (OSError, subprocess.TimeoutExpired) as e:
        logger.debug(f"mfg917 info failed: {e}")
        return False, ""
    if done.returncode != 0:
        logger.debug(f"mfg917 info returned {done.returncode}")
        return False, ""

    try:
        info = json.loads(done.stdout)["result"]["mfg917_info"]
    except (ValueError, KeyError, TypeError) as e:
        logger.debug(f"Cannot parse mfg917 info output: {e}")
        return False, ""

    version = str(info.get("nwp_firmware_version", "")).strip()
    return True, "" if _version_is_blank(version) else version


def _choose_action(commander, device, serial, port, logger) -> str:
    """
    Decide what to write. Returns one of FLASH_ACTIONS, or "" to abort.

    The application cannot run without NWP/TA firmware, so a device that has
    none gets both. A device that already has some keeps it, whatever version
    it is: writing NWP firmware erases it first, which is a window where losing
    power leaves the radio unusable, and the image bundled here is not
    necessarily newer than what a board shipped with. Replacing a working one
    is a decision for whoever has a reason, spelled SIWX917_FLASH=ta.

    Only a device that answered can be said to have no firmware. When nothing
    could be asked, this writes the application alone and says so -- guessing
    the other way would erase NWP firmware over a probe that merely hiccuped.
    """
    forced = os.environ.get(FLASH_ACTION_ENV, "").strip().lower()
    # SIWX917_FLASH_TA=1 predates this and meant "write the TA firmware instead
    # of the application". Keep it working as a name for that.
    if not forced and os.environ.get("SIWX917_FLASH_TA") == "1":
        forced = "ta"
    if forced:
        if forced not in FLASH_ACTIONS:
            logger.error(f"{FLASH_ACTION_ENV}={forced} is not one of "
                         f"{'/'.join(FLASH_ACTIONS)}")
            return ""
        logger.info(f"{FLASH_ACTION_ENV}={forced} -- honouring it as given.")
        return forced

    answered, device_ver = _device_nwp_version(
        commander, device, serial, port, logger)

    if answered and device_ver:
        logger.info(f"Device NWP/TA firmware: {device_ver} -- flashing the "
                    "application only.")
        return "app"

    if answered:
        logger.warning("Device reports no NWP/TA firmware -- flashing it "
                       f"first, from {TA_FIRMWARE_RELPATH} "
                       f"({_bundled_ta_version(logger) or 'version unknown'}).")
        return "both"

    logger.warning("Could not read the device's NWP/TA firmware version, so "
                   "this will not write one: flashing the application only.")
    logger.warning("If the application does not start, the radio may have no "
                   f"firmware. Write it with {FLASH_ACTION_ENV}=both.")
    return "app"


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
    The device carries two firmwares: the M4 application built here, and the
    NWP/TA firmware the radio runs. The application needs one to start, so a
    device that reports having none gets it written first, from mcu/patch. A
    device that already has some keeps it. SIWX917_FLASH=app|ta|both overrides
    the whole decision.
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

    # Settle the channel first: reading the device's NWP firmware version, which
    # decides what gets written, goes over the same connection.
    if port:
        logger.info(f"Port given ({port}) -- using serial/ISP.")
        serial, chosen = True, port
    elif _swd_reachable(commander, device, logger):
        logger.info("Debug probe reached the chip -- using SWD (no keypress "
                    "needed). Pass -p <port> to force serial/ISP.")
        serial, chosen = False, ""
    else:
        ports = _usb_serial_ports()
        chosen = _sole_port(ports, logger)
        if not chosen:
            logger.error("No way to reach the device: no debug probe responded and "
                         f"{'no USB serial adapter was found' if not ports else 'the port is ambiguous'}.")
            logger.error(SWD_HINTS)
            logger.error(SERIAL_HINTS)
            return {"success": False, "message": "no usable flash channel found"}
        serial = True
        logger.info(f"No debug probe responded -- using serial/ISP on {chosen}.")

    action = _choose_action(commander, device, serial, chosen, logger)
    if not action:
        return {"success": False, "message": "flash cancelled"}

    if action in ("ta", "both"):
        if not _flash_ta(commander, device, serial, chosen, fixedspeed, logger):
            return {"success": False, "message": "TA firmware flash failed"}
        if action == "ta":
            return {"success": True, "message": ""}

    channel = "serial/ISP" if serial else "SWD"
    if serial:
        ok = _flash_serial(commander, rps, chosen, device, fixedspeed, logger)
    else:
        ok = _flash_swd(commander, s37, device, logger)
    return {"success": ok, "message": "" if ok else f"{channel} flash failed"}


def _sole_port(ports: list, logger) -> str:
    """One adapter is unambiguous; several need the user to say which."""
    if len(ports) == 1:
        return ports[0]
    if len(ports) > 1:
        logger.error(f"Several USB serial adapters found: {', '.join(ports)}")
        logger.error("Pick one with: tos.py flash -p <port>")
    return ""
