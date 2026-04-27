/ 14 stress: 20 asyncs to fork port; each triggers 3 sync-backs in the
/   child's .m.s - so 60 total sync calls flow through one child-to-parent
/   conn. all three counters should agree at 20.
/
/   barrier sync on the same fork-child conn after the asyncs serializes
/   the wait: it returns only after all 20 .m.s invocations (with their 3
/   sync-backs each) have completed.
{(`;15556) 3:""}'!20
(`;15556) 4:"0"       / barrier
(`;15555) 4:"A,B,C"   / -> 20 20 20
\\
