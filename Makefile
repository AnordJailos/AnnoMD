# AnnoMD build.  Linux cross-compile:  make        |  MSYS2 (Windows):  make CROSS=
CROSS   ?= x86_64-w64-mingw32-
CC      := $(CROSS)gcc
WINDRES := $(CROSS)windres
VERSION := $(strip $(shell cat VERSION))
V_MAJOR := $(word 1,$(subst ., ,$(VERSION)))
V_MINOR := $(word 2,$(subst ., ,$(VERSION)))
V_PATCH := $(word 3,$(subst ., ,$(VERSION)))
B       := build
LIBS    := -lcomctl32 -lcomdlg32 -lgdi32 -lshell32 -ldwmapi -luser32 -ladvapi32
CFLAGS  := -Os -s -w -ffunction-sections -fdata-sections -Wl,--gc-sections -municode -mwindows -static

.PHONY: all exe installer sums clean
all: exe installer
exe: $(B)/AnnoMD.exe
installer: $(B)/AnnoMD-Setup.exe

# VERSION is the single source of truth: it feeds the exe, its file properties and the installer
$(B)/version.h: VERSION
	@mkdir -p $(B)
	printf '#define VER_MAJOR %s\n#define VER_MINOR %s\n#define VER_PATCH %s\n#define VER_STR "%s"\n' $(V_MAJOR) $(V_MINOR) $(V_PATCH) $(VERSION) > $@

$(B)/app.res: src/app.rc src/app.manifest assets/app.ico $(B)/version.h
	$(WINDRES) -I $(B) src/app.rc -O coff -o $@

$(B)/AnnoMD.exe: src/annomd.c $(B)/app.res $(B)/version.h
	$(CC) $(CFLAGS) -I$(B) src/annomd.c $(B)/app.res -o $@ $(LIBS)

$(B)/AnnoMD-Setup.exe: $(B)/AnnoMD.exe installer/annomd.nsi assets/app.ico VERSION
	makensis -V1 -DVERSION=$(VERSION) -DEXE_PATH=$(abspath $(B)/AnnoMD.exe) \
	  -DICON_PATH=$(abspath assets/app.ico) -DOUT_PATH=$(abspath $@) installer/annomd.nsi

# checksums for the release page
sums: all
	cd $(B) && sha256sum AnnoMD.exe AnnoMD-Setup.exe > SHA256SUMS.txt

clean:
	rm -rf $(B)
