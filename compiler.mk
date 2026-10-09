# SPDX-License-Identifier: 0BSD
#
# Per-compiler build settings, chosen by the name of CC. Each branch sets
# the variables it needs and the others stay empty:
#
#   CPUFLAGS, OPT     the defaults a comparison usually overrides
#   LANG_FLAGS        dialect and code-model flags for every object
#   INCLUDE_FLAG, DEFINE_FLAG, EXTRA_INCLUDES   how to spell -I and -D
#   DEFINES           what the binary reports it was built with
#   LIBS, LIBS_040    the libraries to link, and the FPU build's
#   CPUFLAGS_000/020/040   the release binaries' CPU settings
#   COMPILE, LINK     the recipes for one object and for the binary
#   plus the per-directory flags the Makefile's pattern rules add.

# vbcc from the same toolchain: make CC="vc +aos68k" (vc, vbccm68k, vasm and
# vlink on PATH). vc rejects every flag it does not know.
ifeq ($(notdir $(firstword $(CC))),vc)

    BUILD := $(BUILD)-vbcc
    TARGET := $(TARGET)-vbcc
    # vc's -O2 is the full optimizer; -speed adds unrolling and inlining.
    CPUFLAGS ?= -cpu=68020
    OPT ?= -O2
    CPUFLAGS_000 = -cpu=68000
    CPUFLAGS_020 = -cpu=68020
    CPUFLAGS_040 = -cpu=68040 -fpu=68040
    LANG_FLAGS = -c99
    INCLUDE_FLAG = -I
    DEFINE_FLAG = -D
    # vbcc's C library has no sys/types.h and no gcc-style dependency output.
    NDK_INCLUDE = $(dir $(shell command -v vc))../m68k-amigaos/ndk-include
    EXTRA_INCLUDES = compat $(NDK_INCLUDE)
    # vc keeps every argument as one word, so a string with spaces needs the
    # shell quoting that vbccm68k will see. vbcc has no version macro; the
    # name comes from its banner.
    VBCC_VERSION = $(shell vbccm68k -o=/dev/null /dev/null | sed -n '1s/^vbcc \(V[^ ]*\( pre\)\{0,1\}\).*/vbcc \1/p')
    DEFINES = -DBENCH_CFLAGS='"\"$(REPORTED_CFLAGS)\""' \
        -DBENCH_VERSION='"\"$(VERSION)\""' -DBENCH_DATE='"\"$(DATE)\""' \
        -DBENCH_COMPILER='"\"$(VBCC_VERSION)\""'
    # amiga.lib for the exec and timer calls, which vbcc's NDK headers do
    # not inline; mieee.lib is the soft-float math library, m040.lib the
    # FPU one.
    LIBS = -lmieee -lamiga
    LIBS_040 = -lm040 -lamiga
    COMPILE = $(CC) $(ALL_CFLAGS) -c -o $@ $<
    LINK = $(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# SAS/C 6.58: make CC=sc, or CC=sc-volamos to run it under volamos.
else ifneq (,$(filter sc sc-%,$(notdir $(firstword $(CC)))))

    BUILD := $(BUILD)-sasc
    TARGET := $(TARGET)-sasc
    # The linker is slink, named after CC. sc LINK is not used: it runs
    # slink itself, which does not work under volamos.
    LD = $(subst sc,slink,$(CC))
    CPUFLAGS ?= CPU=68020 MATH=IEEE
    OPT ?= OPTIMIZE OPTIMIZERTIME OPTIMIZERINLINELOCAL OPTIMIZERSCHEDULER \
        OPTIMIZERCOMPLEXITY=8 OPTIMIZERDEPTH=8 OPTIMIZERRECURDEPTH=8
    CPUFLAGS_000 = CPU=68000 MATH=IEEE
    CPUFLAGS_020 = CPU=68020 MATH=IEEE
    # sc has no MATH=68040; CPU=68040 with MATH=68881 keeps the compiler
    # to the FPU instructions the 68040 implements, and scm040.lib supplies
    # the transcendentals in software.
    CPUFLAGS_040 = CPU=68040 MATH=68881
    # Register parameters and merged const strings; far code and data
    # because the program and its static tables exceed the near sections;
    # utility.library multiply and divide on the 68000, as libnix does.
    LANG_FLAGS = PARAMETERS=REGISTERS CODE=FAR DATA=FAR STRINGMERGE \
        STRINGSCONST UTILITYLIBRARY NOSTACKCHECK NOMULTIPLEINCLUDES NOICONS \
        NOVERSION
    INCLUDE_FLAG = INCLUDEDIRECTORY=
    DEFINE_FLAG = DEFINE=
    # It has no <stdint.h> and no <sys/times.h>.
    EXTRA_INCLUDES = compat compat/sasc
    # sc keeps the quotes of a DEFINE value, so these need no escaping.
    DEFINES = DEFINE=BENCH_CFLAGS='"$(REPORTED_CFLAGS)"' \
        DEFINE=BENCH_VERSION='"$(VERSION)"' DEFINE=BENCH_DATE='"$(DATE)"'
    # The soft-float math library goes with MATH=IEEE; scm040.lib is the
    # MATH=68881 library without the instructions the 68040 FPU lacks.
    LIBS = LIB:scmieee.lib LIB:sc.lib LIB:amiga.lib
    LIBS_040 = LIB:scm040.lib LIB:sc.lib LIB:amiga.lib
    # Its preprocessor cannot build libpng's interlace masks from their
    # macros; libpng can compute them at run time instead, and the
    # benchmark's image is not interlaced anyway.
    LIBPNG_FLAGS = DEFINE=PNG_USE_COMPILE_TIME_MASKS=0
    COMPILE = $(CC) $(ALL_CFLAGS) NOLINK OBJECTNAME=$@ $<
    LINK = $(LD) FROM LIB:c.o $(OBJS) TO $@ LIB $(LIBS)

# DICE-nx: make CC=dcc, or CC=dcc-volamos to run it under volamos.
else ifneq (,$(filter dcc dcc-%,$(notdir $(firstword $(CC)))))

    BUILD := $(BUILD)-dice
    TARGET := $(TARGET)-dice
    # dc1 always optimizes; it has no levels to choose from.
    CPUFLAGS ?= -020
    OPT ?=
    CPUFLAGS_000 =
    CPUFLAGS_020 = -020
    # No 68040 code generation and no usable FPU code (its -881 is
    # documented as unfinished), so the 040 binary is the 020 one.
    CPUFLAGS_040 = -020
    # The OS 3.2 headers and amiga.lib, and C++ comments. The small data
    # model throughout, since the libraries that auto-open the math and OS
    # libraries exist only for it, except for the Wipeout kernel, whose
    # texture table alone fills most of the 64 KB it reaches.
    LANG_FLAGS = -3.2 -//
    WIPEOUT_FLAGS = -mD
    INCLUDE_FLAG = -I
    DEFINE_FLAG = -D
    # dcpp looks up quoted includes on the include path only, not beside
    # the including file.
    EXTRA_INCLUDES = compat third_party/freetype third_party/wipeout $(BUILD)
    # dcpp does not rescan the result of ##, which libpng's chunk tables
    # need: https://github.com/dice-nx/dice-nx/issues/31
    NO_LIBPNG = 1
    # dcc drops the quoting when it hands a -D on to dcpp, so a string
    # with spaces goes through a generated header instead.
    DEFINES = -DBENCH_NO_LIBPNG -DBENCH_DEFINES_H
    GENERATED_DEFINES = $(BUILD)/bench-defines.h
    DICE_VERSION = $(shell $(CC) 2>&1 | sed -n '1s/^DCC \([0-9.]*\).*/DICE-nx \1/p')
    LIBS = -lm
    COMPILE = $(CC) $(ALL_CFLAGS) -c -o $@ $<
    LINK = $(CC) $(CFLAGS) $(LANG_FLAGS) -o $@ $(OBJS) $(LIBS)

else  # AmigaDev GCC 16.2 or newer

    CPUFLAGS ?= -m68020-60
    OPT ?= -O2 -fomit-frame-pointer
    CPUFLAGS_000 = -m68000
    CPUFLAGS_020 = -m68020
    # -m68881 rather than its synonym -mhard-float: only the former selects
    # the hard-float multilib, and with the soft-float libm the FPU caller
    # reads log() and exp() results from the wrong registers.
    CPUFLAGS_040 = -m68040 -m68881
    INCLUDE_FLAG = -I
    DEFINE_FLAG = -D
    KNR_FLAGS = -std=gnu11
    WIPEOUT_FLAGS = -std=gnu99 -fno-strict-aliasing
    LIBS = -lm
    # The harness is held to a stricter standard than the code it measures,
    # which is compiled the way its own build systems compile it.
    WARNINGS = -Wall -Wextra -Wshadow -Wpointer-arith -Wwrite-strings \
        -Wstrict-prototypes -Wmissing-prototypes -Wvla
    DEFINES = -DBENCH_CFLAGS='"$(REPORTED_CFLAGS)"' \
        -DBENCH_VERSION='"$(VERSION)"' -DBENCH_DATE='"$(DATE)"'
    COMPILE = $(CC) $(ALL_CFLAGS) -MMD -MP -c -o $@ $<
    LINK = $(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

endif
