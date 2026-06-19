\e 0
/ kamend4_ (4-arg amend) on a not-yet-existing target must create the
/ dict and store, not drop the value.  Was: .[`ns;`a;:;1] -> value error.
.[`foo;`a;:;1]
foo.a
foo
.[`bar;`x;,;7]
bar.x
.[`baz;`a;:;2]
.[`baz;`b;:;3]
baz.a
baz.b
/ the empty symbol is not a valid dict key: named amend must reject it
/ (like d[`]:x does), not store it.  This errors (to stderr) and aborts ->
/ no stdout for it; without the guard it would store the key and echo `qux.
.[`qux;`;:;1]
\\
