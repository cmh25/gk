/ 04 .m.c fires on peer disconnect
/ .m.c is a string of K code (not a function). It's eval'd when the
/ conn closes. We bump a global counter; check it in the REPL after the
/ client disconnects:  closed
closed:0
.m.c:"closed+:1"
\m i 15555
