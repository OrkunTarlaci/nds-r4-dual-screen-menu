"""SD kartin sektor sektor ham yedegi (SALT OKUNUR). Kullanim: python raw_backup.py <cikti.img> [baslangic_bayt]
Ortam degiskeni SD_DRIVE (varsayilan F:). Bozuk okuyucularda: hata olunca bekler, aygiti yeniden acar,
ayni yeri tekrar dener (bos birakmaz)."""
import os
import struct
import sys
import time

DEV = "\\\\.\\" + os.environ.get("SD_DRIVE", "F:")   # SD_DRIVE=E: gibi
OUT = sys.argv[1]
START = int(sys.argv[2]) if len(sys.argv) > 2 else 0


def opendev():
    return open(DEV, "rb", buffering=0)


f = opendev()
SIZE = struct.unpack_from("<I", f.read(512), 32)[0] * 512
o = open(OUT, "r+b" if os.path.exists(OUT) else "w+b")
o.truncate(START)
o.seek(START)
pos, fails, giveup = START, 0, 0
t0 = time.time()
while pos < SIZE:
    n = min(65536, SIZE - pos)
    d = None
    for att in range(30):
        try:
            f.seek(pos)
            d = f.read(n)
            if len(d) == n:
                break
            d = None
        except OSError:
            d = None
        fails += 1
        time.sleep(min(2 + att, 10))
        try:
            f.close()
        except Exception:
            pass
        try:
            f = opendev()
        except OSError:
            pass
    if d is None:
        print("VAZGECILDI", pos, flush=True)
        giveup += 1
        d = bytes(n)
    o.write(d)
    pos += n
    if pos % (128 << 20) == 0:
        print(f"{pos >> 20} MB  {time.time() - t0:.0f} sn  hata/yeniden: {fails}", flush=True)
o.close()
print("bitti", pos, "vazgecilen 64KB:", giveup, "toplam yeniden deneme:", fails, flush=True)
