/ knapsack
/ c: knapsack capacity
/ w: weights of items
/ v: values of items
/  : dp matrix
ks:{[c;w;v]            / ks[capacity;weights;values]
  m:{$[x<y;y;x]}       / max
  d::(1+n:#w)#,(1+c)#0 / init dp matrix
  {p:x[l:y-1]z         / previous row dp value
   W:w l               / weight
   V:v l               / value
   .[x;y,z;:;$[~z<W;m[V+.[x;l,z-W];p];p]]}/[`d;1+!n]'[1+!c]
  d}

/ knapsack indices of selected items
/ d: dp matrix from ks{}
/ w: weights
/ i: #rows
/ j: #columns
/  : indices of selected items
ksi:{[d;w;i;j]        / ksi[dpmatrix;weights;#rows;#cols]
  $[~i                        / zero?
    ()                        / base
    d[i][j] > d[i-1]j         / select?
    ksi[d;w;i-1;j-w[i-1]],i-1  / recurse with select
    ksi[d;w;i-1]j]}            / recurse without select

/ Stevan Apter show function (nsl.com)
show0:{[t]                           / show
 if[(@*t[])|1<#?#'t[]                / if not a table
  :((-|/#'$k)$k:!t),/" : ",/5:'t[]]  / use dict display
 i:(^t[])1                           / count
 j:@[t;nul;{(1|i)#x}]                / empty
 f:{$[4:x;x;1_,/" ",/x]}             / nested
 k:f''+$+j[]                         / data
 l:+,$!j                             / labels
 m:1 -1(4:'t[])lin 0 -4              / data justifications
 n:(|/#')'{x,y}'[l;k]                / widths
 l:" ",''(-n)$l                      / padded labels
 k:" ",''(m*n)$k                     / padded data
 o:+," ",'n#'"-"                     / separators
 p:&-2=4:'t[]                        / where float
 p@:&(|/"."in')'k p                  / where decimals
 q:(*&)''"."=k p                     / where decimals in
 r:q-&/'q dv'0                       / rotation matrix
 k[p]:r!''k p                        / align on decimals
 s:,'/l,'o,'k                        / result
 s}                                  / matrix

show:{`0:,," ";{`0:x,,,"\n"}$[~(4:x)in 0 5;,5:x;5 5~4:'x;{x,'" ",'@[(#x)#"|";1;:;"+"],'y}. show0'x;show0 x]}

/knapsack examples

run:{[c;n;w;v]
  d:ks[c;w;v]
  i:ksi[d;w;#w;c]
  s:(#w)#`
  s[i]:"*"
  `0:,"\n\ncapacity: ",$c
  show[.+(`n`w`v`s;(n,`;w,+/w i;v,+/v i;s,`))]
}

c:5
n:`a`b`c
w:1 2 3
v:6 10 12
run[c;n;w;v]

c:5
n:`a`b`c`d
w:2 3 1 4
v:4 5 3 7
run[c;n;w;v]

c:10
n:`a`b`c`d
w:2 3 1 4
v:4 5 3 7
run[c;n;w;v]

c:67
n:`a`b`c`d`e`f`g`h`i`j
w:23 26 20 18 32 27 29 26 30 27
v:505 352 458 220 354 414 498 545 473 543
run[c;n;w;v]

/c:100
/n:`$(ci 97+!26)@draw[;26]'c#3
/w:draw[c;10]
/v:draw[c;10]
/run[c;n;w;v]

\\
