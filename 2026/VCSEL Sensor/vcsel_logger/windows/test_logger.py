import io
import struct
import tempfile
import unittest
from pathlib import Path
import zlib
from logger import Device, csv_bytes, decode, save_download


def fixture(records=((0, 1, 0), (20, 1, 255))):
    return b"VCSL" + bytes((1, 6, 2, 50)) + bytes(8) + b"".join(struct.pack("<IBB", *r) for r in records)


class FakeSerial(io.BytesIO):
    def write(self, data):
        return len(data)


class Tests(unittest.TestCase):
    def test_one_byte_boundaries_and_csv_columns(self):
        self.assertEqual(list(decode(fixture())), [(0, 1, 0), (20, 1, 255)])
        self.assertEqual(csv_bytes(fixture(), {"1": "front_left"}).decode(),
                         "time_ms,channel,position,distance_mm\r\n0,1,front_left,0\r\n20,1,front_left,255\r\n")

    def test_four_channels(self):
        data = b"VCSL" + bytes((1, 6, 30, 50)) + bytes(8)
        data += b"".join(struct.pack("<IBB", 20, c, 60 + c) for c in (1, 2, 3, 4))
        self.assertEqual(len(list(decode(data))), 4)

    def test_invalid_files(self):
        for data in (b"", fixture()[:-1], b"BAD!" + fixture()[4:], fixture(((20, 1, 1), (0, 1, 2))), fixture(((0, 2, 1),))):
            with self.subTest(data=data), self.assertRaises(ValueError):
                list(decode(data))

    def test_download_checksum(self):
        data = fixture()
        response = f"DATA {len(data)}\n".encode() + data + f"\nCRC {zlib.crc32(data):X}\n".encode()
        self.assertEqual(Device(FakeSerial(response)).get("000001.BIN"), data)
        with self.assertRaises(ValueError):
            Device(FakeSerial(f"DATA {len(data)}\n".encode() + data + b"\nCRC 0\n")).get("000001.BIN")

    def test_interrupted_download(self):
        with self.assertRaises(TimeoutError):
            Device(FakeSerial(b"DATA 28\nshort")).get("000001.BIN")

    def test_save_is_repeatable_and_retains_binary(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            path = save_download(fixture(), "000001.BIN", root, {})
            self.assertEqual(path, save_download(fixture(), "000001.BIN", root, {}))
            self.assertEqual(path.with_suffix(".bin").read_bytes(), fixture())
            self.assertEqual(len(list(root.iterdir())), 2)


if __name__ == "__main__":
    unittest.main()
