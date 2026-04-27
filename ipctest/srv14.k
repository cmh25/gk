/ 14 stress: handler does three sync-backs per inbound async, exercising
/   nested-sync-wait reuse on the SAME parent inline conn from the fork
/   child. any re-entrancy glitch would show up as A,B,C out of sync or
/   a "sync re-entry" error on the parent's callback conn.
A:0;B:0;C:0
.m.s:{(`;15555)4:"A+:1";(`;15555)4:"B+:1";(`;15555)4:"C+:1"}
\m i 15555
\m f 15556
