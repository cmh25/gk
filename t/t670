\e 0
/ colon verbs applied via bracket / . / @ trap -- regression for the
/ file-verb (0xcc/0xcd) dispatch added to fe / dotcb / atcb.
0:[("IJE";4 14 8);"eje.txt"]            / bracket: dyadic read
.[0:;(("IJE";4 14 8);"eje.txt");:]      / . trap: spreads (schema;file) -> (0;data)
1:[("ijef";4 8 4 8);"ijef.bin"]         / bracket
.[1:;(("ijef";4 8 4 8);"ijef.bin");:]   / . trap
.[0:;(("XY";4 5);"eje.txt");:]          / trapped error through . apply
@[1:;"ijef.bin";:]                      / @ trap: x as one arg (monadic 1: read)
(0:) . (`;,"asdf")                      / parenthesized file verb via dyadic dot
f:0:                                    / assign file verb
f . (`;,"asdf")                         / assigned file verb via dyadic dot
f[`;,"asdf"]                            / assigned file verb via bracket apply
\\
