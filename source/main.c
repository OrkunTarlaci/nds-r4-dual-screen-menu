// macro DS - cift ekranli R4 menusu ("Polar" temasi)
// Ust ekran: 3B kartus halkasi (DS'in 3D motoru). Alt ekran: dokunmatik oyun listesi (2B bitmap).
// NDS oyunlari kartin kendi YSMenu cekirdegiyle (AUTO_BOOT), GBA oyunlari GBARunner2, GB/GBC GameYob ile acilir.

#include <nds.h>
#include <fat.h>
#include <gl2d.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>

#include "assets.h"
#include "nds_loader_arm9.h"

// ------------------------------------------------------------------ ayarlar
#define NDS_DIR         "fat:/01 - Games"
#define GBA_DIR         "fat:/GBA/gba"
#define GB_DIR          "fat:/GB"
#define LABEL_DIR       "fat:/_macro/labels"
#define YSMENU_KERNEL   "fat:/_macro/ysmenu.dat"     // kartin orijinal R4.dat'i (YSMenu cekirdegi)
#define YSMENU_INI      "fat:/TTMenu/YSMenu.ini"
#define GBAR2_A7        "fat:/_nds/GBARunner2_arm7dldi_ds.nds"
#define GAMEYOB         "fat:/_nds/TWiLightMenu/emulators/gameyob.nds"
#define MAX_GAMES       96
#define MAX_LABELS      48
#define MAX_COVERS      1

#define FX(f) ((s32)((f) * 4096))

// Renk paleti (RGB15)
#define C_BG0       RGB15(1, 1, 3)
#define C_BG1       RGB15(0, 0, 1)
#define C_INK       RGB15(3, 4, 6)
#define C_TEXT      RGB15(31, 31, 31)
#define C_MUTED     RGB15(17, 19, 23)
#define C_FAINT     RGB15(9, 10, 13)
#define C_ACCENT    RGB15(11, 19, 31)
#define C_CYAN      RGB15(12, 27, 31)
#define C_VIOLET    RGB15(20, 10, 31)
#define C_WHITE     RGB15(31, 31, 31)
#define C_BLACK     RGB15(0, 0, 0)
#define C_SHADOW    RGB15(0, 0, 1)
#define C_BG_TOP    C_WHITE
// acik ("Polar") tema
#define P_BG        RGB15(29, 29, 30)
#define P_PANEL     RGB15(31, 31, 31)
#define P_INK       RGB15(3, 4, 6)
#define P_MUTED     RGB15(13, 14, 17)
#define P_FAINT     RGB15(24, 25, 27)
#define P_LINE      RGB15(26, 26, 28)

enum { T_GBA, T_GB, T_GBC, T_NDS };
enum { TAB_NDS, TAB_GBA, TAB_GB, TAB_N };

typedef struct {
    char file[128];     // dosya adi
    char title[72];     // ekranda gorunen ad
    char sub[48];       // ikinci satir (NDS: yayinci)
    int type;
    int tab;            // hangi sekmede
    int sep;            // 1 = bolum basligi (GBA klasorundeki ayirici dosya), oyun degil
    int label;          // etiket dokusu, -1 = yok
    int cover;          // (kullanilmiyor)
    int coverW, coverH;
    u16 c1, c2;         // etiketi olmayan oyunlar icin renk
    u16 tint;           // oyunun baskin rengi
    u8 cgb;             // GB/GBC rom basligi 0x143
    u8 clear;
    u8 hasSave;
    u16 shell;          // kartus plastigi rengi
    u16 *icon;          // 32x32 kucuk resim (liste icin), BIT15 = opak
    u16 *thumb;         // 72x54 kucuk kapak
    u16 *hero;          // alt ekrandaki buyuk kapak (en fazla 104x92)
    int heroW, heroH;
    u16 *tile;          // ust ekran resmi (GBA/GB: etiketin tamami, en fazla 72x64)
    int tileW, tileH;
    u16 *mini;          // alt seritteki kucuk kartus (44x40)
    u16 *bg;            // ust ekran arka plani: kapagin bulanik hali (128x96)
    u8 bgTried;         // .bgb okunmaya calisildi mi (tembel yukleme)
} Game;

static Game games[MAX_GAMES];
static int gameCount = 0;

static glImage fontS[FONT_S_N], fontL[FONT_L_N];
static glImage logoW[1], dotImg[1], cartImg[1], gbCartImg[1], dsCartImg[1];
static glImage labelImg[MAX_LABELS][1];
static int labelCount = 0;
static glImage coverImg[MAX_COVERS][1];
static int coverCount = 0;

// stb_image (stb_impl.c)
extern unsigned char *macro_png_load(const char *path, int *w, int *h, const char **err);
extern void macro_png_free(unsigned char *p);

static int frame = 0;
static int polyId = 1;

// ------------------------------------------------------------------ yardimcilar
static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float easeOut(float t) { t = clampf(t, 0, 1); return 1 - (1 - t) * (1 - t) * (1 - t); }
static inline float easeOut5(float t) { t = clampf(t, 0, 1); float u = 1 - t; return 1 - u * u * u * u * u; }
static inline float easeIn(float t) { t = clampf(t, 0, 1); return t * t * t; }
static inline float easeInOut(float t) { t = clampf(t, 0, 1); return t * t * (3 - 2 * t); }
static inline float easeBack(float t)
{
    t = clampf(t, 0, 1) - 1;
    return 1 + 2.70158f * t * t * t + 1.70158f * t * t;
}
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
// 0..1 araliginda: a'da baslar, b'de biter
static inline float span(float t, float a, float b) { return clampf((t - a) / (b - a), 0, 1); }

static u16 mixColor(u16 a, u16 b, float t)
{
    t = clampf(t, 0, 1);
    int r = (a & 31) + (int)(((b & 31) - (a & 31)) * t);
    int g = ((a >> 5) & 31) + (int)((((b >> 5) & 31) - ((a >> 5) & 31)) * t);
    int bl = ((a >> 10) & 31) + (int)((((b >> 10) & 31) - ((a >> 10) & 31)) * t);
    return RGB15(r, g, bl);
}

// Rengi canli ve parlak tut (koyu temada ortam isigi icin)
static u16 vivid(u16 c, float minBright)
{
    int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    int m = r > g ? (r > b ? r : b) : (g > b ? g : b);
    if (m == 0) return RGB15(10, 14, 28);
    float k = minBright * 31 / m;
    if (k < 1) k = 1;
    r = (int)(r * k); g = (int)(g * k); b = (int)(b * k);
    return RGB15(r > 31 ? 31 : r, g > 31 ? 31 : g, b > 31 ? 31 : b);
}

// Saydamlik: 0 = gorunmez, 31 = tam opak. Her yari saydam cizim farkli polygon ID ister.
static void alpha(int a)
{
    if (a < 1) a = 1;
    if (a > 31) a = 31;
    polyId = polyId % 62 + 1;
    glPolyFmt(POLY_ALPHA(a) | POLY_CULL_NONE | POLY_ID(polyId));
}

static float sinf_(float x)  // hizli sinus (donanim tablosu)
{
    int a = (int)(x * 32768.0f / 6.2831853f);
    return sinLerp((s16)a) / 4096.0f;
}
static inline float cosf_(float x) { return sinf_(x + 1.5707963f); }

// ------------------------------------------------------------------ yazi
static int glyphIndex(u32 cp)
{
    if (cp >= 32 && cp < 127) return cp - 32;          // ASCII: dogrudan
    for (int i = 95; i < FONT_S_N; i++)
        if (font_codepoints[i] == cp) return i;
    return '?' - 32;
}

