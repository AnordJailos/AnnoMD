# AnnoMD build.  Linux cross-compile:  make        |  MSYS2 (Windows):  make CROSS=
CROSS   ?= x86_64-w64-mingw32-
CC      := $(CROSS)gcc
WINDRES := $(CROSS)windres
VERSION := $(shell cat VERSION)
B       := build
LIBS    := -lcomctl32 -lcomdlg32 -lgdi32 -lshell32 -ldwmapi -luser32 -ladvapi32
CFLAGS  := -Os -s -w -ffunction-sections -fdata-sections -Wl,--gc-sections -municode -mwindows -static

.PHONY: all exe installer clean
all: exe installer
exe: $(B)/AnnoMD.exe
installer: $(B)/AnnoMD-Setup.exe

$(B)/app.res: src/app.rc src/app.manifest assets/app.ico
	@mkdir -p $(B)
	$(WINDRES) src/app.rc -O coff -o $@

$(B)/AnnoMD.exe: src/annomd.c $(B)/app.res
	$(CC) $(CFLAGS) src/annomd.c $(B)/app.res -o $@ $(LIBS)

$(B)/AnnoMD-Setup.exe: $(B)/AnnoMD.exe installer/annomd.nsi assets/app.ico VERSION
	makensis -V1 -DVERSION=$(VERSION) -DEXE_PATH=$(abspath $(B)/AnnoMD.exe) \
	  -DICON_PATH=$(abspath assets/app.ico) -DOUT_PATH=$(abspath $@) installer/annomd.nsi

clean:
	rm -rf $(B)
