/ AFL crash (fuzzer01/04/06/07 sig:11, 2026-07-22): amend-assign (`:`) of a
/ value whose deep-COPY trips the stack guard corrupted the target container.
/ The `else` (plain-assign) branches of the index/whole amend did
/    _k(slot); slot=kcp(y); EC(slot);
/ i.e. freed the old cell, stored kcp's result, THEN checked for error.  When
/ kcp/kcp2 hit stack_low mid-copy it returned the KERR_STACK sentinel (11),
/ which has type-tag 0 (list): the store wrote a bare `11` into a live list
/ slot, EC bailed, and the poisoned container escaped into a scope.  A later
/ scope_cp -> dcp -> kcp deep-copied element 11 and dereferenced (void*)0xb.
/ Fixed at 7 sites in k.c by copying into a temp, EC-checking, THEN freeing the
/ old cell and storing -- so the error path leaves the container untouched
/ (matching the if(b) amend_fe branch just above each).  The stack-error branch
/ itself needs an unbounded/cyclic value to trigger deterministically (gk's
/ copy-on-store forbids one, so any buildable value is shallow enough to copy);
/ this locks in that the reordered happy paths still produce the right values.
/
/ single int index, function-value (k.c:1529 kcp2)
@[(0;1;2);1;:;(+)]
/ single int index, nested-list value (k.c:1529 kcp2)
@[(0;1;2);1;:;(3;4 5)]
/ deep single index path, function-value (k.c:1359 kcp2)
.[(0;(1;2));1 0;:;(*)]
/ int-index VECTOR, function-value broadcasts to each slot (k.c:1539 kcp)
@[(0;1;2;3);1 2;:;(+)]
/ int-index VECTOR, list distributes elementwise (k.c:1567 kcp2)
@[(0;1;2;3);1 2;:;((7;8);9 9)]
/ whole-list amend (null index), function-value broadcasts (k.c:1578 kcp)
@[(0;1;2);nul;:;(-)]
/ whole-list amend, list distributes (k.c:1613 kcp2)
@[(0;1;2);nul;:;((1;2);3;4 5)]
/ dict whole-amend to a function-value (k.c:1148 kcp)
@[.((`a;1);(`b;2));nul;:;(+)]
\\
