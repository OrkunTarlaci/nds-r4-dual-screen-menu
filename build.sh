#!/bin/bash
# WSL icinde calisir: bootloader -> load_bin.c -> macroDS.nds
set -e
set -o pipefail
export PATH=/opt/wonderful/bin:/opt/wonderful/toolchain/gcc-arm-none-eabi/bin:/usr/local/bin:/usr/bin:/bin
export BLOCKSDS=/opt/blocksds/core WONDERFUL_TOOLCHAIN=/opt/wonderful
SRC="$(cd "$(dirname "$0")" && pwd)"
W=/root/work/dualds
rm -rf /root/work/dualds   # eski kopyalar (ornegin silinmis dosyalar) kalmasin
mkdir -p $W
cp -r "$SRC/." $W/
cd $W
rm -rf build arm7/build   # her seferinde temiz derleme (eski .o dosyalari kullanilmasin)
make -C bootloader -f Makefile.blocksds -s
make -C arm7 -s 2>&1 | grep -E "error|warning" || true
# load.bin -> C dizisi (TWiLight'in kendi derledigi yukleyici varsa o kullanilir)
LOADBIN=bootloader/build/load.bin
[ -f bootloader/twl_load.bin ] && LOADBIN=bootloader/twl_load.bin
echo "yukleyici: $LOADBIN"
{
  echo '#include <stddef.h>'
  echo 'const unsigned char load_bin[] __attribute__((aligned(4))) = {'
  od -An -v -tu1 "$LOADBIN" | sed 's/^ *//; s/  */, /g; s/$/,/'
  echo '};'
  echo "const size_t load_bin_size = $(stat -c%s "$LOADBIN");"
} > source/load_bin.c
printf '#pragma once\n#include <stddef.h>\nextern const unsigned char load_bin[];\nextern const size_t load_bin_size;\n' > source/load_bin.h
make -j4 "$@" 2>&1 | grep -E "error|Error|NDSTOOL|LD " | head -60
ls -la *.nds
cp *.nds "$SRC/"
