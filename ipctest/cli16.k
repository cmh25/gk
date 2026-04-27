/ 16 peer of a handler that signals "abort" must receive null. If the
/    fix regresses, the client either gets a remote-error (and r is
/    `(1;"abort")` after the trap) or - in interactive mode - prints
/    the help screen. Either way r will not match nul.
h:3:(`;15555)
/ trap returns (0;result) on success, (1;errstr) on error. We expect
/ success with result==nul.
r:.[{h 4:`x};,nul;:]
$[r~(0;nul);;'"expected (0;nul) from aborted handler"]

/ server must still be alive after answering null - replace .m.g with a
/ real handler via the default async eval-string path, then sync.
h 3:".m.g:{1+x}"
$[2=h 4:1;;'"recovery sync: expected 2"]

3:h
\\
