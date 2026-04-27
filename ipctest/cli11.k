/ 11 oneshot - (h;p) on the left of 3: / 4: skips the explicit open step
/   first call opens; subsequent calls to the same (h;p) reuse the fd
/   (ipc_open dedups); handle is NOT closed for you - close with 3:h

/ sync one-shots
(`;15555) 4:"2+2"           / -> 4
(`;15555) 4:(`f;10 20 30)   / apply server-side f -> 60

/ async one-shots (side-effects on server; return null)
(`;15555) 3:"c+:1"
(`;15555) 3:"c+:10"
(`;15555) 3:"c+:100"

/ observe the effect
(`;15555) 4:"c"             / -> 111

/ dedup: grab the cached fd, verify it's the same one one-shot was using
h:3:(`;15555)               / returns existing fd (dedup), no new connect
h 4:"c"                     / -> 111, same peer view
3:h                         / close; drops the only remaining ref

/ after explicit close, next (h;p) call opens a fresh connection
(`;15555) 4:"c"             / -> 111 (state on server is global)

/ trap an open-error on unreachable port
.[{(`;15999) 4: x};,"ok";:]  / -> (1;"conn: Connection refused")

\\
