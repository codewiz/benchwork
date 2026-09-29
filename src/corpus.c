// SPDX-License-Identifier: 0BSD
//
// The shared input for the compressors: a quarter megabyte of synthetic English
// prose, generated rather than bundled so the repository stays small and
// every run compresses byte-identical data.
//
// Text is the honest choice for LZ compressors: it has the short repeats,
// skewed byte frequencies and mid-length matches that their hash chains and
// Huffman coders were designed around. A word list this short compresses
// better than real prose (roughly 4:1 for both zlib and lha), which if
// anything spends more of the time in the match-finding paths that the
// compilers have to get right.

#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>

#include "bench.h"
#include "corpus.h"

static const char *const WORDS[] = {
    "the", "of", "and", "to", "in", "a", "is", "that", "for", "it", "as",
    "was", "with", "be", "by", "on", "not", "he", "this", "are", "or", "his",
    "from", "at", "which", "but", "have", "an", "had", "they", "you", "were",
    "their", "one", "all", "we", "can", "her", "has", "there", "been", "if",
    "more", "when", "will", "would", "who", "so", "no", "she", "other", "its",
    "may", "these", "what", "them", "than", "some", "him", "time", "into",
    "only", "could", "new", "also", "people", "after", "first", "two", "year",
    "over", "work", "then", "now", "any", "such", "where", "most", "through",
    "before", "should", "well", "must", "because", "between", "under", "while",
    "still", "never", "again", "world", "system", "program", "memory", "disk",
    "screen", "window", "device", "library", "function", "value", "address",
    "register", "processor", "compiler", "instruction", "buffer", "file",
    "directory", "command", "shell", "kernel", "driver", "interrupt", "signal",
    "message", "port", "task", "process", "thread", "stack", "heap", "cache",
    "bus", "clock", "cycle", "byte", "word", "long", "pointer", "array",
    "string", "number", "table", "list", "tree", "node", "graph", "edge",
    "search", "sort", "merge", "hash", "key", "lock", "queue", "block",
    "sector", "track", "head", "controller", "board", "chip", "custom",
    "blitter", "copper", "sprite", "bitplane", "palette", "pixel", "line",
    "rectangle", "circle", "area", "layer", "region", "clip", "mask", "shift",
    "rotate", "scale", "draw", "fill", "copy", "move", "read", "write",
    "open", "close", "load", "save", "print", "format", "parse", "check",
    "test", "debug", "trace", "build", "link", "run", "stop", "wait",
    "amiga", "workbench", "kickstart", "exec", "intuition", "graphics",
    "hardware", "software", "version", "release", "update", "patch", "error",
    "warning", "result", "input", "output", "source", "target", "object",
    "module", "section", "segment", "symbol", "reference", "relocation",
    "hunk", "loader", "assembler", "linker", "archive", "compress", "expand",
    "encode", "decode", "filter", "stream", "packet", "frame", "field",
    "image", "sound", "sample", "channel", "volume", "period", "rate",
};

#define NWORDS (sizeof(WORDS) / sizeof(WORDS[0]))

// Park-Miller minimal standard generator: portable, and small enough that
// its output does not depend on the C library's rand().
static ULONG seed = 12345;

static ULONG next(ULONG limit) {
    seed = (ULONG)(((unsigned long long)seed * 48271) % 2147483647UL);
    return seed % limit;
}

// Append s at *p, never past end. Returns the new cursor.
static UBYTE *put(UBYTE *p, UBYTE *end, const char *s) {
    while (*s && p < end)
        *p++ = (UBYTE)*s++;
    return p;
}

// Allocate and fill CORPUS_SIZE bytes of prose: sentences of 4 to 16 words,
// capitalized, one number in about every twenty words, lines wrapped near
// 72 columns and a paragraph break every 6 to 14 sentences. Returns NULL
// when out of memory; the caller frees it with FreeVec().
UBYTE *corpus_alloc(void) {
    UBYTE *buf = bench_alloc(CORPUS_SIZE, "text corpus");
    UBYTE *p = buf, *end = buf + CORPUS_SIZE;
    int column = 0, sentences = 0;

    if (!buf)
        return NULL;
    seed = 12345;
    while (p < end) {
        int words = 4 + (int)next(13);
        int in_paragraph = 6 + (int)next(9);

        for (int i = 0; i < words && p < end; i++) {
            char num[12];
            const char *w = WORDS[next(NWORDS)];

            if (next(20) == 0) {
                ULONG n = next(100000);
                char *q = num + sizeof(num) - 1;

                *q = 0;
                do {
                    *--q = (char)('0' + n % 10);
                    n /= 10;
                } while (n);
                w = q;
            }
            if (column + (int)strlen(w) > 72) {
                *p++ = '\n';
                column = 0;
            } else if (column) {
                *p++ = ' ';
                column++;
            }
            if (i == 0 && p < end && *w >= 'a' && *w <= 'z') {
                *p++ = (UBYTE)(*w - 'a' + 'A');
                w++;
                column++;
            }
            column += (int)strlen(w);
            p = put(p, end, w);
            if (i < words - 1 && next(8) == 0 && p < end) {
                *p++ = ',';
                column++;
            }
        }
        if (p < end) {
            *p++ = '.';
            column++;
        }
        if (++sentences >= in_paragraph && p + 2 <= end) {
            *p++ = '\n';
            *p++ = '\n';
            column = 0;
            sentences = 0;
        }
    }
    return buf;
}
