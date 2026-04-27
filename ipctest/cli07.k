/ 07 dual listener: inline shares state, forking isolates
/ Server's .m.g always pre-increments c and returns c.

/ inline side, first conn: three syncs tick 1,2,3
hi:3:("localhost";15555)
hi 4:"0"          / -> 1
hi 4:"0"          / -> 2
hi 4:"0"          / -> 3
3:hi

/ inline side, reconnect: state is SHARED, continues from 3
hi:3:("localhost";15555)
hi 4:"0"          / -> 4
3:hi

/ forking side, 3 short conns: each child was forked off parent when c=4,
/ so every child ticks 4->5 and returns 5.
hf:3:("localhost";15556)
hf 4:"0"          / -> 5
3:hf
hf:3:("localhost";15556)
hf 4:"0"          / -> 5
3:hf
hf:3:("localhost";15556)
hf 4:"0"          / -> 5
3:hf

/ parent (inline view) c untouched by fork children:
hi:3:("localhost";15555)
hi 4:"c"          / -> 5 (handler also ticked it, parent went 4->5)
3:hi
\\
