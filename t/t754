/ 2026-07-13 final-round audit fixes (rand.c + b.c draw + md5).
/ RAND: deal's shuffle used a broken LCG (`*s *= 1103515245 + 12345` folded
/ to an EVEN multiplier), so after 32 calls the state hit 0 and every deal
/ returned the identity permutation forever.  Now a working xorshift.
/ These check PROPERTIES (the sequence is deterministic but its exact
/ values aren't pinned here): distinct, in range, and NOT the identity even
/ on the 40th deal.
bad:0;i:0
while[i<40; d:12?-12; if[d~!12;bad::bad+1]; if[~12=#?d;bad::bad+1]; if[|/~(d<12)&d>-1;bad::bad+1]; i::i+1]
bad
/ a full deal is a genuine permutation (sorts back to !20)
p:20?-20
(!20)~p@<p
/ DRAW: shaped real range (case 9 was missing)
4:draw[2 3;1.0e]
(2 3)~^draw[2 3;1.0e]
/ MD5/SHA recurse into nested lists (irecur1), non-char leaves type-error
md5 ,,"abc"
(,,"900150983cd24fb0d6963f7d28e17f72")~md5 ,,"abc"
sha1 ,"abc"
/ AES-256-CTR round trip + NIST KAT; decrypt output is a valid char vector
/ (its terminator slot was left uninitialized -> downstream strlen OOB)
"charles"~(17*!16;!32) decrypt (17*!16;!32) encrypt "charles"
(17*!16;!32) encrypt ci 16#0
/ intern a decrypted value as a symbol: strlen must stop at the plaintext
/ end (the uninitialized terminator slot read one byte past it)
kk:(17*!16;!32)
`abc~`$kk decrypt kk encrypt "abc"
\\
