# SPDX-License-Identifier: 0BSD

CC ?= m68k-amigaos-gcc
BUILD ?= build
TARGET = benchwork

CPUFLAGS ?= -m68020-60
OPT ?= -O2 -fomit-frame-pointer
CFLAGS ?= $(OPT) $(CPUFLAGS)

# What the binary reports it was built with: taken now, before the
# per-directory additions below.
REPORTED_CFLAGS := $(CFLAGS)

# The harness is held to a stricter standard than the code it measures,
# which is compiled the way its own build systems compile it.
WARNINGS = -Wall -Wextra -Wshadow -Wpointer-arith -Wwrite-strings \
	-Wstrict-prototypes -Wmissing-prototypes -Wvla

INCLUDES = -Isrc -Ithird_party/zlib -Ithird_party/libpng

ALL_CFLAGS = $(CFLAGS) -noixemul $(INCLUDES) -DBENCH_CFLAGS='"$(REPORTED_CFLAGS)"'

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
	benches/memcpy.c

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

SRCS = $(HARNESS_SRCS) $(DHRY_SRCS) $(LHA_SRCS) $(ZLIB_SRCS) $(LIBPNG_SRCS) \
	$(FREETYPE_SRCS)
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

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(ALL_CFLAGS) -MMD -MP -c -o $@ $<

# Regenerate the glyph tables with the host FreeType.
tools/dumpglyphs: tools/dumpglyphs.c
	cc -O2 -Wall -o $@ $< $$(pkg-config --cflags --libs freetype2)

FONT ?= /usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf

glyphs: tools/dumpglyphs
	tools/dumpglyphs $(FONT) > src/glyphs.c

clean:
	rm -rf $(BUILD) $(TARGET)

-include $(OBJS:.o=.d)

.PHONY: all clean glyphs
