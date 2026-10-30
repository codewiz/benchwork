# SPDX-License-Identifier: 0BSD
#
# Per-compiler build settings, chosen by the name of CC. Each branch sets
# the variables it needs and the others stay empty:
#
#   CPUFLAGS, OPT     the defaults a comparison usually overrides
#   LANG_FLAGS        dialect and code-model flags for every object
#   INCLUDE_FLAG, DEFINE_FLAG, EXTRA_INCLUDES   how to spell -I and -D
#   DEFINES           what the binary reports it was built with
#   LIBS, LIBS_020-060fp/040/060   the libraries to link, and the FPU builds'
#   CPUFLAGS_000/020/020-060fp/040/060   the release binaries' CPU settings
#   COMPILE, LINK     the recipes for one object and for the binary
#   NFS3_SRCS, COMPILE_CXX   the C++ benchmark and its recipe, gcc only
#   AMISSL_SRCS       the AmiSSL benchmarks, for compilers with long long
#   plus the per-directory flags the Makefile's pattern rules add.

# vbcc from the same toolchain: make CC="vc +aos68k" (vc, vbccm68k, vasm and
# vlink on PATH). vc rejects every flag it does not know.
ifeq ($(notdir $(firstword $(CC))),vc)

    BUILD := $(BUILD)-vbcc
    TARGET := $(TARGET)-vbcc
    # Inlining, unrolling and the interprocedural optimizations only come
    # with vc's -O3 and up. -speed adds none; it only prefers speed where
    # vbcc has to trade it against size.
    CPUFLAGS ?= -cpu=68020
    OPT ?= -O3
    # vbcc's interprocedural alias analysis (-O3 and up) crashes on wipeout's
    # kernel.c, so that one file is built at -O2.
    WIPEOUT_KERNEL_OPT = -O2
    CPUFLAGS_000 = -cpu=68000
    CPUFLAGS_020 = -cpu=68020
    # vbcc has nothing like gcc's -m68020-60: this is plain 68020 code.
    CPUFLAGS_020-060fp = -cpu=68020 -fpu=68881
    CPUFLAGS_040 = -cpu=68040 -fpu=68040
    CPUFLAGS_060 = -cpu=68060 -fpu=68060
    LANG_FLAGS = -c99
    INCLUDE_FLAG = -I
    DEFINE_FLAG = -D
    # vbcc's C library has no sys/types.h and no gcc-style dependency output.
    # The rest of the toolchain is found next to vc, on PATH or not.
    VBCC_BIN = $(dir $(shell command -v $(firstword $(CC))))
    NDK_INCLUDE = $(VBCC_BIN)../m68k-amigaos/ndk-include
    EXTRA_INCLUDES = compat $(NDK_INCLUDE)
    # vc keeps every argument as one word, so a string with spaces needs the
    # shell quoting that vbccm68k will see. vbcc has no version macro; the
    # name comes from its banner.
    VBCC_VERSION = $(shell $(VBCC_BIN)vbccm68k -o=/dev/null /dev/null | sed -n '1s/^vbcc \(V[^ ]*\( pre\)\{0,1\}\).*/vbcc \1/p')
    DEFINES = -DBENCH_CFLAGS='"\"$(REPORTED_CFLAGS)\""' \
        -DBENCH_VERSION='"\"$(VERSION)\""' -DBENCH_DATE='"\"$(DATE)\""' \
        -DBENCH_COMPILER='"\"$(VBCC_VERSION)\""' -DBENCH_HAVE_AMISSL
    AMISSL_SRCS = $(AMISSL_LONGLONG_SRCS)
    # amiga.lib for the exec and timer calls, which vbcc's NDK headers do
    # not inline; mieee.lib is the soft-float math library, m881.lib,
    # m040.lib and m060.lib the FPU ones.
    LIBS = -lmieee -lamiga
    LIBS_020-060fp = -lm881 -lamiga
    LIBS_040 = -lm040 -lamiga
    LIBS_060 = -lm060 -lamiga
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
    # Nor does sc have a 68020-to-68060 blend: this is plain 68020 code.
    CPUFLAGS_020-060fp = CPU=68020 MATH=68881
    # sc has no MATH=68040; CPU=68040 with MATH=68881 keeps the compiler
    # to the FPU instructions the 68040 implements, and scm040.lib supplies
    # the transcendentals in software. The 68060 lacks the same ones.
    CPUFLAGS_040 = CPU=68040 MATH=68881
    CPUFLAGS_060 = CPU=68060 MATH=68881
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
    # MATH=68881 library without the instructions the 68040 and 68060 FPUs
    # lack.
    LIBS = LIB:scmieee.lib LIB:sc.lib LIB:amiga.lib
    LIBS_020-060fp = LIB:scm881.lib LIB:sc.lib LIB:amiga.lib
    LIBS_040 = LIB:scm040.lib LIB:sc.lib LIB:amiga.lib
    LIBS_060 = $(LIBS_040)
    # Its preprocessor cannot build libpng's interlace masks from their
    # macros; libpng can compute them at run time instead, and the
    # benchmark's image is not interlaced anyway.
    LIBPNG_FLAGS = DEFINE=PNG_USE_COMPILE_TIME_MASKS=0
    COMPILE = $(CC) $(ALL_CFLAGS) NOLINK OBJECTNAME=$@ $<
    LINK = $(LD) FROM LIB:c.o $(OBJS) TO $@ LIB $(LIBS)

