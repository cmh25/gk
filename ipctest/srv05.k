/ 05 error propagation
/ default . handler will evaluate whatever we send; errors come back to
/ the client as remote errors. run with \e 0 so handler errors are just
/ printed and sync'd back; under \e 1 (the script default) a failing
/ handler would drop into the debug sub-REPL and leave the client
/ hanging until `\` is typed on the server.
\e 0
\m i 15555
