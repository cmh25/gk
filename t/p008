/ fusion candidates: fixed repetition counts make before/after runs comparable
n:!_1e6
h:_5e5

/ comparison consumers
\t do[100;#&n<h]
\t do[100;*&n>h]
\t do[100;+/n<h]
\t do[100;&/n<h]
\t do[100;|/n<h]

/ direct and negated filters
\t do[100;n@&n<h]
\t do[100;n@&~n<h]

/ filtered reductions
v:1+n
\t do[100;+/v@&n<h]
\t do[100;&/v@&n<h]
\t do[100;|/v@&n<h]

/ widening scan
\t do[20;+\0j+n]

/ group counts
g:1000!n
\t do[20;#'=g]

/ direct stable sort and partial grade/top-k
s:1000003!7919*n
sf:0.0+s
\t do[10;sf@<sf]
\t do[10;sf@>sf]
\t do[100;100#<s]
\t do[100;100#>s]
\t do[100;s@100#<s]
\t do[100;s@100#>s]
\\
