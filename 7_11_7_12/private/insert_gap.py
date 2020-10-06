#!/usr/bin/env python3

import sys
import zlib

if len(sys.argv) < 3:
    print("usage: insert_gap.py <archive> <gap_file> <outfile>")
    exit(1)

archive = sys.argv[1]
gap = sys.argv[2]
out = sys.argv[3]

with open(archive, "rb") as f:
    magic_ver = f.read(8)
    crc = int.from_bytes(f.read(4), byteorder='little')

    next_header_offset = int.from_bytes(f.read(8), byteorder='little')
    next_header_size = int.from_bytes(f.read(8), byteorder='little')
    next_header_crc = int.from_bytes(f.read(4), byteorder='little')

    streams = f.read(next_header_offset)
    next_header = f.read(next_header_size)

with open(gap, "rb") as f:
    gap_data = f.read()

print(f"next_header_offset {next_header_offset}")
print(f"next_header_size {next_header_size}")
print(f"gap_data_len {len(gap_data)}")

start_header = (next_header_offset + len(gap_data)).to_bytes(8, byteorder='little')
start_header += next_header_size.to_bytes(8, byteorder='little')
start_header += next_header_crc.to_bytes(4, byteorder='little')

with open(out, "wb") as f:
    f.write(magic_ver)
    f.write(zlib.crc32(start_header).to_bytes(4, byteorder='little'))
    f.write(start_header)
    f.write(streams)
    f.write(gap_data)
    f.write(next_header)
