"""cift ekranli menu: _part1 + _io_dual + (micro'dan ortak 3B kartus/acilis bolumleri) + _ui_dual -> main.c"""
import pathlib

S = pathlib.Path(__file__).resolve().parent.parent / "source"
p1 = (S / "_part1.txt").read_text(encoding="utf-8")
io = (S / "_io_dual.txt").read_text(encoding="utf-8")
old = (S / "_part3.txt").read_text(encoding="utf-8")
ui = (S / "_ui_dual.txt").read_text(encoding="utf-8")

cart = old[:old.index("// ------------------------------------------------------------------ ust durum cubugu")]
boot = old[old.index("// ------------------------------------------------------------------ acilis animasyonu"):
           old.index("// ------------------------------------------------------------------ yukleme (oyun baslatma) animasyonu")]

ds_branch = """    if (g->type == T_NDS) {                       // Nintendo DS oyun karti
        c.shell = dsCartImg;
        c.texW = 128;
        c.bx0 = DS_BODY_X0; c.by0 = DS_BODY_Y0; c.bx1 = DS_BODY_X1; c.by1 = DS_BODY_Y1;
        c.lx0 = -1 + (DS_LABEL_X - 0.5f) / 64.0f; c.lx1 = c.lx0 + (DS_LABEL_W + 1.0f) / 64.0f;
        c.ly0 = 1 - (DS_LABEL_Y - 0.3f) / 64.0f;  c.ly1 = c.ly0 - (DS_LABEL_H + 0.6f) / 64.0f;
        c.bottom = 1 - DS_BOTTOM / 64.0f;
        c.tint = C_WHITE;
        c.lw = DS_LABEL_W;
        c.clear = 0;
        return c;
    }
    if (g->type == T_GBA) {"""
assert cart.count("    if (g->type == T_GBA) {") == 1
cart = cart.replace("    if (g->type == T_GBA) {", ds_branch, 1)

# Game yapisi: tab/sep/icon -> io dosyasindan once tintCur gerekli (mixColor/vivid part1'de)
tint_decl = "static u16 tintCur = RGB15(12, 16, 28);   // animasyonlu ortam rengi\n"
assert tint_decl in cart
cart = cart.replace(tint_decl, "", 1)

fwd = "static u16 tintCur = RGB15(12, 16, 28);\nstatic void backdrop(float fade, float glow, float gx, float gy);\n"
boot = boot.replace('"SINGLE SCREEN EDITION"', '"DUAL SCREEN EDITION"')
out = p1 + fwd + io + cart + boot + ui
(S / "main.c").write_text(out, encoding="utf-8", newline="\n")
print("main.c yazildi", len(out.splitlines()), "satir")
