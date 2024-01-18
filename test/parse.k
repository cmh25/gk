// Rationalized Notation for First-Order Logic

\d .parse

/ parse: E:E;e|e e:nve|te| t:n|v n:t[E]|(E)|N
p:{I::0;{$[l[x]&2=#x;x 1;x]}{$[l x;(,,","),1_ x;x]}t J::"(",x:x dv" "}                  / parse
N:"abcdefghijklmnopqrstuvwxyz01"                                                        / nouns
P:"ABCDEFGHIJKLMNOPQRSTUVWXYZ"                                                          / predicates
V:"~&|/!{}",P                                                                           / verbs
c:{$[~4:x;"n";&/x lin V;"v";"osn"(&/'x lin/(,"[";",)] "))?1]}                           / class of token
j:{i -1+I+:1}                                                                           / next token
i:{$[x<#J;J x;" "]}                                                                     / safe indexing
g:{x(1;)(~3>#x)|"v"=c x 1}                                                              / (v) -> (v)
l:{("("~*x)&~x~"()"}                                                                    / ( but not ()
E:{|/",(["=*j`}{e t`}\                                                                  / (expression)
t:{$["s"=c i I;"";{"o"=c i I}{$["["=*i I;E x;(j`;x)]}/$[~l i I;j`;g E i I]]}            / term ("" = ::)
e:{$["s"=c i I;x;(v[f:t`])>v`;(f;x;e t`);(x;e f)]}                                      / expression
r:{$[(*x)in P,N;x;x,":"]}                                                               / operator:
v:{c[i I-1]in"vo"}                                                                      / verb or (?

/ unparse
u:{$[1=#x;x;~(*x)in V;m x;2=#x;(*x),u x 1;b[u x 1],x[0],u x 2]}                         / unparse
m:{$[@x;x;x[0],a s@u'1_ x]}                                                             / atom or f[
a:1!"][",                                                                               / x -> [x]
b:{$[("("=*x)|@x;x;1!")(",x]}                                                           / parenthesize
s:1_,/",",'                                                                             / separate

\

the grammar is simple.  there are two syntax categories:  nouns and verbs.  nouns are variables (single lower-case letters) and constants (0 and 1).  verbs fall into three subcategories:  predicates (single upper-case letters), and monadic and dyadic operators.  dyads are further divided into overlapping semantic categories:  logical connectives and variable binding operators.

predicates have finite valence:  0 (variables, constants), 1 (properties), 2 (binary relations), and so on.

it is convenient to treat predicate letters as verbs:

    F[x,y]      ->      Fxy
    xFy         ->      Fxy
    xFG[y,z]    ->      ("F";"x";"Gyz")

two special predicates are { (membership) and ~ (identity).

    x{y

the symbol for class-abstraction is chosen for its affinity to membership:

    x}Fx

e.g.

    F[a]~a{x}Fx

i.e. a is F iff a is a member of the class of F-ers.

nouns               symbol
-----               ------
true,false          0,1
variables           a-z

verbs               symbol          meaning
-----               ------          -------
predicates          A-Z { ~         predicates, in, equal
monads              | & ~           some, all, not
connectives         | & / ~         some, all, onlyif, equal
binding             | & / ! }       some, all, lambda, the, class

iverson notation is used - there is no precedence.  a monad is written to the left of its argument.  a dyad is written between its arguments.  dyadic & and | are conjunction and disjunction, and also universal and particular quantification.  monadic & and | are necessitation and possibility.

expressions are evaluated left-of-right:

    a|b&c/d         is  a|(b&c/d)
    (a|b)&~c/d      is  (a|b)&(~(c/d))
    (a|b)&(~c)/d    is  (a|b)&((~c)/d)

variable-binding operators (all, some, the, lambda, class) are written

    <variable><operator><expression>

instead of

    <operator><variable><expression>.

class abstraction:  a is F iff a is a member of the class of x's such that F x:

  q:"F[a]~a{x}Fx"

  p q
("~"
 "Fa"
 ("{"
  "a"
  ("}";"x";"Fx")))

example:

if all the world loves a lover then if one loves one then all loves all:

    (x&(y|xLy)/y|yLx)/(x|y|xLy)/x&y&xLy

parse/unparse:

  q:"(x&(y|xLy)/y|yLx)/(x|y|xLy)/x&y&xLy"

  p q
("/"
 ("&"
  "x"
  ("/"
   ("|";"y";"Lxy")
   ("|";"y";"Lyx")))
 ("/"
  ("|"
   "x"
   ("|";"y";"Lxy"))
  ("&"
   "x"
   ("&";"y";"Lxy"))))

  q~u p q
1

