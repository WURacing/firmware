"""Download Feather logs or convert them offline. Python 3.10+; Windows."""
from __future__ import annotations

import argparse
import csv
import datetime as dt
import hashlib
import io
import json
import os
from pathlib import Path
import re
import struct
import time
import zlib

HEADER = struct.Struct("<4sBBBB8s")
RECORD = struct.Struct("<IBB")
NAME = re.compile(r"[0-9]{6}\.BIN\Z")


def decode(data: bytes):
    if len(data) < HEADER.size:
        raise ValueError("Truncated binary header")
    magic, version, size, mask, hz, reserved = HEADER.unpack_from(data)
    if (magic, version, size, hz, reserved) != (b"VCSL", 1, 6, 50, b"\0" * 8) or not mask:
        raise ValueError("Unsupported or corrupt binary header")
    if (len(data) - HEADER.size) % RECORD.size:
        raise ValueError("Truncated record; retain binary for recovery")
    previous = -1
    for timestamp, channel, distance in RECORD.iter_unpack(data[HEADER.size:]):
        if channel > 7 or not mask & (1 << channel):
            raise ValueError("Record channel does not match header")
        if timestamp < previous:
            raise ValueError("Timestamps went backwards")
        previous = timestamp
        yield timestamp, channel, distance


def csv_bytes(data: bytes, positions: dict[str, str]) -> bytes:
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("time_ms", "channel", "position", "distance_mm"))
    for timestamp, channel, distance in decode(data):
        writer.writerow((timestamp, channel, positions.get(str(channel), f"channel_{channel}"), distance))
    return output.getvalue().encode("utf-8")


def desktop_directory() -> Path:
    # Windows Known Folder API handles OneDrive/redirected Desktop folders.
    if os.name == "nt":
        import ctypes
        path = ctypes.create_unicode_buffer(32768)
        if ctypes.windll.shell32.SHGetFolderPathW(None, 0x10, None, 0, path) == 0:
            return Path(path.value)
    return Path.home() / "Desktop"


def save_download(data: bytes, name: str, output: Path, positions: dict[str, str]) -> Path:
    if not NAME.fullmatch(name):
        raise ValueError("Invalid device filename")
    converted = csv_bytes(data, positions)  # Fully validate before committing files.
    digest = hashlib.sha256(data).hexdigest()[:16]
    output.mkdir(parents=True, exist_ok=True)
    # Content key avoids overwriting another run or repeatedly downloading identical data.
    stem = f"{Path(name).stem}_{digest}"
    binary = output / f"{stem}.bin"
    csv_path = output / f"{stem}.csv"
    for target, content in ((binary, data), (csv_path, converted)):
        temporary = target.with_suffix(target.suffix + ".part")
        with temporary.open("wb") as f:
            f.write(content)
            f.flush()
            os.fsync(f.fileno())
        os.replace(temporary, target)
    return csv_path


class Device:
    def __init__(self, port):
        self.port = port

    def line(self) -> str:
        raw = self.port.readline(256)
        if not raw.endswith(b"\n"):
            raise TimeoutError("Incomplete device response; check connection")
        result = raw.decode("ascii").strip()
        if result.startswith("ERR "):
            raise RuntimeError(result)
        return result

    def command(self, command: str) -> str:
        self.port.write((command + "\n").encode("ascii"))
        self.port.flush()
        return self.line()

    def files(self) -> list[str]:
        result = self.command("LIST")
        names = []
        while result != "END":
            fields = result.split()
            if len(fields) != 3 or fields[0] != "FILE" or not NAME.fullmatch(fields[1]):
                raise ValueError(f"Unexpected listing: {result}")
            names.append(fields[1])
            result = self.line()
        return sorted(names)

    def get(self, name: str) -> bytes:
        if not NAME.fullmatch(name):
            raise ValueError("Expected a filename such as 000001.BIN")
        fields = self.command(f"GET {name}").split()
        if len(fields) != 2 or fields[0] != "DATA":
            raise ValueError("Missing download header")
        size = int(fields[1])
        if not 16 <= size <= 64 * 1024 * 1024:
            raise ValueError("Implausible file length")
        data = bytearray()
        while len(data) < size:
            block = self.port.read(min(4096, size - len(data)))
            if not block:
                raise TimeoutError("Download interrupted; onboard file has not been deleted")
            data.extend(block)
        if self.port.read(1) != b"\n":
            raise ValueError("Missing transfer delimiter")
        checksum = self.line().split()
        if len(checksum) != 2 or checksum[0] != "CRC":
            raise ValueError("Missing transfer checksum")
        if zlib.crc32(data) != int(checksum[1], 16):
            raise ValueError("CRC mismatch; retry download")
        return bytes(data)


