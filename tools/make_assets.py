"""Macro DS Lite menu - gorsel uretici.

Uretir:
  source/assets.c, source/assets.h   yazi tipleri (A5I3), logo, yumusak nokta, kartus kabugu
  sdroot/_macro/labels/<rom>.lbl      oyun etiketleri (128x64, 256 renk: 512 B palet + 8192 B veri)

Kullanim:  python make_assets.py <proje_klasoru> [oyun_klasoru]
"""
import math
import pathlib
import sys

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont

ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
FD = "C:/Windows/Fonts/"
F = lambda n, s: ImageFont.truetype(FD + n, s)

CHARS = [chr(c) for c in range(32, 127)] + list("ÇĞİÖŞÜçğıöşü•éÉèêëáàâäíìîïóòôúùûñ™®©")


def rgb15(c):
    r, g, b = c[:3]
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)


# ------------------------------------------------------------------ C cikti yardimcilari
class CWriter:
    def __init__(self):
        self.c = ['#include "assets.h"\n']
        self.h = ["#pragma once\n#include <nds.h>\n"]

    def arr(self, name, ctype, data, per_line=24):
        n = len(data)
        self.h.append(f"extern const {ctype} {name}[{n}];")
        lines = []
        for i in range(0, n, per_line):
            lines.append(", ".join(str(v) for v in data[i:i + per_line]))
        self.c.append(f"const {ctype} {name}[{n}] __attribute__((aligned(4))) = {{\n" + ",\n".join(lines) + "\n};\n")

    def define(self, name, value):
        self.h.append(f"#define {name} {value}")

    def write(self):
        (ROOT / "source").mkdir(exist_ok=True)
        (ROOT / "source" / "assets.c").write_text("\n".join(self.c), encoding="utf-8")
        (ROOT / "source" / "assets.h").write_text("\n".join(self.h) + "\n", encoding="utf-8")


def a5i3(img_l, index=0):
    """Gri tonlu (alfa) goruntuyu A5I3 baytlarina cevirir (palet indeksi sabit)."""
    return [((v >> 3) << 3) | index for v in img_l.getdata()]


def a5i3_rgba(img, palette):
    """RGBA goruntu -> A5I3, en yakin palet rengi (8 renk)."""
    out = []
    for r, g, b, a in img.getdata():
        if a < 8:
            out.append(0)
            continue
        best = min(range(len(palette)), key=lambda k: (palette[k][0] - r) ** 2 + (palette[k][1] - g) ** 2 + (palette[k][2] - b) ** 2)
        out.append(((a >> 3) << 3) | best)
    return out


# ------------------------------------------------------------------ yazi tipi atlasi
def font_atlas(cw, name, font, W, H):
    atlas = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(atlas)
    asc, desc = font.getmetrics()
    lh = asc + desc
    x, y = 1, 1
    coords, adv = [], []
    for ch in CHARS:
        bb = font.getbbox(ch)
        w = max(1, bb[2] - min(0, bb[0]) + 1)
        a = round(font.getlength(ch))
        if x + w + 1 > W:
            x, y = 1, y + lh + 1
        assert y + lh <= H, f"{name}: atlas kucuk"
        d.text((x - min(0, bb[0]), y), ch, font=font, fill=255)
        coords += [x, y, w, lh]
        adv.append(a)
        x += w + 1
    cw.arr(f"{name}_tex", "u8", a5i3(atlas))
    cw.arr(f"{name}_coords", "u16", coords)
    cw.arr(f"{name}_adv", "u8", adv)
    cw.define(f"{name.upper()}_W", W)
    cw.define(f"{name.upper()}_H", H)
    cw.define(f"{name.upper()}_LH", lh)
    cw.define(f"{name.upper()}_N", len(CHARS))
    return atlas


def char_table(cw):
    # karakter -> atlas indeksi (UTF-8 cozulmus kod noktalari icin arama tablosu)
    cps = [ord(c) for c in CHARS]
    cw.arr("font_codepoints", "u16", cps)


# ------------------------------------------------------------------ logo
INK = (34, 39, 50)
ACCENT = (64, 120, 240)


