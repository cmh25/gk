/ 01 hello - default .m.g is {. x}, which is monadic `.`
/   x a string       -> execute   (.". as code)
/   x a (fn;args)    -> apply     (f . args)
h:3:("localhost";15555)
h 4:"!10"         / -> 0 1 2 3 4 5 6 7 8 9
h 4:"1+2*3"       / -> 7
h 4:(`f;1 2 3)    / apply f to 1 2 3 -> 6
h 4:(`g;,7)       / apply g to ,7   -> 49
3:h
\\
