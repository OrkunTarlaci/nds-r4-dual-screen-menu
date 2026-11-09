# macro DS Lite - BlocksDS derleme dosyasi
NAME          := dualDS
GAME_TITLE    := Nintendo DS
GAME_SUBTITLE := single screen edition
GAME_AUTHOR   := Macro DS Lite
SOURCEDIRS    := source
INCLUDEDIRS   := source
ARM7ELF       := arm7/build/arm7.elf
include /opt/blocksds/core/sys/default_makefiles/rom_arm9/Makefile
