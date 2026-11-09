"""NDS kart etiketleri: GameTDB kapak resminden (tools/covers/<oyun kodu>.jpg) gercek DS kart etiketi gibi kirpilir.
Kullanim: python nds_labels.py <proje> <NDS oyun klasoru>
Cikti: sdroot/_macro/labels/<oyun dosyasinin adi (uzantisiz)>.lbl  (128x64, 256 renk)"""
import pathlib
import sys
from PIL import Image, ImageEnhance, ImageFilter

ROOT = pathlib.Path(sys.argv[1]).resolve()
GAMES = pathlib.Path(sys.argv[2])
import re
_h = (ROOT / "source" / "assets.h").read_text()
LW = int(re.search(r"DS_LABEL_W (\d+)", _h).group(1))      # kabugun etiket penceresi (fotograftan)
LH = int(re.search(r"DS_LABEL_H (\d+)", _h).group(1))


def rgb15(c):
    r, g, b = c[:3]
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)


def gloss(img):
    """premium cam gorunumu: ustte ice dogru golge, capraz yumusak parlama, yuvarlak koseler (cerceve renginde)"""
    from PIL import ImageDraw
    w, h = img.size
    S = 4
    big = img.resize((w * S, h * S), Image.LANCZOS).convert("RGBA")
    ov = Image.new("RGBA", big.size, (0, 0, 0, 0))
    od = ImageDraw.Draw(ov)
    for y in range(3 * S):                                    # ust kenarda ic golge
        od.line((0, y, w * S, y), fill=(0, 0, 0, int(110 * (1 - y / (3 * S)))))
    band = Image.new("L", big.size, 0)                        # capraz parlama bandi
    bd = ImageDraw.Draw(band)
    bd.polygon([(int(w * 0.10 * S), 0), (int(w * 0.42 * S), 0), (int(w * 0.18 * S), h * S), (int(-w * 0.14 * S), h * S)], fill=58)
    band = band.filter(ImageFilter.GaussianBlur(5 * S))
    white = Image.new("RGBA", big.size, (255, 255, 255, 255))
    big = Image.composite(white, big, band)
    big.alpha_composite(ov)
    # yuvarlak koseler: kose disi cercevenin ic rengi
    m = Image.new("L", big.size, 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, w * S - 1, h * S - 1), radius=6 * S, fill=255)
    bg = Image.new("RGBA", big.size)
    bgd = ImageDraw.Draw(bg)
    for y in range(h * S):                                     # kose disi: krom kenarin o yukseklikteki rengi
        k = y / (h * S)
        bgd.line((0, y, w * S, y), fill=tuple(int(a_ + (b_ - a_) * k) for a_, b_ in zip((232, 234, 242), (114, 118, 130))) + (255,))
    bg.paste(big, (0, 0), m)
    return bg.convert("RGB").resize((w, h), Image.LANCZOS)


def label_to_lbl(small):
    tex = Image.new("RGB", (128, 64), (0, 0, 0))
    tex.paste(small, (0, 0))
    w = small.width
    edge_r = small.crop((w - 1, 0, w, 64)).resize((128 - w, 64))     # bos sutunlar kenar rengiyle (siyah cizgi olmasin)
    tex.paste(edge_r, (w, 0))
    q = tex.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = q.getpalette()[:256 * 3]
    pal += [0] * (768 - len(pal))
    pal16 = b"".join(rgb15(pal[k * 3:k * 3 + 3]).to_bytes(2, "little") for k in range(256))
    return pal16 + bytes(q.getdata())


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


def real_part(code):
    """oyunun gercek kart fotografindan bandin altindaki kisim"""
    img = Image.open(ROOT / "tools" / "carts" / f"{code}.png").convert("RGBA")
    card, lab, cut = cart_geom(img)
    bg = Image.new("RGB", img.size, (255, 255, 255))
    bg.paste(img, (0, 0), img)
    w, h = lab[2] - lab[0], lab[3] - cut
    ix, ib = int(w * 0.022 + 1), int(h * 0.02 + 1)          # fotograftaki etiket kenari/golgesi disarida kalsin
    return bg.crop((lab[0] + ix, cut, lab[2] - ix, lab[3] - ib))


