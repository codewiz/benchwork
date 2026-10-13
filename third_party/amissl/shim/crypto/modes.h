/* Shim for the Benchwork build of gcm128.c: the GCM part of OpenSSL's
   internal crypto/modes.h, without the byte-swap asm that no m68k build
   selects.  From OpenSSL 4.0.3, Apache License 2.0 (../../LICENSE). */
#include <openssl/modes.h>

typedef long long i64;
typedef unsigned long long u64;
#define U64(C) C##ULL

typedef unsigned int u32;
typedef unsigned char u8;

#define STRICT_ALIGNMENT 1

#define GETU32(p) ((u32)(p)[0] << 24 | (u32)(p)[1] << 16 | (u32)(p)[2] << 8 | (u32)(p)[3])
#define PUTU32(p, v) ((p)[0] = (u8)((v) >> 24), (p)[1] = (u8)((v) >> 16), (p)[2] = (u8)((v) >> 8), (p)[3] = (u8)(v))

/*- GCM definitions */ typedef struct {
    u64 hi, lo;
} u128;

typedef void (*gcm_init_fn)(u128 Htable[16], const u64 H[2]);
typedef void (*gcm_ghash_fn)(u64 Xi[2], const u128 Htable[16], const u8 *inp, size_t len);
typedef void (*gcm_gmult_fn)(u64 Xi[2], const u128 Htable[16]);
struct gcm_funcs_st {
    gcm_init_fn ginit;
    gcm_ghash_fn ghash;
    gcm_gmult_fn gmult;
};

struct gcm128_context {
    /* Following 6 names follow names in GCM specification */
    union {
        u64 u[2];
        u32 d[4];
        u8 c[16];
        size_t t[16 / sizeof(size_t)];
    } Yi, EKi, EK0, len, Xi, H;
    /*
     * Relative position of Yi, EKi, EK0, len, Xi, H and pre-computed Htable is
     * used in some assembler modules, i.e. don't change the order!
     */
    u128 Htable[16];
    struct gcm_funcs_st funcs;
    unsigned int mres, ares;
    block128_f block;
    void *key;
#if !defined(OPENSSL_SMALL_FOOTPRINT)
    unsigned char Xn[48];
#endif
};

void ossl_gcm_init_4bit(u128 Htable[16], const u64 H[2]);
void ossl_gcm_ghash_4bit(u64 Xi[2], const u128 Htable[16],
    const u8 *inp, size_t len);
void ossl_gcm_gmult_4bit(u64 Xi[2], const u128 Htable[16]);
