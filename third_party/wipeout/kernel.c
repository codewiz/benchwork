#include "bench.h"
#include "utils.h"
#define RENDER_TRIS_BUFFER_CAPACITY 16
#define TEXTURES_MAX 2048
typedef struct { vec2i_t size; vec2_t scale; uint32_t texId; } render_texture_t;
static tris_t tris_buffer[RENDER_TRIS_BUFFER_CAPACITY] ALIGNED(8);
static uint32_t tris_len;
static render_texture_t textures[TEXTURES_MAX];
static uint32_t textures_len;
static uint16_t texture_index_prev;
/* Replace only GPU submission with an opaque, shared harness callback. */
static void render_flush(void) {
    if (!tris_len) return;
    bench_consume(tris_buffer, tris_len);
    tris_len = 0;
}
void render_push_tris(tris_t tris, uint16_t texture_index) {
	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);

	if (tris_len >= RENDER_TRIS_BUFFER_CAPACITY) {
		render_flush();
	}
	if(texture_index != texture_index_prev){
		render_flush();
	}
	texture_index_prev = texture_index;

	render_texture_t *t = &textures[texture_index];

	for (int i = 0; i < 3; i++) {
		
		// resize back to (0,1) uv space
		tris.vertices[i].uv.x = (tris.vertices[i].uv.x / t->size.x) * t->scale.x;
		tris.vertices[i].uv.y = (tris.vertices[i].uv.y / t->size.y) * t->scale.y;
		if(tris.vertices[i].color.a == 0){
			continue;
		}
		/*
		// move colors back to (0,255)
		uint8_t R = tris.vertices[i].color.r;
		uint8_t G = tris.vertices[i].color.g;
		uint8_t B = tris.vertices[i].color.b;
		if(R == 128){
			R = 255;
		} else {
			R *=2;
		}
		if(G == 128){
			G = 255;
		} else {
			G *=2;
		}
		if(B == 128){
			B = 255;
		} else {
			B *=2;
		}
		tris.vertices[i].color.r = R;
		tris.vertices[i].color.g = G;
		tris.vertices[i].color.b = B;
		*/
		
		tris.vertices[i].color.r = clamp(tris.vertices[i].color.r * 2, 0, 255);
		tris.vertices[i].color.g = clamp(tris.vertices[i].color.g * 2, 0, 255);;
		tris.vertices[i].color.b = clamp(tris.vertices[i].color.b * 2, 0, 255);;
		
	}
	tris_buffer[tris_len++] = tris;
}

void bench_reset(void) {
    tris_len = 0;
    texture_index_prev = 0;
    textures_len = 8;
    for (unsigned i = 0; i < textures_len; ++i) {
        textures[i].size = (vec2i_t){32 << (i % 3), 32 << ((i + 1) % 3)};
        textures[i].scale = (vec2_t){1.0f, 0.5f};
    }
}
void bench_finish(void) { render_flush(); }
