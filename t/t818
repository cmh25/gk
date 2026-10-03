/ a string literal may span lines
a:"asdf
 asdf"
#a
a
b:"one
two
three"
#b
b
/ ( [ { and / inside the open string are text, not nesting or a comment
c:"x(
y/z"
#c
c
/ a backslash before the newline escapes it: the newline is kept
d:"ab\
cd"
#d
d
(1;"p
q";3)
e:"end"  / a quote in a trailing comment: "
#e
a~. "\"asdf\n asdf\""
\\