else  # AmigaDev GCC 16.2 or newer

    CPUFLAGS ?= -m68020-60
    OPT ?= -O2 -fomit-frame-pointer
    CPUFLAGS_000 = -m68000
    CPUFLAGS_020 = -m68020
    # -m68020-60 leaves out the integer instructions the 68060 traps on.
    CPUFLAGS_020-060fp = -m68020-60 -m68881
    # -m68881 rather than its synonym -mhard-float: only the former selects
    # the hard-float multilib, and with the soft-float libm the FPU caller
    # reads log() and exp() results from the wrong registers.
    CPUFLAGS_040 = -m68040 -m68881
    CPUFLAGS_060 = -m68060 -m68881
    INCLUDE_FLAG = -I
    DEFINE_FLAG = -D
    # Strict C11, the newest standard gcc 6.5 knows: no GNU extensions.
    # ISO modes would also round every double assignment through memory on
    # a 68881, which computes in extended precision; -fexcess-precision=fast
    # keeps the code GNU mode makes.
    LANG_FLAGS = -std=c11 -fexcess-precision=fast
    # It also drops the non-reserved AMIGA that zutil.h picks its gzip
    # OS_CODE by.
    ZLIB_FLAGS = -DAMIGA
    WIPEOUT_FLAGS = -std=gnu99 -fno-strict-aliasing
    # Dhrystone splits its procedures between two files so that they are not
    # inlined into the loop; LTO would inline them anyway.
    DHRY_FLAGS = -fno-lto
    LIBS = -lm
    # The harness is held to a stricter standard than the code it measures,
    # which is compiled the way its own build systems compile it.
    WARNINGS = -Wall -Wextra -Wshadow -Wpointer-arith -Wwrite-strings \
        -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wformat=2 \
        -Wcast-align -Wold-style-definition -Wmissing-declarations \
        -Wnull-dereference -Wduplicated-cond -Wlogical-op -Wjump-misses-init
    DEFINES = -DBENCH_CFLAGS='"$(REPORTED_CFLAGS)"' \
        -DBENCH_VERSION='"$(VERSION)"' -DBENCH_DATE='"$(DATE)"' \
        -DBENCH_HAVE_NFS3 -DBENCH_HAVE_AMISSL
    AMISSL_SRCS = $(AMISSL_LONGLONG_SRCS)
    COMPILE = $(CC) $(ALL_CFLAGS) -MMD -MP -c -o $@ $<
    LINK = $(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

    # The NFS3 kernels are C++, so only this branch builds them. Without
    # exceptions, RTTI or thread-safe statics they need nothing from
    # libstdc++, and the C link above takes them as they are.
    NFS3_SRCS = benches/nfs3.c third_party/nfs3/kernel.cpp
    CXX = $(patsubst %gcc,%g++,$(CC))
    CXX_FLAGS = -fno-exceptions -fno-rtti -fno-threadsafe-statics
    COMPILE_CXX = $(CXX) $(CFLAGS) $(CXX_FLAGS) $(INCLUDES) $(DEFINES) \
        -MMD -MP -c -o $@ $<

endif
