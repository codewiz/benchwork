/* SPDX-License-Identifier: 0BSD
 *
 * Replacement for LHa for UNIX's lha.h, for building the -lh5- compressor
 * (slide.c, huf.c, bitio.c, maketbl.c, maketree.c) on its own. Those files
 * are unmodified; this header gives them the declarations and macros they
 * use, taken from lha.h, lha_macro.h and prototypes.h, and redirects their
 * stream I/O -- getc() on infile and fwrite() on outfile in bitio.c, and
 * fread_crc()/fwrite_crc() in slide.c -- to memory buffers.
 */

#ifndef BENCHWORK_LHA_H
#define BENCHWORK_LHA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>

/* What configure would have said. */
#define STDC_HEADERS 1

#define FALSE 0
#define TRUE 1
typedef int boolean;

/* --- from lha_macro.h --------------------------------------------------- */

#define LZHUFF1_METHOD_NUM 1
#define LZHUFF5_METHOD_NUM 5
#define LZHUFF6_METHOD_NUM 6
#define LZHUFF7_METHOD_NUM 7
#define LARC_METHOD_NUM 8
#define PMARC2_METHOD_NUM 13

#define LZHUFF1_DICBIT 12
#define LZHUFF4_DICBIT 12
#define LZHUFF5_DICBIT 13
#define LZHUFF6_DICBIT 15
#define LZHUFF7_DICBIT 16

#define MAX_DICBIT LZHUFF7_DICBIT
#define MAX_DICSIZ (1L << MAX_DICBIT)

#ifndef MIN
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#endif

/* bitio.c */
#define peekbits(n) (bitbuf >> (sizeof(bitbuf) * 8 - (n)))

/* crcio.c */
#define CRCPOLY 0xA001 /* CRC-16 (x^16+x^15+x^2+1) */
#define INITIALIZE_CRC(crc) ((crc) = 0)
#define UPDATE_CRC(crc, c) \
    (crctable[((crc) ^ (unsigned char)(c)) & 0xFF] ^ ((crc) >> CHAR_BIT))

/* huf.c */
#define USHRT_BIT 16 /* (CHAR_BIT * sizeof(ushort)) */
#define NP (MAX_DICBIT + 1)
#define NT (USHRT_BIT + 3)
#define NC (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)

#define PBIT 5 /* smallest integer such that (1 << PBIT) > * NP */
#define TBIT 5 /* smallest integer such that (1 << TBIT) > * NT */
#define CBIT 9 /* smallest integer such that (1 << CBIT) > * NC */

#define NPT 0x80

/* slide.c */
#define MAXMATCH 256 /* formerly F (not more than UCHAR_MAX + 1) */
#define THRESHOLD 3  /* choose optimal value */

/* --- memory streams in place of stdio --------------------------------- */

/* A byte buffer with a read cursor. */
struct lha_stream {
    unsigned char *data;
    size_t len; /* bytes in use */
    size_t cap; /* bytes allocated */
    size_t pos; /* read cursor */
};

int lha_getc(struct lha_stream *s);
size_t lha_fwrite(const void *p, size_t size, size_t n, struct lha_stream *s);

#undef getc
#undef fwrite
#define FILE struct lha_stream
#define getc lha_getc
#define fwrite lha_fwrite

/* --- from lha.h --------------------------------------------------------- */

struct encode_option {
    void (*output)(unsigned int code, unsigned int pos);
    void (*encode_start)(void);
    void (*encode_end)(void);
};

struct decode_option {
    unsigned short (*decode_c)(void);
    unsigned short (*decode_p)(void);
    void (*decode_start)(void);
};

struct interfacing {
    FILE *infile;
    FILE *outfile;
    off_t original;
    off_t packed;
    off_t read_size;
    int dicbit;
    int method;
};

extern boolean extract_broken_archive;
extern boolean dump_lzss;

/* slide.c */
extern int unpackable;
extern off_t origsize, compsize;
extern unsigned short dicbit;
extern unsigned short maxmatch;
extern off_t decode_count;
extern unsigned long loc;
extern unsigned char *text;
extern unsigned char *dtext;

/* huf.c */
extern unsigned short left[], right[];
extern unsigned char c_len[], pt_len[];
extern unsigned short c_freq[], c_table[], c_code[];
extern unsigned short p_freq[], pt_table[], pt_code[], t_freq[];

/* bitio.c */
extern FILE *infile, *outfile;
extern unsigned short bitbuf;

/* crcio.c */
extern unsigned int crctable[UCHAR_MAX + 1];

/* --- from prototypes.h -------------------------------------------------- */

/* bitio.c */
void fillbuf(int n);
unsigned short getbits(int n);
void putcode(int n, int x);
void putbits(int n, int x);
void init_getbits(void);
void init_putbits(void);

/* crcio.c (lha_glue.c here) */
void make_crctable(void);
unsigned int calccrc(unsigned int crc, char *p, unsigned int n);
int fread_crc(unsigned int *crcp, void *p, int n, FILE *fp);
void fwrite_crc(unsigned int *crcp, void *p, int n, FILE *fp);
void init_code_cache(void);

/* huf.c */
void output_st1(unsigned int c, unsigned int p);
unsigned char *alloc_buf(void);
void encode_start_st1(void);
void encode_end_st1(void);
unsigned short decode_c_st1(void);
unsigned short decode_p_st1(void);
void decode_start_st1(void);

/* maketbl.c */
void make_table(int nchar, unsigned char bitlen[], int tablebits,
                unsigned short table[]);

/* maketree.c */
short make_tree(int nchar, unsigned short *freq, unsigned char *bitlen,
                unsigned short *code);

/* slide.c */
int encode_alloc(int method);
unsigned int encode(struct interfacing *interface);
unsigned int decode(struct interfacing *interface);

/* Methods other than -lh4- to -lh7-: slide.c's dispatch tables name them, so
 * lha_glue.c defines them as stubs that abort. */
void output_dyn(unsigned int code, unsigned int pos);
void encode_start_fix(void);
void encode_end_dyn(void);
unsigned short decode_c_dyn(void);
unsigned short decode_p_dyn(void);
void decode_start_dyn(void);
void decode_start_fix(void);
unsigned short decode_c_st0(void);
unsigned short decode_p_st0(void);
void decode_start_st0(void);
unsigned short decode_c_lzs(void);
unsigned short decode_p_lzs(void);
void decode_start_lzs(void);
unsigned short decode_c_lz5(void);
unsigned short decode_p_lz5(void);
void decode_start_lz5(void);
unsigned short decode_c_pm2(void);
unsigned short decode_p_pm2(void);
void decode_start_pm2(void);

/* util.c */
void error(char *fmt, ...);
void fatal_error(char *fmt, ...);
void *xmalloc(size_t size);
void xfree(void *p);

#endif
