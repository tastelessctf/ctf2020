from random import SystemRandom

# SystemRandom is cryptographically secure as it uses os.urandom behind the scenes.
random = SystemRandom()


def keystream():
    while True:
        # Generate a truly random float.
        mantissa = random.random()
        exponent = random.randrange(0, 2047) - 1023
        sign = random.choice([-1, 1])
        yield sign * (1 + mantissa) * (2 ** exponent)
