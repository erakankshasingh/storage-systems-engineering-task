import struct

vals = [0x100, 0x100, 0x100, 0x100]
with open("sample3.bin", "wb") as f:
    for v in vals:
        f.write(struct.pack("<I", v))