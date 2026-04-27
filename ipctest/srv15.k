/ 15 non-callable .m.g must not crash the server.
/    Regression for the `.m.g:"{1+x}"` footgun: the default .m.g
/    is `{. x}`, which evals string *args*; a string in the *slot*
/    itself isn't callable. apply1 used to call fne() on it and
/    SIGSEGV. It now validates via val() and returns the type
/    error to the peer; the server stays up.
.m.g:"{1+x}"
\m i 15555