def logo(cw):
    S = 4
    W, H = 256, 64
    big = Image.new("RGBA", (W * S, H * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    f1, f2 = F("seguibl.ttf", 36 * S), F("segoeuii.ttf", 36 * S)
    a, b = "Nintendo", " DS"
    wa, wb = d.textlength(a, font=f1), d.textlength(b, font=f2)
    sc = min(1.0, (W - 6) * S / (wa + wb))
    if sc < 1:
        f1, f2 = F("seguibl.ttf", int(36 * S * sc)), F("segoeuii.ttf", int(36 * S * sc))
        wa, wb = d.textlength(a, font=f1), d.textlength(b, font=f2)
    x = (W * S - (wa + wb)) / 2
    y = (H * S - f1.getbbox("Nintendo")[3]) / 2 - 2 * S
    d.text((x, y), a, font=f1, fill=INK + (255,))
    d.text((x + wa, y), b, font=f2, fill=ACCENT + (255,))
    img = big.resize((W, H), Image.LANCZOS)
    pal = [(255, 255, 255), INK, ACCENT, (255, 255, 255), (255, 255, 255), (255, 255, 255), (255, 255, 255), (255, 255, 255)]
    cw.arr("logo_tex", "u8", a5i3_rgba(img, pal))
    cw.arr("logo_pal", "u16", [rgb15(c) for c in pal])
    img.save(ROOT / "tools" / "_logo_preview.png")


def soft_dot(cw):
    W = 32
    im = Image.new("L", (W, W), 0)
    px = im.load()
    for y in range(W):
        for x in range(W):
            r = math.hypot(x - 15.5, y - 15.5) / 15.5
            px[x, y] = int(255 * max(0.0, 1 - r) ** 1.6)
    cw.arr("dot_tex", "u8", a5i3(im))
    ring = Image.new("L", (W, W), 0)
    ImageDraw.Draw(ring).ellipse((1, 1, W - 2, W - 2), outline=255, width=2)
    cw.arr("ring_tex", "u8", a5i3(ring.filter(ImageFilter.GaussianBlur(0.6))))


# ------------------------------------------------------------------ kartus kabuklari (128x128, 256 renk)
def shell_to_c(cw, im, name, W=128, H=128):
    img = im.resize((W, H), Image.LANCZOS)
    a = img.getchannel("A").point(lambda v: 255 if v > 110 else 0)
    rgb = Image.new("RGB", (W, H), (255, 0, 255))
    rgb.paste(img.convert("RGB"), (0, 0), a)
    q = rgb.quantize(colors=255, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = (q.getpalette() + [0] * 768)[:255 * 3]
    idx = [v + 1 for v in q.getdata()]
    for i, av in enumerate(a.getdata()):
        if av == 0:
            idx[i] = 0
    pal16 = [0x7C1F] + [rgb15(pal[k * 3:k * 3 + 3]) for k in range(255)]
    cw.arr(name + "_tex", "u8", idx)
    cw.arr(name + "_pal", "u16", pal16)
    prev = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    prev.paste(img, (0, 0), a)
    prev.save(ROOT / "tools" / f"_{name}_preview.png")


def emboss_text(d, xy, text, font, dark, light, S):
    x, y = xy
    d.text((x + S, y + S), text, font=font, fill=light, anchor="mm")
    d.text((x, y), text, font=font, fill=dark, anchor="mm")


GBA = {}   # GBA etiket alani (cartridge() doldurur): x, y, w, h, recess


def cartridge(cw):
    """Game Boy Advance kartusu: kullanicinin gercek kartus fotografindan (tools/photos/gba_cart.png).
    Beyaz zemin silinir, renk notr antrasite cekilir, etiket yeri bosaltilir."""
    from collections import deque
    src = Image.open(ROOT / "tools" / "photos" / "gba_cart.png").convert("RGB")
    W0, H0 = src.size
    px = src.load()
    bg = bytearray(W0 * H0)
    q = deque()
    for x in range(W0):
        q.append((x, 0)); q.append((x, H0 - 1))
    for y in range(H0):
        q.append((0, y)); q.append((W0 - 1, y))
    while q:
        x, y = q.popleft()
        if x < 0 or y < 0 or x >= W0 or y >= H0 or bg[y * W0 + x]:
            continue
        r, g, b_ = px[x, y]
        if not (r > 205 and g > 205 and b_ > 205):
            continue
        bg[y * W0 + x] = 1
        q.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))
    mask = Image.frombytes("L", (W0, H0), bytes(0 if v else 255 for v in bg)).filter(ImageFilter.MinFilter(3))
    bx = mask.getbbox()
    L = (59, 71, 335, 215)                      # fotograftaki etiket
    gray = src.convert("L")
    shell = Image.blend(Image.merge("RGB", (gray, gray, gray)), src, 0.22)
    shell = ImageEnhance.Brightness(shell).enhance(1.0)
    shell = ImageEnhance.Contrast(shell).enhance(1.10)
    # etiket cevresindeki yuva rengi
    ring = Image.new("L", (W0, H0), 0)
    rd = ImageDraw.Draw(ring)
    rd.rectangle((L[0] - 7, L[1] - 6, L[2] + 7, L[3] + 6), fill=255)
    rd.rectangle((L[0] - 1, L[1] - 1, L[2] + 1, L[3] + 1), fill=0)
    from PIL import ImageStat
    recess = tuple(int(v) for v in ImageStat.Stat(shell, ring).mean)
    ImageDraw.Draw(shell).rectangle(L, fill=recess)
    sh, mk = shell.crop(bx), mask.crop(bx)
    CW = 128                                    # kabuk genisligi (doku 256x128)
    sc = CW / sh.width
    nw, nh = CW, round(sh.height * sc)
    ox, oy = (256 - CW) // 2, (128 - nh) // 2
    sh = sh.resize((nw, nh), Image.LANCZOS)
    sh = sh.filter(ImageFilter.UnsharpMask(radius=1, percent=60, threshold=1))
    mk = mk.resize((nw, nh), Image.LANCZOS)
    rgba = sh.convert("RGBA")
    rgba.putalpha(mk)
    im = Image.new("RGBA", (256, 128), (0, 0, 0, 0))
    im.paste(rgba, (ox, oy))
    shell_to_c(cw, im, "cart", 256, 128)
    cw.define("CART_TEX_W", 256)
    cw.define("CART_BODY_X0", ox)
    cw.define("CART_BODY_Y0", oy)
    cw.define("CART_BODY_X1", ox + nw)
    cw.define("CART_BODY_Y1", oy + nh)
    lx0 = ox + (L[0] - bx[0]) * sc
    ly0 = oy + (L[1] - bx[1]) * sc
    lx1 = ox + (L[2] - bx[0]) * sc
    ly1 = oy + (L[3] - bx[1]) * sc
    X, Y = int(lx0), int(ly0)
    GBA.update(x=X, y=Y, w=int(lx1 + 0.999) - X, h=int(ly1 + 0.999) - Y, recess=recess)
    cw.define("CART_LABEL_X", GBA["x"])
    cw.define("CART_LABEL_Y", GBA["y"])
    cw.define("CART_LABEL_W", GBA["w"])
    cw.define("CART_LABEL_H", GBA["h"])
    cw.define("CART_BOTTOM", oy + nh)
    print("GBA etiket alani:", GBA)


GB_K = 1.10
_t = lambda v: 64 + (v - 64) * GB_K
GBL = (round(_t(34)), round(_t(44)), round(_t(94)) - round(_t(34)), round(_t(100)) - round(_t(44)))   # x, y, w, h


def gb_shell(color_only=False):
    """Game Boy kartusu, acik renkli (oyunda renklendirilir: gri, sari, siyah, seffaf...).
    color_only: sadece Game Boy Color'da calisan kartus (sag ustte basamak yerine cikinti, GAME BOY COLOR yazisi)."""
    S = 4
    K = GB_K
    im = Image.new("RGBA", (128 * S, 128 * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    s = lambda *v: [int(round((64 + (t - 64) * K) * S)) for t in v]   # merkeze gore olcekli
    BODY = (236, 236, 240, 255)
    HI = (255, 255, 255, 255)
    LO = (172, 172, 178, 255)
    RECESS = (214, 214, 220, 255)
    TXT_D, TXT_L = (168, 168, 174), (255, 255, 255)
    d.rounded_rectangle(s(23, 16, 105, 112), radius=3 * S, fill=BODY)
    if color_only:
        # kilit cikintisi: sag ustte govdenin ustune tasar
        d.rounded_rectangle(s(95, 12.5, 105, 18), radius=1 * S, fill=BODY)
        d.line(s(96, 13, 104, 13), fill=HI, width=S)
        d.line(s(95.5, 13.5, 95.5, 17), fill=LO, width=S)
    else:
        # sag ust kose: kucuk basamak (klasik kartus)
        d.rectangle(s(101, 16, 105, 19), fill=(0, 0, 0, 0))
        d.line(s(101, 19.5, 105, 19.5), fill=LO, width=S)
    d.rounded_rectangle(s(31, 20, 97, 37), radius=8 * S, fill=RECESS)
    d.arc(s(31, 20, 97, 37), 0, 180, fill=HI, width=S)
    if color_only:
        emboss_text(d, s(64, 26.2), "GAME BOY", F("seguibl.ttf", int(6.6 * S * K)), TXT_D, TXT_L, S // 2)
        emboss_text(d, s(64, 32.4), "COLOR", F("seguibl.ttf", int(5.4 * S * K)), TXT_D, TXT_L, S // 2)
    else:
        emboss_text(d, s(64, 28.5), "GAME BOY", F("seguibl.ttf", int(8.5 * S * K)), TXT_D, TXT_L, S // 2)
    for k in range(7):
        y = 22 + k * 2.6
        d.line(s(23.5, y, 28, y), fill=LO, width=S)
        d.line(s(100, y, 104.5, y), fill=LO, width=S)
    # etiket yuvasi; pencere (34,44)-(94,100) -> 60x56
    d.rectangle(s(32, 42, 96, 102), fill=RECESS)
    d.line(s(32, 102, 96, 102), fill=HI, width=S)
    d.rectangle((GBL[0] * S, GBL[1] * S, (GBL[0] + GBL[2]) * S - 1, (GBL[1] + GBL[3]) * S - 1), fill=(0, 0, 0, 0))
    d.polygon(s(58, 104.5, 70, 104.5, 64, 110), fill=LO)
    d.line(s(58, 104.5, 70, 104.5), fill=HI, width=S)
    d.line(s(24, 26, 24, 108), fill=HI, width=S)
    d.line(s(104, 26, 104, 108), fill=LO, width=S)
    return im


def cart_geom(img):
    """kart fotografi -> (kart kutusu, etiket kutusu, resim baslangici y)"""
    a = img.getchannel("A").point(lambda v: 255 if v > 128 else 0)
    card = a.getbbox()
    rgb = img.convert("RGB")
    px = rgb.load()
    W, H = rgb.size
    wm = Image.new("L", (W, H), 0)
    wp = wm.load()
    for y in range(H):
        for x in range(W):
            r, g, b = px[x, y]
            if min(r, g, b) > 200 and max(r, g, b) - min(r, g, b) < 30:
                wp[x, y] = 255
    lab = wm.getbbox()
    lx0, ly0, lx1, ly1 = lab
    lh = ly1 - ly0
    cut = None
    for y in range(ly0 + int(lh * 0.12), ly0 + int(lh * 0.24)):
        n = sum(1 for x in range(lx0 + 6, lx1 - 6) if wp[x, y])
        if n < 0.80 * (lx1 - lx0 - 12):
            cut = y
            break
    if cut is None:
        cut = ly0 + int(lh * 0.18)
    cut = max(ly0 + int(lh * 0.15), min(ly0 + int(lh * 0.21), cut)) - int(lh * 0.02)
    return card, lab, cut


def ds_cartridge(cw):
    """Nintendo DS karti, gercek kart fotografindan (GameTDB, tools/carts/CPUE.png): gri govde + beyaz etiketin
    NINTENDO DS bandi kabukta; bandin altindaki oyuna ozel kisim etiket dokusu olarak ustune cizilir."""
    src = Image.open(ROOT / "tools" / "carts" / "CPUE.png").convert("RGBA")
    card, lab, cut = cart_geom(src)
    sc = 64.0 / (lab[3] - cut)                       # oyuna ozel kisim tam 64 piksel yuksek
    cw_, ch_ = card[2] - card[0], card[3] - card[1]
    nw, nh = round(cw_ * sc), round(ch_ * sc)
    ox, oy = (128 - nw) // 2, (128 - nh) // 2
    sh = src.crop(card).resize((nw * 4, nh * 4), Image.LANCZOS)
    sh = sh.resize((nw, nh), Image.LANCZOS).filter(ImageFilter.UnsharpMask(radius=1, percent=50, threshold=1))
    im = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
    im.paste(sh, (ox, oy), sh)
    lx0 = ox + round((lab[0] - card[0]) * sc)
    lx1 = ox + round((lab[2] - card[0]) * sc)
    ly0 = oy + round((cut - card[1]) * sc)
    ly1 = ly0 + 64
    ImageDraw.Draw(im).rectangle((lx0, ly0, lx1 - 1, ly1 - 1), fill=(250, 250, 250, 255))   # sablon oyunun resmi kalmasin
    shell_to_c(cw, im, "dscart")
    cw.define("DS_LABEL_X", lx0)
    cw.define("DS_LABEL_Y", ly0)
    cw.define("DS_LABEL_W", lx1 - lx0)
    cw.define("DS_LABEL_H", ly1 - ly0)
    cw.define("DS_BODY_X0", ox)
    cw.define("DS_BODY_Y0", oy)
    cw.define("DS_BODY_X1", ox + nw)
    cw.define("DS_BODY_Y1", oy + nh)
    cw.define("DS_BOTTOM", oy + nh)
    print("DS kart:", nw, "x", nh, "etiket", lx0, ly0, lx1 - lx0, 64)


def gb_cartridge(cw):
    shell_to_c(cw, gb_shell(False), "gbcart")
    cw.define("GB_LABEL_X", GBL[0])
    cw.define("GB_LABEL_Y", GBL[1])
    cw.define("GB_LABEL_W", GBL[2])
    cw.define("GB_LABEL_H", GBL[3])
    cw.define("GB_BOTTOM", round(_t(112)))
    cw.define("GB_BODY_X0", int(_t(23)))
    cw.define("GB_BODY_Y0", int(_t(12)))
    cw.define("GB_BODY_X1", round(_t(105)) + 1)
    cw.define("GB_BODY_Y1", round(_t(112)) + 1)


# ------------------------------------------------------------------ oyun etiketleri
def label_bg(d, w, h, top, bot, S):
    for y in range(h):
        t = y / h
        d.line((0, y, w, y), fill=tuple(round(a + (b - a) * t) for a, b in zip(top, bot)))


def motif_flame(d, S, w, h):
    cx, cy = w * 0.78, h * 0.45
    for k, (r, c) in enumerate(((h * 0.42, (255, 150, 40)), (h * 0.30, (255, 200, 70)), (h * 0.16, (255, 245, 190)))):
        d.polygon([(cx, cy - r * 1.3), (cx + r * 0.75, cy + r * 0.2), (cx, cy + r * 0.9), (cx - r * 0.75, cy + r * 0.2)], fill=c)


def motif_gem(d, S, w, h):
    cx, cy, r = w * 0.78, h * 0.45, h * 0.36
    d.polygon([(cx, cy - r), (cx + r, cy - r * 0.2), (cx, cy + r), (cx - r, cy - r * 0.2)], fill=(140, 240, 170))
    d.polygon([(cx, cy - r), (cx + r, cy - r * 0.2), (cx, cy - r * 0.2)], fill=(225, 255, 235))
    d.polygon([(cx - r, cy - r * 0.2), (cx, cy - r * 0.2), (cx, cy + r)], fill=(50, 160, 95))


def motif_star(d, S, w, h):
    cx, cy, r1, r2 = w * 0.78, h * 0.45, h * 0.38, h * 0.16
    pts = []
    for k in range(10):
        r = r1 if k % 2 == 0 else r2
        a = -math.pi / 2 + k * math.pi / 5
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    d.polygon(pts, fill=(255, 215, 60))


def motif_flag(d, S, w, h):
    x0, y0, c = w * 0.62, h * 0.14, h * 0.12
    for i in range(5):
        for j in range(5):
            d.rectangle((x0 + i * c, y0 + j * c, x0 + (i + 1) * c, y0 + (j + 1) * c), fill=(250, 250, 250) if (i + j) % 2 == 0 else (25, 25, 30))


def motif_speed(d, S, w, h):
    cx, cy, r = w * 0.8, h * 0.42, h * 0.27
    d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=(255, 215, 60), width=int(h * 0.08))
    for k in range(3):
        y = cy - r * 0.6 + k * r * 0.6
        d.line((w * 0.5, y, cx - r * 1.2, y), fill=(200, 230, 255), width=int(h * 0.05))


def motif_blocks(d, S, w, h):
    c = h * 0.13
    cols = [(240, 80, 80), (80, 200, 120), (80, 140, 250), (250, 200, 60)]
    shapes = [[(0, 1), (1, 1), (2, 1), (1, 2)], [(3, 0), (3, 1), (4, 1), (4, 2)], [(2, 3), (3, 3), (4, 3), (4, 4)], [(0, 3), (1, 3), (0, 4), (1, 4)]]
    for col, sh in zip(cols, shapes):
        for x, y in sh:
            X, Y = w * 0.58 + x * c, h * 0.1 + y * c
            d.rectangle((X, Y, X + c - S, Y + c - S), fill=col)


def motif_ball(d, S, w, h):
    cx, cy, r = w * 0.78, h * 0.45, h * 0.33
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(250, 250, 250), outline=(20, 20, 20), width=2 * S)
    d.chord((cx - r, cy - r, cx + r, cy + r), 180, 360, fill=(230, 50, 50), outline=(20, 20, 20), width=2 * S)
    d.line((cx - r, cy, cx + r, cy), fill=(20, 20, 20), width=2 * S)
    d.ellipse((cx - r * 0.3, cy - r * 0.3, cx + r * 0.3, cy + r * 0.3), fill=(250, 250, 250), outline=(20, 20, 20), width=2 * S)


ART = {
    "pokemon fire red": ((235, 80, 40), (130, 20, 25), motif_flame, "FIRE RED", "POKéMON"),
    "pokemon emerald": ((40, 160, 100), (10, 70, 55), motif_gem, "EMERALD", "POKéMON"),
    "super mario advance": ((230, 45, 45), (110, 10, 30), motif_star, "MARIO", "SUPER"),
    "mario kart super circuit": ((250, 175, 35), (200, 70, 20), motif_flag, "KART", "SUPER CIRCUIT"),
    "sonic advance 3": ((35, 165, 225), (20, 55, 150), motif_speed, "SONIC 3", "ADVANCE"),
    "sonic advance": ((45, 110, 235), (20, 35, 140), motif_speed, "SONIC", "ADVANCE"),
    "tetris": ((150, 180, 40), (55, 85, 20), motif_blocks, "TETRIS", "GAME BOY"),
    "pokemon": ((90, 60, 170), (40, 20, 95), motif_ball, "POKéMON", ""),
}


def label_to_lbl(small):
    tex = Image.new("RGB", (128, 64), (0, 0, 0))
    tex.paste(small, (0, 0))
    q = tex.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = q.getpalette()[:256 * 3] + [0] * (768 - len(q.getpalette()[:768]))
    pal16 = b"".join(rgb15(pal[k * 3:k * 3 + 3]).to_bytes(2, "little") for k in range(256))
    return pal16 + bytes(q.getdata())


def round_corners(img, r, color):
    """Etiketin koselerini yuvarlat (gercek etiketlerdeki gibi); kose disi yuva rengiyle boyanir."""
    w, h = img.size
    px = img.load()
    for cy, cx in ((r - 0.5, r - 0.5), (r - 0.5, w - r - 0.5), (h - r - 0.5, r - 0.5), (h - r - 0.5, w - r - 0.5)):
        for y in range(int(cy - r), int(cy + r) + 1):
            for x in range(int(cx - r), int(cx + r) + 1):
                if 0 <= x < w and 0 <= y < h:
                    inside_x = (x < r and cx < w / 2) or (x >= w - r and cx > w / 2)
                    inside_y = (y < r and cy < h / 2) or (y >= h - r and cy > h / 2)
                    if inside_x and inside_y and math.hypot(x - cx, y - cy) > r:
                        px[x, y] = color
    return img


def mirror_pad(im, valid, P):
    """valid dikdortgenini al, her yandan P piksel aynalayarak genislet (dokuyu bozmadan kenar dolgusu)."""
    v = im.crop(valid)
    w, h = v.size
    P = min(P, w - 1, h - 1)
    c = Image.new("RGB", (w + 2 * P, h + 2 * P))
    c.paste(v, (P, P))
    c.paste(v.crop((0, 0, w, P)).transpose(Image.FLIP_TOP_BOTTOM), (P, 0))
    c.paste(v.crop((0, h - P, w, h)).transpose(Image.FLIP_TOP_BOTTOM), (P, P + h))
    c.paste(c.crop((P, 0, 2 * P, h + 2 * P)).transpose(Image.FLIP_LEFT_RIGHT), (0, 0))
    c.paste(c.crop((P + w - P, 0, P + w, h + 2 * P)).transpose(Image.FLIP_LEFT_RIGHT), (P + w, 0))
    return c, (valid[0] - P, valid[1] - P)


def smooth_fill(im, regions):
    """Her bolgeyi kenarlarindaki zemin renginden yatay ve dikey gecisle doldurur (Coons yamasi).
    Bulanik leke yerine zeminin dogal devami gibi gorunur. Bolge: (tur, x0, y0, x1, y1[, kenarlar])
    tur: 'r' dikdortgen, 'e' elips; kenarlar: kullanilacak taraflar 'lrtb' (varsayilan hepsi)."""
    W, H = im.size
    ref = im.filter(ImageFilter.GaussianBlur(5))      # kenar ayrintisi cizgi gibi uzamasin
    rp = ref.load()
    out = im.copy()
    op = out.load()
    for reg in regions:
        kind, x0, y0, x1, y1 = reg[:5]
        sides = reg[5] if len(reg) > 5 else "lrtb"
        m = Image.new("L", im.size, 0)
        (ImageDraw.Draw(m).ellipse if kind == "e" else ImageDraw.Draw(m).rectangle)((x0, y0, x1, y1), fill=255)
        m = m.filter(ImageFilter.MaxFilter(3)).filter(ImageFilter.GaussianBlur(1.0))
        mp = m.load()
        X0, Y0 = max(0, x0 - 4), max(0, y0 - 4)
        X1, Y1 = min(W - 1, x1 + 4), min(H - 1, y1 + 4)
        for y in range(Y0, Y1 + 1):
            for x in range(X0, X1 + 1):
                w = mp[x, y] / 255
                if w <= 0:
                    continue
                acc = [0.0, 0.0, 0.0]
                n = 0
                u = (x - X0) / max(1, X1 - X0)
                v = (y - Y0) / max(1, Y1 - Y0)
                hs = [(1 - u, rp[X0, y]) if "l" in sides else None, (u, rp[X1, y]) if "r" in sides else None]
                vs = [(1 - v, rp[x, Y0]) if "t" in sides else None, (v, rp[x, Y1]) if "b" in sides else None]
                for grp in (hs, vs):
                    g = [t for t in grp if t]
                    if not g:
                        continue
                    tw = sum(t[0] for t in g) or 1
                    for k in range(3):
                        acc[k] += sum(t[0] * t[1][k] for t in g) / tw
                    n += 1
                if not n:
                    continue
                fill = [acc[k] / n for k in range(3)]
                # zeminin dokusu: cevredeki gercek pikselin hafif izi (duz plastik gibi durmasin)
                nx, ny = (X0 - 1 - (x - X0) % 6) if X0 > 6 else (X1 + 1 + (x - X0) % 6), y
                nx = min(W - 1, max(0, nx))
                gr = sum(im.getpixel((nx, ny))) / 3 - sum(rp[nx, ny]) / 3
                fill = [f + gr * 0.0 for f in fill]   # doku izi kapali (noktali desen yapiyordu)
                o = op[x, y]
                op[x, y] = tuple(int(o[k] * (1 - w) + fill[k] * w) for k in range(3))
    return out


def photo_label(path, spec, size, erase=(), rounded=None):
    """Etiket fotografindan: istenmeyen yazi/logolar silinir (cevreden doldurulur), logo/resim tam ortalanir,
    netlestirilip renkleri canlandirilir.
    spec: ("crop", x0, y0, x1, y1)  -> bu kutu (oran zaten dogru)
          ("focus", fx0, fy0, fx1, fy1, pay, vx0, vy0, vx1, vy1) -> odak kutusu ortaya, etrafinda 'pay' kadar bosluk;
          etiket disina tasarsa etiket (v) kenarlari aynalanarak doldurulur."""
    from PIL import ImageStat
    im = Image.open(path).convert("RGB")
    clones = [r for r in erase if r[0] == "c"]
    fills = [r for r in erase if r[0] != "c"]
    for c in clones:
        _, dx0, dy0, dx1, dy1, sx0, sy0, sx1, sy1 = c[:9]
        hard = c[9] if len(c) > 9 else ""
        dw, dh = dx1 - dx0, dy1 - dy0
        patch_ = im.crop((sx0, sy0, sx1, sy1)).resize((dw, dh), Image.LANCZOS)
        m = Image.new("L", (dw, dh), 0)
        ImageDraw.Draw(m).rounded_rectangle((3, 3, dw - 4, dh - 4), radius=6, fill=255)
        m = m.filter(ImageFilter.GaussianBlur(2.5))
        md = ImageDraw.Draw(m)
        if "r" in hard: md.rectangle((dw - 8, 0, dw, dh), fill=255)     # bitisik koyu cizgiye kadar tam ort
        if "t" in hard: md.rectangle((0, 0, dw, 6), fill=255)
        if "l" in hard: md.rectangle((0, 0, 6, dh), fill=255)
        if "b" in hard: md.rectangle((0, dh - 7, dw, dh), fill=255)
        im.paste(patch_, (dx0, dy0), m)
    if fills:
        im = smooth_fill(im, fills)
    asp = size[0] / size[1]
    if spec[0] == "fit":
        # resmin TAMAMI sigdirilir (hicbir yeri kesilmez); kenar bosluklari ayni resmin yumusatilmis haliyle dolar
        art = im.crop(spec[1:5])
        olcek = spec[5] if len(spec) > 5 else 1.0      # <1: kenarlarda nefes payi
        W2, H2 = size[0] * 4, size[1] * 4
        bgi = art.resize((W2, H2), Image.LANCZOS).filter(ImageFilter.GaussianBlur(10))
        bgi = ImageEnhance.Brightness(bgi).enhance(0.97)
        k = min(W2 / art.width, H2 / art.height) * olcek
        fg = art.resize((round(art.width * k), round(art.height * k)), Image.LANCZOS)
        bgi.paste(fg, ((W2 - fg.width) // 2, (H2 - fg.height) // 2))
        im = bgi
        box = (0, 0, W2, H2)
    elif spec[0] == "crop":
        box = spec[1:5]
    elif spec[0] == "compose":
        # logo kutusu kesilir, temiz bir arka plan parcasinin (b) uzerine tam ortaya yumusak kenarla konur
        lx0, ly0, lx1, ly1, pay = spec[1:6]
        bgb = spec[6:10]
        lw, lh = lx1 - lx0, ly1 - ly0
        cw_, ch_ = lw * pay, lh * pay
        if cw_ / ch_ < asp:
            cw_ = ch_ * asp
        else:
            ch_ = cw_ / asp
        cw_, ch_ = round(cw_), round(ch_)
        canvas = im.crop(bgb).resize((cw_, ch_), Image.LANCZOS)
        logo = im.crop((lx0, ly0, lx1, ly1))
        m = Image.new("L", logo.size, 0)
        ImageDraw.Draw(m).rectangle((6, 6, lw - 7, lh - 7), fill=255)
        m = m.filter(ImageFilter.GaussianBlur(4))
        canvas.paste(logo, ((cw_ - lw) // 2, (ch_ - lh) // 2), m)
        im = canvas
        box = (0, 0, cw_, ch_)
    else:
        fx0, fy0, fx1, fy1, pay = spec[1:6]
        valid = spec[6:10]
        fw, fh = (fx1 - fx0) * pay, (fy1 - fy0) * pay
        if fw / fh < asp:
            fw = fh * asp
        else:
            fh = fw / asp
        cx, cy = (fx0 + fx1) / 2, (fy0 + fy1) / 2
        box = (round(cx - fw / 2), round(cy - fh / 2), round(cx + fw / 2), round(cy + fh / 2))
        im, (ox, oy) = mirror_pad(im, valid, 80)
        box = (box[0] - ox, box[1] - oy, box[2] - ox, box[3] - oy)
    small = im.crop(box).resize(size, Image.LANCZOS)
    small = ImageEnhance.Color(small).enhance(1.30)
    small = ImageEnhance.Contrast(small).enhance(1.18)
    small = small.filter(ImageFilter.UnsharpMask(radius=1, percent=80, threshold=2))
    if rounded:
        small = round_corners(small, 3, rounded)
    return label_to_lbl(small), small


PHOTO = {   # sade: logo ve ana resim; kalan kucuk artiklar (muhur, ESRB, TM) zeminin devamiyla doldurulur
    # ("crop", x0, y0, x1, y1) dogrudan kirpma | ("fit", ...) tamami sigdirilir
    # ("focus", odak kutusu, pay, etiket siniri) | ("compose", logo kutusu, pay, temiz zemin kutusu)
    "tetris": ("tetris_cart_photo.png", ("crop", 72, 122, 268, 306), []),
    "pokemon - yellow": ("photos/11.png", ("fit", 33, 11, 219, 228), [
        ("r", 20, 148, 66, 220, "rt"), ("r", 158, 195, 220, 221, "lt"), ("r", 213, 66, 226, 80, "lb")]),
    "super mario bros. deluxe": ("photos/12.png", ("crop", 24, 6, 186, 158), []),
    "pokemon fire red": ("photos/6.png", ("focus", 83, 24, 237, 117, 1.07, 6, 6, 313, 166), [
        ("e", 236, 86, 292, 128), ("r", 64, 115, 182, 134, "lrt"), ("r", 14, 90, 62, 152)]),
    "pokemon emerald": ("photos/7.png", ("focus", 64, 28, 236, 118, 1.04, 9, 9, 292, 162), [
        ("e", 216, 84, 272, 136), ("r", 224, 82, 238, 96, "rt")]),
    "mario kart super circuit": ("photos/8.png", ("compose", 60, 30, 408, 138, 1.04, 175, 135, 345, 200), [
        ("r", 390, 34, 426, 58), ("r", 380, 90, 404, 114, "rtb"), ("e", 346, 132, 434, 202)]),
    "sonic advance": ("photos/9.png", ("compose", 122, 22, 302, 104, 1.10, 20, 18, 122, 100), []),
    # tum sahne (Birdo, kart, logo, cicek); ESRB ve muhur ayni etiketin bulut/gokyuzu parcalariyla kaplanir
    "super mario advance": ("photos/sma2.png", ("crop", 35, 18, 249, 133), [
        ("c", 27, 75, 64, 129, 11, 75, 27, 129, "lrt"), ("c", 233, 92, 284, 132, 248, 58, 288, 94, "")]),   # silme yok: ESRB ve muhur kadraj disinda
}


def make_label(key, gb=False):
    size = (GBL[2], GBL[3]) if gb else (GBA["w"], GBA["h"])
    if key == "super mario advance-SIFIRDAN":   # (denendi, begenilmedi) karakterler+logo kesilir, gokyuzu yeniden cizilir
        import sma_label
        small, _, _ = sma_label.make(ROOT / "tools" / "photos" / "sma2.png", size)
        if not gb:
            small = round_corners(small, 3, GBA["recess"])
        return label_to_lbl(small), small
    if key in PHOTO and (ROOT / "tools" / PHOTO[key][0]).exists():
        f, spec, erase = PHOTO[key]
        return photo_label(ROOT / "tools" / f, spec, size, erase, None if gb else GBA["recess"])
    top, bot, motif, title, sub = ART[key]
    S = 4
    w, h = size[0] * S, size[1] * S
    im = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(im)
    label_bg(d, w, h, top, bot, S)
    # parlak kosegen serit
    d.polygon([(w * 0.45, 0), (w * 0.62, 0), (w * 0.42, h), (w * 0.25, h)], fill=tuple(min(255, c + 18) for c in top))
    motif(d, S, w, h)
    if sub:
        d.text((6 * S, 8 * S), sub, font=F("seguisb.ttf", 7 * S), fill=(255, 255, 255))
    ft = F("seguibl.ttf", (13 if len(title) <= 7 else 10) * S)
    d.text((6 * S + S, 17 * S + S), title, font=ft, fill=(0, 0, 0))
    d.text((6 * S, 17 * S), title, font=ft, fill=(255, 255, 255))
    d.rectangle((0, h - 9 * S, w, h), fill=(20, 20, 26))
    d.text((6 * S, h - 4.5 * S), "Game Boy micro", font=F("seguisb.ttf", 5 * S), fill=(200, 205, 215), anchor="lm")
    small = im.resize((w // S, h // S), Image.LANCZOS)
    if not gb:
        small = round_corners(small, 3, GBA["recess"])
    return label_to_lbl(small), small


def clean_title(name):
    """menudeki makeTitle ile ayni: "001 - Tetris.gb" -> "Tetris" """
    p = name
    i = 0
    while i < len(p) and p[i].isdigit():
        i += 1
    while i < len(p) and p[i] in "- _":
        i += 1
    t = p[i:] or p
    if "." in t:
        t = t.rsplit(".", 1)[0]
    return t.rstrip(" -")


def match_key(name):
    n = clean_title(name).lower()
    have = {k for k, v in PHOTO.items() if (ROOT / "tools" / v[0]).exists()}   # fotografi olmayan PHOTO satirlari atlanir
    for k in sorted(set(ART) | have, key=len, reverse=True):   # en uzun eslesme once
        if n.startswith(k):
            return k
    return None


def labels(game_dir):
    out = ROOT / "sdroot" / "_macro" / "labels"
    out.mkdir(parents=True, exist_ok=True)
    made = []
    dirs = [pathlib.Path(d) for d in game_dir.split(";")] if game_dir else []    # "GBA;GB" klasorleri
    names = sorted(p.name for d in dirs if d.exists() for p in d.iterdir()
                   if p.suffix.lower() in (".gba", ".gb", ".gbc") and "---" not in p.name)
    if not names:
        names = ["Pokemon Fire Red.gba", "Pokemon Emerald.gba", "Super Mario Advance.gba", "Mario Kart Super Circuit.gba",
                 "Sonic Advance.gba", "Sonic Advance 3.gba", "Tetris.gb"]
    prev = []
    for n in names:
        k = match_key(n)
        if not k:
            continue
        data, small = make_label(k, gb=n.lower().endswith((".gb", ".gbc")))
        (out / (clean_title(n) + ".lbl")).write_bytes(data)
        made.append(n)
        prev.append(small)
    sheet = Image.new("RGB", (104 * 4, 64 * ((len(prev) + 3) // 4)), (60, 60, 60))
    for i, p in enumerate(prev):
        sheet.paste(p, ((i % 4) * 104, (i // 4) * 64))
    pv = ROOT / "tools" / "_labels_preview.png"
    sheet.save(ROOT / "tools" / "_labels_preview.png")
    return made


if __name__ == "__main__":
    (ROOT / "tools").mkdir(exist_ok=True)
    cw = CWriter()
    font_atlas(cw, "font_s", F("seguisb.ttf", 11), 256, 128)
    font_atlas(cw, "font_l", F("segoeuib.ttf", 17), 256, 256)
    char_table(cw)
    logo(cw)
    soft_dot(cw)
    cartridge(cw)
    gb_cartridge(cw)
    ds_cartridge(cw)
    cw.write()
    made = labels(sys.argv[2] if len(sys.argv) > 2 else None)
    print("assets.c yazildi; etiketler:", ", ".join(made))
