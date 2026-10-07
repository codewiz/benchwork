# Wipeout render_push_tris()

The triangle submission function of arczi84's AmigaOS port of the Wipeout
rewrite (https://github.com/arczi84/wipeout-sdl1, a fork of
https://github.com/phoboslab/wipeout-rewrite), packaged by arczi84 as a
standalone benchmark for AmigaPorts/m68k-amigaos-gcc#89.

`kernel.c` is the function verbatim, with the GPU submission replaced by the
`bench_consume()` callback the harness provides; `source.sha256` records the
hash of the function body. `types.h`, `utils.h` and
`render_gl_legacy_types.h` are the game headers it needs; `types.h` and
`utils.h` carry fallbacks for compilers without the GNU extensions they use.
