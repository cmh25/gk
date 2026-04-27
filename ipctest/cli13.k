/ 13 stress: fire 100 asyncs at parent's fork port over one dedup'd conn.
/   a single fork child handles them all in order; per-msg .m.s does a
/   sync callback to parent:15555 that bumps A. if the in_dispatch
/   guard fails, the server errors with "sync re-entry on same handle"
/   or drops msgs.
/
/   barrier trick: messages on one conn are dispatched in order, so a
/   sync on the same fork-child conn after the asyncs blocks until all
/   prior asyncs (and their .m.s sync-backs) have completed. no sleep
/   needed.
{(`;15556) 3:""}'!100
(`;15556) 4:"0"       / barrier: returns once all 100 asyncs are done
(`;15555) 4:"A"       / -> 100
\\
