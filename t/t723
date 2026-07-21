/ nul indexes a generic list elementwise.  When an element is not a valid index
/ the partly built result has to be released exactly once: the prefix used to be
/ freed by hand and then freed again with the container, so a list still owned by
/ the parse tree dropped to a zero refcount and was reused after free.
/ AFL found this as `nul(0 0;>)` -- the first element indexes, the second does not.

/ ---- elementwise, every element a valid index ----
nul(0 0;1 2)
nul(1;2;3)
nul(0 0;())
nul(1 2;3 4;5 6)
nul(,1;,2)

/ ---- a symbol element misses and gives nul; it is not an error ----
nul(1;`a;2)
nul(`a;`b)
nul(0 0;`a;1 2)

/ ---- one bad element anywhere: type error, and no double free ----
@[.;"nul(0 0;>)";:]
@[.;"nul(>;0 0)";:]
@[.;"nul(0 0;>;1 2)";:]
@[.;"nul(0 0;\"b\")";:]
@[.;"nul(\"b\";0 0)";:]
@[.;"nul(0 0;1.5)";:]
@[.;"nul(1.5;0 0)";:]
@[.;"nul(0 0;+)";:]
@[.;"nul(0 0;{x})";:]
@[.;"nul((0 0;>))";:]

/ ---- the bad element is reached after a long valid prefix ----
@[.;"nul(0;1;2;3;4;5;6;7;>)";:]

/ ---- and the same list is still intact afterwards ----
a:(0 0;1 2)
@[.;"nul(0 0;>)";:]
nul a
a
\\
