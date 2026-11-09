# Nintendo DS — Dual Screen Edition (custom menu for R4 flashcarts)

**English | [Türkçe](README.tr.md)**

A two-screen game menu with 3D cartridges for classic R4-style DS flashcarts.
It launches NDS games through the cart's own software (YSMenu), GBA games through **GBARunner2** and GB/GBC games
through **GameYob**.

> This repository is meant to be installed **with the help of Claude (Claude Code)**. Open the folder in Claude Code,
> insert your SD card and say: *"Read README.md and set this menu up for my card."* The section
> [Instructions for Claude](#instructions-for-claude) tells Claude exactly what to ask and what to do.
> You can talk to Claude in any language.

---

## Contents

1. [Features](#features)
2. [Important warnings](#important-warnings)
3. [Installing with Claude (for users)](#installing-with-claude-for-users)
4. [Instructions for Claude](#instructions-for-claude)
5. [How it works (architecture)](#how-it-works-architecture)
6. [SD card layout and file names](#sd-card-layout-and-file-names)
7. [Adding games and artwork](#adding-games-and-artwork)
8. [Changing the menu language](#changing-the-menu-language)
9. [Build environment](#build-environment)
10. [Testing with melonDS](#testing-with-melonds)
11. [Controls](#controls)
12. [Troubleshooting and lessons learned](#troubleshooting-and-lessons-learned)
13. [Source files](#source-files)
14. [Credits and licenses](#credits-and-licenses)

---

## Features

- **Bottom screen (touch):** a 3D carousel of cartridges.
  - DS cards look like real DS game cards. The label comes from a photo of the real card: NINTENDO DS band, ESRB,
    Nintendo seal, `NTR-XXXX-USA` code.
  - Games without a card photo get a card built from their cover art in the same layout.
  - GBA games sit in a grey GBA shell, GB games in a Game Boy shell.
  - Cartridges reflect on the floor and the selected one floats gently.
- **Top screen (showcase):** the selected game's icon, title, publisher and save status, over a blurred version of its
  cover. Switching games fades smoothly.
- **Sections:** `NDS`, `GBA`, `GB`. Switch with L/R or by tapping.
- **Boot:** a 4-second "Nintendo DS" animation. The SD card is read while it plays, so there is no white loading screen.
- **Brightness:** 7 levels (SELECT).
  - 1–5: hardware backlight.
  - 6–7: backlight at full power plus a software brightening of the image (menu only).
- **Last game:** the menu starts on the last game you launched. This is read from YSMenu's own `AUTO_BOOT` line; the
  menu writes no extra file to the card.
- **Performance:** every frame stays under 16.7 ms (60 FPS).
  - The top-screen page is drawn in 8 slices spread over several frames.
  - The fade is done by the display's brightness hardware.
  - Images are loaded in the background while the menu is idle.

> The menu's on-screen text is currently **Turkish**. Claude asks which language you want and translates it; see
> [Changing the menu language](#changing-the-menu-language).

## Important warnings

- **This repository contains no games.** ROMs, save files (`.sav`), cover/card artwork and the cart's original
  firmware are **not included and must not be added**. Everyone uses their own cart's software and their own backups.
- **Always take a full backup before writing to the SD card.** `tools/sd/raw_backup.py` makes a sector-by-sector
  image.
- **Cheap or faulty USB card readers can corrupt the SD card.** In this project one reader wrote its own USB packets
  ("USBS/USBC") to the card instead of data and broke a folder and the file allocation table. Use a good reader.
- If Windows asks *"Do you want to scan and fix this drive?"*, **do not let it repair.** The repair (chkdsk) can delete
  save files. Back up first, then let Claude inspect the card.

---

## Installing with Claude (for users)

1. Install **Claude Code** (desktop app or terminal).
2. Download this repository and open the folder in Claude Code.
3. Connect the SD card to the computer with a **good card reader**.
4. Tell Claude:
   > Read README.md and set this menu up for my card.
5. Claude will ask you some questions (listed below). Answer them; it does the rest, shows you each step and asks for
   your approval.

Have these ready:
- The **exact model name** printed on the cart. You can also take a photo.
- The cart's **currently working software** (the files in the root of the SD card). If the cart already works, they
  are on the SD card.
- Your console model (DS, DS Lite, DSi, 3DS).
- The language you want the menu in.

---

## Instructions for Claude

> This section is for the Claude that opens this repository. Follow these steps even if the user did not ask for them
> explicitly.

### 0. Ask the user before you start

Ask these one by one, in plain language. The user may not be technical; explain in one sentence why each question
matters. Keep notes of the answers.

1. **Menu language.** Which language: English, Turkish, something else? The current texts are Turkish; see
   [Changing the menu language](#changing-the-menu-language).
2. **Console model.** Original Nintendo DS, DS Lite, DSi or 3DS?
   - The menu was written for DS and DS Lite.
   - On DSi/3DS the flashcart runs in DS mode. Brightness and backlight may behave differently; say so before
     installing.
3. **Cart model.** What exactly is printed on the cart? Ask for a photo. Examples: "R4 Upgrade Revolution for DS
   (NDSL/NDS)", "R4i Gold 3DS", "R4i-SDHC", "Acekard 2i", "DSTT".
   - The label may not reflect the real hardware; clones with the same label run different kernels.
   - Work out which software (kernel) it runs from the files in the SD card root:
     - `R4.dat` + `TTMenu/` + `TTMenu.dat` → YSMenu-based.
     - `_wfwd/` → Wood R4.
     - `__rpg/` → AKAIO.
     - `_DS_MENU.DAT` → original R4 kernel.
4. **Does the cart have its original working software?**
   - Does the cart currently boot into its normal menu? This install goes **on top of** that software. NDS games are
     still launched by it; our menu sits in front of it.
   - If not, or if the card is empty: find the right kernel for the model together with the user (the cart maker's
     site or well-known archives). Ask the user to download it and approve it.
5. **SD card.** How many GB, which file system? It must be FAT32. Are there game saves on it? If so, back up first.
6. **Card reader.** Which reader is the card in? If it is an unknown/cheap reader, explain the risk; see
   [Troubleshooting](#troubleshooting-and-lessons-learned).
7. **Games.** Which folders hold the NDS, GBA and GB/GBC games? Does the user want numbered names
   (`0001 - Game Title.nds`)? Always present renaming as a proposal for approval, and rename the `.sav` files along
   with their games.
8. **Look and feel.** Is the current design (real-card look, dark theme) fine, or do they want changes?

### 1. Safety rules (always)

- **First thing: a full raw backup:**
  ```
  set SD_DRIVE=F:
  python tools/sd/raw_backup.py card_backup.img
  ```
  If the reader stalls, the tool waits and retries. Do not write to the card until the backup is complete.
- **Never** write content into ROMs or `.sav` files. Only rename them, and only with the user's approval. Rename in
  two steps (temporary name first) so names cannot collide.
- Do not open large game files unnecessarily. Read the game code from the first 16 bytes of the ROM header (`0x0C`,
  4 bytes).
- After writing to the card, verify **bypassing the Windows cache**:
  `python tools/sd/verify_card.py <card_path> <local_file> ...`.
  The tool checks file contents, that both FAT copies match, every file's cluster chain, and traces of reader
  packets. Windows' `Get-FileHash` can come from the cache and is not enough on its own.
- After writing, flush the cache with `Write-VolumeCache -DriveLetter F` and have the user use "Safely Remove".
- Tell the user what you are about to write to the card before writing. Get approval unless the user explicitly said
  "write without asking".
- Do not run `chkdsk` or Windows repair. If something is wrong, take a backup first and investigate on the backup.

### 2. Set up the cart's boot chain

The path chosen for this project (YSMenu-based R4 clone):

```
DS boots → the cart runs R4.dat from the SD root
         → R4.dat = TWiLight Menu++ "Autoboot" file (the one for this cart model)
         → fat:/_nds/TWiLightMenu/main.srldr runs = THIS MENU
When an NDS game is chosen:
         → the menu rewrites the AUTO_BOOT line in TTMenu/YSMenu.ini in place
         → fat:/_macro/ysmenu.dat (the cart's ORIGINAL R4.dat = YSMenu kernel) is run
         → YSMenu launches the game in AUTO_BOOT directly
```

Steps:
1. Copy the cart's **original** root `R4.dat` to `/_macro/ysmenu.dat`. This is the YSMenu kernel itself. Keep a
   separate backup of the original.
2. Put the matching `R4.dat` (and any files next to it) from the TWiLight Menu++ release's `Autoboot/<cart model>/`
   folder in the SD root.
   - In this project the `Autoboot/R4i-SDHC/` files worked.
   - For another cart, first look it up in TWiLight's "Flashcard users" docs. Choose the right file together with the
     user.
3. Append this line to the end of `TTMenu/YSMenu.ini` **once**: `;MACRO_PAD` + 300 spaces + CRLF.
   - When the menu changes the `AUTO_BOOT` value it takes the length difference out of this line, so the file size
     stays the same. The file is written in place (`r+b`) and the file allocation table (FAT) is never touched.
   - If `YSMenu.ini` has no `AUTO_BOOT=` line, add one together with the user first.
4. Build the menu and copy it as `main.srldr`: `/_nds/TWiLightMenu/main.srldr`.
5. For GBA: `/_nds/GBARunner2_arm7dldi_ds.nds`. This is GBARunner2's DS (arm7 DLDI) build; it also needs a GBA BIOS,
   see the GBARunner2 docs.
6. For GB/GBC: `/_nds/TWiLightMenu/emulators/gameyob.nds` (from the TWiLight Menu++ package).

**If the cart is not YSMenu-based:** NDS games are launched differently. Options:
- an "auto boot" setting of the cart's own kernel;
- TWiLight Menu++'s nds-bootstrap;
- the kernel accepting the ROM path through argv.

Find out by testing on the user's cart. Adapt `launchGame()` and `setAutoBoot()` accordingly.
The settings are the `#define`s at the top of `source/_part1.txt`:

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

### 3. Generate artwork, build, test, install

**First prepare two template images.** They are not in the repository for copyright reasons, but `make_assets.py`
needs them:

| File | What | Where from |
|---|---|---|
| `tools/carts/CPUE.png` | DS card shell and "NINTENDO DS" band template (Pokémon Platinum card) | `https://art.gametdb.com/ds/cart/US/CPUE.png` |
| `tools/photos/gba_cart.png` | GBA cartridge shell (the label area is cut out, so any GBA game works) | A flat front photo the user takes, or a product photo the user approves (~400×240, white background) |

Ask the user for permission before downloading.

Then, in order (Windows, project root; `F:` is the user's SD drive):

```
python tools/make_assets.py . "F:/GBA/gba;F:/GB"            # fonts, shells, GBA/GB labels -> source/assets.c/.h
python tools/nds_labels.py . "F:/01 - Games"                 # NDS card labels -> sdroot/_macro/labels
python tools/hero.py . "F:/01 - Games" "F:/GBA/gba" "F:/GB"  # top-screen backgrounds -> sdroot/_macro/hero
python tools/assemble_dual.py                                # source/*.txt parts -> source/main.c
wsl -d Ubuntu -u root -- bash /mnt/c/.../build.sh           # build with BlocksDS -> dualDS.nds
```

Then:
- Test in melonDS ([Testing with melonDS](#testing-with-melonds)), show the user screenshots, get approval.
- Install:
  - `dualDS.nds` → `SD:/_nds/TWiLightMenu/main.srldr`
  - `sdroot/_macro/labels/*` → `SD:/_macro/labels/`
  - `sdroot/_macro/hero/*.bgb` → `SD:/_macro/hero/`
- Verify with `tools/sd/verify_card.py`.

### 4. Way of working (this worked well in this project)

- **Show** every visual change to the user first: a melonDS screenshot or a preview PNG.
- If the user does not like a design, produce 3–4 options side by side and let them choose.
- Measure performance, don't guess: in a test build `#define MACRO_PERF` prints frame times on screen.
- Write only changed files to the card. Verify each write with a raw read.

---

## How it works (architecture)

| Part | Role |
|---|---|
| **Bottom screen** (main engine, 3D) | Cartridge carousel: GL2D + videoGL. Each cartridge = shell texture + label texture. `lcdMainOnBottom()` puts the 3D engine on the bottom screen. |
| **Top screen** (sub engine, 16-bit bitmap) | The showcase page is drawn by the CPU (`fb[]`). A new page is drawn into a back buffer in 8 half-stages, then switched with a hardware brightness fade. |
| **Launching NDS** | `AUTO_BOOT` in `YSMenu.ini` is updated in place, then `ysmenu.dat` (the cart's original kernel) is run with `runNdsFile`. |
| **Launching GBA / GB** | `runNdsFile(emulator, {emulator, rom})`: GBARunner2 / GameYob take the ROM path from argv. |
| **Boot** | The screens go black immediately. Reading the card (`bootWork`) is spread over the boot animation's frames: file list → 2 games' titles/icons per frame → 2 labels per frame. |
| **Memory** | VRAM A+B+D = 384 KB for textures. Fonts and shells ~177 KB. An NDS label is 64×64 (4 KB), a GBA/GB label 128×64 (8 KB). At most `MAX_LABELS` (48) labels. |
| **Files read** | The menu **never writes** to the card. The only exception is the `AUTO_BOOT` value in `YSMenu.ini`, written in place at the same size. |

File formats:
- **`.lbl`:** 512-byte palette (256 × RGB15) + 8192 bytes of pixels (128×64, row length 128). For NDS the top-left 64×64
  is used.
- **`.bgb`:** 128×96 RGB15 blurred background; shown at 2× on screen.
- **`.hrb`:** Large cover left over from an older version; no longer read.

## SD card layout and file names

```
SD:/
├── R4.dat                       # TWiLight Autoboot (starts the menu)
├── TTMenu.dat, TTMenu/          # the cart's YSMenu files (YSMenu.ini is here)
├── 01 - Games/                  # NDS games
│   ├── 0002 - Pokemon Platinum Version.nds
│   ├── 0002 - Pokemon Platinum Version.sav
│   └── ...
├── GBA/gba/                     # GBA games
│   ├── 0000-------Pokemon----------.gba   # a file with "---" = section separator (not a game)
│   ├── 0001- Pokemon Fire Red Version.gba
│   └── ...
├── GB/                          # GB / GBC games
│   └── Tetris.gb
├── _macro/
│   ├── ysmenu.dat               # the cart's original R4.dat (YSMenu kernel)
│   ├── labels/                  # cartridge labels (.lbl)
│   └── hero/                    # top-screen backgrounds (.bgb)
└── _nds/
    ├── GBARunner2_arm7dldi_ds.nds
    └── TWiLightMenu/
        ├── main.srldr           # THIS MENU (dualDS.nds)
        └── emulators/gameyob.nds
```

Rules:
- **Order follows the file name.** Numbered names (`0001 - …`) keep things in order. Number games of the same series
  next to each other (e.g. all Pokémon games 0002–0006).
- **Displayed title:** for NDS it comes from the game's own title (ROM banner), for GBA/GB from the file name. The
  leading number and dashes are stripped from the file name.
- **Label matching:**
  - An NDS label matches the file name itself: `0011 - Mario Kart DS.nds` → `0011 - Mario Kart DS.lbl`.
  - A GBA/GB label matches the cleaned title: `0001- Pokemon Fire Red Version.gba` → `Pokemon Fire Red Version.lbl`.
- **When you rename a game file,** rename its `.sav`, `.lbl` and `.bgb` too. Also update `tools/nds_codes.json` and the
  `AUTO_BOOT` line in `YSMenu.ini` (keeping the same length).

## Adding games and artwork

### NDS game

1. Put the ROM in `01 - Games/` with a numbered name.
2. Find the game code: the 4 letters at offset `0x0C` in the ROM header (e.g. `AMCE`). Claude does this by reading only
   the first 16 bytes.
3. Add it to `tools/nds_codes.json`: `"0011 - Mario Kart DS.nds": "AMCE"`.
4. Download the artwork (GameTDB):
   - **Real card photo:** `https://art.gametdb.com/ds/cart/US/<CODE>.png` → `tools/carts/<CODE>.png`
   - **Cover:** `https://art.gametdb.com/ds/coverM/US/<CODE>.jpg` → `tools/covers/<CODE>.jpg`
   - If `US` is missing, try `EN` or `EU`. The last letter of the code is the region: `E` USA, `P` Europe, `J` Japan.
5. Run `python tools/nds_labels.py . "F:/01 - Games"` and `python tools/hero.py ...`.

How the label is chosen:
- **If** `tools/carts/<CODE>.png` exists, the label is cut from the real card photo. This gives the best result.
- **Otherwise** a label is generated in the same layout: white top band and logo, cover art, ESRB / Nintendo / seal,
  `NTR-CODE-USA`.
- For games whose logo is at the bottom of the cover (where it would sit under the ESRB box or seal), add the code to
  the `FIT` and `BOTTOM` sets in `nds_labels.py`. The art is shrunk and moved up; the sides are filled with a blurred
  background.
- Preview: `python tools/preview_all.py .` → `tools/_all.png`. Show this to the user.

### GBA / GB game

Labels are made from cartridge **photos**. Add a line to the `PHOTO` dictionary in `tools/make_assets.py`:

```python
"mario kart super circuit": ("photos/8.png", ("compose", 60, 30, 408, 138, 1.04, 175, 135, 345, 200), [...]),
"tetris": ("tetris_cart_photo.png", ("crop", 72, 122, 268, 306), []),
```

- **Key:** the start of the game's cleaned title, lower case.
- **Crop modes:**
  - `crop`: crops directly.
  - `fit`: fits the whole image.
  - `focus`: takes the area around a focus box.
  - `compose`: places the logo on a clean background.
- **Erase list:** covers small leftovers such as ESRB or the seal with the surrounding colour.
- **Without a photo,** a drawn label is generated from the `ART` dictionary (colours and title).
- Then run `python tools/make_assets.py . "F:/GBA/gba;F:/GB"` and `hero.py`, and rebuild the menu (the GBA/GB shells
  live in `assets.c`).

The user can photograph their own cartridge: flat, in good light, with the whole label visible. Claude adjusts the
crop coordinates by looking at the photo and shows a preview.

## Changing the menu language

The texts are in the code:
- `source/_ui_dual.txt`: section names (`tabLong`), hints, warnings, "Kayıt var" (save present), day/month names
  (`gun[]`, `ay[]`).
- `source/_part3.txt`: boot and error screens.

Find the strings with Turkish characters (`çğıöşü`) using `grep`; have the user approve the translation list.

The font only contains the characters in the `CHARS` list in `tools/make_assets.py`:

```python
CHARS = [chr(c) for c in range(32, 127)] + list("ÇĞİÖŞÜçğıöşü•éÉèêëáàâäíìîïóòôúùûñ™®©")
```

- Add the special letters of the new language (e.g. `ßäöü`, `ąęłńśźż`, `ãõç`) here and run `make_assets.py` again.
- Large character sets such as Japanese or Chinese do not fit this bitmap font approach. Tell the user.
- The bitmap font is generated from Segoe UI on Windows (`C:/Windows/Fonts`). On Linux/macOS, switch to an open font
  (e.g. Noto Sans).

## Build environment

- **BlocksDS** inside **Windows + WSL (Ubuntu)**:
  - `/opt/blocksds/core`, toolchain in `/opt/wonderful`.
  - Setup: <https://blocksds.skylyrac.net/docs/setup/>.
- **Python 3 + Pillow** (on the Windows side): `pip install pillow`.
- `build.sh`:
  - copies the project to `/root/work/dualds`;
  - builds `bootloader/` (nds-bootloader, GPL) and turns `load.bin` into a C array;
  - builds `arm7/` and the main program;
  - puts `dualDS.nds` in the project root.
- `source/main.c` is not edited by hand. `tools/assemble_dual.py` generates it from these parts:
  `_part1.txt`, `_io_dual.txt`, `_part3.txt` (cartridges/background/boot), `_ui_dual.txt`.
- `source/assets.c/.h` are generated files too (`make_assets.py`).

## Testing with melonDS

1. In melonDS set up the **DLDI / SD card folder** (Config → Emulation settings → DLDI). Point "Sync SD to folder" at
   the test folder.
2. Lay the test folder out like the real card: `_macro/`, `01 - Games/` (a few small ROMs), `GBA/gba/`, `GB/`.
3. Open `dualDS.nds` directly in melonDS.
   - Launching an NDS game in the emulator needs the YSMenu kernel. It is not needed for testing the menu's look and
     speed.
4. Test build flags (at the very top of `main.c`):
   - `#define MACRO_AUTOTEST`: the menu scrolls right and switches sections by itself (for screenshots).
   - `#define MACRO_PERF`: prints frame times and the duration of the page-drawing stages on screen.
   - **Remove these lines before writing to the card.**

## Controls

| Button / touch | Action |
|---|---|
| ◀ ▶ (or swipe) | Select game |
| A / tap the middle card | Launch game |
| L / R / tabs at the top | NDS – GBA – GB section |
| START | Open YSMenu (the cart's own menu) |
| SELECT | Brightness 1–7 |

## Troubleshooting and lessons learned

- **`fat:/_nds/TWiLightMenu/main.srldr not found`:** the file is missing or the folder record is corrupt.
  1. Take a backup with `raw_backup.py`.
  2. Inspect with `verify_card.py`.
  - In this project a faulty reader had written USB packets ("USBS…") into the folder's directory cluster and into two
    FAT sectors.
  - Fix: the overwritten FAT sectors were recomputed from the contiguous file chains, the directory was rewritten as an
    empty directory, the files were copied again, the two FAT copies were synced and orphaned clusters were freed.
  - Everything was first tried on a copy of the backup, then applied to the card.
- **The reader fails on large reads (errno 22):** read in 64 KB pieces; on error wait and reopen the device
  (`raw_backup.py` does this).
- **Windows says "Warning / Dirty":** the dirty flag in byte `0x41` of the boot sector. It can be cleared once the card
  has been verified. Do not run chkdsk blindly.
- **The menu stutters:** measure the frame time with `MACRO_PERF`. Common causes:
  - drawing the whole top screen in a single frame;
  - slow per-pixel colour blending;
  - reading many files at boot.
- **Some games have blank labels:** VRAM may be full. Check the label count and `MAX_LABELS`.
- **A new game has no label:** was it added to `nds_codes.json`? Does the label name exactly match the game's file
  name?

## Source files

| File | Contents |
|---|---|
| `source/_part1.txt` | Settings, colours, `Game` struct |
| `source/_io_dual.txt` | Card reading: game scan, NDS banner, label and background loading |
| `source/_part3.txt` | 3D cartridge drawing, floor/reflection, boot animation |
| `source/_ui_dual.txt` | Top-screen showcase, menu loop, game launching, `main()` |
| `tools/make_assets.py` | Fonts, logo, cartridge shells, GBA/GB labels |
| `tools/nds_labels.py` | NDS card labels (from a real card photo or the cover) |
| `tools/hero.py` | Top-screen blurred backgrounds |
| `tools/assemble_dual.py` | Joins the parts into `main.c` |
| `tools/preview_all.py` | Preview sheet of all NDS cards |
| `tools/sd/raw_backup.py` | Raw backup of the SD card (read-only) |
| `tools/sd/verify_card.py` | Verification by reading raw from the card |
| `bootloader/`, `arm7/` | NDS file launcher (nds-bootloader) and ARM7 code |

## Credits and licenses

- [BlocksDS](https://github.com/blocksds/sdk): NDS development kit.
- [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu): Autoboot files, the GameYob package, nds-bootloader.
- [GBARunner2](https://github.com/Gericom/GBARunner2): GBA games on DS.
- GameYob: GB/GBC emulator.
- YSMenu: the cart's kernel. **Not included** in this repository; it comes from the user's own cart.
- [GameTDB](https://www.gametdb.com): cover and card artwork. The artwork is downloaded by the user and is not
  included in this repository.
- `bootloader/` and `COPYING`: GNU GPL v2.

Nintendo, Nintendo DS, Game Boy and the game titles are trademarks of their respective owners. This project is not
affiliated with Nintendo. Use it only with backups of games you legally own.
