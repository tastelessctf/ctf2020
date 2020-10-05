#!/usr/bin/env python3
import struct
import typing
import socketserver
from random import SystemRandom
from binascii import crc32

class Hey(Exception):
    pass

class Oops(Exception):
    pass

random = SystemRandom()

def clip(x: float, minimum: float, maximum: float) -> float:
    if x < minimum:
        return minimum
    if x > maximum:
        return maximum
    return x


def chaotic_map(x: float, p: float, q: float):
    r = p if x <= p else 1 - p
    return ((-q) / (r ** 2)) * (p - x) ** 2 + q


def coupled_chaotic_maps(
    v: typing.Tuple[float, float], a: float, b: float, c: float
) -> typing.Tuple[float, float]:
    x, y = v
    x = (1 - y) * chaotic_map(x, a, b)
    y = (1 - x) * chaotic_map(y, a, c)

    if v == (x,y):
        raise Oops

    return x, y


def prng(a, b, c, init=(0.45, 0.55), transient=1024):
    v = init

    for _ in range(transient):
        v = coupled_chaotic_maps(v, a, b, c)

    while True:
        new_v = coupled_chaotic_maps(v, a, b, c)
        v = new_v
        random_bytes = struct.pack("d", v[0]+v[1])
        yield from struct.pack("I", crc32(random_bytes))


class Handler(socketserver.BaseRequestHandler):
    def handle(self):
        data = self.request.recv(1024)
        client_secret = struct.unpack("d", data)[0]

        if client_secret >= 5 or client_secret <= 0:
            raise Hey

        # my secret
        q = 1 - (random.random()/10)
        r = 1 - (random.random()/10)

        # Create a shared state from the client secret and my secret
        keystream = prng((5+client_secret)/10, q,r)

        from secret import long_text_containing_flag
        ct = bytes(a ^ b for a, b in zip(long_text_containing_flag.encode(), keystream))

        self.request.sendall(ct)
        self.request.close()

class ThreadingTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    pass

if __name__ == "__main__":
    HOST, PORT = "okboomer.tasteless.eu", 10701

    with ThreadingTCPServer((HOST, PORT), Handler) as server:
        server.serve_forever()
