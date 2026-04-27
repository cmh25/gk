/ 12 callback - drive the parent's fork port; each async spawns a child
/   whose .m.s syncs back to parent:15555 and bumps A.
/   the parent's inline listener processes the children's sync callbacks.
/
/ all 5 asyncs go on one dedup'd TCP connection, so a single fork child
/ services them. messages queue per-conn on the server side and are
/ processed one at a time, even though they hit the kernel buffer
/ back-to-back.
/
/ barrier trick: a sync on the same fork-child conn after the 5 asyncs
/ blocks until all of them (and their .m.s sync-backs to the parent)
/ have completed. no sleep needed.

(`;15556) 3:""
(`;15556) 3:""
(`;15556) 3:""
(`;15556) 3:""
(`;15556) 3:""

(`;15556) 4:"0"       / barrier: returns once all 5 asyncs are done
(`;15555) 4:"A"       / -> 5
\\
