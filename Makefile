# SPDX-License-Identifier: 0BSD

# make predefines CC as "cc", so "?=" would never apply; override only that
# built-in default and keep a CC given on the command line or in the
# environment.
ifeq ($(origin CC),default)
CC = m68k-amigaos-gcc
endif
BUILD ?= build
TARGET = benchwork

VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null | sed 's/^v//' || echo unknown)
DATE := $(shell date '+%-d.%-m.%Y')

include compiler.mk

CFLAGS ?= $(OPT) $(CPUFLAGS)

# What the binary reports it was built with: taken now, before the
# per-directory additions below.
REPORTED_CFLAGS := $(CFLAGS)

INCLUDES = $(addprefix $(INCLUDE_FLAG),. src third_party/zlib third_party/libpng \
	$(EXTRA_INCLUDES))
ALL_CFLAGS = $(CFLAGS) $(LANG_FLAGS) $(INCLUDES) $(DEFINES)

HARNESS_SRCS = \
	src/main.c \
	src/bench.c \
	src/benchlist.c \
	src/timer.c \
	src/corpus.c \
	src/glyphs.c \
	benches/dhry.c \
	benches/scene.c \
	benches/lha.c \
	benches/zlib.c \
	benches/png.c \
	benches/ftgrays.c \
	benches/memcpy.c \
	benches/wipeout.c

DHRY_SRCS = \
	third_party/dhry/dhry_1.c \
	third_party/dhry/dhry_2.c

LHA_SRCS = \
	third_party/lha/bitio.c \
	third_party/lha/huf.c \
	third_party/lha/maketbl.c \
	third_party/lha/maketree.c \
	third_party/lha/slide.c \
	third_party/lha/lha_glue.c

ZLIB_SRCS = \
	third_party/zlib/adler32.c \
	third_party/zlib/crc32.c \
	third_party/zlib/deflate.c \
	third_party/zlib/inffast.c \
	third_party/zlib/inflate.c \
	third_party/zlib/inftrees.c \
	third_party/zlib/trees.c \
	third_party/zlib/zutil.c

LIBPNG_SRCS = \
	third_party/libpng/png.c \
	third_party/libpng/pngerror.c \
	third_party/libpng/pngget.c \
	third_party/libpng/pngmem.c \
	third_party/libpng/pngread.c \
	third_party/libpng/pngrio.c \
	third_party/libpng/pngrtran.c \
	third_party/libpng/pngrutil.c \
	third_party/libpng/pngset.c \
	third_party/libpng/pngtrans.c \
	third_party/libpng/pngwio.c \
	third_party/libpng/pngwrite.c \
	third_party/libpng/pngwtran.c \
	third_party/libpng/pngwutil.c

FREETYPE_SRCS = third_party/freetype/ftgrays.c

WIPEOUT_SRCS = third_party/wipeout/kernel.c

# SHA-512, GHASH, Poly1305 and the X25519 field arithmetic need long long, so
# compiler.mk only picks these up for compilers that have it.
AMISSL_LONGLONG_SRCS = \
	benches/sha512.c \
	benches/aesgcm.c \
	benches/chacha20poly1305.c \
	benches/x25519.c \
	third_party/amissl/sha512.c \
	third_party/amissl/aes_core.c \
	third_party/amissl/gcm128.c \
	third_party/amissl/chacha_enc.c \
	third_party/amissl/poly1305.c \
	third_party/amissl/curve25519.c

SRCS = $(HARNESS_SRCS) $(DHRY_SRCS) $(LHA_SRCS) $(ZLIB_SRCS) $(LIBPNG_SRCS) \
	$(FREETYPE_SRCS) $(WIPEOUT_SRCS) $(NFS3_SRCS) $(AMISSL_SRCS)
OBJS = $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS:%.c=$(BUILD)/%.o))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LINK)

$(BUILD)/src/%.o $(BUILD)/benches/%.o: CFLAGS += $(WARNINGS)

# LHa for UNIX is K&R C, which C23 rejects; the rest of the vendored code
# compiles at the compiler's default standard.
$(BUILD)/third_party/lha/%.o: CFLAGS += $(KNR_FLAGS)

# Z_SOLO: no gz file layer and no default allocators, which the benchmarks
# do not use (they bring their own); it also keeps zconf.h and zutil.h
# clear of the stdio and 64-bit literal that vbcc and SAS/C trip over.
$(BUILD)/third_party/zlib/%.o: CFLAGS += $(DEFINE_FLAG)Z_SOLO

$(BUILD)/third_party/libpng/%.o: CFLAGS += $(LIBPNG_FLAGS)

# ftgrays.c's stand-alone mode: no FreeType build system or headers needed.
$(BUILD)/third_party/freetype/%.o: CFLAGS += $(DEFINE_FLAG)STANDALONE_

