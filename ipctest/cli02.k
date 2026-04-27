/ 02 async + side-effect
h:3:("localhost";15555)
h 3:"c+:1"        / async: no response
h 3:"c+:10"       / async
h 3:"c+:100"      / async
h 4:"c"           / sync: read counter -> 111
3:h
\\
