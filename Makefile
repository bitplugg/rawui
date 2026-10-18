# ============================================================================
#  RAW UI - a Material Design 3 Expressive shell for PlayStation 4 (GoldHEN)
#
#  Build system modelled on the OpenOrbis-PS4-Toolchain sample Makefiles so it
#  behaves identically inside the official Docker image and on a plain Linux
#  host with clang/lld + OO_PS4_TOOLCHAIN set.
#
#  Quick start:
#      tools/docker-build.sh          # builds a .pkg in a container
#      OO_PS4_TOOLCHAIN=/opt/OO make  # or build directly on the host
# ============================================================================

# ---------------------------------------------------------------- metadata --
TITLE       := RAW UI
VERSION     := 1.00
TITLE_ID    := RAWUI0001
CONTENT_ID  := IV0000-RAWUI0001_00-RAWUIUI000000000

# ------------------------------------------------------------ directories ---
SRCDIR      := src
BUILDDIR    := build
PKGDIR      := pkg
DISTDIR     := dist
INTDIR      := $(BUILDDIR)/obj

# --------------------------------------------------------------- libraries --
LIBS        := -lc -lkernel -lc++ \
               -lSceUserService -lSceSysUtil -lSceSysmodule -lSceSystemService \
               -lSceVideoOut -lSceAudioOut -lScePad -lSceAppInstUtil \
               -lSceNet -lSceNetCtl -lSceFreeType \
               -lSDL2 -lSDL2_image

# Kernel prx modules that get bundled into the package.
LIBMODULES  := libc libSceFios2

# ------------------------------------------------------------------ flags ---
# _GNU_SOURCE is required: libc++'s threading support calls nanosleep(), which
# the toolchain's musl headers only expose when a feature-test macro is set.
EXTRAFLAGS  := -std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable \
               -D_GNU_SOURCE -DRAWUI_TITLE_ID=\"$(TITLE_ID)\"

# ------------------------------------------------------------- toolchain ----
TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)

ifeq ($(TOOLCHAIN),)
$(error OO_PS4_TOOLCHAIN is not set. Use `tools/docker-build.sh`, or point \
        OO_PS4_TOOLCHAIN at an extracted OpenOrbis-PS4-Toolchain release.)
endif

UNAME_S     := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CC      ?= clang
	CCX     ?= clang++
	CDIR    := linux
else
	CC      ?= clang
	CCX     ?= clang++
	CDIR    := macos
endif

# `LD` is a make built-in (it defaults to `ld`), so `?=` would silently keep GNU
# ld, which cannot handle the conditional ALIGN() expressions in link.x.  A
# `:=` assignment still lets the caller override with `make LD=...`.
LD          := ld.lld

TOOLBIN     := $(TOOLCHAIN)/bin/$(CDIR)

# PkgTool.Core is a .NET assembly; without this it aborts on hosts that do not
# ship a matching ICU package ("Couldn't find a valid ICU package").
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT := 1

# Detect libc++ v1 include directory (handles layout differences)
LIBCPP_V1   := $(TOOLCHAIN)/include/c++/v1
ifeq ($(wildcard $(LIBCPP_V1)/cstdint),)
  _CSTDINT := $(shell find $(TOOLCHAIN) -name cstdint 2>/dev/null | head -n 1)
  ifneq ($(_CSTDINT),)
    LIBCPP_V1 := $(dir $(_CSTDINT))
  endif
endif

CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -nostdinc \
               $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) \
               -isystem $(TOOLCHAIN)/include -I$(SRCDIR)
CXXFLAGS    := $(CFLAGS) -nostdinc++ -I$(LIBCPP_V1) -isystem $(LIBCPP_V1)
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr \
               -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

# ---------------------------------------------------------------- sources ---
CFILES      := $(shell find $(SRCDIR) -name '*.c' 2>/dev/null)
CPPFILES    := $(shell find $(SRCDIR) -name '*.cpp' 2>/dev/null)
OBJS        := $(patsubst $(SRCDIR)/%.c,$(INTDIR)/%.o,$(CFILES)) \
               $(patsubst $(SRCDIR)/%.cpp,$(INTDIR)/%.o,$(CPPFILES))