static u32 nextCodepoint(const char **s)
{
    const u8 *p = (const u8 *)*s;
    u32 c = *p++;
    if (c >= 0xC0 && c < 0xE0) c = ((c & 0x1F) << 6) | (*p++ & 0x3F);
    else if (c >= 0xE0) { c = ((c & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F); p += 2; }
    *s = (const char *)p;
    return c;
}

static int textWidthSp(int large, const char *s, int sp)
{
    const u8 *adv = large ? font_l_adv : font_s_adv;
    int w = 0, n = 0;
    while (*s) { w += adv[glyphIndex(nextCodepoint(&s))]; n++; }
    return w + (n > 1 ? (n - 1) * sp : 0);
}
static int textWidth(int large, const char *s) { return textWidthSp(large, s, 0); }

// align: 0 sol, 1 orta, 2 sag ; sp: harf araligi (piksel)
static void drawTextSp(int large, int x, int y, const char *s, u16 color, int a, int align, int sp)
{
    if (a <= 0) return;
    const glImage *f = large ? fontL : fontS;
    const u8 *adv = large ? font_l_adv : font_s_adv;
    if (align == 1) x -= textWidthSp(large, s, sp) / 2;
    else if (align == 2) x -= textWidthSp(large, s, sp);
    glColor(color);
    alpha(a);
    while (*s) {
        int g = glyphIndex(nextCodepoint(&s));
        if (g != 0) glSprite(x, y, GL_FLIP_NONE, &f[g]);
        x += adv[g] + sp;
    }
    glColor(C_WHITE);
}
static void drawText(int large, int x, int y, const char *s, u16 color, int a, int align)
{
    drawTextSp(large, x, y, s, color, a, align, 0);
}

// Yaziyi hafif golgeli ciz (koyu zeminde okunakli)
static void drawTextShadow(int large, int x, int y, const char *s, u16 color, int a, int align)
{
    drawText(large, x + 1, y + 1, s, C_BLACK, a * 2 / 3, align);
    drawText(large, x, y, s, color, a, align);
}

// Uzun adlari sigdir
static void fitText(int large, char *out, const char *in, int maxW)
{
    strcpy(out, in);
    if (textWidth(large, out) <= maxW) return;
    int n = strlen(out);
    while (n > 1) {
        n--;
        while (n > 0 && ((u8)out[n] & 0xC0) == 0x80) n--;  // UTF-8 ortasinda kesme
        out[n] = 0;
        char tmp[96];
        snprintf(tmp, sizeof(tmp), "%s...", out);
        if (textWidth(large, tmp) <= maxW) { strcpy(out, tmp); return; }
    }
}

// ------------------------------------------------------------------ 2B cizim parcalari
static void sprite(const glImage *img, float cx, float cy, float sx, float sy, u16 color, int a)
{
    if (a <= 0) return;
    glColor(color);
    alpha(a);
    glSpriteScaleXY((int)(cx - img->width * sx / 2), (int)(cy - img->height * sy / 2),
                    FX(sx), FX(sy), GL_FLIP_NONE, img);
    glColor(C_WHITE);
}

static void box(int x1, int y1, int x2, int y2, u16 color, int a)
{
    if (a <= 0) return;
    alpha(a);
    glBoxFilled(x1, y1, x2, y2, color);
}

static void gbox(int x1, int y1, int x2, int y2, u16 top, u16 bot, int a)
{
    if (a <= 0) return;
    alpha(a);
    glBoxFilledGradient(x1, y1, x2, y2, top, bot, bot, top);
}

static void roundBox(int x1, int y1, int x2, int y2, u16 color, int a)
{
    if (a <= 0) return;
    alpha(a);
    glBoxFilled(x1 + 2, y1, x2 - 2, y2, color);
    glBoxFilled(x1, y1 + 2, x1 + 1, y2 - 2, color);
    glBoxFilled(x2 - 1, y1 + 2, x2, y2 - 2, color);
    glBoxFilled(x1 + 1, y1 + 1, x1 + 1, y2 - 1, color);
    glBoxFilled(x2 - 1, y1 + 1, x2 - 1, y2 - 1, color);
}

// Hap (pill) seklinde: uclari yarim daire (yumusak nokta dokusuyla)
static void pill(int x1, int y1, int x2, int y2, u16 color, int a)
{
    if (a <= 0) return;
    int h = y2 - y1 + 1, r = h / 2;
    alpha(a);
    glBoxFilled(x1 + r, y1, x2 - r, y2, color);
    for (int yy = 0; yy < h; yy++) {               // yarim daireler, satir satir
        float d = (yy + 0.5f - h / 2.0f) / (h / 2.0f);
        int w = (int)(r * sqrtf(clampf(1 - d * d, 0, 1)) + 0.5f);
        if (w <= 0) continue;
        glBoxFilled(x1 + r - w, y1 + yy, x1 + r - 1, y1 + yy, color);
        glBoxFilled(x2 - r + 1, y1 + yy, x2 - r + w, y1 + yy, color);
    }
}

// 2B dokulu dortgen, dokunun bir parcasini (u0..u1, v0..v1 orani) cizer
static void quad2D(const glImage *img, int x0, int y0, int x1, int y1, float fu0, float fu1, u16 col, int a)
{
    if (a <= 0 || x1 <= x0) return;
    int u0 = img->u_off + (int)(img->width * fu0), u1 = img->u_off + (int)(img->width * fu1);
    int v0 = img->v_off, v1 = img->v_off + img->height;
    alpha(a);
    glBindTexture(0, img->textureID);
    glColor(col);
    glBegin(GL_QUADS);
    glTexCoord2t16(inttot16(u0), inttot16(v0)); glVertex3v16(x0, y0, 400);
    glTexCoord2t16(inttot16(u0), inttot16(v1)); glVertex3v16(x0, y1, 400);
    glTexCoord2t16(inttot16(u1), inttot16(v1)); glVertex3v16(x1, y1, 400);
    glTexCoord2t16(inttot16(u1), inttot16(v0)); glVertex3v16(x1, y0, 400);
    glEnd();
    glColor(C_WHITE);
}

// Ucgen (ok isaretleri, isik huzmeleri)
static void tri(int x1, int y1, int x2, int y2, int x3, int y3, u16 c1, u16 c2, u16 c3, int a)
{
    if (a <= 0) return;
    alpha(a);
    glTriangleFilledGradient(x1, y1, x2, y2, x3, y3, c1, c2, c3);
}

// ------------------------------------------------------------------ katmanlar
// Arka plan katmani: derinlikte en arkada.  Arayuz katmani: en onde.
static void beginBack(void) { glBegin2D(); glTranslatef32(0, 0, -3890); }   // z = -0.95
static void beginFront(void) { glBegin2D(); glTranslatef32(0, 0, 3072); }   // z = +0.75
static void end2D(void) { glEnd2D(); }

static void endFrame(void)
{
    glFlush(GL_TRANS_MANUALSORT);
    swiWaitForVBlank();
    frame++;
}

// ------------------------------------------------------------------ 3B sahne
// Kamera orijinde, -z'ye bakar. Dikey gorus acisi 50 derece.
// Ozel izdusum: derinlik degerleri 2B katmanlarin arasina sigsin diye sikistirilir
// (uzaklik 2 -> NDC -0.3, sonsuz -> NDC 0.7).
#define CAM_F     2.1445069f            // 1 / tan(25 derece)
#define CAM_D     3.30f                 // ortadaki kartusun uzakligi
#define CART_Y    0.45f                 // ortadaki kartusun yuksekligi (ekranda ~y 74)
#define CART_S    1.15f                 // ortadaki kartusun olcegi (doku 1:1 piksel = net)

static void begin3D(void)
{
    m4x4 m;
    memset(&m, 0, sizeof(m));
    m.m[0] = floattof32(CAM_F * 192.0f / 256.0f);
    m.m[5] = floattof32(CAM_F);
    m.m[10] = floattof32(-0.7f);
    m.m[11] = floattof32(-1.0f);
    m.m[14] = floattof32(-2.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&m);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Dunya noktasini ekran pikseline cevir (2B isiklari 3B nesnelerle hizalamak icin)
static void project(float x, float y, float z, float *sx, float *sy)
{
    float d = -z;
    if (d < 0.1f) d = 0.1f;
    *sx = 128 + x * CAM_F * 96 / d;
    *sy = 96 - y * CAM_F * 96 / d;
}

static inline int deg(float d) { return (int)(d * 32768.0f / 360.0f); }

// Yerel koordinatlarda dokulu dortgen (y yukari). Ust ve alt koseler farkli renk alabilir.
static void quad3(const glImage *img, float x0, float yT, float x1, float yB, float z,
                  u16 cT, u16 cB, int flipV)
{
    int u0 = img->u_off, v0 = img->v_off, u1 = u0 + img->width, v1 = v0 + img->height;
    if (flipV) { int t = v0; v0 = v1; v1 = t; }
    v16 X0 = floattov16(x0), X1 = floattov16(x1), YT = floattov16(yT), YB = floattov16(yB), Z = floattov16(z);
    glBindTexture(0, img->textureID);
    glBegin(GL_QUADS);
    glColor(cT); glTexCoord2t16(inttot16(u0), inttot16(v0)); glVertex3v16(X0, YT, Z);
    glColor(cB); glTexCoord2t16(inttot16(u0), inttot16(v1)); glVertex3v16(X0, YB, Z);
    glTexCoord2t16(inttot16(u1), inttot16(v1)); glVertex3v16(X1, YB, Z);
    glColor(cT); glTexCoord2t16(inttot16(u1), inttot16(v0)); glVertex3v16(X1, YT, Z);
    glEnd();
}

// Paralelkenar (egik isik seridi)
static void quad3skew(const glImage *img, float x0, float yT, float x1, float yB, float skew, float z, u16 c)
{
    int u0 = img->u_off, v0 = img->v_off, u1 = u0 + img->width, v1 = v0 + img->height;
    glBindTexture(0, img->textureID);
    glColor(c);
    glBegin(GL_QUADS);
    glTexCoord2t16(inttot16(u0), inttot16(v0)); glVertex3v16(floattov16(x0 + skew), floattov16(yT), floattov16(z));
    glTexCoord2t16(inttot16(u0), inttot16(v1)); glVertex3v16(floattov16(x0), floattov16(yB), floattov16(z));
    glTexCoord2t16(inttot16(u1), inttot16(v1)); glVertex3v16(floattov16(x1), floattov16(yB), floattov16(z));
    glTexCoord2t16(inttot16(u1), inttot16(v0)); glVertex3v16(floattov16(x1 + skew), floattov16(yT), floattov16(z));
    glEnd();
}

static u16 tintCur = RGB15(12, 16, 28);
static void backdrop(float fade, float glow, float gx, float gy);

// ================================================================== oyun listesi, etiketler, dokular
// Paletli bir goruntunun baskin rengi (doygun pikseller agirlikli)
static u16 averageColor(const u16 *pal, const u8 *pix, int w, int h, int stride)
{
    u32 r = 0, g = 0, b = 0, n = 0;
    for (int y = 0; y < h; y += 2)
        for (int x = 0; x < w; x += 2) {
            u8 i = pix[y * stride + x];
            u16 c = pal[i];
            int R = c & 31, G = (c >> 5) & 31, B = (c >> 10) & 31;
            int mx = R > G ? (R > B ? R : B) : (G > B ? G : B);
            int mn = R < G ? (R < B ? R : B) : (G < B ? G : B);
            u32 wgt = 1 + (mx - mn) * (mx - mn);
            r += R * wgt; g += G * wgt; b += B * wgt; n += wgt;
        }
    if (!n) return RGB15(12, 16, 28);
    return RGB15(r / n, g / n, b / n);
}

// 16 bit goruntunun baskin rengi
static u16 average16(const u16 *pix, int w, int h, int stride)
{
    u32 r = 0, g = 0, b = 0, n = 0;
    for (int y = 0; y < h; y += 2)
        for (int x = 0; x < w; x += 2) {
            u16 c = pix[y * stride + x];
            if (!(c & BIT(15))) continue;
            int R = c & 31, G = (c >> 5) & 31, B = (c >> 10) & 31;
            int mx = R > G ? (R > B ? R : B) : (G > B ? G : B);
            int mn = R < G ? (R < B ? R : B) : (G < B ? G : B);
            u32 wgt = 1 + (mx - mn) * (mx - mn);
            r += R * wgt; g += G * wgt; b += B * wgt; n += wgt;
        }
    if (!n) return RGB15(12, 16, 28);
    return RGB15(r / n, g / n, b / n);
}

static const char *ext(const char *f)
{
    const char *d = strrchr(f, '.');
    return d ? d : "";
}

// Dogal siralama ("2-" < "10-"), buyuk/kucuk harf duyarsiz
static int cmpName(const char *x, const char *y)
{
    while (*x && *y) {
        if (*x >= '0' && *x <= '9' && *y >= '0' && *y <= '9') {
            long nx = strtol(x, (char **)&x, 10), ny = strtol(y, (char **)&y, 10);
            if (nx != ny) return nx < ny ? -1 : 1;
            continue;
        }
        int cx = (*x >= 'A' && *x <= 'Z') ? *x + 32 : *x;
        int cy = (*y >= 'A' && *y <= 'Z') ? *y + 32 : *y;
        if (cx != cy) return cx - cy;
        x++; y++;
    }
    return (u8)*x - (u8)*y;
}
static int cmpGame(const void *a, const void *b)
{
    const Game *ga = a, *gb = b;
    if (ga->tab != gb->tab) return ga->tab - gb->tab;
    return cmpName(ga->file, gb->file);
}

// "0003 - Pokemon HeartGold .nds" -> "Pokemon HeartGold"; "0000-----Pokemon-----.gba" -> "Pokemon" (ayirici)
static void makeTitle(char *out, const char *file, int *sep)
{
    *sep = strstr(file, "---") != NULL;
    const char *p = file;
    while (*p >= '0' && *p <= '9') p++;
    while (*p == '-' || *p == ' ' || *p == '_') p++;
    if (!*p) p = file;
    strncpy(out, p, 71);
    out[71] = 0;
    char *d = strrchr(out, '.');
    if (d) *d = 0;
    int n = strlen(out);
    while (n > 0 && (out[n - 1] == ' ' || out[n - 1] == '-')) out[--n] = 0;
}

static void gamePath(const Game *g, char *out, int n)
{
    const char *dir = g->tab == TAB_NDS ? NDS_DIR : (g->tab == TAB_GBA ? GBA_DIR : GB_DIR);
    snprintf(out, n, "%s/%s", dir, g->file);
}

static int hasWord(const char *s, const char *w)
{
    for (; *s; s++) {
        const char *a = s, *b = w;
        while (*a && *b && (*a | 32) == (*b | 32)) { a++; b++; }
        if (!*b) return 1;
    }
    return 0;
}

// GB kartus plastigi: Game Boy gri, GB+GBC ortak siyah, sadece GBC mor-gri; bazi oyunlarin kendi rengi
static void shellStyle(Game *g)
{
    g->clear = 0;
    if (g->cgb == 0xC0) g->shell = RGB15(21, 21, 27);
    else if (g->cgb == 0x80) g->shell = RGB15(6, 6, 8);
    else g->shell = RGB15(20, 20, 21);
    static const struct { const char *k; u16 c; } T[] = {
        { "yellow", RGB15(31, 25, 3) }, { "red version", RGB15(26, 5, 5) }, { "blue version", RGB15(5, 10, 27) },
        { "green version", RGB15(6, 20, 8) }, { "gold", RGB15(27, 21, 6) }, { "silver", RGB15(25, 25, 27) },
        { "crystal", RGB15(17, 24, 30) },
    };
    for (unsigned i = 0; i < sizeof(T) / sizeof(T[0]); i++)
        if (hasWord(g->title, T[i].k)) g->shell = T[i].c;
}

// UTF-16 -> UTF-8 (BMP)
static int putUtf8(char *o, int room, u16 c)
{
    if (c < 0x80) { if (room < 1) return 0; o[0] = c; return 1; }
    if (c < 0x800) { if (room < 2) return 0; o[0] = 0xC0 | (c >> 6); o[1] = 0x80 | (c & 63); return 2; }
    if (room < 3) return 0;
    o[0] = 0xE0 | (c >> 12); o[1] = 0x80 | ((c >> 6) & 63); o[2] = 0x80 | (c & 63);
    return 3;
}

// NDS banner: 32x32 simge + Ingilizce baslik ("Ad\nAlt ad\nYayinci")
static void readNdsBanner(Game *g, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return;
    u32 off = 0;
    if (fseek(f, 0x68, SEEK_SET) != 0 || fread(&off, 4, 1, f) != 1 || off == 0 || off > 0x10000000) { fclose(f); return; }
    static u8 bn[0x440];
    if (fseek(f, off, SEEK_SET) != 0 || fread(bn, 1, sizeof(bn), f) != sizeof(bn)) { fclose(f); return; }
    fclose(f);
    // simge: 4x4 karo, karo basina 8x8 piksel, 4 bit; palet 16 renk (0 = saydam)
    const u8 *bmp = bn + 0x20;
    const u16 *pal = (const u16 *)(bn + 0x220);
    g->icon = malloc(32 * 32 * 2);
    if (g->icon) {
        for (int ty = 0; ty < 4; ty++)
            for (int tx = 0; tx < 4; tx++)
                for (int y = 0; y < 8; y++)
                    for (int x = 0; x < 8; x++) {
                        u8 b = bmp[(ty * 4 + tx) * 32 + y * 4 + x / 2];
                        int idx = (x & 1) ? (b >> 4) : (b & 15);
                        g->icon[(ty * 8 + y) * 32 + tx * 8 + x] = idx ? (pal[idx] | BIT(15)) : 0;
                    }
        g->tint = average16(g->icon, 32, 32, 32);
    }
    // Ingilizce baslik: satirlara bol; son satir yayinci, oncekiler ad
    const u16 *t = (const u16 *)(bn + 0x340);
    char lines[3][72];
    int nl = 0, k = 0;
    memset(lines, 0, sizeof(lines));
    for (int i = 0; i < 128 && t[i]; i++) {
        if (t[i] == '\n') { if (nl < 2) { nl++; k = 0; } continue; }
        if (t[i] == '\r') continue;
        k += putUtf8(lines[nl] + k, 70 - k, t[i]);
    }
    if (!lines[0][0]) return;
    int last = nl;
    if (last >= 1) {
        strncpy(g->sub, lines[last], sizeof(g->sub) - 1);
        g->title[0] = 0;
        for (int i = 0; i < last; i++) {
            if (i) strncat(g->title, " ", sizeof(g->title) - strlen(g->title) - 1);
            strncat(g->title, lines[i], sizeof(g->title) - strlen(g->title) - 1);
        }
    } else {
        strncpy(g->title, lines[0], sizeof(g->title) - 1);
    }
}

static int tabCount[TAB_N];         // oyun sayisi (ayiricilar haric)

static void scanDir(const char *dirPath, int tab)
{
    DIR *dir = opendir(dirPath);
    if (!dir) return;
    struct dirent *e;
    while ((e = readdir(dir)) && gameCount < MAX_GAMES) {
        if (e->d_name[0] == '.') continue;
        const char *x = ext(e->d_name);
        int type;
        if (tab == TAB_NDS && !strcasecmp(x, ".nds")) type = T_NDS;
        else if (tab == TAB_GBA && !strcasecmp(x, ".gba")) type = T_GBA;
        else if (tab == TAB_GB && !strcasecmp(x, ".gbc")) type = T_GBC;
        else if (tab == TAB_GB && !strcasecmp(x, ".gb")) type = T_GB;
        else continue;
        if (type == T_NDS && hasWord(e->d_name, "GBARunner")) continue;   // emulator, oyun degil
        Game *g = &games[gameCount++];
        memset(g, 0, sizeof(*g));
        strncpy(g->file, e->d_name, sizeof(g->file) - 1);
        makeTitle(g->title, g->file, &g->sep);
        g->type = type;
        g->tab = tab;
        g->label = -1;
        g->cover = -1;
        u32 h = 2166136261u;
        for (const char *c = g->file; *c; c++) h = (h ^ (u8)*c) * 16777619u;
        g->c1 = RGB15(8 + (h & 15), 8 + ((h >> 4) & 15), 14 + ((h >> 8) & 15));
        g->c2 = mixColor(g->c1, RGB15(2, 2, 6), 0.6f);
        g->tint = g->c1;
        g->shell = C_WHITE;
        if (g->sep) continue;
        tabCount[tab]++;
    }
    closedir(dir);
}

// tek oyunun dosya okumalari (kayit var mi, NDS basligi/simgesi, GB renk bayragi); acilista karede bir
static void probeGame(Game *g)
{
    if (g->sep) return;
    char path[256], sav[256];
    gamePath(g, path, sizeof(path));
    strcpy(sav, path);
    strcpy(strrchr(sav, '.'), ".sav");
    struct stat st;
    g->hasSave = stat(sav, &st) == 0 && st.st_size > 0;
    if (g->type == T_NDS) {
        readNdsBanner(g, path);
    } else if (g->type != T_GBA) {
        FILE *rf = fopen(path, "rb");
        if (rf) {
            if (fseek(rf, 0x143, SEEK_SET) == 0) { int c = fgetc(rf); g->cgb = c < 0 ? 0 : (u8)c; }
            fclose(rf);
        }
        if (g->cgb == 0xC0 || g->cgb == 0x80) g->type = T_GBC;
        shellStyle(g);
    }
}

static void scanGames(void)
{
    scanDir(NDS_DIR, TAB_NDS);
    scanDir(GBA_DIR, TAB_GBA);
    scanDir(GB_DIR, TAB_GB);
    qsort(games, gameCount, sizeof(Game), cmpGame);
}

// ------------------------------------------------------------------ etiketler
static void labelDims(const Game *g, int *w, int *h)
{
    if (g->type == T_NDS) { *w = DS_LABEL_W; *h = DS_LABEL_H; }
    else if (g->type == T_GBA) { *w = CART_LABEL_W; *h = CART_LABEL_H; }
    else { *w = GB_LABEL_W; *h = GB_LABEL_H; }
}

// 16 bitlik kucuk resim (liste simgesi): etiketin ortasindan kare kesip 32x32'ye kucult
static u16 *thumbFrom(const u16 *pal, const u8 *pix, const u16 *pix16, int w, int h, int stride)
{
    u16 *o = malloc(32 * 32 * 2);
    if (!o) return NULL;
    int side = w < h ? w : h, x0 = (w - side) / 2, y0 = (h - side) / 2;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
            int sx = x0 + x * side / 32, sy = y0 + y * side / 32;
            u16 c = pix16 ? pix16[sy * stride + sx] : pal[pix[sy * stride + sx]];
            o[y * 32 + x] = c | BIT(15);
        }
    return o;
}

// ust ekran icin 64x64 kare: etiketin ortasindan kare, kutu filtreli kucultme, yuvarlak koseler (r=9)
static u16 *makeTile(const u16 *pal, const u8 *pix, int w, int h, int stride, int *ow, int *oh)
{
    // etiketin tamami (kesilmez), en fazla 72x64, kutu filtreli kucultme, yuvarlak koseler
    int dw = 72, dh = h * 72 / w;
    if (dh > 64) { dh = 64; dw = w * 64 / h; }
    u16 *o = malloc(dw * dh * 2);
    if (!o) return NULL;
    for (int y = 0; y < dh; y++) {
        int sy0 = y * h / dh, sy1 = (y + 1) * h / dh;
        if (sy1 <= sy0) sy1 = sy0 + 1;
        for (int x = 0; x < dw; x++) {
            int sx0 = x * w / dw, sx1 = (x + 1) * w / dw;
            if (sx1 <= sx0) sx1 = sx0 + 1;
            int r = 0, g = 0, b = 0, n = 0;
            for (int yy = sy0; yy < sy1; yy++)
                for (int xx = sx0; xx < sx1; xx++) {
                    u16 c = pal[pix[yy * stride + xx]];
                    r += c & 31; g += (c >> 5) & 31; b += (c >> 10) & 31; n++;
                }
            o[y * dw + x] = RGB15(r / n, g / n, b / n) | BIT(15);
        }
    }
    const int R = 7;
    for (int y = 0; y < R; y++)
        for (int x = 0; x < R; x++) {
            int dx = R - x, dy = R - y;
            if (dx * dx + dy * dy > R * R + 2) {
                o[y * dw + x] = 0; o[y * dw + dw - 1 - x] = 0;
                o[(dh - 1 - y) * dw + x] = 0; o[(dh - 1 - y) * dw + dw - 1 - x] = 0;
            }
        }
    *ow = dw; *oh = dh;
    return o;
}

#define TW 72
#define TH 54
static u16 *makeThumb(const u16 *pal, const u8 *pix, int w, int h, int stride)
{
    u16 *o = malloc(TW * TH * 2);
    if (!o) return NULL;
    for (int i = 0; i < TW * TH; i++) o[i] = RGB15(3, 3, 5) | BIT(15);
    int dw, dh;
    if (w * TH >= h * TW) { dw = TW; dh = h * TW / w; } else { dh = TH; dw = w * TH / h; }
    int ox = (TW - dw) / 2, oy = (TH - dh) / 2;
    for (int y = 0; y < dh; y++) {
        int sy0 = y * h / dh, sy1 = (y + 1) * h / dh;
        if (sy1 <= sy0) sy1 = sy0 + 1;
        for (int x = 0; x < dw; x++) {
            int sx0 = x * w / dw, sx1 = (x + 1) * w / dw;
            if (sx1 <= sx0) sx1 = sx0 + 1;
            int r = 0, g = 0, b = 0, n = 0;
            for (int yy = sy0; yy < sy1; yy++)
                for (int xx = sx0; xx < sx1; xx++) {
                    u16 c = pal[pix[yy * stride + xx]];
                    r += c & 31; g += (c >> 5) & 31; b += (c >> 10) & 31; n++;
                }
            o[(oy + y) * TW + ox + x] = RGB15(r / n, g / n, b / n) | BIT(15);
        }
    }
    return o;
}

// Uretilmis etiketler: _macro/labels/<oyun adi>.lbl (128x64, 256 renk)
static int loadLabels(int maxN)
{
    static u8 buf[512 + 8192];
    int done = 0;
    for (int i = 0; i < gameCount && labelCount < MAX_LABELS; i++) {
        Game *g = &games[i];
        if (g->sep || g->label != -1) continue;
        if (done >= maxN) return 0;
        done++;
        g->label = -2;                              // denendi (bulunamazsa -2 kalir)
        char path[256];
        if (g->type == T_NDS) {                     // NDS: kapak resminden hazirlanan etiket, dosya adiyla
            snprintf(path, sizeof(path), "%s/%s", LABEL_DIR, g->file);
            strcpy(strrchr(path, '.'), ".lbl");
        } else {
            snprintf(path, sizeof(path), "%s/%s.lbl", LABEL_DIR, g->title);
        }
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        size_t n = fread(buf, 1, sizeof(buf), f);
        fclose(f);
        if (n != sizeof(buf)) continue;
        int lw, lh;
        labelDims(g, &lw, &lh);
        int id;
        if (g->type == T_NDS) {                     // NDS etiketi 64x64 doku (4 KB): kartustaki pencereye gerilir
            static u8 p64[64 * 64];
            for (int y = 0; y < 64; y++) memcpy(p64 + y * 64, buf + 512 + y * 128, 64);
            id = glLoadTileSet(labelImg[labelCount], 64, 64, 64, 64, GL_RGB256,
                               TEXTURE_SIZE_64, TEXTURE_SIZE_64, TEXGEN_OFF, 256, (u16 *)buf, p64);
        } else
            id = glLoadTileSet(labelImg[labelCount], lw, lh, 128, 64, GL_RGB256,
                               TEXTURE_SIZE_128, TEXTURE_SIZE_64, TEXGEN_OFF, 256, (u16 *)buf, buf + 512);
        if (id < 0) break;
        g->label = labelCount++;
        g->tint = averageColor((u16 *)buf, buf + 512, lw, lh, 128);
        if (!g->icon) g->icon = thumbFrom((u16 *)buf, buf + 512, NULL, lw, lh, 128);
        if (g->type != T_NDS) g->tile = makeTile((u16 *)buf, buf + 512, lw, lh, 128, &g->tileW, &g->tileH);
        g->thumb = makeThumb((u16 *)buf, buf + 512, lw, lh, 128);
    }
    return 1;
}

// NDS etiketi (gercek DS kart etiketine benzer): ustte beyaz serit ve "NINTENDO DS", altinda
// oyunun simgesi 2 kat buyutulmus, simgenin renginden yumusak bir zemin uzerinde. 256 renk:
//   1..15 simge, 16..79 zemin gecisi, 80..87 serit -> yazi, 88 serit alt cizgisi
#define BAND_H 0      // NINTENDO DS logosu artik kabuktaki beyaz bantta
static void makeNdsLabels(void)
{
    static u16 pal[256];
    static u8 pix[128 * 64];
    for (int i = 0; i < gameCount && labelCount < MAX_LABELS; i++) {
        Game *g = &games[i];
        if (g->sep || g->type != T_NDS || g->label >= 0 || !g->icon) continue;
        u16 base = vivid(g->tint, 0.85f);
        u16 top = mixColor(base, C_WHITE, 0.45f), bot = mixColor(base, C_BLACK, 0.30f);
        for (int k = 0; k < 64; k++) pal[16 + k] = mixColor(top, bot, k / 63.0f);
        u16 band = RGB15(30, 30, 31), ink = RGB15(6, 6, 8);
        for (int k = 0; k < 8; k++) pal[80 + k] = mixColor(band, ink, k / 7.0f);
        pal[88] = RGB15(22, 22, 24);
        pal[0] = band;
        int iconPal = 1;
        static u16 ipal[16];
        for (int p = 0; p < 32 * 32; p++) {
            u16 c = g->icon[p];
            if (!(c & BIT(15))) continue;
            c &= 0x7FFF;
            int found = 0;
            for (int q = 1; q < iconPal; q++) if (ipal[q] == c) { found = 1; break; }
            if (!found && iconPal < 16) ipal[iconPal++] = c;
        }
        for (int q = 1; q < 16; q++) pal[q] = q < iconPal ? ipal[q] : 0;
        memset(pix, 0, sizeof(pix));
        int artH = DS_LABEL_H - BAND_H;
        for (int y = 0; y < DS_LABEL_H; y++)
            for (int x = 0; x < DS_LABEL_W; x++) {
                if (y < BAND_H) { pix[y * 128 + x] = (y == BAND_H - 1) ? 88 : 80; continue; }
                int yy = y - BAND_H;
                int dx = x - DS_LABEL_W / 2, dy = yy - artH / 3;
                int k = yy * 63 / artH + (dx * dx + dy * dy) / 90;
                if (k > 63) k = 63;
                pix[y * 128 + x] = 16 + k;
            }
        // "NINTENDO DS" yazisi (kucuk yazi tipinden, alfa -> serit/yazi rampasi)
        {
            const char *txt = "NINTENDO DS";
            int x = 4;
            for (const char *c = txt; *c; c++) {
                int gi = glyphIndex((u8)*c);
                int gx = font_s_coords[gi * 4], gy = font_s_coords[gi * 4 + 1];
                int gw = font_s_coords[gi * 4 + 2], gh = font_s_coords[gi * 4 + 3];
                for (int j = 0; j < gh; j++) {
                    int yy = j - 1;                    // yazi tipinin ust boslugu
                    if (yy < 0 || yy >= BAND_H - 1) continue;
                    for (int k = 0; k < gw; k++) {
                        int al = font_s_tex[(gy + j) * 256 + gx + k] >> 3;
                        if (al && x + k < DS_LABEL_W) pix[yy * 128 + x + k] = 80 + (al >> 2);
                    }
                }
                x += font_s_adv[gi] - (*c == ' ' ? 1 : 0);
            }
        }
        // simge 2x, resim alaninin ortasina (alan 50 px: simgenin ust/alt bos kenarlari kirpilir)
        int ox = (DS_LABEL_W - 64) / 2, oy = BAND_H + (artH - 64) / 2;
        for (int y = 0; y < 64; y++) {
            int py = oy + y;
            if (py < BAND_H || py >= DS_LABEL_H) continue;
            for (int x = 0; x < 64; x++) {
                u16 c = g->icon[(y / 2) * 32 + x / 2];
                if (!(c & BIT(15))) continue;
                c &= 0x7FFF;
                int q = 1;
                while (q < iconPal && ipal[q] != c) q++;
                if (q < iconPal) pix[py * 128 + ox + x] = q;
            }
        }
        int id = glLoadTileSet(labelImg[labelCount], DS_LABEL_W, DS_LABEL_H, 128, 64, GL_RGB256,
                               TEXTURE_SIZE_128, TEXTURE_SIZE_64, TEXGEN_OFF, 256, pal, pix);
        if (id < 0) break;
        g->label = labelCount++;
        g->thumb = makeThumb(pal, pix, DS_LABEL_W, DS_LABEL_H, 128);
    }
}

// ------------------------------------------------------------------ buyuk kapaklar (_macro/hero/<oyun>.hrb)
static inline u16 bgDark15(u16 d, u16 c, int a)
{
    int r = d & 31, g = (d >> 5) & 31, b = (d >> 10) & 31;
    int R = c & 31, G = (c >> 5) & 31, B = (c >> 10) & 31;
    a += a >> 4;
    r += ((R - r) * a) >> 5; g += ((G - g) * a) >> 5; b += ((B - b) * a) >> 5;
    return RGB15(r, g, b) | BIT(15);
}
static void loadBg(Game *g)
{
    if (!g || g->sep || g->bgTried) return;
    g->bgTried = 1;
    char path[256];
    snprintf(path, sizeof(path), "fat:/_macro/hero/%s", g->file);
    char *d = strrchr(path, '.');
    if (!d) return;
    strcpy(d, ".bgb");                                 // bulanik arka plan (.hrb artik kullanilmiyor, okunmaz)
    FILE *f = fopen(path, "rb");
    if (!f) return;
    g->bg = malloc(128 * 96 * 2);
    if (g->bg && fread(g->bg, 2, 128 * 96, f) != 128 * 96) { free(g->bg); g->bg = NULL; }
    if (g->bg)                                         // vitrin karartmasi bir kez (ekran satiri = 2 x kaynak)
        for (int y = 0; y < 96; y++) {
            int Y = 2 * y, a = Y < 18 ? 20 - Y : (Y >= 124 ? (Y - 124) * 22 / 68 : 0);
            if (a > 0)
                for (int x = 0; x < 128; x++) g->bg[y * 128 + x] = bgDark15(g->bg[y * 128 + x], RGB15(0, 0, 1), a);
        }
    fclose(f);
}

// ------------------------------------------------------------------ dokular
static void loadTextures(void)
{
    static u16 white8[8];
    for (int i = 0; i < 8; i++) white8[i] = C_WHITE;
    glLoadSpriteSet(fontS, FONT_S_N, font_s_coords, GL_RGB8_A5, TEXTURE_SIZE_256, TEXTURE_SIZE_128,
                    TEXGEN_OFF, 8, white8, font_s_tex);
    glLoadSpriteSet(fontL, FONT_L_N, font_l_coords, GL_RGB8_A5, TEXTURE_SIZE_256, TEXTURE_SIZE_256,
                    TEXGEN_OFF, 8, white8, font_l_tex);
    {
        static u16 wpal[8];
        for (int i = 0; i < 8; i++) wpal[i] = C_WHITE;
        wpal[2] = RGB15(13, 21, 31);
        glLoadTileSet(logoW, 256, 64, 256, 64, GL_RGB8_A5, TEXTURE_SIZE_256, TEXTURE_SIZE_64,
                      TEXGEN_OFF, 8, wpal, logo_tex);
    }
    glLoadTileSet(dotImg, 32, 32, 32, 32, GL_RGB8_A5, TEXTURE_SIZE_32, TEXTURE_SIZE_32,
                  TEXGEN_OFF, 8, white8, dot_tex);
    glLoadTileSet(gbCartImg, 128, 128, 128, 128, GL_RGB256, TEXTURE_SIZE_128, TEXTURE_SIZE_128,
                  TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT, 256, gbcart_pal, gbcart_tex);
    glLoadTileSet(dsCartImg, 128, 128, 128, 128, GL_RGB256, TEXTURE_SIZE_128, TEXTURE_SIZE_128,
                  TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT, 256, dscart_pal, dscart_tex);
    glLoadTileSet(cartImg, 256, 128, 256, 128, GL_RGB256, TEXTURE_SIZE_256, TEXTURE_SIZE_128,
                  TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT, 256, cart_pal, cart_tex);
}

// ------------------------------------------------------------------ 3B kartus
typedef struct { float x, y, z, ry, rx, rz, s; } Pose;

// Kartus turune gore geometri. Doku pikseli -> yerel birim: 64 px = 1, doku merkezi = (0, 0)
typedef struct {
    const glImage *shell;
    float texW;                         // doku genisligi (piksel)
    float bx0, by0, bx1, by1;           // kabugun dokudaki kutusu (piksel) - sadece bu kisim cizilir
    float lx0, ly0, lx1, ly1, bottom;   // etiket ve alt kenar (yerel birim)
    u16 tint; int lw; int clear;
} CartGeo;

static u16 mulColor(u16 a, u16 b)
{
    return RGB15((a & 31) * (b & 31) / 31, ((a >> 5) & 31) * ((b >> 5) & 31) / 31, ((a >> 10) & 31) * ((b >> 10) & 31) / 31);
}

static CartGeo cartGeo(const Game *g)
{
    CartGeo c;
    if (g->type == T_NDS) {                       // Nintendo DS oyun karti
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
    if (g->type == T_GBA) {
        c.shell = cartImg;
        c.texW = CART_TEX_W;
        c.bx0 = CART_BODY_X0; c.by0 = CART_BODY_Y0; c.bx1 = CART_BODY_X1; c.by1 = CART_BODY_Y1;
        c.lx0 = (CART_LABEL_X - CART_TEX_W / 2) / 64.0f; c.lx1 = c.lx0 + CART_LABEL_W / 64.0f;
        c.ly0 = 1 - CART_LABEL_Y / 64.0f;  c.ly1 = c.ly0 - CART_LABEL_H / 64.0f;
        c.bottom = 1 - CART_BODY_Y1 / 64.0f;
        c.tint = C_WHITE;
        c.lw = CART_LABEL_W;
        c.clear = 0;
    } else {
        c.shell = gbCartImg;     // tum Game Boy / Color oyunlari ayni duzgun kabuk (renk oyuna gore)
        c.texW = 128;
        c.bx0 = GB_BODY_X0; c.by0 = GB_BODY_Y0; c.bx1 = GB_BODY_X1; c.by1 = GB_BODY_Y1;
        c.lx0 = -1 + GB_LABEL_X / 64.0f; c.lx1 = c.lx0 + GB_LABEL_W / 64.0f;
        c.ly0 = 1 - GB_LABEL_Y / 64.0f;  c.ly1 = c.ly0 - GB_LABEL_H / 64.0f;
        c.bottom = 1 - GB_BOTTOM / 64.0f;
        c.tint = g->shell;      // plastik rengi (oyuna gore)
        c.clear = g->clear;
        c.lw = GB_LABEL_W;
    }
    return c;
}
#define THICK 0.16f                     // kartus kalinligi

// Dokusuz renkli dortgen (yerel koordinat)
static void quadColor(float x0, float yT, float x1, float yB, float z, u16 cT, u16 cB)
{
    glBindTexture(0, 0);
    glBegin(GL_QUADS);
    glColor(cT); glVertex3v16(floattov16(x0), floattov16(yT), floattov16(z));
    glColor(cB); glVertex3v16(floattov16(x0), floattov16(yB), floattov16(z));
    glVertex3v16(floattov16(x1), floattov16(yB), floattov16(z));
    glColor(cT); glVertex3v16(floattov16(x1), floattov16(yT), floattov16(z));
    glEnd();
}

// Dort koseli serbest yuzey (kalinlik kenarlari)
static void face(float ax, float ay, float az, float bx, float by, float bz,
                 float cx, float cy, float cz, float dx, float dy, float dz, u16 c1, u16 c2)
{
    glBindTexture(0, 0);
    glBegin(GL_QUADS);
    glColor(c1); glVertex3v16(floattov16(ax), floattov16(ay), floattov16(az));
    glVertex3v16(floattov16(bx), floattov16(by), floattov16(bz));
    glColor(c2); glVertex3v16(floattov16(cx), floattov16(cy), floattov16(cz));
    glVertex3v16(floattov16(dx), floattov16(dy), floattov16(dz));
    glEnd();
}

// Yansima: zemin cizgisinde 'top' renginde baslar, REFL_LEN kadar asagida zemin rengine erir
#define REFL_LEN 0.55f
static u16 reflBg = RGB15(2, 2, 5);
static u16 reflAt(u16 top, float y, float f)
{
    return mixColor(top, reflBg, clampf((f - y) / REFL_LEN, 0, 1));
}

// Dokunun bir dikdortgen parcasini cizer (u/v: dokuda 0..1)
static void quadUV(const glImage *img, float x0, float yT, float x1, float yB, float z,
                   u16 cT, u16 cB, float fu0, float fv0, float fu1, float fv1)
{
    int u0 = img->u_off + (int)(img->width * fu0 + 0.5f), u1 = img->u_off + (int)(img->width * fu1 + 0.5f);
    int v0 = img->v_off + (int)(img->height * fv0 + 0.5f), v1 = img->v_off + (int)(img->height * fv1 + 0.5f);
    v16 X0 = floattov16(x0), X1 = floattov16(x1), YT = floattov16(yT), YB = floattov16(yB), Z = floattov16(z);
    glBindTexture(0, img->textureID);
    glBegin(GL_QUADS);
    glColor(cT); glTexCoord2t16(inttot16(u0), inttot16(v0)); glVertex3v16(X0, YT, Z);
    glColor(cB); glTexCoord2t16(inttot16(u0), inttot16(v1)); glVertex3v16(X0, YB, Z);
    glTexCoord2t16(inttot16(u1), inttot16(v1)); glVertex3v16(X1, YB, Z);
    glColor(cT); glTexCoord2t16(inttot16(u1), inttot16(v0)); glVertex3v16(X1, YT, Z);
    glEnd();
}
static void quad3part(const glImage *img, float x0, float yT, float x1, float yB, float z,
                      u16 cT, u16 cB, float t0, float t1)
{
    quadUV(img, x0, yT, x1, yB, z, cT, cB, 0, t0, 1, t1);
}

// Kabugun sadece govde kutusunu ciz (bos/saydam doku alanlari cizilmez -> DS'in satir butcesi korunur)
static void shellQuad(const CartGeo *g, float z, u16 cT, u16 cB)
{
    float L = (g->bx0 - g->texW / 2) / 64.0f, R = (g->bx1 - g->texW / 2) / 64.0f;
    float T = 1 - g->by0 / 64.0f, Bm = 1 - g->by1 / 64.0f;
    quadUV(g->shell, L, T, R, Bm, z, cT, cB, g->bx0 / g->texW, g->by0 / 128.0f, g->bx1 / g->texW, g->by1 / 128.0f);
}

static void cart3D(const Game *g, Pose p, u16 col, int a, float shine, int refl)
{
    if (a <= 0) return;
    glPushMatrix();
    glTranslatef32(floattof32(p.x), floattof32(p.y), floattof32(p.z));
    if (p.rz != 0) glRotateZi(deg(p.rz));
    glRotateYi(deg(p.ry));
    if (p.rx != 0) glRotateXi(deg(p.rx));
    glScalef32(floattof32(p.s), floattof32(p.s), floattof32(p.s));

    CartGeo geo = cartGeo(g);
    float floorY = geo.bottom;
    u16 shellCol = mulColor(col, geo.tint);
    // isik: kartus yana dondukce on yuz kararir, kenarlar parlar
    float c = fabsf(cosf_(p.ry * 0.0174533f));
    u16 front = mixColor(col, C_BLACK, (1 - c) * 0.45f);              // etiket
    u16 sFront = mixColor(shellCol, C_BLACK, (1 - c) * 0.45f);        // kabuk on yuz
    u16 frontB = sFront;
    int clr = geo.clear;                                               // seffaf plastik
    float L = (geo.bx0 - geo.texW / 2) / 64.0f + 0.03f, R = (geo.bx1 - geo.texW / 2) / 64.0f - 0.03f;
    float T = 1 - geo.by0 / 64.0f - 0.02f, Bm = 1 - geo.by1 / 64.0f + 0.02f;

    // arka yuz + dort ince kenar (eskiden 7 tam katman: gercek DS'te satir tasmasina yol aciyordu)
    if (a >= 31) {
        u16 back = mixColor(shellCol, C_BLACK, 0.55f);
        u16 sideA = mixColor(shellCol, C_BLACK, 0.35f + 0.25f * c);
        u16 sideB = mixColor(sideA, C_BLACK, 0.35f);
        int sa = clr ? 9 : 31;
        if (c < 0.5f) {
            alpha(sa);
            shellQuad(&geo, -THICK, back, back);
        }
        alpha(sa);
        face(L, T, 0, L, Bm, 0, L, Bm, -THICK, L, T, -THICK, sideA, sideB);
        alpha(sa);
        face(R, T, 0, R, Bm, 0, R, Bm, -THICK, R, T, -THICK, sideA, sideB);
        // ust kenar yuzu cizilmez: kartusun ust hatti basamakli/cikintili, duz bir yuzey ustunde havada kaliyordu
        alpha(sa);
        face(L, Bm, 0, R, Bm, 0, R, Bm, -THICK, L, Bm, -THICK, sideB, sideB);
    }
    alpha(clr ? a * 14 / 31 : a);
    shellQuad(&geo, 0, sFront, frontB);

    float lx0 = geo.lx0, lx1 = geo.lx1, ly0 = geo.ly0, ly1 = geo.ly1;
    alpha(a);
    if (g->label >= 0) quad3(labelImg[g->label], lx0, ly0, lx1, ly1, 0.006f, front, front, 0);
    else quadColor(lx0, ly0, lx1, ly1, 0.006f, mixColor(g->c1, C_BLACK, (1 - c) * 0.5f), mixColor(g->c2, C_BLACK, (1 - c) * 0.5f));

    // egik isik seridi (parlak etiket hissi)
    if (0 && shine > 0 && shine < 1 && a >= 31) {
        float w = lx1 - lx0;
        float sx = lx0 - 0.25f + (w + 0.3f) * shine;
        int sa = (int)(10 * sinf_(shine * 3.14159f));
        if (sa > 0) {
            alpha(sa);
            quad3skew(dotImg, sx - 0.10f, ly0, sx + 0.10f, ly1, 0.22f, 0.012f, C_WHITE);
        }
    }

    // zemindeki kisa yansima (arka plan rengine erir)
    if (refl > 0) {
        u16 rt = mixColor(col, C_BLACK, 0.30f);
        u16 rs = mixColor(shellCol, C_BLACK, 0.30f);
        float f = floorY, yEnd = f - REFL_LEN;
        float L0 = (geo.bx0 - geo.texW / 2) / 64.0f, R0 = (geo.bx1 - geo.texW / 2) / 64.0f;
        // kabuk: zemin satirindan yukari dogru REFL_LEN kadarlik dilim, ters
        alpha(clr ? refl / 2 : refl);
        quadUV(geo.shell, L0, f, R0, yEnd, 0, rs, reflBg, geo.bx0 / geo.texW, (1 - f) / 2,
               geo.bx1 / geo.texW, (1 - f - REFL_LEN) / 2);
        float yT = 2 * f - ly1, yB = 2 * f - ly0;
        if (yB < yEnd) yB = yEnd;
        if (yT > yB) {
            float tB = (yB - 2 * f + ly0) / (ly0 - ly1);   // 1 = etiketin alt satiri
            alpha(refl);
            if (g->label >= 0)
                quad3part(labelImg[g->label], lx0, yT, lx1, yB, 0.006f, reflAt(rt, yT, f), reflAt(rt, yB, f), 1, tB);
            else
                quadColor(lx0, yT, lx1, yB, 0.006f, reflAt(g->c2, yT, f), reflAt(g->c1, yB, f));
        }
    }
    glPopMatrix(1);
}

// Kaydirma seridindeki bir yuvanin duzeni (off: ortadan uzaklik, kesirli olabilir)
static Pose slotPose(float off)
{
    Pose p;
    float ao = fabsf(off), sg = off < 0 ? -1.0f : 1.0f;
    float near = clampf(ao, 0, 1), far = clampf(ao - 1, 0, 4);
    p.x = sg * (near * 2.05f + far * 0.85f);
    p.z = -CAM_D - near * 1.45f - far * 0.60f;
    p.y = CART_Y - near * 0.06f;
    p.ry = -sg * near * 62.0f;
    p.rx = 0;
    p.rz = 0;
    p.s = CART_S;
    return p;
}

// ------------------------------------------------------------------ arka plan (koyu, oyunun rengine burunen)

static float floorSy = 108;               // ortadaki kartusun zemin cizgisi (ekran y), animasyonlu
#define FLOOR_SY ((int)floorSy)

static float floorFor(const Game *g)
{
    float b = g->cover >= 0 ? -(g->coverH / 128.0f) : cartGeo(g).bottom;
    return 96 - (CART_Y + b * CART_S) * CAM_F * 96 / CAM_D;
}

static void dust(float fade)
{
    // yavasca yukselen, parildayan toz parcaciklari (3 derinlik katmani)
    for (int i = 0; i < 12; i++) {
        u32 h = (u32)i * 2654435761u;
        float depth = 0.35f + (h >> 28) / 16.0f * 0.65f;             // 0.35..1
        float sp = 0.10f + 0.25f * depth;
        float x = (float)((h >> 8) & 255);
        float y = 192 - fmodf(((h >> 16) & 255) * 0.75f + frame * sp, 200.0f);
        x += sinf_(frame / 90.0f + i) * 6 * depth;
        int tw = (int)((5 + 6 * depth) * (0.6f + 0.4f * sinf_(frame / 23.0f + i * 1.7f)) * fade);
        float s = 0.05f + 0.07f * depth;
        sprite(dotImg, x, y, s, s, mixColor(C_WHITE, vivid(tintCur, 0.8f), 0.4f), tw);
    }
}

// fade: ortam isiginin gucu (0 = bos uzay), glow: kartusun arkasindaki isik
static void backdrop(float fade, float glow, float gx, float gy)
{
    u16 t = vivid(tintCur, 0.80f);
    float T = frame / 60.0f;
    u16 top = mixColor(C_BG0, t, 0.08f * fade);
    u16 hor = mixColor(RGB15(1, 1, 4), t, 0.28f * fade);
    reflBg = mixColor(hor, C_BG1, 0.35f);
    u16 bot = mixColor(C_BG1, t, 0.05f * fade);
    alpha(31);
    glBoxFilledGradient(0, 0, 255, FLOOR_SY, top, hor, hor, top);
    alpha(31);
    glBoxFilledGradient(0, FLOOR_SY, 255, 191, hor, bot, bot, hor);

    // aurora: yavasca suzulen buyuk renk bulutlari
    (void)T;

    // kartusun arkasindaki isik + zemine dusen isik havuzu
    if (glow > 0) {
        sprite(dotImg, gx, gy - 4, 6.0f, 3.6f, t, (int)(11 * glow));
        sprite(dotImg, gx, FLOOR_SY + 2, 6.0f, 0.6f, mixColor(t, C_WHITE, 0.3f), (int)(12 * glow));
    }
    // ufuk cizgisi: ortada parlak, kenarlarda kaybolur
    if (fade > 0) {
        u16 hc = mixColor(t, C_WHITE, 0.35f);
        alpha((int)(14 * fade));
        glBoxFilledGradient(8, FLOOR_SY, 127, FLOOR_SY, hor, hor, hc, hc);
        alpha((int)(14 * fade));
        glBoxFilledGradient(128, FLOOR_SY, 247, FLOOR_SY, hc, hc, hor, hor);
    }
    dust(fade);
}

// Noktalardan halka (buyuk olceklerde bile puruzsuz)
static void dotRing(float cx, float cy, float rx, float ry, float s, u16 col, int a, float rot)
{
    if (a <= 0) return;
    int n = 40;
    for (int i = 0; i < n; i++) {
        float an = i * 6.2831853f / n + rot;
        sprite(dotImg, cx + cosf_(an) * rx, cy + sinf_(an) * ry, s, s, col, a);
    }
}

// ------------------------------------------------------------------ acilis animasyonu
// 0.00-1.38  bos uzay: yuzlerce parcacik girdap gibi merkeze akar, ustten isik huzmeleri iner
// 1.38-1.70  cekirdek patlar: flas, iki sok dalgasi, yatay (anamorfik) lens parlamasi
// 1.40-2.70  "GAME BOY micro" logosu 3B, kalinligiyla uzaktan donerek gelir
// 2.75-3.40  logonun uzerinden isik gecer, alt yazi harf aralikli belirir
// 3.55-4.00  logo kameraya dogru akip kaybolur -> kartuslar sirayla gelir (menu)
#define N_P 140

static void logo3D(float z, float y, float ry, float rx, float rz, float s, int a, float shine)
{
    if (a <= 0) return;
    glPushMatrix();
    glTranslatef32(0, floattof32(y), floattof32(z));
    if (rz != 0) glRotateZi(deg(rz));
    glRotateYi(deg(ry));
    if (rx != 0) glRotateXi(deg(rx));
    glScalef32(floattof32(s), floattof32(s), floattof32(s));
    // ekstruzyon: arkaya dogru koyulasan mavi katmanlar
    for (int k = 6; k >= 1; k--) {
        alpha(a);
        u16 c = mixColor(RGB15(8, 14, 31), C_BLACK, k * 0.11f);
        quad3(logoW, -2, 0.5f, 2, -0.5f, -k * 0.035f, c, mixColor(c, C_BLACK, 0.3f), 0);
    }
    alpha(a);
    quad3(logoW, -2, 0.5f, 2, -0.5f, 0, C_WHITE, RGB15(24, 27, 31), 0);
    if (shine > 0 && shine < 1) {
        float sx = -2.4f + 4.8f * shine;
        alpha((int)(a * 0.6f * sinf_(shine * 3.14159f)));
        quad3skew(dotImg, sx - 0.18f, 0.55f, sx + 0.18f, -0.55f, 0.3f, 0.01f, C_WHITE);
    }
    glPopMatrix(1);
}

static void (*bootTick)(void) = NULL;  // acilista yapilacak yukleme adimi (bitince NULL)
static u16 bootDoneTint = 0;
static void bootAnimation(void)
{
    static float pa[N_P], pr[N_P], pw[N_P];
    static u16 pc[N_P];
    u32 rnd = 0x9E3779B9;
    for (int i = 0; i < N_P; i++) {
        rnd = rnd * 1103515245 + 12345;
        pa[i] = (rnd >> 8 & 0xFFFF) / 65536.0f * 6.2831853f;
        rnd = rnd * 1103515245 + 12345;
        pr[i] = 90 + (rnd >> 8 & 0xFFFF) / 65536.0f * 150;
        rnd = rnd * 1103515245 + 12345;
        pw[i] = 1.6f + (rnd >> 8 & 0xFFFF) / 65536.0f * 1.8f;
        int k = (rnd >> 20) & 3;
        pc[i] = k == 0 ? C_VIOLET : (k == 1 ? C_WHITE : C_CYAN);
    }
    u16 gameTint = tintCur;
    tintCur = RGB15(8, 13, 30);        // acilista notr mavi ortam isigi
    const int LEN = 240;               // 4 sn
    for (int f = 0; f < LEN; f++) {
        scanKeys();
        if (bootTick) bootTick();      // kart okuma: karede bir kucuk adim (animasyon akarken)
        if (!bootTick && f > 15 && (keysDown() & (KEY_A | KEY_B | KEY_START | KEY_TOUCH))) break;
        float t = f / 60.0f;

        // ---- logo konumu (3B)
        float c = span(t, 1.40f, 2.70f);
        float k = easeOut5(c);
        float out = easeIn(span(t, 3.55f, 4.0f));
        float lz = lerpf(-26.0f, -CAM_D, k) + out * 1.6f;
        float ly = lerpf(-0.1f, 0.22f, easeOut(c)) + out * 0.25f;
        float lry = (1 - k) * 540, lrx = (1 - k) * -22, lrz = (1 - k) * 16;
        float gx, gy;
        project(0, ly, lz, &gx, &gy);

        // ---- 1) arka plan
        beginBack();
        float amb = easeInOut(span(t, 1.5f, 2.9f));
        backdrop(amb, amb * (1 - out), 128, gy + 10);

        float rays = span(t, 0.15f, 0.8f) * (1 - span(t, 2.3f, 3.3f));
        if (rays > 0) {
            float sw = sinf_(t * 0.9f) * 22;
            u16 rc = C_CYAN;
            tri(128 + (int)sw, -12, 40 + (int)sw * 2, 192, 120 + (int)sw * 2, 192, rc, C_BG0, C_BG0, (int)(9 * rays));
            tri(128 - (int)sw, -12, 136 - (int)sw * 2, 192, 216 - (int)sw * 2, 192, mixColor(rc, C_VIOLET, 0.5f), C_BG0, C_BG0, (int)(7 * rays));
            tri(128, -12, 104, 192, 152, 192, C_WHITE, C_BG0, C_BG0, (int)(6 * rays));
        }

        for (int i = 0; i < N_P; i++) {
            float x, y, x2, y2;
            int a;
            float s = 0.07f + (i & 3) * 0.025f;
            if (t < 1.38f) {
                float q = clampf(t / 1.38f, 0, 1), q2 = clampf(t / 1.38f - 0.045f, 0, 1);
                float r = pr[i] * (1 - q * q), r2 = pr[i] * (1 - q2 * q2);
                float an = pa[i] + q * q * pw[i], an2 = pa[i] + q2 * q2 * pw[i];
                x = 128 + cosf_(an) * r; y = 92 + sinf_(an) * r * 0.78f;
                x2 = 128 + cosf_(an2) * r2; y2 = 92 + sinf_(an2) * r2 * 0.78f;
                a = (int)(26 * span(t, 0.0f, 0.5f) * (0.5f + 0.5f * (1 - r / pr[i])));
            } else {
                float e = t - 1.38f;
                float rb = 30 + pr[i] * 1.1f;
                float r = rb * easeOut(e / 1.3f), r2 = rb * easeOut((e - 0.05f) / 1.3f);
                float an = pa[i] + pw[i] + e * 0.5f;
                x = 128 + cosf_(an) * r; y = 92 + sinf_(an) * r * 0.78f;
                x2 = 128 + cosf_(an) * r2; y2 = 92 + sinf_(an) * r2 * 0.78f;
                a = (int)(28 * (1 - span(e, 0.25f, 1.4f)));
            }
            if (a <= 0) continue;
            sprite(dotImg, x2, y2, s * 0.8f, s * 0.8f, pc[i], a / 2);
            sprite(dotImg, x, y, s, s, pc[i], a);
        }

        float core = easeIn(span(t, 0.45f, 1.40f)) * (1 - span(t, 1.40f, 1.75f));
        if (core > 0) {
            sprite(dotImg, 128, 92, 1 + 4.5f * core, 1 + 4.0f * core, C_CYAN, (int)(18 * core));
            sprite(dotImg, 128, 92, 0.4f + 1.4f * core, 0.4f + 1.4f * core, C_WHITE, (int)(30 * core));
        }
        // logonun arkasindaki genis parlama
        if (c > 0) sprite(dotImg, gx, gy, 3 + 5 * k, 1.5f + 2.2f * k, C_ACCENT, (int)(13 * span(c, 0, 0.3f) * (1 - out)));
        end2D();

        // ---- 2) 3B logo
        begin3D();
        if (c > 0) logo3D(lz, ly, lry, lrx, lrz, 0.82f, (int)(31 * (1 - out)), span(t, 2.75f, 3.40f));

        // ---- 3) on katman
        beginFront();
        float fl = (t >= 1.38f) ? 1 - span(t, 1.38f, 1.68f) : 0;
        if (fl > 0) box(0, 0, 255, 191, C_WHITE, (int)(30 * fl * fl));
        float sw1 = span(t, 1.38f, 2.15f), sw2 = span(t, 1.50f, 2.35f);
        if (sw1 > 0 && sw1 < 1) {
            float r = easeOut(sw1);
            dotRing(128, 92, 8 + r * 170, 6 + r * 130, 0.30f + 0.2f * r, C_CYAN, (int)(26 * (1 - r)), r * 0.6f);
        }
        if (sw2 > 0 && sw2 < 1) {
            float r = easeOut(sw2);
            dotRing(128, 92, 6 + r * 110, 5 + r * 85, 0.18f + 0.1f * r, C_WHITE, (int)(24 * (1 - r)), -r * 0.4f);
        }
        float lf = (t >= 1.36f) ? 1 - span(t, 1.36f, 2.3f) : span(t, 1.1f, 1.36f);
        if (lf > 0) {
            sprite(dotImg, 128, 92, 16 * (0.5f + 0.5f * lf), 0.30f, C_CYAN, (int)(22 * lf));
            sprite(dotImg, 128, 92, 10 * (0.5f + 0.5f * lf), 0.10f, C_WHITE, (int)(30 * lf));
            for (int g = 1; g <= 3; g++) {
                float d = g * 0.45f;
                sprite(dotImg, 128 + 70 * d, 92 + 40 * d, 0.5f * g, 0.5f * g, g == 2 ? C_VIOLET : C_CYAN, (int)(8 * lf));
            }
        }
        // alt yazi
        float st = span(t, 2.95f, 3.35f) * (1 - span(t, 3.5f, 3.8f));
        if (st > 0) {
            int lw = (int)(74 * easeOut(span(t, 2.95f, 3.35f)));
            int yy = (int)gy + 30;
            box(128 - lw, yy, 128 + lw, yy, C_ACCENT, (int)(31 * st));
            drawTextSp(0, 128, yy + 5 + (int)(6 * (1 - st)), "DUAL SCREEN EDITION", C_MUTED, (int)(31 * st), 1, 2);
        }
        end2D();
        endFrame();
    }
    while (bootTick) bootTick();      // animasyon atlandiysa yuklemeyi bitir
    tintCur = bootDoneTint ? bootDoneTint : gameTint;
}


// ================================================================== alt ekran: yazilimla cizilen 16 bit tuval
static u16 fbMain[256 * 192] __attribute__((aligned(4)));   // ekrandaki tuval
static u16 *fb = fbMain;                                      // cizim hedefi
static u16 *subVram;
static int subDirty = 1;
static int liftLv = 0;                                      // ekstra parlaklik: goruntu beyaza dogru acilir (0..16)
static int subBright = 0, subBrightSet = 0;                 // ust ekran parlaklik (0..16 karartma), VBlank'te yazilir
static int clipY0 = 0, clipY1 = 191;

static inline u16 blend15(u16 d, u16 c, int a)
{
    // uc kanal tek carpimla: R 0-4, B 10-14, G 21-25 (aralarda tasma payi)
    u32 A = a + (a >> 4);                           // 0..31 -> 0..32
    u32 x = (d | ((u32)d << 16)) & 0x03E07C1F;
    u32 y = (c | ((u32)c << 16)) & 0x03E07C1F;
    u32 r = ((x * (32 - A) + y * A) >> 5) & 0x03E07C1F;
    return (u16)(r | (r >> 16)) | BIT(15);
}

static void fbFill(int x0, int y0, int x1, int y1, u16 c)
{
    if (x0 < 0) x0 = 0;
    if (x1 > 255) x1 = 255;
    if (y0 < clipY0) y0 = clipY0;
    if (y1 > clipY1) y1 = clipY1;
    if (x0 > x1 || y0 > y1) return;
    c |= BIT(15);
    u32 cc = c | ((u32)c << 16);
    for (int y = y0; y <= y1; y++) {
        u16 *row = fb + y * 256;
        int x = x0;
        if (x & 1) row[x++] = c;                     // 4 bayt hizasina gel
        int count = x1 - x + 1;
        u32 *w = (u32 *)(row + x);
        for (int n = count >> 1; n > 0; n--) *w++ = cc;
        if (count & 1) row[x1] = c;
    }
}

static void fbBlend(int x0, int y0, int x1, int y1, u16 c, int a)
{
    if (a <= 0) return;
    if (a >= 31) { fbFill(x0, y0, x1, y1, c); return; }
    if (x0 < 0) x0 = 0;
    if (x1 > 255) x1 = 255;
    if (y0 < clipY0) y0 = clipY0;
    if (y1 > clipY1) y1 = clipY1;
    u32 A = a + (a >> 4), IA = 32 - A;
    u32 cy = ((c | ((u32)c << 16)) & 0x03E07C1F) * A;
    for (int y = y0; y <= y1; y++) {
        u16 *row = fb + y * 256;
        for (int x = x0; x <= x1; x++) {
            u32 d = row[x];
            u32 v = (((((d | (d << 16)) & 0x03E07C1F) * IA + cy) >> 5) & 0x03E07C1F);
            row[x] = (u16)(v | (v >> 16)) | BIT(15);
        }
    }
}

// Yuvarlak koseli dikdortgen (yumusak kenarli)
static u8 insetTab[13][13][2];      // [yaricap][kenardan satir] = {tam piksel, kesir 0..31}
static void initInsetTab(void)
{
    for (int r = 1; r <= 12; r++)
        for (int k = 0; k < r; k++) {
            float dy = r - k - 0.5f;
            float in = r - sqrtf(clampf(r * r - dy * dy, 0, r * r));
            insetTab[r][k][0] = (u8)in;
            insetTab[r][k][1] = (u8)((in - (int)in) * 31);
        }
}

static void fbRound(int x0, int y0, int x1, int y1, int r, u16 c, int a)
{
    if (r > 12) r = 12;
    if (r * 2 > y1 - y0) r = (y1 - y0) / 2;
    for (int y = y0; y <= y1; y++) {
        if (y < clipY0 || y > clipY1) continue;
        int k = -1;
        if (y < y0 + r) k = y - y0;
        else if (y > y1 - r) k = y1 - y;
        int in = 0, fr = 0;
        if (k >= 0 && r > 0) { in = insetTab[r][k][0]; fr = insetTab[r][k][1]; }
        int xi0 = x0 + in, xi1 = x1 - in;
        if (xi0 > xi1) continue;
        int ea = (a * (31 - fr)) >> 5;            // kenar yumusatma
        fbBlend(xi0, y, xi0, y, c, ea);
        fbBlend(xi1, y, xi1, y, c, ea);
        if (xi1 - xi0 > 1) fbBlend(xi0 + 1, y, xi1 - 1, y, c, a);
    }
}

static int fbTextW(int large, const char *s) { return textWidth(large, s); }

static void fbText(int large, int x, int y, const char *s, u16 color, int a, int align)
{
    const u8 *tex = large ? font_l_tex : font_s_tex;
    const u16 *crd = large ? font_l_coords : font_s_coords;
    const u8 *adv = large ? font_l_adv : font_s_adv;
    if (align == 1) x -= fbTextW(large, s) / 2;
    else if (align == 2) x -= fbTextW(large, s);
    while (*s) {
        int gi = glyphIndex(nextCodepoint(&s));
        int gx = crd[gi * 4], gy = crd[gi * 4 + 1], gw = crd[gi * 4 + 2], gh = crd[gi * 4 + 3];
        if (gi != 0)
            for (int j = 0; j < gh; j++) {
                int yy = y + j;
                if (yy < clipY0 || yy > clipY1) continue;
                for (int i = 0; i < gw; i++) {
                    int xx = x + i;
                    if (xx < 0 || xx > 255) continue;
                    int al = tex[(gy + j) * 256 + gx + i] >> 3;
                    if (al) fb[yy * 256 + xx] = blend15(fb[yy * 256 + xx], color, (al * a) >> 5);
                }
            }
        x += adv[gi];
    }
}

static void fbIcon(int x, int y, const u16 *icon)
{
    for (int j = 0; j < 32; j++) {
        int yy = y + j;
        if (yy < clipY0 || yy > clipY1) continue;
        for (int i = 0; i < 32; i++) {
            u16 c = icon[j * 32 + i];
            if ((c & BIT(15)) && x + i >= 0 && x + i < 256) fb[yy * 256 + x + i] = c;
        }
    }
}

static void subPresent(void)
{
    DC_FlushRange(fbMain, sizeof(fbMain));
    dmaCopyWords(3, fbMain, subVram, sizeof(fbMain));
}

// Kare sonu: ust ekrani isle, VBlank'te alt ekrani aktar
static void endFrame2(void)
{
    glFlush(GL_TRANS_MANUALSORT);
    swiWaitForVBlank();
    if (subDirty) { subPresent(); subDirty = 0; }
    if (subBrightSet) {                             // gecis karartmasi oncelikli, yoksa ekstra parlaklik
        REG_MASTER_BRIGHT_SUB = subBright ? (2 << 14) | subBright : (liftLv ? (1 << 14) | liftLv : 0);
        REG_MASTER_BRIGHT = liftLv ? (1 << 14) | liftLv : 0;
        subBrightSet = 0;
    }
    frame++;
}

// ================================================================== durum
// Alt ekran (dokunmatik, 3B): kartuslar. Ust ekran (2B tuval): secili oyunun vitrini.
static int curTab = TAB_NDS;
static int car[TAB_N][MAX_GAMES];
static int carN[TAB_N];
static int sel[TAB_N];

static void buildLists(void)
{
    memset(carN, 0, sizeof(carN));
    for (int i = 0; i < gameCount; i++)
        if (!games[i].sep) car[games[i].tab][carN[games[i].tab]++] = i;
    memset(sel, 0, sizeof(sel));
}
static Game *gameAt(int t, int k) { return &games[car[t][k]]; }
static Game *curGame(void) { return carN[curTab] ? gameAt(curTab, sel[curTab]) : NULL; }
static const char *sysName2(int type)
{
    return type == T_NDS ? "Nintendo DS" : (type == T_GBA ? "Game Boy Advance" : (type == T_GBC ? "Game Boy Color" : "Game Boy"));
}
static const char *tabLong[TAB_N] = { "Nintendo DS oyunları", "Game Boy Advance oyunları", "Game Boy oyunları" };
static const char *tabName[TAB_N] = { "NDS", "GBA", "GB" };

// ================================================================== alt ekran: 3B kartuslar (dokunmatik)
static void flow(float pos, float introF, float explode, int skipCenter, int bump)
{
    int n = carN[curTab];
    if (!n) return;
    int center = (int)(pos + 0.5f);
    for (int d = 3; d >= 0; d--) {
        for (int side = -1; side <= 1; side += 2) {
            if (d == 0 && side == 1) continue;
            int i = center + d * side;
            if (i < 0 || i >= n) continue;
            float off = i - pos, ao = fabsf(off);
            if (ao > 2.7f) continue;
            if (skipCenter && i == center) continue;
            Pose p = slotPose(off);
            float near = clampf(ao, 0, 1), far = clampf(ao - 1, 0, 3);
            u16 col = mixColor(C_WHITE, C_BLACK, near * 0.40f + far * 0.16f);
            int a = 31;
            if (far > 0.8f) a = (int)(31 * clampf(1 - (far - 0.8f) / 0.9f, 0, 1));
            int refl = (int)(9 - near * 3 - far * 2);
            float sg = off < 0 ? -1.0f : 1.0f;
            float c = clampf(1 - ao * 2, 0, 1);
            if (c > 0) {
                p.ry += sinf_(frame / 110.0f) * 5 * c;
                p.rx += sinf_(frame / 83.0f) * 2.5f * c;
                p.y += sinf_(frame / 52.0f) * 0.03f * c;
                p.s *= 1 + 0.05f * sinf_(bump / 12.0f * 3.14159f) * c;
            }
            float ik = easeOut5(span(introF, ao * 5, 30 + ao * 5));
            if (ik < 1) {
                p.x += (1 - ik) * 4.2f;
                p.z -= (1 - ik) * 1.2f;
                p.ry -= (1 - ik) * 110;
                a = (int)(a * clampf(ik * 2.5f, 0, 1));
            }
            if (explode > 0 && ao > 0.3f) {
                p.x += sg * explode * 3.0f;
                p.z -= explode * 1.5f;
                a = (int)(a * (1 - explode));
            }
            cart3D(gameAt(curTab, i), p, col, a, -1, a >= 31 ? refl : 0);
        }
    }
}

// alt ekranin ust kenari: bolum dugmeleri (dokunulur)
static void tabRect(int t, int *x0, int *x1)
{
    int w = 60, sx = 128 - (w * TAB_N) / 2;
    *x0 = sx + t * w;
    *x1 = *x0 + w - 1;
}
static void bottomOverlay(float a)
{
    if (a <= 0) return;
    int A = (int)(31 * a);
    u16 t = vivid(tintCur, 0.85f);
    int sx0, sx1, ex0, ex1;
    tabRect(0, &sx0, &sx1);
    tabRect(TAB_N - 1, &ex0, &ex1);
    roundBox(sx0 - 3, 3, ex1 + 3, 21, RGB15(4, 5, 8), (int)(26 * a));
    for (int k = 0; k < TAB_N; k++) {
        int x0, x1;
        tabRect(k, &x0, &x1);
        char lbl[16];
        snprintf(lbl, sizeof(lbl), "%s %d", tabName[k], tabCount[k]);
        if (k == curTab) {
            pill(x0 + 2, 5, x1 - 2, 19, mixColor(t, C_BLACK, 0.3f), A);
            drawText(0, (x0 + x1) / 2, 4, lbl, C_WHITE, A, 1);
        } else drawText(0, (x0 + x1) / 2, 4, lbl, C_MUTED, A, 1);
    }
    // alt ipucu
    drawText(0, 128, 177, carN[curTab] ? "Kaydır: seç   •   Ortadakine dokun: oyna" : "Bu bölümde oyun yok",
             C_MUTED, (int)(A * 0.8f), 1);
}

// ================================================================== ust ekran: vitrin (yazilimla cizilir)
static u16 page[256 * 192];          // yeni sayfa (gecis icin)
static u16 prevPage[256 * 192];      // eski sayfa

static void fbBlitMask(int x, int y, int w, int h, const u16 *src)
{
    for (int j = 0; j < h; j++) {
        int yy = y + j;
        if (yy < clipY0 || yy > clipY1) continue;
        u16 *d = fb + yy * 256;
        const u16 *s = src + j * w;
        for (int i = 0; i < w; i++) {
            int xx = x + i;
            if (xx >= 0 && xx < 256 && (s[i] & BIT(15))) d[xx] = s[i];
        }
    }
}

// kelimelere gore satira bol
static int wrapText(int large, const char *s, char lines[3][64], int maxW, int maxLines)
{
    int n = 0;
    char buf[80];
    strncpy(buf, s, 79);
    buf[79] = 0;
    char *p = buf;
    while (*p && n < maxLines) {
        char line[64] = "";
        while (*p) {
            char *sp = strchr(p, ' ');
            int len = sp ? sp - p : (int)strlen(p);
            char cand[96];
            snprintf(cand, sizeof(cand), "%s%s%.*s", line, line[0] ? " " : "", len, p);
            if (textWidth(large, cand) > maxW && line[0]) break;
            strncpy(line, cand, 63);
            p += len;
            while (*p == ' ') p++;
        }
        if (!line[0]) break;
        if (n == maxLines - 1 && *p) {
            char tmp[160];
            snprintf(tmp, sizeof(tmp), "%s %s", line, p);
            fitText(large, line, tmp, maxW);
            p += strlen(p);
        }
        strcpy(lines[n++], line);
    }
    return n;
}

static void drawStatusTop(void)
{
    static const char *gun[7] = { "Paz", "Pzt", "Sal", "Çar", "Per", "Cum", "Cmt" };
    static const char *ay[12] = { "Oca", "Şub", "Mar", "Nis", "May", "Haz", "Tem", "Ağu", "Eyl", "Eki", "Kas", "Ara" };
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char clk[8], dt[24];
    snprintf(clk, sizeof(clk), "%02d:%02d", t->tm_hour, t->tm_min);
    snprintf(dt, sizeof(dt), "%s %d %s", gun[t->tm_wday % 7], t->tm_mday, ay[t->tm_mon % 12]);
    fbText(0, 8, 2, "Nintendo", C_TEXT, 31, 0);
    fbText(0, 8 + fbTextW(0, "Nintendo") + 3, 2, "DS", C_ACCENT, 31, 0);
    // pil: DS yalnizca yuksek/dusuk bildirir
    static u32 bat = 0;
    static int lowSince = -1;
    bat = getBatteryLevel() | 0x100;
    int lvl = bat & BATTERY_LEVEL_MASK, chg = (bat & BATTERY_CHARGER_CONNECTED) != 0;
    if (lvl <= 3 && !chg) { if (lowSince < 0) lowSince = frame; } else lowSince = -1;
    int stage = lvl >= 11 ? 3 : (lvl >= 7 ? 2 : ((lvl >= 3 && (lowSince < 0 || frame - lowSince < 36000)) ? 1 : 0));
    if (chg) stage = 3;
    static const u16 fillCol[4] = { RGB15(17, 2, 5), RGB15(31, 8, 6), RGB15(31, 25, 4), RGB15(11, 27, 13) };
    static const int fillW[4] = { 3, 6, 11, 16 };
    int x = 228, y = 5;
    fbRound(x, y, x + 19, y + 9, 2, C_MUTED, 31);
    fbFill(x + 1, y + 1, x + 18, y + 8, RGB15(1, 1, 3));
    fbFill(x + 20, y + 3, x + 21, y + 6, C_MUTED);
    fbFill(x + 2, y + 2, x + 1 + fillW[stage], y + 7, fillCol[stage]);
    int bx = x - 6;
    fbText(0, bx, 2, clk, C_TEXT, 31, 2);
    bx -= fbTextW(0, clk) + 6;
    fbText(0, bx, 2, dt, C_MUTED, 31, 2);
}

// vitrin sayfasini 'fb' uzerine ciz
static void composeStage(const Game *g, int st)
{
    u16 t = vivid(g ? g->tint : tintCur, 0.85f);
    if (st == 0) {
    // arka plan: kapagin bulanik hali (128x96 -> 2 kat), yoksa koyu renk gecisi
    if (g && g->bg) {
        for (int y = clipY0; y <= clipY1; y++) {          // 2 kat buyutme, 32 bit yazim
            const u16 *s = g->bg + (y >> 1) * 128;
            u32 *d = (u32 *)(fb + y * 256);
            for (int x = 0; x < 128; x++) { u32 c = s[x]; d[x] = c | (c << 16); }
        }
    } else {
        for (int y = 0; y < 192; y++) fbFill(0, y, 255, y, mixColor(mixColor(RGB15(3, 3, 6), t, 0.2f), RGB15(0, 0, 1), y / 191.0f));
    }
    // okunabilirlik: ust ve alt kenarlar koyulasir (bulanik arka planda yuklemede bir kez yapildi)
    if (!(g && g->bg)) {
        for (int y = 0; y < 18; y++) fbBlend(0, y, 255, y, RGB15(0, 0, 1), 20 - y);
        for (int y = 124; y < 192; y++) fbBlend(0, y, 255, y, RGB15(0, 0, 1), (y - 124) * 22 / 68);
    }
    drawStatusTop();
    if (!g) fbText(1, 128, 80, "Bu bölümde oyun yok", C_TEXT, 31, 1);
    return;
    }
    if (!g) return;
    if (st == 1) {
    // oyunun simgesi (konsol ana menusundeki uygulama simgesi gibi): cam kare icinde 2 kat buyutulmus
    int ix = 14, iy = 30;
    if (g->tile) {                                    // GBA/GB: etiketin tamami, yatay cam cerceve
        int tw = g->tileW, th = g->tileH;
        int bx = ix, by = iy + (72 - (th + 8)) / 2, bx1 = bx + tw + 8, by1 = by + th + 8;
        fbRound(bx + 3, by + 4, bx1 + 3, by1 + 4, 10, C_BLACK, 12);          // golge
        fbRound(bx, by, bx1, by1, 10, C_WHITE, 9);                           // cam
        fbRound(bx + 1, by + 1, bx1 - 1, by + 2, 2, C_WHITE, 14);            // ust isik
        for (int y = 0; y < th; y++) {
            int yy = by + 4 + y;
            if (yy < clipY0 || yy > clipY1) continue;
            u16 *d = fb + yy * 256 + bx + 4;
            const u16 *sr = g->tile + y * tw;
            for (int x = 0; x < tw; x++) { u16 c = sr[x]; if (c & BIT(15)) d[x] = c; }
        }
        return;
    }
    fbRound(ix + 3, iy + 4, ix + 75, iy + 76, 12, C_BLACK, 12);              // golge
    fbRound(ix, iy, ix + 72, iy + 72, 12, C_WHITE, 9);                       // cam
    fbRound(ix + 1, iy + 1, ix + 71, iy + 2, 2, C_WHITE, 14);                // ust isik
    if (0) {
    } else if (g->icon) {
        for (int y = 0; y < 64; y++) {
            if (iy + 4 + y < clipY0 || iy + 4 + y > clipY1) continue;
            u16 *d = fb + (iy + 4 + y) * 256 + ix + 4;
            const u16 *sr = g->icon + (y >> 1) * 32;
            for (int x = 0; x < 64; x++) { u16 c = sr[x >> 1]; if (c & BIT(15)) d[x] = c; }
        }
    }
    return;
    }
    // sag: sistem rozeti, iri baslik, yayinci, kayit
    int cx = 98;
    if (st == 2) {
    const char *sys = sysName2(g->type);
    int w1 = fbTextW(0, sys) + 14;
    fbRound(cx, 28, cx + w1, 41, 6, mixColor(t, C_BLACK, 0.25f), 31);
    fbText(0, cx + w1 / 2, 27, sys, C_WHITE, 31, 1);
    }
    char lines[3][64];
    int nl = wrapText(1, g->title, lines, 150, 3);
    int ty = 46;
    for (int i = 0; i < nl; i++, ty += 21) {
        if (st != 2) continue;
        fbText(1, cx + 1, ty + 1, lines[i], C_BLACK, 18, 0);
        fbText(1, cx, ty, lines[i], C_WHITE, 31, 0);
    }
    if (st != 3) return;
    if (g->sub[0]) {
        char pub[48];
        fitText(0, pub, g->sub, 150);
        fbText(0, cx, ty + 3, pub, RGB15(24, 25, 28), 31, 0);
        ty += 18;
    }
    if (g->hasSave) {
        fbRound(cx, ty + 7, cx + 6, ty + 13, 3, RGB15(11, 27, 13), 31);
        fbText(0, cx + 10, ty + 2, "Kayıt var", RGB15(22, 24, 26), 31, 0);
    }
    // alt bant: bolum, sira, kumanda ipuclari
    char cnt[48];
    snprintf(cnt, sizeof(cnt), "%s  •  %d / %d", tabLong[curTab], sel[curTab] + 1, carN[curTab]);
    fbText(0, 12, 134, cnt, RGB15(22, 23, 26), 31, 0);
    int n = carN[curTab];
    int dots = n > 20 ? 20 : n;
    int sd = n > 20 ? sel[curTab] * 19 / (n - 1) : sel[curTab];
    for (int i = 0; i < dots; i++) {
        int x = 12 + i * 8;
        if (i == sd) fbRound(x, 154, x + 10, 158, 2, t, 31);
        else fbRound(x + (i > sd ? 4 : 0), 155, x + (i > sd ? 6 : 2), 157, 1, RGB15(14, 15, 18), 31);
    }
    // ipuclari
    int y = 170;
    fbRound(12, y, 25, y + 13, 6, C_WHITE, 31);
    fbText(0, 19, y - 1, "A", C_INK, 31, 1);
    fbText(0, 30, y - 1, "Oyna", C_TEXT, 31, 0);
    fbRound(74, y, 103, y + 13, 6, RGB15(8, 9, 12), 31);
    fbText(0, 89, y - 1, "L R", C_TEXT, 31, 1);
    fbText(0, 108, y - 1, "Bölüm", RGB15(20, 21, 24), 31, 0);
    fbRound(160, y, 202, y + 13, 6, RGB15(8, 9, 12), 31);
    fbText(0, 181, y - 1, "START", C_TEXT, 31, 1);
    fbText(0, 207, y - 1, "YSMenu", RGB15(20, 21, 24), 31, 0);
}

static void composePage(const Game *g)
{
    clipY0 = 0; clipY1 = 191;
    for (int st = 0; st < 4; st++) composeStage(g, st);
}

// sayfa gecisi: eski sayfadan yeniye yumusak karisim
static int fadeStep = 8;
#ifdef MACRO_PERF
static u32 pcomp = 0, pcopy = 0;
#endif
#ifdef MACRO_PERF
static u32 pst[6];
u32 perfLoop;
#endif
static const Game *compG = NULL;
static int compSt = -1;                                     // -1: cizim yok, 0..7: siradaki yarim asama
static int compQuiet = 0;                                   // saat guncellemesi: gecissiz
static void startPage(const Game *g)
{
    loadBg((Game *)g);
    compG = g; compSt = 0; compQuiet = 0;
    if (fadeStep < 8) { fadeStep = 8; subBright = 0; subBrightSet = 1; }   // yarim kalan gecis: parlakligi geri al
}
static void stepCompose(void)
{
    if (compSt < 0) return;
    fb = page;
#ifdef MACRO_PERF
    u32 t0 = cpuGetTiming();
#endif
    clipY0 = (compSt & 1) ? 96 : 0; clipY1 = (compSt & 1) ? 191 : 95;
    composeStage(compG, compSt >> 1);
    clipY0 = 0; clipY1 = 191;
#ifdef MACRO_PERF
    u32 dt = cpuGetTiming() - t0; if (dt > pst[compSt >> 1]) pst[compSt >> 1] = dt;
#endif
    fb = fbMain;
    if (++compSt < 8) return;
    compSt = -1;
    if (compQuiet) { memcpy(fbMain, page, sizeof(fbMain)); subDirty = 1; return; }
    fadeStep = 0;
}
static void startPageOld(const Game *g)
{
#ifdef MACRO_PERF
    u32 t0 = cpuGetTiming();
#endif
    memcpy(prevPage, fb, sizeof(fb));
#ifdef MACRO_PERF
    u32 t1 = cpuGetTiming();
#endif
    composePage(g);
#ifdef MACRO_PERF
    pcomp = cpuGetTiming() - t1; pcopy = t1 - t0;
#endif
    memcpy(page, fb, sizeof(fb));
    memcpy(fb, prevPage, sizeof(fb));
    fadeStep = 0;
}
static void stepPage(void)
{
    static const u8 lv[9] = { 0, 3, 6, 9, 11, 8, 5, 2, 0 };
    if (fadeStep >= 8) return;
    fadeStep++;
    if (fadeStep == 4) { memcpy(fbMain, page, sizeof(fbMain)); subDirty = 1; }   // en koyu anda takas
    subBright = lv[fadeStep];
    subBrightSet = 1;
}

// ================================================================== baslatma
static char toastMsg[64] = "";
static int toastT = 0;
static void setToast(const char *m, int frames)
{
    strncpy(toastMsg, m, sizeof(toastMsg) - 1);
    toastMsg[sizeof(toastMsg) - 1] = 0;
    toastT = frames;
}
static void drawToast(void)
{
    if (toastT <= 0) return;
    float a = clampf(toastT > 150 ? 1 : toastT / 20.0f, 0, 1);
    char l1[64], *l2;
    strcpy(l1, toastMsg);
    l2 = strchr(l1, '\n');
    if (l2) *l2++ = 0;
    int w = textWidth(0, l1);
    if (l2 && textWidth(0, l2) > w) w = textWidth(0, l2);
    w = w / 2 + 14;
    int y0 = 28, y1 = l2 ? 62 : 46;
    roundBox(128 - w, y0, 128 + w, y1, RGB15(4, 5, 8), (int)(28 * a));
    drawText(0, 128, y0 + 2, l1, C_TEXT, (int)(31 * a), 1);
    if (l2) drawText(0, 128, y0 + 18, l2, C_MUTED, (int)(31 * a), 1);
    toastT--;
}

// TTMenu/YSMenu.ini: AUTO_BOOT degerini YERINDE degistir (dosya boyu degismez; FAT'a yazilmaz)
static int setAutoBoot(const char *value)
{
    FILE *f = fopen(YSMENU_INI, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(n + 1), *nb = malloc(n + 1);
    if (!buf || !nb || fread(buf, 1, n, f) != (size_t)n) { fclose(f); free(buf); free(nb); return 0; }
    fclose(f);
    buf[n] = 0;
    char *hit = NULL, *p = buf;
    while ((p = strstr(p, "AUTO_BOOT=")) != NULL) {
        if (p == buf || p[-1] == '\n') { hit = p; break; }
        p++;
    }
    char *pad = strstr(buf, "\n;MACRO_PAD");
    if (!hit || !pad || pad < hit) { free(buf); free(nb); return 0; }
    char *hend = hit;
    while (*hend && *hend != '\r' && *hend != '\n') hend++;
    pad++;
    char *pend = pad;
    while (*pend && *pend != '\r' && *pend != '\n') pend++;
    long fixed = (hit - buf) + 10 + (long)strlen(value) + (pad - hend) + 10 + (n - (pend - buf));
    long spaces = n - fixed;
    if (spaces < 0) { free(buf); free(nb); return 0; }
    char *o = nb;
    memcpy(o, buf, hit - buf); o += hit - buf;
    memcpy(o, "AUTO_BOOT=", 10); o += 10;
    memcpy(o, value, strlen(value)); o += strlen(value);
    memcpy(o, hend, pad - hend); o += pad - hend;
    memcpy(o, ";MACRO_PAD", 10); o += 10;
    memset(o, ' ', spaces); o += spaces;
    memcpy(o, pend, n - (pend - buf)); o += n - (pend - buf);
    int ok = 0;
    if (o - nb == n) {
        f = fopen(YSMENU_INI, "r+b");
        if (f) { ok = fwrite(nb, 1, n, f) == (size_t)n; fclose(f); }
    }
    free(buf);
    free(nb);
    return ok;
}

static void restoreLast(void)
{
    FILE *f = fopen(YSMENU_INI, "rb");
    if (!f) return;
    char line[300];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "AUTO_BOOT=/01 - Games/", 22)) continue;
        line[strcspn(line, "\r\n")] = 0;
        for (int k = 0; k < carN[TAB_NDS]; k++)
            if (!strcmp(gameAt(TAB_NDS, k)->file, line + 22)) { sel[TAB_NDS] = k; curTab = TAB_NDS; }
        break;
    }
    fclose(f);
}

