/ 05 error propagation
/ sync errors from the server raise locally; use protected eval (3-arg .)
/ to turn them into (1;"msg") pairs so the script can keep going.
h:3:("localhost";15555)
.[{h 4:x};,"1+`foo";   :]     / -> (1;"type")
.[{h 4:x};,"nosuchvar";:]     / -> (1;"value")
.[{h 4:x};,"'\"oops\"";:]     / client-raised custom error -> (1;"oops")
h 4:"2+2"                     / still alive -> 4
3:h
\\
