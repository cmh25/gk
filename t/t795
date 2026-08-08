/ AFL regression: a train member that is not a verb (e.g. the naked adverb in
/ (\:) or (\>)) must compile to a parse error.  bc()'s t7 train loop stored
/ train_member_bc's raw error sentinel into the 0xc5 member list, and the
/ sentinel token also skipped the eval error funnel (case-0 continue), so the
/ bare sentinel got deref'd by kprint/kcp/where/proj_captures downstream.
@[.;"(\\:)sin cos";:]
@[.;"(\\>)in in";:]
@[.;"2+(\\:)sin cos";:]
@[.;"(\\:)sin cos[3]";:]
/ real trains and folds are unaffected
(+-)1 2
(*|)1 2 3
+/1 2 3
\\
