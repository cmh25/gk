/ 06 fork isolation.
/ Three short conns: each child starts at c=0 and returns 1.
h:3:("localhost";15556)
h 4:"0"           / -> 1
3:h
h:3:("localhost";15556)
h 4:"0"           / -> 1
3:h
h:3:("localhost";15556)
h 4:"0"           / -> 1
3:h
/ One long conn, 3 msgs through it: same child, counter ticks 1 2 3.
h:3:("localhost";15556)
h 4:"0"           / -> 1
h 4:"0"           / -> 2
h 4:"0"           / -> 3
3:h
\\
