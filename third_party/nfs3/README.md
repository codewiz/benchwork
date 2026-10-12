# NFS3 floating-point kernels

Two functions of arczi84's 68k port of Need for Speed III: Hot Pursuit,
`projectObjectVertex()` and `PolygonClipWork::run()`, packaged by arczi84
as the standalone benchmark `nfs3_fpu_bench.cpp` and contributed under the
0BSD license.

`kernel.cpp` is that file with its `main()` replaced by the two entry points
of `nfs3.h`; `source.sha256` records the hash of the file as received. It is
C++, so only the gcc build includes it.
