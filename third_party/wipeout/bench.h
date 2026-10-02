#ifndef TRISBENCH_H
#define TRISBENCH_H
#include "types.h"
typedef char bench_vertex_must_be_32_bytes[sizeof(vertex_t) == 32 ? 1 : -1];
typedef char bench_triangle_must_be_96_bytes[sizeof(tris_t) == 96 ? 1 : -1];
void render_push_tris(tris_t tris, uint16_t texture_index);
void bench_reset(void);
void bench_finish(void);
void bench_consume(const tris_t *tris, uint32_t count);
#endif
