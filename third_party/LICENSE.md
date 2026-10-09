# Third-party licenses

The harness itself is 0BSD, see [../LICENSE](../LICENSE). The code under
`third_party/` keeps its own license, in the `LICENSE` file of each
directory:

* newlib string functions (BSD, Red Hat)
* zlib (zlib)
* libpng (PNG Reference Library License)
* FreeType (FTL)
* LHa for UNIX (its redistribution terms, in Japanese)
* Dhrystone (Reinhold Weicker's original terms) via xSysInfo (BSD-2-Clause)
* The Wipeout kernel in `wipeout/` has no license file yet: its upstream
  has not stated one
* The glyph outlines in `../src/glyphs.c` are derived from DejaVu Sans
  (Bitstream Vera license).

Portions of this software are copyright (C) 1996-2022 The FreeType Project
(www.freetype.org). All rights reserved.

Because of the LHa terms, binaries must be distributed together with this
source, and the program must not be used as the main product of a
commercial offering.
