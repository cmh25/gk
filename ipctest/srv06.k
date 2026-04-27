/ 06 fork isolation
/ .m.g increments c and returns it. inline would accumulate across conns;
/ forking: each NEW conn is a fresh child forked off the parent, so each
/ new conn starts at c=0 and returns 1 on its first sync.
c:0
.m.g:{c+:1; c}
\m f 15556
