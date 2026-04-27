/ 08 big message round-trip
h:3:("localhost";15555)
/ big response: request server to build 100k vector.
r: h 4:"!100000"
#r                / -> 100000
-3#r              / -> 99997 99998 99999
/ big request: send a big vector to server's szlen.
h 4:(`szlen;,!50000)    / -> 50000
3:h
\\
