/ 16 .m.g signalling "abort" - server must answer the peer with null,
/    not propagate the abort error. Under \e 1 the user can drop into a
/    debug sub-REPL when a handler errors and exit it via `\`; that
/    surfaces as kerror("abort") out of fne, same as a handler that
/    explicitly does `'"abort"`. Forwarding the literal "abort" to the
/    peer used to fire help() on the peer's top-level REPL because
/    abort-as-an-expression-value is indistinguishable there from `\`
/    typed at the prompt. dispatch_sync now intercepts and answers null.
.m.g:{'"abort"}
\m i 15555
