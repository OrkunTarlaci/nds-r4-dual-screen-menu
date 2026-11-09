"""Alt ekrandaki buyuk kapak resimleri (_macro/hero/<oyun dosyasi>.hrb): 16 bit, en fazla HW x HH.
NDS: GameTDB kutu kapagi (tools/covers/<kod>.jpg). GBA/GB: etiket fotografi (make_assets PHOTO tanimlari).
Bicim: u16 genislik, u16 yukseklik, ardindan genislik*yukseklik adet RGB15 (bit15 = opak).
Kullanim: python hero.py <proje> <NDS klasoru> <GBA klasoru>"""
import pathlib
import struct
import sys

from PIL import Image, ImageEnhance, ImageFilter

ROOT = pathlib.Path(sys.argv[1]).resolve()
NDS = pathlib.Path(sys.argv[2])
GBA = pathlib.Path(sys.argv[3])
sys_argv4 = sys.argv[4] if len(sys.argv) > 4 else None   # GB klasoru
HW, HH = 104, 92

sys.argv = [sys.argv[0], str(ROOT)]
sys.path.insert(0, str(ROOT / "tools"))
import make_assets as ma  # noqa: E402  (ROOT sys.argv[1]'den okunur)


def to_hrb(img):
    img = img.convert("RGB")
    w, h = img.size
    data = bytearray(struct.pack("<HH", w, h))
    for r, g, b in img.getdata():
        data += struct.pack("<H", (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10) | 0x8000)
    return bytes(data)


def fit(img, w, h):
    k = min(w / img.width, h / img.height)
    return img.resize((max(1, round(img.width * k)), max(1, round(img.height * k))), Image.LANCZOS)


def backdrop(img):
    """ust ekranin arka plani: kapagi 128x96'ya doldur, bulaniklastir, karart (calisirken 2 kat buyutulur)"""
    w, h = 128, 96
    k = max(w / img.width, h / img.height)
    big = img.resize((round(img.width * k) + 1, round(img.height * k) + 1), Image.LANCZOS)
    x0, y0 = (big.width - w) // 2, (big.height - h) // 2
    b = big.crop((x0, y0, x0 + w, y0 + h)).filter(ImageFilter.GaussianBlur(5))
    b = ImageEnhance.Color(b).enhance(1.35)
    return ImageEnhance.Brightness(b).enhance(0.55)


def to_bgb(img):
    data = bytearray()
    for r, g, b in img.convert("RGB").getdata():
        data += struct.pack("<H", (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10) | 0x8000)
    return bytes(data)


def polish(img):
    img = ImageEnhance.Color(img).enhance(1.15)
    img = ImageEnhance.Contrast(img).enhance(1.06)
    return img.filter(ImageFilter.UnsharpMask(radius=1, percent=60, threshold=2))


out = ROOT / "sdroot" / "_macro" / "hero"
out.mkdir(parents=True, exist_ok=True)
prev = []
import json
CODES = json.load(open(ROOT / "tools" / "nds_codes.json", encoding="utf-8"))   # oyun kodlari (bir kez okundu)
for name, code in sorted(CODES.items()):
    rom = NDS / name
    cv = ROOT / "tools" / "covers" / f"{code}.jpg"
    if not cv.exists():
        continue
    src = Image.open(cv).convert("RGB")
    im = polish(fit(src, HW, HH))
    (out / (rom.stem + ".hrb")).write_bytes(to_hrb(im))
    (out / (rom.stem + ".bgb")).write_bytes(to_bgb(backdrop(src)))
    prev.append(im)

GBDIR = pathlib.Path(sys_argv4) if sys_argv4 else None
for rom in sorted(pathlib.Path(p) for d in [GBA] + ([GBDIR] if GBDIR else []) for p in __import__("glob").glob(str(d / "*"))):
    if rom.suffix.lower() not in (".gba", ".gb", ".gbc") or "---" in rom.name:
        continue
    key = ma.match_key(rom.name)
    if not key:
        continue
    gb = rom.suffix.lower() != ".gba"
    size = (HW, 56) if not gb else (HW - 30, HH - 10)
    if key in ma.PHOTO and (ma.ROOT / "tools" / ma.PHOTO[key][0]).exists():
        f, spec, erase = ma.PHOTO[key]
        _, small = ma.photo_label(ma.ROOT / "tools" / f, spec, size, erase, None)
    else:
        _, small = ma.make_label(key, True)    # GB boyutunda ciz (GBA olcusu import sirasinda henuz yok), sonra olcekle
        small = small.resize(size, Image.LANCZOS)
    im = polish(small)
    (out / (rom.stem + ".hrb")).write_bytes(to_hrb(im))
    (out / (rom.stem + ".bgb")).write_bytes(to_bgb(backdrop(small)))
    prev.append(im)

sheet = Image.new("RGB", (108 * 6, 96 * ((len(prev) + 5) // 6)), (30, 30, 36))
for i, p in enumerate(prev):
    sheet.paste(p, ((i % 6) * 108, (i // 6) * 96))
sheet.save(ROOT / "tools" / "_hero_sheet.png")
print(len(prev), "kapak")
