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
REPORTED_CFLAGS := $(strip $(CFLAGS))

INCLUDES = $(addprefix $(INCLUDE_FLAG),. src third_party/zlib third_party/libpng \
	$(EXTRA_INCLUDES))
ALL_CFLAGS = $(CFLAGS) $(LANG_FLAGS) $(INCLUDES) $(DEFINES)

HARNESS_SRCS = \
	src/main.c \
	src/timer.c \
	src/corpus.c \
	src/glyphs.c \
	benches/dhry.c \
	benches/backdrop.c \
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

# NO_LIBPNG (set by compiler.mk for a compiler that cannot build libpng)
# leaves it out; png-encode and png-decode then fail their setup.
SRCS = $(HARNESS_SRCS) $(DHRY_SRCS) $(LHA_SRCS) $(ZLIB_SRCS) \
	$(if $(NO_LIBPNG),,$(LIBPNG_SRCS)) $(FREETYPE_SRCS) $(WIPEOUT_SRCS)
OBJS = $(SRCS:%.c=$(BUILD)/%.o)

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

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(COMPILE)

# The BENCH_* strings as a header, for compilers that set GENERATED_DEFINES.
ifdef GENERATED_DEFINES
$(BUILD)/src/main.o: $(GENERATED_DEFINES)

$(GENERATED_DEFINES):
	@mkdir -p $(dir $@)
	printf '#define BENCH_CFLAGS "%s"\n#define BENCH_VERSION "%s"\n#define BENCH_DATE "%s"\n#define BENCH_COMPILER "%s"\n' \
		'$(REPORTED_CFLAGS)' '$(VERSION)' '$(DATE)' '$(DICE_VERSION)' > $@
endif

# Regenerate the glyph tables with the host FreeType.
tools/dumpglyphs: tools/dumpglyphs.c
	cc -O2 -Wall -o $@ $< $$(pkg-config --cflags --libs freetype2)

FONT ?= /usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf

glyphs: tools/dumpglyphs
	tools/dumpglyphs $(FONT) > src/glyphs.c

# The release binaries: one per CPU generation, named after it. The 68040 one
# uses the FPU; the others are soft float, as a program shipped for those
# machines would be.
RELEASE_CPUS = 000 020 040

release: $(RELEASE_CPUS:%=release-%)

$(RELEASE_CPUS:%=release-%): release-%:
	$(MAKE) BUILD=$(BUILD)-$* TARGET=$(TARGET)-$* CPUFLAGS="$(CPUFLAGS_$*)" \
		LIBS="$(or $(LIBS_$*),$(LIBS))"

# Shortcuts for the vbcc build: benchwork-vbcc, and its three release
# binaries. Other variables (CPUFLAGS, OPT) pass through.
vbcc:
	$(MAKE) CC="vc +aos68k"

vbcc-release:
	$(MAKE) CC="vc +aos68k" release

# The same for SAS/C; SC=sc-volamos runs it under volamos.
SC ?= sc

sasc:
	$(MAKE) CC=$(SC)

sasc-release:
	$(MAKE) CC=$(SC) release

clean:
	rm -rf $(BUILD) $(TARGET) $(RELEASE_CPUS:%=$(BUILD)-%) $(RELEASE_CPUS:%=$(TARGET)-%)

-include $(OBJS:.o=.d)

.PHONY: all clean glyphs release $(RELEASE_CPUS:%=release-%) vbcc vbcc-release \
	sasc sasc-release