# The game's own flags, which its port is built with.
$(BUILD)/third_party/wipeout/%.o: CFLAGS += $(WIPEOUT_FLAGS)

# For a compiler that cannot build kernel.c at OPT (compiler.mk says why).
# It replaces OPT rather than adding to CFLAGS: vc accepts a single -O.
ifneq ($(WIPEOUT_KERNEL_OPT),)
$(BUILD)/third_party/wipeout/kernel.o: override OPT = $(WIPEOUT_KERNEL_OPT)
endif

# AmiSSL's AmigaOS 3 configuration defines B_ENDIAN; the shim stands in for
# the OpenSSL headers the AmiSSL sources include.
$(BUILD)/third_party/amissl/%.o: CFLAGS += $(DEFINE_FLAG)B_ENDIAN \
	$(INCLUDE_FLAG)third_party/amissl/shim
$(BUILD)/benches/sha512.o $(BUILD)/benches/aesgcm.o \
$(BUILD)/benches/chacha20poly1305.o $(BUILD)/benches/x25519.o: \
	CFLAGS += $(INCLUDE_FLAG)third_party/amissl/shim

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(COMPILE)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(COMPILE_CXX)

# Regenerate the glyph tables with the host FreeType.
tools/dumpglyphs: tools/dumpglyphs.c
	cc -O2 -Wall -o $@ $< $$(pkg-config --cflags --libs freetype2)

FONT ?= /usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf

glyphs: tools/dumpglyphs
	tools/dumpglyphs $(FONT) > src/glyphs.c

# The release binaries: 000 and 020 soft float, as a program shipped for
# those machines would be; 020-060fp for any 68020 or later with an FPU, the
# build most FPU programs ship as; 040 and 060 tuned for those machines, to
# measure on real hardware what that buys.
RELEASE_CPUS = 000 020 020-060fp 040 060

release-binaries: $(RELEASE_CPUS:%=release-%)

$(RELEASE_CPUS:%=release-%): release-%:
	$(MAKE) BUILD=$(BUILD)-$* TARGET=$(TARGET)-$* CPUFLAGS="$(CPUFLAGS_$*)" \
		LIBS="$(or $(LIBS_$*),$(LIBS))"

# Shortcuts for the vbcc build: benchwork-vbcc, and its release binaries.
# Other variables (CPUFLAGS, OPT) pass through.
vbcc:
	$(MAKE) CC="vc +aos68k"

vbcc-release:
	$(MAKE) CC="vc +aos68k" release-binaries

# The same for SAS/C; SC=sc-volamos runs it under volamos.
SC ?= sc

sasc:
	$(MAKE) CC=$(SC)

sasc-release:
	$(MAKE) CC=$(SC) release-binaries

# The release: the binaries, and the Aminet upload made of them.
# benchwork.lha holds a Benchwork drawer with the binaries, the readme and
# the license texts; benchwork.readme also goes beside it. The source stays
# out to keep the archive small: the readme links the tagged tree instead.
# The readme says gcc, so the vbcc and SAS/C builds above stop at the
# binaries.
AMINET_DIR = build-aminet/Benchwork

release: release-binaries
	rm -rf build-aminet benchwork.lha benchwork.readme
	mkdir -p $(AMINET_DIR)
	cp $(RELEASE_CPUS:%=$(TARGET)-%) LICENSE $(AMINET_DIR)/
	cp --parents third_party/LICENSE.md third_party/*/LICENSE $(AMINET_DIR)/
	sed 's/@VERSION@/$(VERSION)/' aminet/benchwork.readme.in > benchwork.readme
	cp benchwork.readme $(AMINET_DIR)/
	cd build-aminet && lha aq ../benchwork.lha Benchwork

clean:
	rm -rf $(BUILD) $(TARGET) $(RELEASE_CPUS:%=$(BUILD)-%) $(RELEASE_CPUS:%=$(TARGET)-%)
	rm -rf build-aminet benchwork.lha benchwork.readme

# Lint the scripts in tools/: shellcheck for the shell ones, ruff for the
# Python ones. Only the files listed in RUFF_FORMATTED follow ruff's
# formatting; the others keep their own. The C sources have no checker yet.
RUFF_FORMATTED = tools/results-splice.py tools/results-sizes.py \
	tools/results-compilers-fixup.py

check:
	shellcheck tools/*.sh
	ruff check tools/*.py
	ruff format --check $(RUFF_FORMATTED)

-include $(OBJS:.o=.d)

.PHONY: all check clean glyphs release release-binaries $(RELEASE_CPUS:%=release-%) vbcc vbcc-release \
	sasc sasc-release
