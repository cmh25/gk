/ 13 stress: 100 back-to-back callbacks through a single fork-child conn.
/   verifies in_dispatch queueing holds up at volume - if the nested
/   poll re-enters dispatch, A ends up < 100 and the server reports
/   "sync re-entry on same handle".
A:0
.m.s:{(`;15555)4:"A+:1"}
\m i 15555
\m f 15556
