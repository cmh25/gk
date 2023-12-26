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
 l:" ",//(-n)$l                      / padded labels
 k:" ",//(m*n)$k                     / padded data
 o:+," ",/n#\"-"                     / separators
 p:&-2=4:'t[]                        / where float
 p@:&{|/"."in/x}'k p                 / where decimals
 q:(*&)''"."=k p                     / where decimals in
 r:q-&/'q dv\0                       / rotation matrix
 k:.[k;(p;);{y!x};r]                 / align on decimals
 t:{x,y,z}'[l;o;k]                   / pre result
 s:{,/t[;x]}'!#*t                    / result
 s}                                  / matrix

show:{`0:,," ";{`0:x,,,"\n"}$[~(4:x)in 0 5;,5:x;5 5~4:'x;{x,'" ",'@[(#x)#"|";1;:;"+"],'y}. show0'x;show0 x]}