static void topMessage(const char *l1, const char *l2)
{
    clipY0 = 0; clipY1 = 191;
    fbBlend(0, 0, 255, 191, RGB15(0, 0, 1), 20);
    fbRound(24, 70, 231, 124, 10, RGB15(4, 5, 8), 29);
    fbText(1, 128, 76, l1, C_TEXT, 31, 1);
    if (l2) {
        char s[80];
        fitText(0, s, l2, 196);
        fbText(0, 128, 102, s, C_MUTED, 31, 1);
    }
    subDirty = 1;
}

// kartus one alinir, sonra yuvaya takilir; ekran kararir
static void launchAnim(const Game *g, float pos)
{
    u16 t = vivid(tintCur, 0.85f);
    topMessage("Başlatılıyor", g->title);
    for (int f = 0; f <= 50; f++) {
        float tt = f / 60.0f;
        float k1 = easeInOut(span(tt, 0.0f, 0.30f));
        float k2 = easeIn(span(tt, 0.26f, 0.58f));
        float ex = easeOut(span(tt, 0.0f, 0.35f));
        float black = span(tt, 0.66f, 0.83f);
        Pose p = slotPose(0);
        p.z += 0.55f * k1;
        p.y += 0.18f * k1 - 2.9f * k2;
        p.rx = -12 * k1 - 55 * k2;
        float gx, gy;
        project(p.x, p.y, p.z, &gx, &gy);
        beginBack();
        backdrop(1, 1 - k2, gx, gy);
        end2D();
        begin3D();
        flow(pos, 999, ex, 1, 0);
        cart3D(g, p, C_WHITE, 31, -1, (int)(9 * (1 - k1)));
        beginFront();
        float burst = span(tt, 0.52f, 0.60f) * (1 - span(tt, 0.60f, 0.83f));
        if (burst > 0) {
            sprite(dotImg, 128, 191, 14, 0.9f, t, (int)(24 * burst));
            sprite(dotImg, 128, 191, 9, 0.25f, C_WHITE, (int)(30 * burst));
        }
        if (black > 0) box(0, 0, 255, 191, C_BLACK, (int)(31 * black));
        end2D();
        endFrame2();
    }
}

