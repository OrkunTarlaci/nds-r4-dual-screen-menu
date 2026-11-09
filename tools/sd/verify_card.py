"""Kullanim: python verify_card.py [kart_yolu yerel_dosya]...  (SD_DRIVE=F: varsayilan)
Karti HAM oku (Windows onbellegi yok): FAT1==FAT2, verilen dosyalarin icerigi kaynakla ayni mi, tum dosyalarin zinciri saglam mi, yeni USB paketi var mi (meta veri ve yazilan dosyalarda)."""
import hashlib
import os
import struct
import sys
import time

dev = open("\\\\.\\" + os.environ.get("SD_DRIVE", "F:"), "rb", buffering=0)   # SD_DRIVE=E: gibi


def rd(off, n):
    a = off - off % 512
    m = ((off + n - a + 511) // 512) * 512
    out = bytearray()
    p = a
    while len(out) < m:
        k = min(65536, m - len(out))
        for att in range(20):
            try:
                dev.seek(p)
                d = dev.read(k)
                if len(d) == k:
                    break
            except OSError:
                time.sleep(min(1 + att, 5))
        else:
            raise SystemExit(f"okunamadi {p}")
        out += d
        p += k
    return bytes(out[off - a:off - a + n])


bs = rd(0, 512)
bps, spc, res, nfat = struct.unpack_from("<HBHB", bs, 11)
fatsz = struct.unpack_from("<I", bs, 36)[0]
root = struct.unpack_from("<I", bs, 44)[0]
csz = bps * spc
fat_off = res * bps
data_off = (res + nfat * fatsz) * bps
fat = rd(fat_off, fatsz * bps)
fat2 = rd(fat_off + fatsz * bps, fatsz * bps)
print("FAT1 == FAT2:", fat == fat2)
meta = rd(0, data_off)
print("meta veride USB paketi:", sum(1 for i in range(0, len(meta), 512) if meta[i:i + 4] in (b"USBS", b"USBC")))
NCL = len(fat) // 4


def nxt(c):
    return struct.unpack_from("<I", fat, c * 4)[0] & 0x0FFFFFFF


def chain(c):
    out = []
    while 2 <= c < 0x0FFFFFF8 and c < NCL and len(out) < 1100000:
        out.append(c)
        c = nxt(c)
    return out


def coff(c):
    return data_off + (c - 2) * csz


def entries(clus):
    data = b"".join(rd(coff(c), csz) for c in chain(clus))
    out, lfn = [], []
    for i in range(0, len(data), 32):
        e = data[i:i + 32]
        if e[0] == 0:
            break
        if e[0] == 0xE5:
            lfn = []
            continue
        if e[11] == 0x0F:
            lfn.insert(0, e[1:11] + e[14:26] + e[28:32])
            continue
        if lfn:
            name = b"".join(lfn).decode("utf-16le", "replace").split("\x00")[0]
        else:
            b8, x3 = e[:8].decode("latin1").rstrip(), e[8:11].decode("latin1").rstrip()
            name = b8 + ("." + x3 if x3 else "")
        lfn = []
        cl = struct.unpack_from("<H", e, 20)[0] << 16 | struct.unpack_from("<H", e, 26)[0]
        out.append((name.strip(), e[11], cl, struct.unpack_from("<I", e, 28)[0]))
    return out


def find(path):
    c = root
    e = None
    for part in path.strip("/").split("/"):
        hit = [x for x in entries(c) if x[0].upper() == part.upper()]
        if not hit:
            return None
        e = hit[0]
        c = e[2]
    return e


# 1) yazilan dosyalar
for path, src in [(a, b) for a, b in zip(sys.argv[1::2], sys.argv[2::2])]:
    e = find(path)
    if not e:
        print("YOK:", path)
        continue
    ch = chain(e[2])
    data = b"".join(rd(coff(c), csz) for c in ch)[:e[3]]
    ref = open(src, "rb").read()
    usb = sum(1 for i in range(0, len(data), 512) if data[i:i + 4] in (b"USBS", b"USBC"))
    print(f"{path}: {'AYNI' if data == ref else 'FARKLI'}  ({e[3]} B, {len(ch)} kume, USB paketi {usb})")

# 2) tum agac: zincir/boyut/capraz bag
owner, prob, n = {}, [], 0


def walk(c, p, depth=0):
    global n
    for name, attr, cl, size in entries(c):
        if name in (".", "..") or attr & 0x08:
            continue
        q = p + "/" + name
        n += 1
        if cl >= NCL:
            prob.append((q, "gecersiz kume"))
            continue
        if cl < 2:
            continue
        ch = chain(cl)
        last = nxt(ch[-1]) if ch else 0
        if last < 0x0FFFFFF8:
            prob.append((q, f"zincir bitis isareti yok ({hex(last)})"))
        if not attr & 0x10 and len(ch) != (size + csz - 1) // csz:
            prob.append((q, f"zincir {len(ch)} != boyut {(size + csz - 1) // csz}"))
        for k in ch:
            if k in owner:
                prob.append((q, f"capraz bag {owner[k]}"))
            owner[k] = q
        if attr & 0x10 and depth < 8:
            walk(cl, q, depth + 1)


walk(root, "")
print("denetlenen:", n, "sorun:", len(prob))
for x in prob:
    print("  ", x)
