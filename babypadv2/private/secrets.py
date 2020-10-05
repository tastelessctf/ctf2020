flag = b"tstlss{wh4t3v3r_fl04t5_ur_g04t}"

__all__ = ["flag"]

try:
    from fractions import gcd
    assert gcd(len(flag), 8) == 1
except AssertionError:
    print("Error: make sure the flag length is co-prime to 8.")
    exit()