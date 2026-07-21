/ fused &a<x / &a>x / &a=x
x:5 3 1 4 2
f:1.5 0.5 2.5
j:5 3 1 4 2j
e:1.5 0.5 2.5e

&x>2                 / int vec / int atom
&x<3
&x=3
&x>9                 / zero matches -> empty
&x<0
&x=9
&x>0                 / every element matches
&x<9

&f>1.0               / float
&f<1.0
&f=0.5
&f>9.0               / zero matches
&f=9.0

&j>2                 / long
&j=3
&j>9                 / zero matches

&e>1.0e              / real
&e>9.0e              / zero matches

a:1 2 3 4            / vector / vector
b:4 3 2 1
&a>b
&a=b
&a<b
&a>a                 / zero matches
&a=a                 / all match

&0 1 0 1 0           / trailing zero is dropped
&!0                  / empty input
&0#1

g:1.5 0n 0i -0i 0.0  / NaN sorts below everything; +-inf compare natively
&g>1.0
&g<1.0
&g=0n
&g>0n
&g<0n
&g=0i

#&x>2                / count of a fused result
+/x>2                / unfused: no `&` token
(&x>2),99

2&x>1                / dyadic `&` (min) must NOT fuse
p:&
p x>2                / projected `&` must NOT fuse
&(x>2)               / parenthesised: compare is not adjacent
&x>2.0               / mixed types -> wherecmp declines, falls back
&x>2 3 1 4 2         / vec/vec via literal
\\
