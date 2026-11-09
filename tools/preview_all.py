from PIL import Image, ImageDraw
import struct, re, json, pathlib, sys
R = pathlib.Path(sys.argv[1])
h = open(R/'source/assets.h').read(); g = lambda k: int(re.search(k + r' (\d+)', h).group(1))
X, Y, W, H = g('DS_LABEL_X'), g('DS_LABEL_Y'), g('DS_LABEL_W'), g('DS_LABEL_H')
shell = Image.open(R/'tools/_dscart_preview.png').convert('RGBA')
codes = json.load(open(R/'tools/nds_codes.json', encoding='utf-8'))
names = sorted(codes)
sheet = Image.new('RGBA', (6 * 200, ((len(names) + 5) // 6) * 210), (18, 19, 26, 255))
d = ImageDraw.Draw(sheet)
for i, n in enumerate(names):
    stem = n.rsplit('.', 1)[0]
    raw = open(R/'sdroot/_macro/labels'/(stem + '.lbl'), 'rb').read()
    pal = [struct.unpack_from('<H', raw, k * 2)[0] for k in range(256)]
    lab = Image.new('RGBA', (W, H))
    for y in range(H):
        for x in range(W):
            c = pal[raw[512 + y * 128 + x]]; lab.putpixel((x, y), ((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3, 255))
    c = shell.copy(); c.alpha_composite(lab, (X, Y))
    c = c.crop((14, 12, 114, 116)).resize((200, 208), Image.LANCZOS)
    sheet.alpha_composite(c, ((i % 6) * 200, (i // 6) * 210))
    d.text(((i % 6) * 200 + 4, (i // 6) * 210 + 2), codes[n], fill=(255, 255, 0))
sheet.save(R/'tools/_all.png')
