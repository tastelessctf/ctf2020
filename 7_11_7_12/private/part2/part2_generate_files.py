#!/usr/bin/env python3

import os
import sys
import zlib
import random
import subprocess

sz = "\\Program Files\\7-Zip\\7z.exe"

def insert_gap(archive, gap_data, out):
    with open(archive, "rb") as f:
        magic_ver = f.read(8)
        crc = int.from_bytes(f.read(4), byteorder='little')

        next_header_offset = int.from_bytes(f.read(8), byteorder='little')
        next_header_size = int.from_bytes(f.read(8), byteorder='little')
        next_header_crc = int.from_bytes(f.read(4), byteorder='little')

        streams = f.read(next_header_offset)
        next_header = f.read(next_header_size)

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

if len(sys.argv) < 1:
    print("usage: part2_generate_files.py image_file")
    exit(1)

with (open(sys.argv[1], "rb")) as f:
    data = f.read()

idx = 0
while data:
    chunk_size = random.randrange(10, 40)
    junk_files = random.randrange(1,6)

    params = [sz, "a", "archive.7z"]
    for i in range(junk_files):
        with open(f"junk_{i}.bin", "wb") as f:
            f.write(os.urandom(random.randrange(20, 150)))
        params += [f"junk_{i}.bin"]

    subprocess.run(params)
    insert_gap("archive.7z", data[:chunk_size], f"part2/part2_{idx}.7z")
    os.remove("archive.7z")

    data = data[chunk_size:]
    idx += 1
