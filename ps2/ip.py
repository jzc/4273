def tobin(s):
    octets = s.split(".")
    octets = [bin(int(o))[2:].zfill(8) for o in octets]
    return ".".join(octets)

def toint(s):
    octets = s.split(".")
    return sum(256**i*int(o) for i, o in enumerate(reversed(octets)))