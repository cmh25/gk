"630. Group with interleaved data (np counter bug)"
/ Tests fix for shared np counter in group function
/ Bug: np was shared across all groups, causing out-of-bounds writes
/ when values interleaved (e.g., 0 1 0 1 0 1)

/ Basic interleaved case
=0 1 0 1 0 1

/ More groups, more interleaving
=0 1 2 0 1 2 0 1 2

/ Larger interleaved dataset
a:1000000#0 1 2 3 4
(#=a)~5

/ Verify group contents are correct
r:=0 1 0 1 0 1
(r 0)~0 2 4
(r 1)~1 3 5

\\
