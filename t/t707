/ cv zero-termination: numeric casts must parse strings sliced from 0: reads
"z707" 0:("12";"-3";"1e2";,"7")
L:0:"z707"
0.0$'L
0.0e$'L
_0.0$'L
0$L 0
0j$'L@0 3
b:6:"z707"
0.0$(b?"\n")#b
del"z707"

/ 3-arg seeded fold/scan over a 1-element list: unpack the element, keep the seed
f:{[a;c;u](5j*a)+c}
f/[0j;,4;nul]
f\[0j;,4;nul]
f/[0j;4 3;nul]
f\[0j;4 3;nul]
f/[0j;4;nul]
f\[0j;4;nul]
f/[0j;!0;nul]
g:{[a;c;u]a+c}
g/[10;,5;nul]
g\[10;,5;nul]
/ each-collect over such folds promotes to a plain long vector
h:{{[a;c;u](5j*a)+c}/[0j;-2+"=-012"?/x;nul]}
w:h'("2";"12";"1=");(4:w;w;+/w)
/ fixed-width text 0: -- C fields are fresh cv's, casts parse them
"z707" 0:("12345678";"87654321")
t:("CC";3 5)0:"z707"
t
0$'t 0
0.0$'t 1
del"z707"

/ fixed-width binary 1: -- C fields likewise (int cells zero: endian-free)
"z707" 6:(ci 0 0 0 0),"12",(ci 0 0 0 0),"34"
u:("iC";4 2)1:"z707"
u
0$'u 1
del"z707"

/ empty lines and float specials from 0: lines parse too
"z707" 0:("12";"";"-3")
0.0$'0:"z707"
del"z707"
"z707" 0:("0n";"0i";"-0i";"1e300")
0.0$'0:"z707"
del"z707"

/ seeded fold/scan: zipped 1-element lists, subtype items, empties, errors
{[a;c;u](10*a)+c+u}/[0j;,4;,5]
{[a;c;u](10*a)+c+u}\[0j;,4;,5]
4:{[a;c;u]c}/[0;,.();nul]
({[a;c;u]c}/[0;,{x+7};nul]) 5
{[a;c;u](5j*a)+c}\[0j;!0;nul]
{[a;c;u](5j*a)+c}/[0j;0#0.0;nul]
.[{{[a;c;u]a+c}/[0;x;nul]};,(1;"ab");:]
\\
