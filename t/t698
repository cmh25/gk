/ Coverage: group (=) and grade (< >) on >64-element vectors. Normal build uses the
/ i32 path; under -DBIGV=64 (gkabig) any vector >64 routes through groupj/gradej (the
/ i64 big-vector paths, otherwise unreachable without a 2.1G-element vector). Covers
/ NOTE: big-path group/grade return LONG (i64) indices, so the union check uses
/ tolerant = (not ~). groupj's per-type arms: dense-int, long, float, real, char, sym, and general-list
/ (hash) path. Build-independent correctness properties.
/ -- grade up/down sort monotonically (various element types)
u:(100#7 3 1 9 2 8 4 5)@<100#7 3 1 9 2 8 4 5; &/~(1_u)<-1_u
u:(100#7 3 1 9 2 8 4 5j)@<100#7 3 1 9 2 8 4 5j; &/~(1_u)<-1_u
u:(100#7.5 3.5 1.5 9.5 2.5)@<100#7.5 3.5 1.5 9.5 2.5; &/~(1_u)<-1_u
u:(100#7.5 3.5 1.5 9.5 2.5e)@<100#7.5 3.5 1.5 9.5 2.5e; &/~(1_u)<-1_u
u:(100#"hjadbf")@<100#"hjadbf"; &/~(1_u)<-1_u
u:(100#`h`j`a`d`b)@<100#`h`j`a`d`b; &/~(1_u)<-1_u
d:(100#7 3 1 9 2 8 4 5)@>100#7 3 1 9 2 8 4 5; &/~(1_d)>-1_d
/ -- group: razing index-groups and sorting yields every index once (per type)
g:,/=100#7 3 1 9 2 8 4 5; &/(g@<g)=!100
g:,/=100#7 3 1 9 2 8 4 5j; &/(g@<g)=!100
g:,/=100#7.5 3.5 1.5 9.5 2.5; &/(g@<g)=!100
g:,/=100#7.5 3.5 1.5 9.5 2.5e; &/(g@<g)=!100
g:,/=100#"hjadbf"; &/(g@<g)=!100
g:,/=100#`h`j`a`d`b; &/(g@<g)=!100
/ -- group a general list (groupj hash path, case 0)
g:,/=100#(1 2;3;`a;"xy");&/(g@<g)=!100
/ -- long grade/group match int on same values
(<100#7 3 1 9 2 8 4 5j)~<100#7 3 1 9 2 8 4 5
(=100#7 3 1 9 2 8 4 5j)~=100#7 3 1 9 2 8 4 5
\\
