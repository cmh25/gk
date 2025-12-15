"631. Large allocation crossing 32MB boundary (buddy_level bug)"
/ Tests fix for buddy_level() not detecting oversized allocations
/ Bug: allocations >32MB were handled by buddy instead of malloc fallback
/ because buddy_level() capped at level 20 without checking size fit

/ Single large group >32MB (9M elements * 4 bytes = 36MB)
a:9000000#42
r:=a
#r

/ Verify the group content is correct
#(=a)0

/ Large group from random data (15M * 4 = 60MB per group worst case)
a:15000000#7
(#=a)~1

\\