static void launchGame(Game *g, float pos)
{
    char rom[256];
    gamePath(g, rom, sizeof(rom));
    if (g->type == T_NDS) {
        if (access(YSMENU_KERNEL, F_OK) != 0) { setToast("YSMenu çekirdeği bulunamadı\n_macro/ysmenu.dat", 220); return; }
        if (!setAutoBoot(rom + 4)) { setToast("YSMenu.ini güncellenemedi", 200); return; }
        launchAnim(g, pos);
        const char *argv[1] = { YSMENU_KERNEL };
        REG_MASTER_BRIGHT = 0; REG_MASTER_BRIGHT_SUB = 0;   // ekstra parlaklik oyuna tasinmasin
        int err = runNdsFile(YSMENU_KERNEL, 1, argv, 0);
        char msg[64];
        snprintf(msg, sizeof(msg), "YSMenu açılamadı (hata %d)", err);
        setToast(msg, 240);
        return;
    }
    const char *emu = g->type == T_GBA ? GBAR2_A7 : GAMEYOB;
    if (access(emu, F_OK) != 0) { setToast("Emülatör bulunamadı", 180); return; }
    launchAnim(g, pos);
    const char *argv[2] = { emu, rom };
    REG_MASTER_BRIGHT = 0; REG_MASTER_BRIGHT_SUB = 0;   // ekstra parlaklik oyuna tasinmasin
    int err = runNdsFile(emu, 2, argv, 0);
    char msg[64];
    snprintf(msg, sizeof(msg), "Açılamadı (hata %d)", err);
    setToast(msg, 240);
}