def connect(port_name: str):
    import serial
    port = serial.Serial(port_name, 115200, timeout=5, write_timeout=5)
    try:
        time.sleep(0.4)
        port.reset_input_buffer()
        device = Device(port)
        if device.command("HELLO") != "VCSEL_LOGGER 1":
            raise RuntimeError("This port is not running the Feather logger")
        return device
    except Exception:
        port.close()
        raise


def run_device(device: Device, args, positions) -> None:
    if args.action == "status":
        print(device.command("INFO"))
        return
    if args.action == "start":
        print(device.command("START"))
        return
    print(device.command("STOP"))
    print(device.command("INFO"))
    if args.action == "stop":
        return
    if args.action == "delete":
        if not args.name or not NAME.fullmatch(args.name):
            raise ValueError("delete requires --name 000001.BIN")
        print(device.command(f"DELETE {args.name}"))
        return
    for name in device.files():
        data = device.get(name)
        path = save_download(data, name, args.output, positions)
        print(f"Saved {path} ({(len(data) - 16) // 6} readings)")
    print("Download complete. Onboard files retained; recording remains stopped.")


def watch(args, positions) -> None:
    from serial.tools import list_ports
    handled: set[tuple] = set()
    retry_after: dict[tuple, float] = {}
    print("Waiting for Feather M4. Leave this window open; Ctrl+C exits.")
    while True:
        ports = [p for p in list_ports.comports()
                 if (p.device == args.port if args.port else p.vid == 0x239A)]
        current = {(p.device, p.serial_number) for p in ports}
        handled.intersection_update(current)
        for p in ports:
            key = (p.device, p.serial_number)
            if key in handled or time.monotonic() < retry_after.get(key, 0):
                continue
            try:
                device = connect(p.device)
                try:
                    print(f"Connected: {p.device}")
                    run_device(device, args, positions)
                finally:
                    device.port.close()
                handled.add(key)
            except Exception as exc:
                print(f"{p.device}: {exc}; will retry in 10 seconds")
                retry_after[key] = time.monotonic() + 10
        time.sleep(1)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("watch", "download", "convert", "status", "start", "stop", "delete"))
    parser.add_argument("--port", help="COM port; required except watch/convert")
    parser.add_argument("--output", type=Path, default=desktop_directory() / "VCSEL_Logs")
    parser.add_argument("--positions", type=Path, default=Path(__file__).with_name("positions.json"))
    parser.add_argument("--input", type=Path, help="Binary file for offline conversion")
    parser.add_argument("--name", help="Exact onboard filename for explicit deletion")
    args = parser.parse_args()
    positions = json.loads(args.positions.read_text(encoding="utf-8"))
    if not isinstance(positions, dict) or not all(isinstance(k, str) and isinstance(v, str) for k, v in positions.items()):
        parser.error("positions.json must map channel strings to position strings")
    if args.action == "convert":
        if args.input is None:
            parser.error("convert requires --input")
        data = args.input.read_bytes()
        args.output.mkdir(parents=True, exist_ok=True)
        output = args.output / f"{args.input.stem}_{dt.datetime.now():%Y%m%d_%H%M%S_%f}.csv"
        converted = csv_bytes(data, positions)
        with output.open("xb") as f:
            f.write(converted)
        print(output)
    elif args.action == "watch":
        watch(args, positions)
    else:
        if not args.port:
            parser.error("Specify --port COM5 (use your Feather's port)")
        device = connect(args.port)
        try:
            run_device(device, args, positions)
        finally:
            device.port.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("Stopped.")
    except Exception as error:
        print(f"Error: {error}")
        raise SystemExit(1)
