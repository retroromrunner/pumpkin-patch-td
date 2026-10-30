import hashlib
import pathlib
import sys


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: build_rom.py input.bin output.gba")

    source = pathlib.Path(sys.argv[1])
    target = pathlib.Path(sys.argv[2])
    image = bytearray(source.read_bytes())
    if len(image) < 0xC0:
        raise SystemExit("linked image is too small to contain a GBA header")

    image[0xBD] = (-sum(image[0xA0:0xBD]) - 0x19) & 0xFF
    rom_size = max(32 * 1024, 1 << (len(image) - 1).bit_length())
    if rom_size > 32 * 1024 * 1024:
        raise SystemExit("linked image exceeds the GBA ROM size limit")
    image.extend(b"\xff" * (rom_size - len(image)))

    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(image)
    print(f"ROM: {target} ({len(image)} bytes)")
    print(f"SHA-256: {hashlib.sha256(image).hexdigest()}")


if __name__ == "__main__":
    main()
