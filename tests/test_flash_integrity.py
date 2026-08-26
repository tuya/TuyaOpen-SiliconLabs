#!/usr/bin/env python3
"""Cover the NWP integrity check in platform_flash_bridge.py with a fake port."""

import importlib.util
import logging
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PLAT = os.path.dirname(HERE)
BRIDGE = os.path.join(PLAT, "platform_flash_bridge.py")

failures = []
checks = 0


def expect(cond, what):
    global checks
    checks += 1
    if cond:
        print(f"  ok   {what}")
    else:
        print(f"  FAIL {what}")
        failures.append(what)


class FakePort:
    def __init__(self, menu=True, replies=None, default="Integrity Passed"):
        self.menu = menu
        self.replies = replies or {}
        self.default = default
        self.pending = b""
        self.writes = []

    def reset_input_buffer(self):
        self.pending = b""

    def flush(self):
        pass

    def close(self):
        pass

    def write(self, data):
        self.writes.append(data)
        if data == b"U":
            self.pending += (b"SiWx917 BootLoader menu\r\n" if self.menu
                             else b"garbage\r\n")
        elif data == b"K":
            self.pending += b"Image No (0-f): "
        elif len(data) == 1 and data.decode() in "0123456789abcdef":
            text = self.replies.get(data.decode(), self.default)
            self.pending += b"" if text is None else text.encode()

    def read(self, _n):
        out, self.pending = self.pending, b""
        return out


class SerialStub:
    def __init__(self, port):
        self.port = port

    def Serial(self, *_a, **_k):
        if self.port is None:
            raise OSError("no such port")
        return self.port


def load(serial_stub):
    spec = importlib.util.spec_from_file_location("bridge", BRIDGE)
    b = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(b)
    b._serial_module = lambda logger: serial_stub
    b._drain = lambda sp, total=2.0, quiet=0.4: sp.read(4096)
    return b


def main():
    if not os.path.isfile(BRIDGE):
        print(f"  skip {BRIDGE} not found")
        return 0
    log = logging.getLogger("t")
    logging.basicConfig(level=logging.CRITICAL)

    b = load(SerialStub(FakePort()))
    expect(b._report_integrity("/dev/fake", 115200, log) is True,
           "every slot intact -> True")

    b = load(SerialStub(FakePort(default="Integrity Failed")))
    expect(b._report_integrity("/dev/fake", 115200, log) is False,
           "every slot broken -> False, not None")

    b = load(SerialStub(FakePort(menu=False)))
    expect(b._report_integrity("/dev/fake", 115200, log) is None,
           "no bootloader menu -> None, distinct from 'no intact slot'")

    b = load(SerialStub(None))
    expect(b._report_integrity("/dev/fake", 115200, log) is None,
           "port will not open -> None")

    b = load(SerialStub(FakePort(default=None)))
    expect(b._report_integrity("/dev/fake", 115200, log) is None,
           "slots never reply -> None, not False")

    b = load(SerialStub(FakePort(default="wibble")))
    expect(b._report_integrity("/dev/fake", 115200, log) is None,
           "replies nobody recognises -> None, never mistaken for broken")

    b = load(SerialStub(FakePort(
        replies={"0": "Integrity Failed"}, default="Integrity Passed")))
    expect(b._report_integrity("/dev/fake", 115200, log) is True,
           "bad slot 0 beside good slots -> True")

    b = load(SerialStub(FakePort(
        replies={"5": "Integrity Passed"}, default="Integrity Failed")))
    expect(b._report_integrity("/dev/fake", 115200, log) is True,
           "one good slot among broken ones -> True")

    b = load(SerialStub(FakePort(
        replies={"0": "Integrity Failed"}, default=None)))
    expect(b._report_integrity("/dev/fake", 115200, log) is False,
           "one slot says broken and the rest are silent -> False")

    b = load(SerialStub(FakePort(
        replies={"0": "wibble"}, default=None)))
    expect(b._report_integrity("/dev/fake", 115200, log) is None,
           "unreadable plus silent, nothing says broken -> None")

    port = FakePort()
    b = load(SerialStub(port))
    b._report_integrity("/dev/fake", 115200, log)
    asked = sorted(set(w.decode() for w in port.writes
                       if len(w) == 1 and w.decode() in "0123456789abcdef"))
    expect(asked == list("0123456789abcdef"), "all 16 slots are asked")
    expect(all(w in (b"U", b"K", b"\x1c") or len(w) == 1
               for w in port.writes),
           "nothing but wake/U/K/slot is written, so no flash is touched")

    b = load(SerialStub(FakePort()))
    os.environ["SIWX917_ISP_BAUD"] = "921600"
    expect(b._isp_baud(log) == 921600, "SIWX917_ISP_BAUD is honoured")
    os.environ["SIWX917_ISP_BAUD"] = "nonsense"
    expect(b._isp_baud(log) == 115200, "a bad SIWX917_ISP_BAUD falls back")
    del os.environ["SIWX917_ISP_BAUD"]
    expect(b._isp_baud(log) == 115200, "default ISP baud is 115200")

    print(f"\n{checks - len(failures)}/{checks} checks passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
