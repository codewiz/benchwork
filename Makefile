# SPDX-License-Identifier: 0BSD

# make predefines CC as "cc", so "?=" would never apply; override only that
# built-in default and keep a CC given on the command line or in the
# environment.
ifeq ($(origin CC),default)
CC = m68k-amigaos-gcc
endif
BUILD ?= build
TARGET = benchwork

CPUFLAGS ?= -m68020-60
OPT ?= -O2 -fomit-frame-pointer
CFLAGS ?= $(OPT) $(CPUFLAGS)

# What the binary reports it was built with: taken now, before the
# per-directory additions below.
REPORTED_CFLAGS := $(CFLAGS)
VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo unknown)

# The harness is held to a stricter standard than the code it measures,
# which is compiled the way its own build systems compile it.
WARNINGS = -Wall -Wextra -Wshadow -Wpointer-arith -Wwrite-strings \
	-Wstrict-prototypes -Wmissing-prototypes -Wvla

INCLUDES = -Isrc -Ithird_party/zlib -Ithird_party/libpng

ALL_CFLAGS = $(CFLAGS) -noixemul $(INCLUDES) -DBENCH_CFLAGS='"$(REPORTED_CFLAGS)"' \
	-DBENCH_VERSION='"$(VERSION)"'

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

# newlib's C versions of the string functions the timed code calls out of
# line, linked ahead of the C library so the calls land in code the
# compiler under test generated: libnix's memcpy hands off to exec CopyMem,
# and the rest of libnix.a was compiled by whatever built the toolchain.
# The functions libnix's string.h inlines (memmove, memcmp, strcpy, strlen)
# stay as they are: callers get the best implementation the headers offer.
NEWLIB_SRCS = \
	third_party/newlib/memcpy.c \
	third_party/newlib/memset.c \
	third_party/newlib/strcmp.c

WIPEOUT_SRCS = third_party/wipeout/kernel.c

SRCS = $(HARNESS_SRCS) $(DHRY_SRCS) $(LHA_SRCS) $(ZLIB_SRCS) $(LIBPNG_SRCS) \
	$(FREETYPE_SRCS) $(NEWLIB_SRCS) $(WIPEOUT_SRCS)
OBJS = $(SRCS:%.c=$(BUILD)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -noixemul -o $@ $(OBJS) -lm

$(BUILD)/src/%.o $(BUILD)/benches/%.o: CFLAGS += $(WARNINGS)

# LHa for UNIX is K&R C, which C23 rejects; the rest of the vendored code
# compiles at the compiler's default standard.
$(BUILD)/third_party/lha/%.o: CFLAGS += -std=gnu11

# ftgrays.c's stand-alone mode: no FreeType build system or headers needed.
$(BUILD)/third_party/freetype/%.o: CFLAGS += -DSTANDALONE_

# A function named memcpy must not have its loop turned into a memcpy call.
$(BUILD)/third_party/newlib/%.o: CFLAGS += -fno-builtin -Ithird_party/newlib

# The game's own flags, which its port is built with.
$(BUILD)/third_party/wipeout/%.o: CFLAGS += -std=gnu99 -fno-strict-aliasing

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(ALL_CFLAGS) -MMD -MP -c -o $@ $<

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
CPUFLAGS_000 = -m68000
CPUFLAGS_020 = -m68020
CPUFLAGS_040 = -m68040 -mhard-float

release: $(RELEASE_CPUS:%=release-%)

$(RELEASE_CPUS:%=release-%): release-%:
	$(MAKE) BUILD=build-$* TARGET=benchwork-$* CPUFLAGS="$(CPUFLAGS_$*)"

clean:
	rm -rf $(BUILD) $(TARGET) $(RELEASE_CPUS:%=build-%) $(RELEASE_CPUS:%=benchwork-%)

-include $(OBJS:.o=.d)

.PHONY: all clean glyphs release $(RELEASE_CPUS:%=release-%)
