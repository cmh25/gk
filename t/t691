/ monadic 1: mmap binary read round-trip (-1 int, -2 float, -3 char).
/ Regression: onecolon1 maps the file and points the vector at offset 24, so the
/ mapping must span header+data (24+len). The buggy mmap(len) left the data's
/ trailing 24 bytes on an UNMAPPED page whenever len was page-aligned -> SEGV on
/ read (and __k unmapped the wrong base/length -> leak). Sizes below are chosen
/ PAGE-ALIGNED (1024*4 = 512*8 = 4096 = one page) so they trip the bug at small
/ scale -- a non-aligned size passes on the buggy code too. (Big-vector mmap is
/ also covered in t64.)
a:1024#7;"z6"1:a;a~1:"z6"
a:512#1.5;"z6"1:a;a~1:"z6"
a:4096#"x";"z6"1:a;a~1:"z6"
del"z6"
\\
