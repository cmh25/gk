/ 15 non-callable .m.g -> client sees a "type" error, server stays
/    alive, async recovery installs a real .m.g and a follow-up sync
/    succeeds. If the server had crashed, the second handle op would
/    fail with a "conn"-class error instead.
h:3:(`;15555)

/ sync into bad .m.g -- expect (1;"type") from the trap.
r:.[{h 4:`a};,nul;:]
$[(1=r 0)&"type"~r 1;;'"first sync: expected (1;\"type\")"]

/ recover: default .m.s is `{. x}`, so async-eval can fix .m.g.
h 3:".m.g:{1+x}"

/ now sync should succeed -- proves both that the server didn't die
/ and that the connection is still usable.
$[2=h 4:1;;'"recovery sync: expected 2"]

3:h
\\
