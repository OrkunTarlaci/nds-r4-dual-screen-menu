"""Super Mario Advance etiketi, sifirdan:
fotograftan sadece karakterler ve logo (koyu dis cizgileriyle) kesilir, arka plan (gokyuzu + bulutlar) yeniden cizilir.
ESRB kutusu ve Nintendo muhru hic alinmaz -> kapatilmis/yamali bir yer yoktur."""
from collections import deque
from PIL import Image, ImageDraw, ImageFilter, ImageEnhance

REGION = (34, 17, 250, 133)                       # fotografta kullanilan bolge
EXCLUDE = [(26, 74, 65, 128), (228, 90, 284, 134), (20, 128, 74, 152), (186, 128, 292, 152)]  # ESRB, muhur: on plana asla alinmaz


def _bg_like(r, g, b):
    """gokyuzu mavisi ya da bulut beyazi/gri-mavisi mi?"""
    lum = (r + g + b) / 3
    if lum < 110:
        return False                   # koyu: dis cizgi
    sat = max(r, g, b) - min(r, g, b)
    if lum > 160 and sat < 42:
        return True                    # bulut (fotograftaki bulutlar hafif sarimsi)
    if b >= r - 8 and b >= g - 18 and (sat < 60 or (b > 150 and b > r + 20)):
        return True                    # mavi gokyuzu
    return False


def foreground_mask(im):
    """on plan = gokyuzu/bulut olmayan, BUYUK ve birbirine bagli parcalar (logo, karakterler, bitkiler)
    + bunlarin icinde kalan delikler (harflerin ici). Bulutlar ve kucuk kirintilar arka plana kalir."""
    W, H = im.size
    px = im.load()
    ox, oy = REGION[0], REGION[1]
    ex = [(x0 - ox, y0 - oy, x1 - ox, y1 - oy) for x0, y0, x1, y1 in EXCLUDE]
    inex = lambda x, y: any(a <= x <= c and b <= y <= d for a, b, c, d in ex)
    fg = bytearray(W * H)
    for y in range(H):
        for x in range(W):
            if not inex(x, y) and not _bg_like(*px[x, y]):
                fg[y * W + x] = 1
    # baglantili parcalar; kucukleri at
    lab = [0] * (W * H)
    keep = bytearray(W * H)
    cur = 0
    for i in range(W * H):
        if fg[i] and not lab[i]:
            cur += 1
            comp = [i]
            lab[i] = cur
            k = 0
            while k < len(comp):
                j = comp[k]; k += 1
                x, y = j % W, j // W
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < W and 0 <= ny < H:
                            n = ny * W + nx
                            if fg[n] and not lab[n]:
                                lab[n] = cur
                                comp.append(n)
            if len(comp) >= 120:
                for j in comp:
                    keep[j] = 1
    # delik doldurma: kenardan ulasilamayan arka plan pikselleri on plandir
    reach = bytearray(W * H)
    q = deque()
    for x in range(W):
        q.append((x, 0)); q.append((x, H - 1))
    for y in range(H):
        q.append((0, y)); q.append((W - 1, y))
    while q:
        x, y = q.popleft()
        if x < 0 or y < 0 or x >= W or y >= H:
            continue
        i = y * W + x
        if reach[i] or keep[i]:
            continue
        reach[i] = 1
        q.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))
    m = Image.frombytes("L", (W, H), bytes(255 if (keep[i] or not reach[i]) and not inex(i % W, i // W) else 0
                                            for i in range(W * H)))
    return m


def cloud(d, cx, cy, s, fill, shade):
    """cizgi film tarzi kabarik bulut: dairelerin birlesimi, alti hafif golgeli"""
    parts = [(-1.0, 0.15, 0.55), (-0.45, -0.2, 0.7), (0.2, -0.35, 0.8), (0.85, -0.05, 0.6), (1.35, 0.2, 0.42)]
    for dx, dy, r in parts:
        x, y, rr = cx + dx * s, cy + dy * s + s * 0.18, r * s
        d.ellipse((x - rr, y - rr, x + rr, y + rr), fill=shade)
    for dx, dy, r in parts:
        x, y, rr = cx + dx * s, cy + dy * s, r * s
        d.ellipse((x - rr, y - rr, x + rr, y + rr), fill=fill)
    d.rectangle((cx - 1.0 * s, cy + 0.05 * s, cx + 1.35 * s, cy + 0.62 * s), fill=fill)


def background(W, H):
    S = 4
    bg = Image.new("RGB", (W * S, H * S))
    d = ImageDraw.Draw(bg)
    top, bot = (64, 150, 232), (150, 205, 246)
    for y in range(H * S):
        t = y / (H * S)
        d.line((0, y, W * S, y), fill=tuple(round(a + (b - a) * t) for a, b in zip(top, bot)))
    white, shade = (255, 255, 255), (206, 226, 244)
    for cx, cy, s in ((0.86, 0.20, 0.13), (0.10, 0.62, 0.15), (0.44, 0.86, 0.17), (0.86, 0.82, 0.16), (0.30, 0.10, 0.08)):
        cloud(d, cx * W * S, cy * H * S, s * W * S, white, shade)
    return bg.resize((W, H), Image.LANCZOS).filter(ImageFilter.GaussianBlur(0.6))


def make(photo_path, size):
    src = Image.open(photo_path).convert("RGB").crop(REGION)
    src = ImageEnhance.Color(src).enhance(1.15)
    m = foreground_mask(src)
    bg = background(*src.size)
    out = bg.copy()
    out.paste(src, (0, 0), m.filter(ImageFilter.GaussianBlur(0.5)))
    small = out.resize(size, Image.LANCZOS)
    small = ImageEnhance.Contrast(small).enhance(1.10)
    small = small.filter(ImageFilter.UnsharpMask(radius=1, percent=70, threshold=2))
    return small, out, m


if __name__ == "__main__":
    import sys
    small, big, m = make(sys.argv[1], (93, 50))
    sheet = Image.new("RGB", (big.width * 2 + 8, big.height + 8 + 50 * 4), (30, 30, 40))
    sheet.paste(big, (0, 0))
    sheet.paste(m.convert("RGB"), (big.width + 8, 0))
    sheet.paste(small.resize((93 * 4, 50 * 4), Image.NEAREST), (0, big.height + 8))
    sheet.save(sys.argv[2])
