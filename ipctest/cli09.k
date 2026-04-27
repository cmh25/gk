/ 09 one client of many - does 10 syncs then exits.
/ driver: for i in $(seq 1 20); do ./gk ipctest/cli09.k & done; wait
/ server's `n should reach 200.  Check it in the server REPL:  n
h:3:("localhost";15555)
h 4:"0"
h 4:"0"
h 4:"0"
h 4:"0"
h 4:"0"
h 4:"0"
h 4:"0"
h 4:"0"
h 4:"0"
h 4:"0"
3:h
\\