ELF         := $(BUILDDIR)/$(TITLE_ID).elf
OELF        := $(BUILDDIR)/$(TITLE_ID).oelf

# Package payload, listed relative to $(PKGDIR).
ASSETS      := $(patsubst $(PKGDIR)/%,%,$(shell find $(PKGDIR)/assets -type f 2>/dev/null))
MODULES     := $(addprefix sce_module/,$(addsuffix .prx,$(LIBMODULES)))
PKG_FILES   := eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo \
               sce_sys/icon0.png $(MODULES) $(ASSETS)

# ----------------------------------------------------------------- targets ---
.PHONY: all clean distclean assets info

all: $(DISTDIR)/$(CONTENT_ID).pkg

assets:
	bash tools/fetch-assets.sh

info:
	@echo "title      : $(TITLE)"
	@echo "title id   : $(TITLE_ID)"
	@echo "content id : $(CONTENT_ID)"
	@echo "toolchain  : $(TOOLCHAIN) [$(CDIR)]"
	@echo "sources    : $(words $(CPPFILES)) cpp / $(words $(CFILES)) c"

$(DISTDIR)/$(CONTENT_ID).pkg: $(PKGDIR)/$(CONTENT_ID).pkg
	@mkdir -p $(DISTDIR)
	cp $< $@
	@echo "==> $(DISTDIR)/$(CONTENT_ID).pkg"

$(PKGDIR)/$(CONTENT_ID).pkg: $(PKGDIR)/pkg.gp4
	cd $(PKGDIR) && $(TOOLBIN)/PkgTool.Core pkg_build pkg.gp4 .

$(PKGDIR)/pkg.gp4: $(PKGDIR)/eboot.bin $(PKGDIR)/sce_sys/param.sfo \
                   $(PKGDIR)/sce_sys/icon0.png $(PKGDIR)/sce_sys/about/right.sprx \
                   $(addprefix $(PKGDIR)/,$(MODULES)) $(shell find $(PKGDIR)/assets -type f 2>/dev/null)
	cd $(PKGDIR) && $(TOOLBIN)/create-gp4 -out pkg.gp4 \
	    --content-id=$(CONTENT_ID) --files "$(PKG_FILES)"

$(PKGDIR)/eboot.bin: $(ELF)
	$(TOOLBIN)/create-fself -in=$(ELF) -out=$(OELF) --eboot "$@" --paid 0x3800000000000011

$(ELF): $(OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(OBJS) -o $@ $(LDFLAGS)

$(INTDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CCX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $<

# ------------------------------------------------------------ package meta ---
$(PKGDIR)/sce_sys/param.sfo: Makefile
	@mkdir -p $(dir $@)
	$(TOOLBIN)/PkgTool.Core sfo_new $@
	$(TOOLBIN)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLBIN)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLBIN)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLBIN)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(TOOLBIN)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLBIN)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLBIN)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(TOOLBIN)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLBIN)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLBIN)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

$(PKGDIR)/sce_sys/about/right.sprx:
	@mkdir -p $(dir $@)
	cp $(TOOLCHAIN)/src/modules/right.sprx $@

$(PKGDIR)/sce_module/%.prx:
	@mkdir -p $(dir $@)
	cp $(TOOLCHAIN)/src/modules/$*.prx $@

# ----------------------------------------------------------------- cleanup ---
clean:
	rm -rf $(BUILDDIR) $(DISTDIR) $(PKGDIR)/pkg.gp4 $(PKGDIR)/eboot.bin \
	       $(PKGDIR)/sce_sys/param.sfo $(PKGDIR)/sce_sys/about \
	       $(PKGDIR)/sce_module $(PKGDIR)/$(CONTENT_ID).pkg

distclean: clean
	rm -f $(PKGDIR)/assets/fonts/*.ttf
