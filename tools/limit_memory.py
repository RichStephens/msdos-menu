#!/usr/bin/env python3
import struct
import sys
from pathlib import Path


def main():
    executable = Path(sys.argv[1])
    image = bytearray(executable.read_bytes())
    if len(image) < 14 or image[:2] != b"MZ":
        raise SystemExit(f"{executable} is not an MZ executable")

    minimum = struct.unpack_from("<H", image, 10)[0]
    struct.pack_into("<H", image, 12, minimum)
    executable.write_bytes(image)


if __name__ == "__main__":
    main()
