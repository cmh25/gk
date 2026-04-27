/ 12 callback - fork workers report back to parent over the inline listener
/   parent listens on both ports: inline 15555 (control) and forking 15556 (work)
/   each async hit on 15556 spawns a child; the child's .m.s reaches back
/   to the parent's 15555 and sync-increments A.
A:0
.m.s:{(`;15555)4:"A+:1"}
\m i 15555
\m f 15556
