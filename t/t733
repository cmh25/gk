/ metamorphic properties across the type matrix.  The data is hundreds of random
/ elements, so the PROPERTY is the interesting value: each type's block prints
/ one boolean per property, in this order --
/   sort gradelen updown findrt ser lin group
/ (sort/grade agree, grade keeps the count, up is down reversed, find is a
/  roundtrip through the distinct values, db bd roundtrips, every element is in
/  itself, and = groups back to a permutation of !#v)
sorted:{~|/(1_x)<-1_x}
blk:{[v]
  s:v@<v
  u:?v
  g:,/=v
  (sorted s;(#v)=#<v;s~|v@>v;(v v?/u)~u;(db bd v)~v;&/v lin v;(g@<g)~!#v)}
/ binl against a brute-force count of the elements below each probe; numeric only
binlq:{[v] s:v@<v; p:v@draw[20;#v]; (0j+s binl p)~0j+{+/s<x}'p} / 0j+ both: binl returns longs past the BIGV cap
vi:draw[200;50];vi:vi,0N,0I,vi
vj:0j+draw[200;1000];vj:vj,-5j,vj
vf:0.25*draw[200;40];vf:vf,0n,0i,-0i,-0.0,0.0,vf
ve:0.0e+draw[200;30];ve:ve,0ne,0ie,ve
vc:ci 97+draw[200;26]
sy:`a`b`c`d`ee@draw[200;5]
blk vi                          / int    (with 0N/0I)
binlq vi
blk vj                          / long
binlq vj
blk vf                          / float  (with 0n/0i/-0i/-0.0)
binlq vf
blk ve                          / real   (with 0ne/0ie)
binlq ve
blk vc                          / char
blk sy                          / symbol
/ vs/sv roundtrips
n:draw[50;1000000]
n~{2 sv 2 vs x}'n               / base 2
n~{10 sv 10 vs x}'n             / base 10
nj:0j+draw[50;1000000]
nj~{16 sv 16 vs x}'nj           / base 16, long
/ amend roundtrip: the amended slots hold the new values, the rest is untouched
ix:?draw[30;200]
vv:draw[#ix;99]
w:@[vi;ix;:;vv]
w[ix]~vv
(vi di ix)~w di ix
wj:@[vj;ix;:;0j+vv]
wj[ix]~0j+vv                    / long base
/ format/parse roundtrips
n~0$$n
nj~0j$'$nj
fv:0.5*draw[50;1000]
fv~0.0$$fv
/ factor/prime cross-checks
pr:2+draw[40;100000]
&/pr={*/x}'factor'0j+pr         / the factors multiply back to the number
(prime pr)~1={#?factor 0j+x}'pr / prime iff exactly one distinct factor
g1:1+draw[40;1000];h1:1+draw[40;1000]
gg:gcd'[g1;h1]
&/0=(g1!'gg)|h1!'gg             / the gcd divides both
(0j+g1*h1)~0j+gg*lcm'[g1;h1]    / gcd*lcm = the product
/ string search vs brute force
str:ci 97+draw[300;3]
pat:ci 97+draw[3;3]
idx:!1+(#str)-#pat
brute:idx@&{pat~str x+!#pat}'idx
brute~ss[str;pat]
\\