static void launchYSMenu(void)
{
    if (access(YSMENU_KERNEL, F_OK) != 0) { setToast("YSMenu çekirdeği bulunamadı", 200); return; }
    if (!setAutoBoot("/01 - Games/")) { setToast("YSMenu.ini güncellenemedi", 200); return; }
    topMessage("YSMenu açılıyor", NULL);
    for (int f = 0; f <= 16; f++) {
        beginFront();
        box(0, 0, 255, 191, C_BLACK, f * 2);
        end2D();
        endFrame2();
    }
    const char *argv[1] = { YSMENU_KERNEL };
    REG_MASTER_BRIGHT = 0; REG_MASTER_BRIGHT_SUB = 0;   // ekstra parlaklik oyuna tasinmasin
    int err = runNdsFile(YSMENU_KERNEL, 1, argv, 0);
    char msg[48];
    snprintf(msg, sizeof(msg), "YSMenu açılamadı (hata %d)", err);
    setToast(msg, 220);
}

// ================================================================== ana menu
static void menu(void)
{
    float pos = sel[curTab];
    int intro = 0, bump = 0, bright = 4;
    int dragging = 0, tx0 = 0, moved = 0;
    float pos0 = 0;
    int lastSel = -2, lastTab = -1, lastMin = -1;
    keysSetRepeat(18, 6);
#ifdef MACRO_AUTOTEST
    int autoWait = 220;
#endif
    while (1) {
#ifdef MACRO_PERF
        { extern u32 perfLoop; perfLoop = cpuGetTiming(); }
#endif
        scanKeys();
        int down = keysDown(), held = keysHeld(), up = keysUp();
        int rep = keysDownRepeat();
#ifdef MACRO_AUTOTEST
        if (autoWait > 0 && --autoWait == 150) rep |= KEY_RIGHT;
        if (autoWait == 60) down |= KEY_R;
#endif
        int n = carN[curTab];
        if (intro > 20) {
            if (n) {
                if ((rep & (KEY_RIGHT | KEY_DOWN)) && sel[curTab] < n - 1) { sel[curTab]++; bump = 12; }
                if ((rep & (KEY_LEFT | KEY_UP)) && sel[curTab] > 0) { sel[curTab]--; bump = 12; }
            }
            if (down & KEY_R) curTab = (curTab + 1) % TAB_N;
            if (down & KEY_L) curTab = (curTab + TAB_N - 1) % TAB_N;
            touchPosition tp;
            if (down & KEY_TOUCH) {
                touchRead(&tp);
                if (tp.py < 24) {                          // bolum dugmeleri
                    for (int k = 0; k < TAB_N; k++) {
                        int x0, x1;
                        tabRect(k, &x0, &x1);
                        if (tp.px >= x0 && tp.px <= x1) curTab = k;
                    }
                } else if (n) { dragging = 1; tx0 = tp.px; pos0 = pos; moved = 0; }
            }
            if (dragging && (held & KEY_TOUCH)) {          // parmakla kaydirma
                touchRead(&tp);
                int dx = tp.px - tx0;
                if (abs(dx) > 5) moved = 1;
                if (moved) pos = clampf(pos0 - dx / 72.0f, -0.4f, n - 0.6f);
            }
            if (dragging && (up & KEY_TOUCH)) {
                dragging = 0;
                if (moved) {
                    int k = (int)(pos + 0.5f);
                    sel[curTab] = k < 0 ? 0 : (k >= n ? n - 1 : k);
                } else {                                    // dokunma: ortadaki -> oyna, yandaki -> sec
                    if (tx0 > 82 && tx0 < 174) down |= KEY_A;
                    else if (tx0 <= 82 && sel[curTab] > 0) { sel[curTab]--; bump = 12; }
                    else if (tx0 >= 174 && sel[curTab] < n - 1) { sel[curTab]++; bump = 12; }
                }
            }
            if ((down & KEY_A) && curGame()) {
                launchGame(curGame(), pos);
                intro = 0;
                lastSel = -2;
            }
            if (down & KEY_START) { launchYSMenu(); lastSel = -2; }
            if (down & KEY_SELECT) {
                bright = bright % 7 + 1;                    // 1-5 arka isik, 6-7: tam isik + goruntu acma
                systemSetBacklightLevel(bright > 5 ? 5 : bright);
                liftLv = bright == 6 ? 3 : (bright == 7 ? 6 : 0);
                subBrightSet = 1;
                char m[40];
                if (bright > 5) snprintf(m, sizeof(m), "Parlaklık %d / 7  (ekstra)", bright);
                else snprintf(m, sizeof(m), "Parlaklık %d / 7", bright);
                setToast(m, 90);
            }
        }
        if (curTab != lastTab && lastTab >= 0) { pos = sel[curTab]; intro = 0; }

        Game *g = curGame();
        n = carN[curTab];
        if (g) tintCur = mixColor(tintCur, g->tint, 0.10f);
        if (!dragging) {
            pos += (sel[curTab] - pos) * 0.20f;
            if (fabsf(sel[curTab] - pos) < 0.002f) pos = sel[curTab];
        } else if (n) {                                    // surukleme sirasinda vitrin de en yakin oyunu gostersin
            int k = (int)(pos + 0.5f);
            k = k < 0 ? 0 : (k >= n ? n - 1 : k);
            if (k != sel[curTab]) sel[curTab] = k;
        }
        if (bump > 0) bump--;
        if (intro < 90) intro++;
        if (g) floorSy += (floorFor(g) - floorSy) * 0.15f;

        // ust ekran: secim degisince yumusak gecisle yeni sayfa; dakikada bir saat
        time_t now = time(NULL);
        int minute = (int)(now / 60);
        static int settle = 0, seenSel = -3;
        if (sel[curTab] != seenSel) { seenSel = sel[curTab]; settle = dragging ? 6 : 2; }
        if (settle > 0) settle--;
        if ((sel[curTab] != lastSel || curTab != lastTab) && (settle == 0 || curTab != lastTab)) {
            startPage(g);
            lastSel = sel[curTab];
            lastTab = curTab;
            lastMin = minute;
        } else if (minute != lastMin && fadeStep >= 8 && compSt < 0) {
            startPage(g);
            compQuiet = 1;
            lastMin = minute;
        }
#ifdef MACRO_PERF
        u32 tc0 = cpuGetTiming();
#endif
        if (compSt >= 0) stepCompose(); else stepPage();
        // bosta: siradaki arka plani yukle (secilene en yakin once; kaydirma/gecis sirasinda yapilmaz)
        if (compSt < 0 && fadeStep >= 8 && !dragging && fabsf(sel[curTab] - pos) < 0.01f) {
            static int bgDone = 0;
            if (!bgDone) {
                Game *nx = NULL;
                int nn = carN[curTab];
                for (int dd = 1; dd < nn && !nx; dd++)
                    for (int sg = -1; sg <= 1 && !nx; sg += 2) {
                        int k = sel[curTab] + dd * sg;
                        if (k >= 0 && k < nn && !gameAt(curTab, k)->bgTried) nx = gameAt(curTab, k);
                    }
                for (int t2 = 0; t2 < TAB_N && !nx; t2++)
                    for (int k = 0; k < carN[t2] && !nx; k++)
                        if (!gameAt(t2, k)->bgTried) nx = gameAt(t2, k);
                if (nx) loadBg(nx); else bgDone = 1;
            }
        }
#ifdef MACRO_PERF
        u32 tc1 = cpuGetTiming();
#endif

        Pose cp = slotPose(0);
        float gx, gy;
        project(cp.x, cp.y, cp.z, &gx, &gy);
        float ui = easeOut(span(intro, 10, 40));
#ifdef MACRO_PERF
        static u32 pmax = 0, pshow = 0, tPrev = 0;
        u32 tNow = cpuGetTiming();
        if (tPrev && tNow - tPrev > pmax) pmax = tNow - tPrev;
        tPrev = tNow;
        if ((frame % 30) == 0) { pshow = pmax; pmax = 0; }
#endif
        beginBack();
        backdrop(1, g ? 1 : 0, gx, gy);
        end2D();
        begin3D();
        flow(pos, intro, 0, 0, bump);
        beginFront();
        bottomOverlay(ui);
        drawToast();
#ifdef MACRO_PERF
        { static u32 bmax = 0, bshow = 0, tLoop = 0; u32 te = cpuGetTiming();
          extern u32 perfLoop; tLoop = perfLoop; u32 busy = (te - tLoop) - (tc1 - tc0); if (tLoop && busy > bmax) bmax = busy;
          if ((frame % 30) == 0) { bshow = bmax; bmax = 0; }
          pst[5] = bshow; }
        { char pm[40]; snprintf(pm, sizeof(pm), "m%d %d %d %d %d f%d", (int)(pshow / 33514), (int)(pst[0] / 33514), (int)(pst[1] / 33514), (int)(pst[2] / 33514), (int)(pst[3] / 33514), (int)(pst[4] / 33514)); snprintf(pm + strlen(pm), 8, " b%d", (int)(pst[5] / 33514));
          drawText(0, 128, 30, pm, C_WHITE, 31, 1); }
#endif
        end2D();
        endFrame2();
    }
}

