/ 10 self-loop: one process listens AND connects to itself.
/ this stresses the inline poll loop; the sync to itself must not
/ deadlock even though .m.g eval runs on the same repl.
\m i 15555
h:3:("localhost";15555)
h 4:"2+2"         / -> 4
h 4:"!5"          / -> 0 1 2 3 4
3:h
\\
