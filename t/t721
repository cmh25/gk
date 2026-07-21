/ -0.0 prints as -0.0 (it used to print 0.0, while $ printed "-0" and 1% gave -0i:
/ two values that rendered identically reciprocated to opposite infinities).
/ IEEE ties -0.0 with 0.0 under = < ~ and sort, but keeps the sign visible to %.

-0.0
0.0
0.0*-1.0
-0.0e
0.0e*-1.0e

/ still equal, still ties, still folds in distinct -- only the rendering changed
0.0=-0.0
0.0~-0.0
-0.0<0.0
<-0.0 0.0
?0.0 -0.0

/ the sign was always observable here; now the display agrees
1%-0.0
1%0.0
$-0.0

/ vectors: a bare "-0" would lex back as the int 0, so -0.0 carries its own `.0`
(0.0*-1.0),0.0
-0.0 1.5
1.5 -0.0 2.5
,-0.0
(0.0e*-1.0e),0.0e
0#0.0

/ gk's own rendering round-trips
1%*-0.0 0
1%(1.5 -0.0 2.5)@1
1%*-0.0 0.0e
1%. "-0.0"
1%*. "-0.0 0"

/ inside a FLOAT vector literal, `-0` is the float -0.0 -- it used to be lexed
/ as the int 0 and then widened, losing the sign.  In an int vector it stays 0.
-0 1.0
1%*-0 1.0
1.0 -0
1%(1.0 -0)@1
1.0 -0 2.0
1%(1.0 -0 2.0)@1
-0 1.0e
1%*-0 1.0e
/ `-0j` is a long, and `j` cannot be mixed with a float token at all (t722), so
/ the "what sign does a long zero promote to" question never arises.  Only the
/ untyped `-0` adopts the vector's float type; `-0.0`/`-0.0e` are typed float.
@[.;"-0j 1.0";:]
@[.;"-0j 1.0e";:]
@[.;"-0 1j 2.0";:]
@[.;"-0.0 1j";:]
1%(-0j)+0.0
-0 1j
4:-0 1j
-0 0N 1.0
5 -0 1.0
1%(5 -0 1.0)@1
0 1.0
1%*0 1.0

/ ints have no signed zero: atoms and int vectors are untouched
-0
4:-0
1%-0
-0j
1%-0j
-0 1
4:-0 1
-0 0N 1

/ these are genuine IEEE negative zeros, not a formatting artifact
ceil -0.1
rint -0.1
trunc -0.1
floor -0.0
sin -0.0
tanh -0.0

/ ? distinct keeps the first occurrence, so a leading -0.0 keeps its sign.
/ (0.0 is the hash's empty marker, so zeros bypass it and used to emit a bare 0.)
?-0.0 0.0
?0.0 -0.0
1%*?-0.0 0.0
1%*?0.0 -0.0
#?0.0 -0.0 1.5 0.0 1.5
?0.0 -0.0 1.5 0.0 1.5
#?100#0.0

/ f32 mirrors f64 in every one of these
-0.0e
1%-0.0e
$-0.0e
0.0e=-0.0e
0.0e~-0.0e
<-0.0e 0.0e
?-0.0e 0.0e
1%*?-0.0e 0.0e
?0.0e -0.0e
#?100#0.0e
&/-0.0e 0.0e
|/-0.0e 0.0e
(-0.0e 0.0e)?-0.0e
1%*(-0.0e 1.5e)@<-0.0e 1.5e
-0.0e 1.5e
1.5e -0.0e
1.5e -0.0e 2.5e
,-0.0e
1%*. "-0.0e"

/ zeroclamp folds tiny noise to +0.0 but must not invent or erase a sign
\zc 1
-0.0
-1e-15
1e-15
0.0
\zc 0
-1e-15
\\