def synth_part(code, size):
    """fotografi olmayan oyun: gercek kartin duzeni birebir (sablon: Pokemon Platinum karti).
    ust beyaz serit + logo alti sablondan, kapak resmi ortada, ESRB / Nintendo / muhur sablondan ayni yerde,
    altta beyaz seritte NTR kodu."""
    from PIL import ImageDraw, ImageFont
    tpl = real_part("CPUE").resize(size, Image.LANCZOS)
    W, H = size
    out = Image.new("RGB", (W, H), (250, 250, 250))
    top, bot = int(H * 0.062), int(H * 0.885)         # resim alani (sablon kartta resmin oldugu yer)
    out.paste(tpl.crop((0, 0, W, top)), (0, 0))       # beyaz serit + NINTENDO DS logosunun alt kismi
    aw, ah = W, bot - top
    cov = Image.open(ROOT / "tools" / "covers" / f"{code}.jpg").convert("RGB")
    cw_, chh = cov.size
    x0, y1 = int(cw_ * 0.142), int(chh * 0.79)        # soldaki dikey serit ve alttaki ESRB disarida
    rw, rh = cw_ - x0, y1
    if rw / rh > aw / ah:
        nw = int(rh * aw / ah); box = (x0 + (rw - nw) // 2, 0, x0 + (rw - nw) // 2 + nw, y1)
    else:
        nh = int(rw * ah / aw); box = (x0, y1 - nh if code in BOTTOM else 0, cw_, y1 if code in BOTTOM else nh)
    crop = ImageEnhance.Color(cov.crop(x0 and (x0, 0, cw_, y1))).enhance(1.12)
    # arka: ayni resmin bulanik, alani dolduran hali (ESRB/muhur bunun ustunde durur, gercek karttaki gibi)
    from PIL import ImageFilter as _F
    bw, bh = crop.size
    k = max(aw / bw, ah / bh)
    back = crop.resize((int(bw * k) + 1, int(bh * k) + 1), Image.LANCZOS)
    back = back.crop(((back.width - aw) // 2, 0, (back.width - aw) // 2 + aw, ah)).filter(_F.GaussianBlur(7))
    back = ImageEnhance.Brightness(back).enhance(0.92)
    out.paste(back, (0, top))
    if code not in FIT:                                # logosu ustte: resim tam alan (gercek kart gibi)
        art = crop.resize((int(bw * aw / bw), int(bh * aw / bw)), Image.LANCZOS)
        art = art.crop((0, 0, aw, min(ah, art.height)))
        out.paste(art, (0, top))

    # on: resmin tamami, ESRB/muhur sirasinin ustundeki alana sigar (logo kapanmaz), yumusak kenarli

    fh = int(H * 0.70) - top
    fw = int(bw * fh / bh)
    if fw > W - 6:
        fw = W - 6; fh = int(bh * fw / bw)
    front = crop.resize((fw, fh), Image.LANCZOS)
    fm = Image.new("L", (fw, fh), 0)
    ImageDraw.Draw(fm).rectangle((4, 4, fw - 5, fh - 5), fill=255)
    fm = fm.filter(_F.GaussianBlur(3))
    if code in FIT:
        out.paste(front, ((W - fw) // 2, top + 2), fm)
    def put(fx0, fy0, fx1, fy1, shape, pad=0):
        bx = (int(W * fx0), int(H * fy0), int(W * fx1), int(H * fy1))
        piece = tpl.crop(bx)
        m = Image.new("L", piece.size, 0)
        md = ImageDraw.Draw(m)
        if shape == "rect":
            md.rectangle((0, 0, piece.size[0], piece.size[1]), fill=255)
        elif shape == "pill":
            md.rounded_rectangle((pad, pad, piece.size[0] - 1 - pad, piece.size[1] - 1 - pad), radius=piece.size[1] // 2, fill=255)
        else:
            md.ellipse((pad, pad, piece.size[0] - 1 - pad, piece.size[1] - 1 - pad), fill=255)
        out.paste(piece, bx[:2], m)
    put(0.020, 0.585, 0.200, 0.870, "rect")           # ESRB kutusu
    put(0.268, 0.790, 0.518, 0.868, "pill", 0)        # Nintendo logosu
    put(0.700, 0.655, 0.958, 0.865, "ellipse", 1)     # Nintendo muhru
    d = ImageDraw.Draw(out)
    d.rectangle((0, bot, W, H), fill=(250, 250, 250))
    f = ImageFont.truetype("C:/Windows/Fonts/consola.ttf", int(H * 0.075))
    txt = " ".join(f"NTR-{code}-USA").replace("   ", "  ")
    d.text((W * 0.965, (bot + H) / 2 + 1), f"NTR-{code}-USA", font=f, fill=(28, 28, 32), anchor="rm")
    return out


BOTTOM = {"IPKE", "IREO", "YKWE"}                              # logosu kapagin altinda olanlar
FIT = {"IPKE", "IREO", "BWBE", "YKWE"}                         # logo ESRB/muhur sirasina denk gelir: resim kucultulup sigdirilir


out = ROOT / "sdroot" / "_macro" / "labels"
out.mkdir(parents=True, exist_ok=True)
prev = []
import json
CODES = json.load(open(ROOT / "tools" / "nds_codes.json", encoding="utf-8"))   # oyun kodlari (bir kez okundu)
ref = real_part("CPUE").size
for name, code in sorted(CODES.items()):
    rom = GAMES / name
    if (ROOT / "tools" / "carts" / f"{code}.png").exists():
        part, kind = real_part(code), "foto"
    elif (ROOT / "tools" / "covers" / f"{code}.jpg").exists():
        part, kind = synth_part(code, ref), "sentetik"
    else:
        print("resim yok:", code, rom.name)
        continue
    small = part.resize((64 * 4, LH * 4), Image.LANCZOS).resize((64, LH), Image.LANCZOS)   # 64x64 doku, pencereye gerilir
    small = small.filter(ImageFilter.UnsharpMask(radius=1, percent=60, threshold=2))
    (out / (rom.stem + ".lbl")).write_bytes(label_to_lbl(small))
    prev.append(small)
    print("etiket:", code, kind, rom.stem)

sheet = Image.new("RGB", ((LW + 6) * 6, (LH + 2) * ((len(prev) + 5) // 6)), (40, 40, 40))
for i, p in enumerate(prev):
    sheet.paste(p, ((i % 6) * (LW + 6), (i // 6) * (LH + 2)))
sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(ROOT / "tools" / "_nds_labels.png")
