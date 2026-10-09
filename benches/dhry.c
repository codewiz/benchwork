// SPDX-License-Identifier: 0BSD
//
// Dhrystone 2.1, the classic integer benchmark, in the form xSysInfo uses:
// the measurement loop is Dhry_Run() and the timing is ours. Dhrystone is
// synthetic, but it is the number every Amiga owner already knows for their
// machine, so a compiler that moves it is immediately noticed.
//
// Dhrystone's ground rules ask for separate compilation of dhry_1.c and
// dhry_2.c and no procedure merging; the Makefile builds them as separate
// objects and leaves LTO off.

#include "bench.h"
#include "third_party/dhry/dhry.h"

// dhry_1.c's globals, which dhry.h does not declare.
extern Rec_Pointer Ptr_Glob;
extern int Int_Glob;
extern char Ch_1_Glob, Ch_2_Glob;
extern int Arr_2_Glob[50][50];

// Iterations of the inner loop per timed run. About a second on a 25 MHz
// 68040.
#define RUNS 20000UL

static bool dhry_setup(void) {
    return Dhry_Initialize() != 0;
}

// The globals Dhrystone leaves behind are a function of the run count, so
// they double as the check value. Proc_8() accumulates into Arr_2_Glob, so
// the records and arrays are reset before every run; that is a memset of
// 10 KiB against a second of work.
static bool dhry_run(ULONG *check) {
    if (!Dhry_Initialize())
        return false;
    Dhry_Run(RUNS);
    *check = checksum(0, &Int_Glob, sizeof(Int_Glob));
    *check = checksum(*check, &Ch_1_Glob, sizeof(Ch_1_Glob));
    *check = checksum(*check, &Ch_2_Glob, sizeof(Ch_2_Glob));
    *check = checksum(*check, Arr_2_Glob, sizeof(Arr_2_Glob));
    *check = checksum(*check, Ptr_Glob->variant.var_1.Str_Comp,
                      sizeof(Ptr_Glob->variant.var_1.Str_Comp));
    return true;
}

const struct bench bench_dhry = {
    "dhrystone",
    "Dhrystone 2.1, 20000 runs",
    dhry_setup,
    dhry_run,
    NULL,
};
