/ 03 custom .m.g
h:3:("localhost";15555)
h 4:"2+2"         / -> 4
h 4:"10*10"       / -> 100
h 4:"#hist"       / -> 3 (this query itself is appended first)
h 4:"hist"        / -> ("2+2";"10*10";"#hist";"hist")
3:h
\\