// ================================================================== giris
// acilis yukleme adimlari: her cagri bir kucuk is (animasyon karesi basina)
static bool fatOk = false;
static void bootWork(void)
{
    static int step = 0, pi = 0;
    switch (step) {
    case 0:
        fatOk = fatInitDefault();               // menu karta hicbir dosya olusturmaz/silmez
        if (!fatOk) { bootTick = NULL; return; }
        scanGames();                            // yalniz dosya adlari (hizli)
        step = 1;
        break;
    case 1:                                     // karede 2 oyun: kayit / baslik / simge
        for (int k = 0; k < 2 && pi < gameCount; k++) probeGame(&games[pi++]);
        if (pi >= gameCount) step = 2;
        break;
    case 2:                                     // karede 2 etiket
        if (loadLabels(2)) step = 3;
        break;
    default: {
        makeNdsLabels();
        buildLists();
        restoreLast();
        if (!carN[curTab])
            for (int k = 0; k < TAB_N; k++) if (carN[k]) { curTab = k; break; }
        Game *g = curGame();
        if (g) { bootDoneTint = g->tint; floorSy = floorFor(g); }
        loadBg(g);                              // yalniz ilk gorunen oyunun arka plani; digerleri menu bosken
        bootTick = NULL;
    }
    }
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    powerOn(PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM);
    BG_PALETTE[0] = 0; BG_PALETTE_SUB[0] = 0;   // yukleyiciden kalan beyaz zemin hemen siyah
    lcdMainOnBottom();                      // 3B kartuslar alt (dokunmatik) ekranda
    videoSetMode(MODE_0_3D);
    videoSetModeSub(MODE_5_2D);             // ust ekran: vitrin tuvali
    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_TEXTURE);
    vramSetBankC(VRAM_C_SUB_BG);
    vramSetBankD(VRAM_D_TEXTURE);
    vramSetBankE(VRAM_E_TEX_PALETTE);
    initInsetTab();
#ifdef MACRO_PERF
    cpuStartTiming(0);
#endif
    int bg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    subVram = bgGetGfxPtr(bg);
    clipY0 = 0; clipY1 = 191;
    fbFill(0, 0, 255, 191, RGB15(0, 0, 1));
    subPresent();

    glScreen2D();
    glClearColor(0, 0, 1, 31);
    glClearPolyID(63);
    glClearDepth(GL_MAX_DEPTH);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    loadTextures();

    bootTick = bootWork;                    // kart okuma animasyon karelerine dagitilir (beyaz bekleme yok)
    bootAnimation();
    if (!fatOk) {
        while (1) {
            beginBack();
            backdrop(1, 0, 128, 80);
            end2D();
            beginFront();
            drawTextShadow(1, 128, 76, "Kart okunamadı", C_TEXT, 31, 1);
            drawText(0, 128, 102, "microSD kartı kontrol et", C_MUTED, 31, 1);
            end2D();
            endFrame2();
        }
    }
    menu();
    return 0;
}
