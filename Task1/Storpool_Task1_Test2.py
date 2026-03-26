import struct

vals = [0x100, 0x100, 0xfff, 0xfff]
with open("sample2.bin", "wb") as f:
    for v in vals:
        f.write(struct.pack("<I", v))