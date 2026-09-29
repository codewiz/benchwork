/* SPDX-License-Identifier: 0BSD
 *
 * What the LHa compressor core needs from the rest of LHa for UNIX: the
 * globals declared in lha.h, the CRC table and the checksummed reads and
 * writes from crcio.c (over memory streams instead of stdio), the error
 * functions from util.c, and stubs for the compression methods that
 * slide.c's dispatch tables refer to but this build never selects.
 */

#include <stdarg.h>

#include "bench.h"

#include "lha.h"

boolean extract_broken_archive;
boolean dump_lzss;

int unpackable;
off_t origsize, compsize;
unsigned short dicbit;
unsigned short maxmatch;
off_t decode_count;
unsigned long loc;
unsigned char *text;
unsigned char *dtext;

FILE *infile, *outfile;
unsigned short bitbuf;

unsigned int crctable[UCHAR_MAX + 1];

/* --- memory streams --------------------------------------------------- */

/* The next byte of s, or EOF. */
int lha_getc(struct lha_stream *s) {
    if (s->pos >= s->len)
        return EOF;
    return s->data[s->pos++];
}

/* Append size*n bytes to s. Returns n, or 0 when the buffer is full. */
size_t lha_fwrite(const void *p, size_t size, size_t n, struct lha_stream *s) {
    size_t bytes = size * n;

    if (s->len + bytes > s->cap)
        return 0;
    memcpy(s->data + s->len, p, bytes);
    s->len += bytes;
    return n;
}

/* --- crcio.c ---------------------------------------------------------- */

void make_crctable(void) {
    unsigned int i, j, r;

    for (i = 0; i <= UCHAR_MAX; i++) {
        r = i;
        for (j = 0; j < CHAR_BIT; j++)
            if (r & 1)
                r = (r >> 1) ^ CRCPOLY;
            else
                r >>= 1;
        crctable[i] = r;
    }
}

unsigned int calccrc(unsigned int crc, char *p, unsigned int n) {
    while (n-- > 0)
        crc = UPDATE_CRC(crc, *p++);
    return crc;
}

/* Read up to n bytes from fp into p, updating *crcp. Returns the count. */
int fread_crc(unsigned int *crcp, void *p, int n, FILE *fp) {
    size_t left = fp->len - fp->pos;

    if ((size_t)n > left)
        n = (int)left;
    memcpy(p, fp->data + fp->pos, (size_t)n);
    fp->pos += (size_t)n;
    *crcp = calccrc(*crcp, p, (unsigned int)n);
    return n;
}

/* Append n bytes from p to fp, updating *crcp. */
void fwrite_crc(unsigned int *crcp, void *p, int n, FILE *fp) {
    *crcp = calccrc(*crcp, p, (unsigned int)n);
    if (lha_fwrite(p, 1, (size_t)n, fp) == 0)
        fatal_error("output buffer full");
}

/* crcio.c resets its EUC conversion cache here; there is none to reset. */
void init_code_cache(void) {
}

/* --- util.c ----------------------------------------------------------- */

void error(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fputs("lha: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

void fatal_error(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fputs("lha: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(20);
}

// Through the harness allocator rather than malloc(), so --fastmem covers
// lha's own buffers too: the decode dictionary is the memory this benchmark
// works in, and chip memory costs the CPU bus arbitration against DMA.
void *xmalloc(size_t size) {
    void *p = bench_alloc((ULONG)size, "lha buffer");

    if (!p)
        fatal_error("out of memory");
    return p;
}

void xfree(void *p) {
    bench_free(p);
}

/* --- unused methods --------------------------------------------------- */

#define STUB_VOID(name) \
    void name(void) { fatal_error(#name " is not built"); }
#define STUB_USHORT(name) \
    unsigned short name(void) { fatal_error(#name " is not built"); return 0; }

void output_dyn(unsigned int code, unsigned int pos) {
    (void)code;
    (void)pos;
    fatal_error("output_dyn is not built");
}

STUB_VOID(encode_start_fix)
STUB_VOID(encode_end_dyn)
STUB_USHORT(decode_c_dyn)
STUB_USHORT(decode_p_dyn)
STUB_VOID(decode_start_dyn)
STUB_VOID(decode_start_fix)
STUB_USHORT(decode_c_st0)
STUB_USHORT(decode_p_st0)
STUB_VOID(decode_start_st0)
STUB_USHORT(decode_c_lzs)
STUB_USHORT(decode_p_lzs)
STUB_VOID(decode_start_lzs)
STUB_USHORT(decode_c_lz5)
STUB_USHORT(decode_p_lz5)
STUB_VOID(decode_start_lz5)
STUB_USHORT(decode_c_pm2)
STUB_USHORT(decode_p_pm2)
STUB_VOID(decode_start_pm2)
