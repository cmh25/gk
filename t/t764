/ 1: (mmap file read) trusted the on-disk element count without checking it
/ against the real file size, and count*elemsize could overflow: a crafted
/ 24-byte header claiming ~2^62 ints made len=count*4 wrap small, so mmap
/ mapped a tiny region while the vector's count was set to billions -- any
/ use read off the end (SEGV, and an out-of-bounds read of live memory from
/ an untrusted file before that).  1: now validates count against the file
/ size (divide, no overflow) and errors cleanly.  (found by the round-6
/ IO-enabled fuzz harness, 2026-07-14.)
/ build a valid int blob, corrupt its count field to the overflow shape,
/ then confirm 1: rejects it instead of crashing.
(`z764) 1:1 2 3
b:"c"1:`z764
b:@[b;16;:;ci 1]
b:@[b;23;:;ci 64]
(`z764) 6:b
@[{1: x};`z764;:]
@[{+/1: x};`z764;:]
/ a well-formed blob still round-trips
(`z764) 1:10 20 30 40
1: `z764
+/1: `z764
del"z764"
\\
