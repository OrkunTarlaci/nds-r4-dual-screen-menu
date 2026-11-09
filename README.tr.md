# Nintendo DS — Dual Screen Edition (R4 kartı için özel menü)

**[English](README.md) | Türkçe**

Klasik R4 türü flash kartlar için, iki ekranlı, 3B kartuş görünümlü bir oyun menüsü.
NDS oyunlarını kartın kendi yazılımıyla (YSMenu), GBA oyunlarını **GBARunner2**, GB/GBC oyunlarını **GameYob** ile açar.

---

## İçindekiler

1. [Neler var](#neler-var)
2. [Önemli uyarılar](#önemli-uyarılar)
3. [Claude ile kurulum (kullanıcı için)](#claude-ile-kurulum-kullanıcı-için)
4. [Claude için talimatlar](#claude-için-talimatlar)
5. [Nasıl çalışıyor (mimari)](#nasıl-çalışıyor-mimari)
6. [SD karttaki klasör yapısı ve dosya adları](#sd-karttaki-klasör-yapısı-ve-dosya-adları)
7. [Oyun ekleme ve resimler](#oyun-ekleme-ve-resimler)
8. [Menü dilini değiştirme](#menü-dilini-değiştirme)
9. [Derleme ortamı](#derleme-ortamı)
10. [melonDS ile test](#melonds-ile-test)
11. [Kontroller](#kontroller)
12. [Sorun giderme ve öğrendiklerimiz](#sorun-giderme-ve-öğrendiklerimiz)
13. [Kaynak dosyalar](#kaynak-dosyalar)
14. [Teşekkür ve lisanslar](#teşekkür-ve-lisanslar)

---

## Neler var

- **Alt ekran (dokunmatik):** 3B kartuş halkası. DS kartları gerçek DS oyun kartı gibi görünür.
  - Etiket gerçek kart fotoğrafından gelir: NINTENDO DS bandı, ESRB, Nintendo mührü, `NTR-XXXX-USA` kodu.
  - Fotoğrafı bulunamayan oyunlar için aynı düzende, kapak resminden kart üretilir.
  - GBA kartuşları gri GBA kabuğunda, GB kartuşları Game Boy kabuğunda durur.
  - Kartuşlar yerde yansır, seçilen kart hafifçe süzülür.
- **Üst ekran (vitrin):** Seçili oyunun simgesi, adı, yayıncısı ve kayıt durumu, arka planda kapağın bulanık hali.
  Oyun değişince yumuşak geçiş olur.
- **Bölümler:** `NDS`, `GBA`, `GB`. L/R tuşları veya dokunarak geçilir.
- **Açılış:** 4 saniyelik "Nintendo DS" animasyonu. Kart bu sırada okunur, beyaz bekleme ekranı yok.
- **Parlaklık:** 7 kademe (SELECT).
  - 1–5: donanım arka ışığı.
  - 6–7: arka ışık tam güçte, ayrıca görüntü yazılımla aydınlatılır (yalnızca menüde).
- **Son oyun:** Menü, en son açılan oyunda başlar. Bu bilgi YSMenu'nün kendi `AUTO_BOOT` satırından okunur; menü karta
  ayrı bir dosya yazmaz.
- **Performans:** Her kare 16,7 ms'nin altında, 60 FPS.
  - Üst ekran sayfası 8 parçaya bölünüp birkaç kareye yayılır.
  - Geçiş efektini ekranın parlaklık donanımı yapar.
  - Resimler menü boştayken arka planda yüklenir.

## Önemli uyarılar

- **Bu depoda oyun yok.** ROM, kayıt dosyası (`.sav`), kapak/kart resmi ve kartın orijinal yazılımı **bulunmaz ve
  eklenmemelidir.** Herkes kendi kartının yazılımını ve kendi yedeklediği oyunları kullanır.
- **SD karta yazmadan önce mutlaka tam yedek alın.** `tools/sd/raw_backup.py` sektör sektör yedek alır.
- **Ucuz/bozuk USB kart okuyucular SD kartı bozabilir.** Bu projede bir okuyucu, karta veri yerine kendi USB paketini
  ("USBS/USBC") yazıp bir klasörü ve dosya tablosunu bozdu. Sağlam bir okuyucu kullanın.
- Windows *"Bu sürücüyü tarayıp onarmak ister misiniz?"* diye sorarsa **onarmayın.** Onarım (chkdsk) kayıt dosyalarını
  silebilir. Önce yedek alın, sonra Claude'a inceletin.

---

## Claude ile kurulum (kullanıcı için)

1. Bilgisayara **Claude Code**'u kurun (masaüstü uygulaması veya terminal).
2. Bu depoyu indirin ve klasörü Claude Code'da açın.
3. SD kartı **sağlam bir kart okuyucuyla** bilgisayara takın.
4. Claude'a şunu yazın:
   > README.md'yi oku ve bu menüyü benim kartıma kur.
5. Claude size sorular soracak (aşağıda listelendi). Cevaplayın; gerisini o yapar, her adımda size gösterir ve
   onayınızı alır.

Hazırlamanız gerekenler:
- Kartın üzerinde yazan **tam model adı**. Bir fotoğraf da çekebilirsiniz.
- Kartın **şu anki çalışan yazılımı** (SD kartın kökündeki dosyalar). Kart zaten çalışıyorsa bunlar SD kartta vardır.
- Konsol modeli (DS, DS Lite, DSi, 3DS).
- Menünün hangi dilde olmasını istediğiniz.

---

## Claude için talimatlar

> Bu bölüm, bu depoyu açan Claude içindir. Kullanıcı bir şey yazmadıysa bile bu adımları izle.

### 0. Başlamadan önce kullanıcıya sor

Bunları tek tek, sade bir dille sor. Kullanıcı teknik olmayabilir; her sorunun neden önemli olduğunu bir cümleyle
açıkla. Cevapları bir yere not et.

1. **Menü dili.** Hangi dilde olsun: Türkçe, İngilizce, başka? Şu anki metinler Türkçe;
   [Menü dilini değiştirme](#menü-dilini-değiştirme) bölümüne bak.
2. **Konsol modeli.** Nintendo DS (ilk model), DS Lite, DSi veya 3DS?
   - Menü DS ve DS Lite için yazıldı.
   - DSi/3DS'te flash kart DS modunda çalışır. Parlaklık ve arka ışık farklı davranabilir; kurulum öncesi bunu belirt.
3. **Kartın modeli.** Kartın üzerinde tam olarak ne yazıyor? Fotoğraf iste. Örnek: "R4 Upgrade Revolution for DS
   (NDSL/NDS)", "R4i Gold 3DS", "R4i-SDHC", "Acekard 2i", "DSTT".
   - Etiket kartın gerçek donanımını göstermeyebilir; aynı etiketli klonlar farklı çekirdek kullanır.
   - SD kartın kökündeki dosyalara bakarak hangi yazılımı (çekirdeği) çalıştırdığını belirle:
     - `R4.dat` + `TTMenu/` + `TTMenu.dat` → YSMenu tabanlı.
     - `_wfwd/` → Wood R4.
     - `__rpg/` → AKAIO.
     - `_DS_MENU.DAT` → orijinal R4 çekirdeği.
4. **Kartın çalışan orijinal yazılımı var mı?**
   - Kart şu an normal menüsüyle açılıyor mu? Bu kurulum o yazılımın **üzerine** yapılır. NDS oyunları hâlâ o yazılımla
     açılır, menümüz onun önüne geçer.
   - Yoksa ya da kart boşsa: modele uygun çekirdeği kullanıcıyla birlikte bul (kart üreticisinin sitesi veya bilinen
     arşivler). Kullanıcının indirmesini ve onaylamasını iste.
5. **SD kart.** Boyutu kaç GB, dosya sistemi ne? FAT32 olmalı. Kartta kayıtlı oyun ilerlemesi var mı? Varsa önce yedek.
6. **Kart okuyucu.** Kart hangi okuyucuyla takılı? Bilinmeyen/ucuz bir okuyucuysa riski anlat.
   [Sorun giderme](#sorun-giderme-ve-öğrendiklerimiz) bölümüne bak.
7. **Oyunlar.** NDS, GBA, GB/GBC oyunları hangi klasörlerde? Kullanıcı numaralı düzen istiyor mu
   (`0001 - Oyun Adı.nds`)? Dosyaları yeniden adlandırma önerisini mutlaka onaya sun; `.sav` dosyalarını da birlikte
   adlandır.
8. **Görünüm tercihleri.** Mevcut tasarım (gerçek kart görünümü, koyu tema) tamam mı, istediği değişiklik var mı?

### 1. Güvenlik kuralları (her zaman)

- **İlk iş tam ham yedek:**
  ```
  set SD_DRIVE=F:
  python tools/sd/raw_backup.py kart_yedek.img
  ```
  Okuyucu takılırsa araç bekleyip yeniden dener. Yedek bitmeden karta yazma.
- ROM'lara ve `.sav` dosyalarına **asla** içerik yazma. Yalnızca, kullanıcı onaylarsa, yeniden adlandırma yap.
  Yeniden adlandırmayı iki aşamada yap (önce geçici ad), böylece ad çakışması olmaz.
- Büyük oyun dosyalarını gereksiz yere açma. Oyun kodunu ROM başlığının ilk 16 baytından oku (`0x0C`, 4 bayt).
- Karta yazdıktan sonra **Windows önbelleğini atlayarak** doğrula: `python tools/sd/verify_card.py <kart_yolu> <yerel_dosya> ...`.
  Bu araç dosya içeriğini, iki FAT kopyasının eşliğini, tüm dosya zincirlerini ve okuyucu paketi izlerini kontrol eder.
  Windows'un `Get-FileHash` sonucu önbellekten gelebilir, tek başına yetmez.
- Yazdıktan sonra `Write-VolumeCache -DriveLetter F` ile önbelleği boşalt ve kullanıcıya "Güvenle Kaldır" dedir.
- Karta yazmadan önce kullanıcıya ne yazacağını söyle. Kullanıcı açıkça "sormadan yaz" demediyse onay al.
- `chkdsk` veya Windows onarımı çalıştırma. Sorun varsa önce yedek al, yedeğin üzerinde incele.

### 2. Kartın açılış zincirini kur

Bu proje için seçilen yol (YSMenu tabanlı R4 klonu):

```
DS açılır → kart, SD kökündeki R4.dat'ı çalıştırır
          → R4.dat = TWiLight Menu++ "Autoboot" dosyası (kartın modeline uygun olanı)
          → fat:/_nds/TWiLightMenu/main.srldr çalışır = BU MENÜ
NDS oyunu seçilince:
          → menü, TTMenu/YSMenu.ini içindeki AUTO_BOOT satırını yerinde değiştirir
          → fat:/_macro/ysmenu.dat (kartın ORİJİNAL R4.dat dosyası = YSMenu çekirdeği) çalıştırılır
          → YSMenu, AUTO_BOOT'taki oyunu doğrudan açar
```

Adımlar:
1. Kartın **orijinal** kök `R4.dat` dosyasını `/_macro/ysmenu.dat` olarak kopyala. Bu, YSMenu çekirdeğinin kendisi.
   Orijinali ayrıca yedekle.
2. TWiLight Menu++ sürümündeki `Autoboot/<kart modeli>/` klasöründen uygun `R4.dat` dosyasını (ve varsa yanındaki
   dosyaları) SD köküne koy.
   - Bu projede `Autoboot/R4i-SDHC/` dosyaları çalıştı.
   - Farklı kartta önce TWiLight'ın "Flashcard users" belgelerinde kartı ara. Doğru dosyayı kullanıcıyla birlikte seç.
3. `TTMenu/YSMenu.ini` sonuna **bir kez** bu satırı ekle: `;MACRO_PAD` + 300 boşluk + CRLF.
   - Menü `AUTO_BOOT` değerini değiştirirken dosya boyunu sabit tutmak için fazlalığı bu satırdan alır. Böylece dosya
     yerinde (`r+b`) yazılır, dosya tablosuna (FAT) dokunulmaz.
   - `YSMenu.ini` içinde `AUTO_BOOT=` satırı yoksa önce kullanıcıyla birlikte ekle.
4. Menüyü derle ve `main.srldr` olarak kopyala: `/_nds/TWiLightMenu/main.srldr`.
5. GBA için `/_nds/GBARunner2_arm7dldi_ds.nds`. GBARunner2'nin DS (arm7 DLDI) sürümü, ayrıca GBA BIOS gerekir; bkz.
   GBARunner2 belgeleri.
6. GB/GBC için `/_nds/TWiLightMenu/emulators/gameyob.nds` (TWiLight Menu++ paketinden).

**Kart YSMenu tabanlı değilse:** NDS oyunlarını açma yolu farklı olur. Seçenekler:
- kartın kendi çekirdeğinin bir "otomatik başlat" ayarı;
- TWiLight Menu++'ın nds-bootstrap'i;
- çekirdeğin argv ile ROM yolunu kabul etmesi.

Kullanıcının kartında deneyerek belirle. `launchGame()` ve `setAutoBoot()` fonksiyonlarını buna göre uyarla.
Ayarlar `source/_part1.txt` başındaki `#define`'larda:

```c
#define NDS_DIR         "fat:/01 - Games"
#define GBA_DIR         "fat:/GBA/gba"
#define GB_DIR          "fat:/GB"
#define LABEL_DIR       "fat:/_macro/labels"
#define YSMENU_KERNEL   "fat:/_macro/ysmenu.dat"
#define YSMENU_INI      "fat:/TTMenu/YSMenu.ini"
#define GBAR2_A7        "fat:/_nds/GBARunner2_arm7dldi_ds.nds"
#define GAMEYOB         "fat:/_nds/TWiLightMenu/emulators/gameyob.nds"
```

### 3. Resimleri üret, derle, test et, kur

**Önce iki şablon resmi hazırla.** Telif nedeniyle depoda yoklar ama `make_assets.py` için şart:

| Dosya | Ne | Nereden |
|---|---|---|
| `tools/carts/CPUE.png` | DS kart kabuğu ve "NINTENDO DS" bandı şablonu (Pokémon Platinum kartı) | `https://art.gametdb.com/ds/cart/US/CPUE.png` |
| `tools/photos/gba_cart.png` | GBA kartuş kabuğu (etiket alanı kesilip atılır, herhangi bir GBA oyunu olur) | Kullanıcının kendi çektiği düz, önden fotoğraf veya kullanıcının onayladığı bir ürün fotoğrafı (~400×240, beyaz zemin) |

İndirmeden önce kullanıcıdan izin al.

Sırayla (Windows, proje kökünde; `F:` kullanıcının SD sürücüsü):

```
python tools/make_assets.py . "F:/GBA/gba;F:/GB"            # yazı tipleri, kabuklar, GBA/GB etiketleri -> source/assets.c/.h
python tools/nds_labels.py . "F:/01 - Games"                 # NDS kart etiketleri -> sdroot/_macro/labels
python tools/hero.py . "F:/01 - Games" "F:/GBA/gba" "F:/GB"  # üst ekran arka planları -> sdroot/_macro/hero
python tools/assemble_dual.py                                # source/*.txt parçaları -> source/main.c
wsl -d Ubuntu -u root -- bash /mnt/c/.../build.sh           # BlocksDS ile derle -> dualDS.nds
```

Sonra:
- melonDS'te test et ([melonDS ile test](#melonds-ile-test)), ekran görüntüsünü kullanıcıya göster, onay al.
- Kur:
  - `dualDS.nds` → `SD:/_nds/TWiLightMenu/main.srldr`
  - `sdroot/_macro/labels/*` → `SD:/_macro/labels/`
  - `sdroot/_macro/hero/*.bgb` → `SD:/_macro/hero/`
- `tools/sd/verify_card.py` ile doğrula.

### 4. Çalışma tarzı (bu projede iyi sonuç verdi)

- Kullanıcıya her görsel değişikliği **önce göster**: melonDS ekran görüntüsü veya önizleme PNG'si.
- Tasarımda kullanıcı beğenmezse 3–4 seçenek üret, yan yana göster, seçtir.
- Performansı ölç, tahmin etme: test derlemesinde `#define MACRO_PERF` kare sürelerini ekrana yazar.
- Karta sadece değişen dosyaları yaz. Her yazımdan sonra ham okuma ile doğrula.

---

## Nasıl çalışıyor (mimari)

| Parça | Görev |
|---|---|
| **Alt ekran** (ana motor, 3B) | Kartuş halkası: GL2D + videoGL. Her kartuş = kabuk dokusu + etiket dokusu. `lcdMainOnBottom()` ile 3B motor alt ekranda. |
| **Üst ekran** (alt motor, 16 bit bitmap) | Vitrin sayfası işlemcide çizilir (`fb[]`). Yeni sayfa arka tamponda 8 yarım aşamada çizilir, sonra donanım parlaklığıyla geçilir. |
| **NDS başlatma** | `YSMenu.ini` → `AUTO_BOOT` yerinde güncellenir, `ysmenu.dat` (kartın orijinal çekirdeği) `runNdsFile` ile çalışır. |
| **GBA / GB başlatma** | `runNdsFile(emülatör, {emülatör, rom})`: GBARunner2 / GameYob ROM yolunu argv'den alır. |
| **Açılış** | Ekranlar anında siyaha çekilir. Kart okuma (`bootWork`) açılış animasyonunun karelerine dağıtılır: dosya listesi → her karede 2 oyunun başlığı/simgesi → her karede 2 etiket. |
| **Bellek** | Dokular için VRAM A+B+D = 384 KB. Yazı tipleri ve kabuklar ~177 KB. NDS etiketi 64×64 (4 KB), GBA/GB etiketi 128×64 (8 KB). En fazla `MAX_LABELS` (48) etiket. |
| **Okunan dosyalar** | Menü karta **hiç yazmaz**. Tek istisna `YSMenu.ini` içindeki `AUTO_BOOT` değeri; o da aynı boyutta, yerinde yazılır. |

Dosya biçimleri:
- **`.lbl`:** 512 bayt palet (256 × RGB15) + 8192 bayt piksel (128×64, satır uzunluğu 128). NDS'de sol üst 64×64 kullanılır.
- **`.bgb`:** 128×96 RGB15 bulanık arka plan; ekranda 2 kat büyütülür.
- **`.hrb`:** Eski sürümden kalma büyük kapak; artık okunmuyor.

## SD karttaki klasör yapısı ve dosya adları

```
SD:/
├── R4.dat                       # TWiLight Autoboot (menüyü açar)
├── TTMenu.dat, TTMenu/          # kartın YSMenu dosyaları (YSMenu.ini burada)
├── 01 - Games/                  # NDS oyunları
│   ├── 0002 - Pokemon Platinum Version.nds
│   ├── 0002 - Pokemon Platinum Version.sav
│   └── ...
├── GBA/gba/                     # GBA oyunları
│   ├── 0000-------Pokemon----------.gba   # "---" içeren dosya = bölüm ayırıcısı (oyun değil)
│   ├── 0001- Pokemon Fire Red Version.gba
│   └── ...
├── GB/                          # GB / GBC oyunları
│   └── Tetris.gb
├── _macro/
│   ├── ysmenu.dat               # kartın orijinal R4.dat'ı (YSMenu çekirdeği)
│   ├── labels/                  # kartuş etiketleri (.lbl)
│   └── hero/                    # üst ekran arka planları (.bgb)
└── _nds/
    ├── GBARunner2_arm7dldi_ds.nds
    └── TWiLightMenu/
        ├── main.srldr           # BU MENÜ (dualDS.nds)
        └── emulators/gameyob.nds
```

Kurallar:
- **Sıralama dosya adına göredir.** Numaralı adlar (`0001 - …`) düzeni sağlar. Aynı seriden oyunları yan yana numarala
  (ör. tüm Pokémon'lar 0002–0006).
- **Görünen ad:** NDS'de oyunun kendi başlığından (ROM banner'ı), GBA/GB'de dosya adından gelir. Dosya adındaki baştaki
  numara ve tireler atılır.
- **Etiket eşleşmesi:**
  - NDS etiketi dosya adının kendisiyle eşleşir: `0011 - Mario Kart DS.nds` → `0011 - Mario Kart DS.lbl`.
  - GBA/GB etiketi temizlenmiş adla eşleşir: `0001- Pokemon Fire Red Version.gba` → `Pokemon Fire Red Version.lbl`.
- **Oyun dosyasını yeniden adlandırınca** aynı adlı `.sav`, `.lbl` ve `.bgb` dosyalarını da adlandır. Ayrıca
  `tools/nds_codes.json`'u ve `YSMenu.ini`'deki `AUTO_BOOT`'u (aynı uzunlukta kalacak şekilde) güncelle.

## Oyun ekleme ve resimler

### NDS oyunu

1. ROM'u `01 - Games/` klasörüne numaralı adla koy.
2. Oyun kodunu bul. ROM başlığında `0x0C` konumundaki 4 harf (ör. `AMCE`). Claude bunu yalnızca ilk 16 baytı okuyarak
   yapar.
3. `tools/nds_codes.json` dosyasına ekle: `"0011 - Mario Kart DS.nds": "AMCE"`.
4. Resimleri indir (GameTDB):
   - **Gerçek kart fotoğrafı:** `https://art.gametdb.com/ds/cart/US/<KOD>.png` → `tools/carts/<KOD>.png`
   - **Kapak:** `https://art.gametdb.com/ds/coverM/US/<KOD>.jpg` → `tools/covers/<KOD>.jpg`
   - `US` yoksa `EN`, `EU` dene. Kodun son harfi bölgedir: `E` ABD, `P` Avrupa, `J` Japonya.
5. `python tools/nds_labels.py . "F:/01 - Games"` ve `python tools/hero.py ...` çalıştır.

Nasıl seçilir:
- `tools/carts/<KOD>.png` **varsa** etiket gerçek kart fotoğrafından kesilir. Bu en iyi sonuçtur.
- **Yoksa** etiket aynı düzenle üretilir: üst beyaz bant ve logo, kapak resmi, ESRB / Nintendo / mühür, `NTR-KOD-USA`.
- Logosu kapağın altında olan oyunlarda (logo ESRB/mührün altında kalıyorsa) kodu `nds_labels.py` içindeki `FIT` ve
  `BOTTOM` kümelerine ekle. Resim küçültülüp yukarı alınır, yanlar bulanık arka planla dolar.
- Önizleme için `python tools/preview_all.py .` → `tools/_all.png`. Kullanıcıya bunu göster.

### GBA / GB oyunu

Etiketler kartuş **fotoğraflarından** yapılır. `tools/make_assets.py` içindeki `PHOTO` sözlüğüne bir satır ekle:

```python
"mario kart super circuit": ("photos/8.png", ("compose", 60, 30, 408, 138, 1.04, 175, 135, 345, 200), [...]),
"tetris": ("tetris_cart_photo.png", ("crop", 72, 122, 268, 306), []),
```

- **Anahtar:** Oyunun temizlenmiş adının başı, küçük harf.
- **Kırpma modları:**
  - `crop`: doğrudan kırpar.
  - `fit`: resmin tamamını sığdırır.
  - `focus`: bir odak kutusunun çevresinden alır.
  - `compose`: logoyu temiz bir zemin üzerine yerleştirir.
- **Silme listesi:** ESRB, mühür gibi küçük artıkları çevredeki renkle örter.
- **Fotoğraf yoksa** `ART` sözlüğüne renk ve başlık verilerek çizim etiket üretilir.
- Sonra `python tools/make_assets.py . "F:/GBA/gba;F:/GB"` ve `hero.py` çalıştır, menüyü yeniden derle (GBA/GB kabukları
  `assets.c` içinde).

Kullanıcı kendi kartuşunun fotoğrafını çekebilir: düz, iyi ışıkta, etiket tam görünecek şekilde. Claude kırpma
koordinatlarını fotoğrafa bakarak ayarlar ve önizlemeyi gösterir.

## Menü dilini değiştirme

Metinler kodun içinde:
- `source/_ui_dual.txt`: bölüm adları (`tabLong`), ipuçları, uyarılar, "Kayıt var", gün/ay adları (`gun[]`, `ay[]`).
- `source/_part3.txt`: açılış ve hata ekranları.

`grep` ile Türkçe karakter içeren dizgileri bul (`çğıöşü`); çeviri listesini kullanıcıya onaylat.

Yazı tipi yalnızca `tools/make_assets.py` içindeki `CHARS` listesindeki karakterleri içerir:

```python
CHARS = [chr(c) for c in range(32, 127)] + list("ÇĞİÖŞÜçğıöşü•éÉèêëáàâäíìîïóòôúùûñ™®©")
```

- Yeni dilin özel harflerini (ör. `ßäöü`, `ąęłńśźż`, `ãõç`) buraya ekle ve `make_assets.py`'yi yeniden çalıştır.
- Japonca/Çince gibi büyük karakter setleri bu bitmap yazı tipi yöntemine sığmaz. Kullanıcıya söyle.
- Bitmap yazı tipi Windows'taki Segoe UI'dan üretilir (`C:/Windows/Fonts`). Linux/macOS'ta açık bir yazı tipiyle
  (ör. Noto Sans) değiştir.

## Derleme ortamı

- **Windows + WSL (Ubuntu)** içinde **BlocksDS**:
  - `/opt/blocksds/core`, araç zinciri `/opt/wonderful`.
  - Kurulum: <https://blocksds.skylyrac.net/docs/setup/>.
- **Python 3 + Pillow** (Windows tarafında): `pip install pillow`.
- `build.sh`:
  - Projeyi `/root/work/dualds`'a kopyalar.
  - `bootloader/` (nds-bootloader, GPL) derler ve `load.bin`'i C dizisine çevirir.
  - `arm7/` ve ana programı derler.
  - `dualDS.nds`'i proje köküne koyar.
- `source/main.c` elle düzenlenmez. `tools/assemble_dual.py` onu şu parçalardan üretir:
  `_part1.txt`, `_io_dual.txt`, `_part3.txt` (kartuş/arka plan/açılış), `_ui_dual.txt`.
- `source/assets.c/.h` da üretilen dosyalardır (`make_assets.py`).

## melonDS ile test

1. melonDS'te **DLDI / SD kart klasörü** ayarla (Config → Emulation settings → DLDI). "Sync SD to folder" ile test
   klasörünü göster.
2. Test klasörünü gerçek kart gibi düzenle: `_macro/`, `01 - Games/` (birkaç küçük ROM), `GBA/gba/`, `GB/`.
3. `dualDS.nds`'i melonDS ile doğrudan aç.
   - NDS oyununu başlatmak emülatörde YSMenu çekirdeği gerektirir. Menü görünümü ve hız testi için gerekmez.
4. Test derlemesi bayrakları (`main.c`'nin en başına):
   - `#define MACRO_AUTOTEST`: menü kendi kendine sağa kaydırır ve bölüm değiştirir (ekran görüntüsü için).
   - `#define MACRO_PERF`: kare sürelerini ve sayfa çizim aşamalarının süresini ekrana yazar.
   - **Karta yazmadan önce bu satırları kaldır.**

## Kontroller

| Tuş / dokunma | İşlev |
|---|---|
| ◀ ▶ (veya parmakla kaydırma) | Oyun seç |
| A / ortadaki karta dokun | Oyunu başlat |
| L / R / üstteki sekmeler | NDS – GBA – GB bölümü |
| START | YSMenu'yü aç (kartın kendi menüsü) |
| SELECT | Parlaklık 1–7 |

## Sorun giderme ve öğrendiklerimiz

- **`fat:/_nds/TWiLightMenu/main.srldr not found`:** Dosya kayıp ya da klasör kaydı bozuk.
  1. `raw_backup.py` ile yedek al.
  2. `verify_card.py` ile incele.
  - Bu projede bozuk bir okuyucu, klasörün dizin kümesine ve FAT'ın iki sektörüne USB paketi ("USBS…") yazmıştı.
  - Çözüm: ezilen FAT sektörleri bitişik dosya zincirlerinden yeniden hesaplandı, dizin boş bir dizin olarak yeniden
    yazıldı, dosyalar tekrar kopyalandı, iki FAT kopyası eşitlendi, sahipsiz kümeler boşaltıldı.
  - Hepsi önce yedeğin kopyasında denendi, sonra karta uygulandı.
- **Okuyucu büyük okumalarda hata veriyor (errno 22):** 64 KB parçalarla oku, hata olunca bekle ve aygıtı yeniden aç
  (`raw_backup.py` bunu yapar).
- **Windows "Warning / Dirty" diyor:** Açılış sektöründeki `0x41` baytının kirli bayrağı. Kart doğrulandıktan sonra
  temizlenebilir. Rastgele chkdsk çalıştırma.
- **Menü takılıyor / kasıyor:** Kare süresini `MACRO_PERF` ile ölç. Sık nedenler:
  - üst ekranın tek karede çizilmesi;
  - piksel başına yavaş renk karıştırma;
  - açılışta çok dosya okuma.
- **Etiketler bazı oyunlarda boş:** VRAM dolmuş olabilir. Etiket sayısını ve `MAX_LABELS`'ı kontrol et.
- **Yeni oyunun etiketi yok:** `nds_codes.json`'a eklendi mi? Etiket adı oyun dosyasının adıyla birebir aynı mı?

## Kaynak dosyalar

| Dosya | İçerik |
|---|---|
| `source/_part1.txt` | Ayarlar, renkler, `Game` yapısı |
| `source/_io_dual.txt` | Kart okuma: oyun tarama, NDS başlığı, etiket ve arka plan yükleme |
| `source/_part3.txt` | 3B kartuş çizimi, zemin/yansıma, açılış animasyonu |
| `source/_ui_dual.txt` | Üst ekran vitrini, menü döngüsü, oyun başlatma, `main()` |
| `tools/make_assets.py` | Yazı tipleri, logo, kartuş kabukları, GBA/GB etiketleri |
| `tools/nds_labels.py` | NDS kart etiketleri (gerçek kart fotoğrafı veya kapaktan) |
| `tools/hero.py` | Üst ekran bulanık arka planları |
| `tools/assemble_dual.py` | Parçaları `main.c`'de birleştirir |
| `tools/preview_all.py` | Tüm NDS kartlarının önizleme tablosu |
| `tools/sd/raw_backup.py` | SD kartın ham yedeği (salt okunur) |
| `tools/sd/verify_card.py` | Karttan ham okuyarak doğrulama |
| `bootloader/`, `arm7/` | NDS dosyası başlatıcı (nds-bootloader) ve ARM7 kodu |

## Teşekkür ve lisanslar

- [BlocksDS](https://github.com/blocksds/sdk): NDS geliştirme ortamı.
- [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu): Autoboot dosyaları, GameYob paketi, nds-bootloader.
- [GBARunner2](https://github.com/Gericom/GBARunner2): DS'te GBA oyunları.
- GameYob: GB/GBC emülatörü.
- YSMenu: kartın çekirdeği. Bu depoda **yer almaz**, kullanıcının kendi kartından gelir.
- [GameTDB](https://www.gametdb.com): kapak ve kart görselleri. Görseller kullanıcı tarafından indirilir, bu depoda
  yer almaz.
- `bootloader/` ve `COPYING`: GNU GPL v2.

Nintendo, Nintendo DS, Game Boy ve oyun adları sahiplerinin ticari markalarıdır. Bu proje Nintendo ile bağlantılı
değildir. Sadece yasal olarak sahip olduğunuz oyunların yedekleriyle kullanın.
