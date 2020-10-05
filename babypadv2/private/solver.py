#!/usr/bin/env python3

from pwn import *
from struct import unpack_from

bytes_received = 0

def grouper(n, iterable):
    import itertools
    it = iter(iterable)
    while True:
        chunk = bytes(itertools.islice(it, n))
        if not chunk:
            return
        yield chunk


def collect():
    global bytes_received
    s = remote('okboomer.tasteless.eu', 10501)
    #s = remote('localhost', 1337)


    while True:
        s.send(b'1000')
        ctext = s.recvn(31*1000)
        bytes_received += len(ctext)

        yield from grouper(31, ctext)

def analyze():
    global bytes_received

    import sys

    import string
    allowed = set(string.ascii_lowercase+string.digits+'{}_')
    charset = set(i for i in range(2**11) if chr(i>>4) in allowed)
    candidates = [set(charset) for _ in range(31)]

    prev = None
    for j, ctext in enumerate(collect()):
        pos = (j+7) % 8
        if prev is not None:
            for i in range(pos, len(ctext), 8):
                val, = unpack_from('<Q', prev+ctext, len(prev)+i-7)
                val >>= 52
                val &= 0x7ff
                candidates[i].discard(val ^ 0x7ff)
        prev = ctext

        if j % 5000 == 0:
            ptext = []
            for i in range(len(ctext)):
                e = next(iter(candidates[i])) >> 4
                ptext.append(e)
            print(f'{j:6d} : {bytes(ptext).decode()}')
            print(f'{bytes_received:x}')

analyze()
