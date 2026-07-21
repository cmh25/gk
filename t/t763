/ 0x45 adverb-with-args compose: when the combined adverb string blows the
/ 255 cap ('length), the error path k_'d the node's operand pair but freed
/ only half of it -- every such error leaked the composition node (found by
/ the 2026-07-14 corpus ASan sweep; the trap makes the error a value).
@[{{""}'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''/*$};();:]
/ an in-cap compound adverb next door still evaluates
+/'(1 2;3 4)
\\
