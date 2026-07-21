\p 5
\p
\p 0
\e 0
\m i 0
\m f 0

/ \p, \e and \zc take a bare digit run or nothing.  A malformed argument used to
/ be left on the line and reduce against the nul the lexer pushes as the missing
/ argument, so `\p "x"` evaluated as `nul "x"` -- echoing its own argument while
/ the command never ran.  They reject it now, as \m and \d already did.
@[.;"\\p \"x\"";:]
@[.;"\\p `a";:]
@[.;"\\p 1.5";:]
@[.;"\\p 3+2";:]
@[.;"\\p 5x";:]
@[.;"\\e \"x\"";:]
@[.;"\\e 1.5";:]
@[.;"\\zc \"x\"";:]
@[.;"\\zc 1x";:]
/ and the well-formed forms still work: a digit run sets, nothing queries
@[.;"\\p 5";:]
@[.;"\\p";:]
@[.;"\\e 0";:]
@[.;"\\zc 0";:]
\\
