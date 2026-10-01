import struct, math

def seg_dist(px, py, a, b):
    ax, ay = a; bx, by = b
    dx, dy = bx - ax, by - ay
    t = max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - ax - t * dx, py - ay - t * dy)

# white "MD" lettering, normalised 0..1
def poly(p): return list(zip(p, p[1:]))
def arc(cx, cy, rx, ry, a0, a1, n=16):
    return [(cx + rx * math.cos(a0 + (a1 - a0) * i / n), cy + ry * math.sin(a0 + (a1 - a0) * i / n)) for i in range(n + 1)]
STROKES = (poly([(.12, .72), (.12, .28), (.275, .53), (.43, .28), (.43, .72)])
           + poly([(.58, .28), (.58, .72)])
           + poly([(.58, .28)] + arc(.66, .50, .21, .22, -math.pi / 2, math.pi / 2) + [(.58, .72)]))
HW = 0.052
BLUE = (176, 108, 43)  # BGR of #2B6CB0

def inside_round(u, v, lo=.04, hi=.96, r=.19):
    if not (lo <= u <= hi and lo <= v <= hi): return False
    cx = min(max(u, lo + r), hi - r); cy = min(max(v, lo + r), hi - r)
    return math.hypot(u - cx, v - cy) <= r

def render(S, ss=4):
    rows = []
    for y in range(S):
        row = bytearray()
        for x in range(S):
            a = w = 0
            for sy in range(ss):
                for sx in range(ss):
                    u = (x + (sx + .5) / ss) / S; v = (y + (sy + .5) / ss) / S
                    if inside_round(u, v):
                        a += 1
                        if any(seg_dist(u, v, p, q) <= HW for p, q in STROKES): w += 1
            n = ss * ss
            if a == 0: row += bytes((0, 0, 0, 0)); continue
            f = w / a
            col = [int(BLUE[i] * (1 - f) + 255 * f) for i in range(3)]
            row += bytes((col[0], col[1], col[2], int(255 * a / n)))
        rows.append(bytes(row))
    return rows

def dib(S):
    rows = render(S)
    hdr = struct.pack('<IiiHHIIiiII', 40, S, S * 2, 1, 32, 0, 0, 0, 0, 0, 0)
    pix = b''.join(reversed(rows))                      # bottom-up
    mask = b'\x00' * (((S + 31) // 32) * 4 * S)          # AND mask (alpha channel does the work)
    return hdr + pix + mask

sizes = [16, 24, 32, 48, 64]
imgs = [dib(s) for s in sizes]
out = struct.pack('<HHH', 0, 1, len(sizes))
off = 6 + 16 * len(sizes)
for s, d in zip(sizes, imgs):
    out += struct.pack('<BBBBHHII', s, s, 0, 0, 1, 32, len(d), off); off += len(d)
open('assets/app.ico', 'wb').write(out + b''.join(imgs))
print('assets/app.ico', len(out) + sum(map(len, imgs)), 'bytes')

if '--logo' in __import__('sys').argv:      # 256px PNG for the README
    import zlib
    S = 256; rows = render(S, 3)
    raw = b''.join(b'\x00' + b''.join(bytes((r[x*4+2], r[x*4+1], r[x*4], r[x*4+3])) for x in range(S)) for r in rows)
    ch = lambda t, c: struct.pack('>I', len(c)) + t + c + struct.pack('>I', zlib.crc32(t + c) & 0xffffffff)
    open('assets/logo.png', 'wb').write(b'\x89PNG\r\n\x1a\n' + ch(b'IHDR', struct.pack('>IIBBBBB', S, S, 8, 6, 0, 0, 0)) + ch(b'IDAT', zlib.compress(raw)) + ch(b'IEND', b''))
    print('assets/logo.png')
