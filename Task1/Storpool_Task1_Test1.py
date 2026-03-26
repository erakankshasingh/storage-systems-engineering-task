import struct

vals = [0x100, 0x100, 0x800, 0xfff]

with open("sample.bin", "wb") as f:
    for v in vals:
        f.write(struct.pack("<I", v))
        