/ 04 .m.c - connect, do one msg, close.
/ run this client twice; server's `closed should become 2.
h:3:("localhost";15555)
h 4:"1+1"         / -> 2
3:h               / close: triggers .m.c on server
\\
