/ --- \l GKPATH search ---
/ a flat file found only via GKPATH (here ./ext) loads, and its defs are usable.
/ literal-first misses (no ./mylib), so the GKPATH entry resolves it.
setenv[`GKPATH;"ext"]
\l mylib
extfoo
extbar 5
/ --- package directory: \l pkg resolves ext/pkg to its load.k entry ---
\l pkg
pkgfoo
/ .z.filepath was set to this script's resolved path during the load ...
pkgpath
/ ... and restored afterwards, so it no longer equals that path (-> 0)
.z.filepath~pkgpath
\\
