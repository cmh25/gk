/ db (deserialize) structural-subtype trust bugs (bughunt r7, 2026-07-15).
/ A K value's 8-bit subtype tag routes it to apply/eval machinery that
/ dereferences fixed slots as verbs, args, scopes, or -- for a 2:-linked C
/ function (0xdc) -- a raw C function POINTER that linkcall CALLs.  db rebuilt
/ the subtype straight from untrusted bytes (an IPC body or `db 6:file`), so a
/ hostile blob forged a value whose shape the rest of the interpreter trusts:
/   - 0xdc with a chosen slot-0 pointer -> arbitrary code execution
/   - 0xc3 (lambda) whose slot 0 isn't the definition TEXT -> wild deref in fnd
/   - 0x80 (dict) whose values vector is shorter than its keys -> dget/dset OOB
/   - 0xda/0xd9/0xd0/0xd7 on a <2-slot list -> over-read in the wrapper
/   - 0xc6/0xc7 (int-index builtin) atom with an OOB index -> a garbage
/     function pointer that apply CALLs (a second RCE); name-verb subtypes on a
/     non-symbol atom -> pointer deref of an int payload
/ Fix: reject 0xdc on BOTH sides (bd refuses to serialize a process-local
/ pointer; db refuses to trust one), and validate every other structural
/ subtype's shape at the deserialize trust boundary.
/
/ @[db;x;:] traps the decode so a rejected blob prints (1;"type") instead of
/ halting; printing the trap result USES it, so a value that slipped through
/ would crash the test rather than silently pass.
.[db;,bh"0300000000000000dc000000000000000200000000000000080000000000000000000000414141414100000001000000000000000000000000000000";:]
.[db;,bh"0300000000000000c3000000000000000400000000000000010000000000000000000000414141410600000000000000000000000000000080000000000000000300000000000000ffffffff00000000000000000000000000000000ffffffff0000000000000000000000000000000001000000000000000000000000000000060000000000000000000000";:]
.[db;,bh"030000000000000080000000000000000300000000000000fcffffff00000000000000000300000000000000610062006300ffffffff0000000000000000000000000000000001000000000000000000000003000000";:]
.[db;,bh"0300000000000000da00000000000000010000000000000001000000000000000000000000000000";:]
.[db;,bh"0300000000000000d9000000000000000100000000000000060000000000000000000000";:]
.[db;,bh"0300000001000000c600000000000000ffffff7f";:]
.[db;,bh"0300000001000000c700000000000000ffffff7f";:]
.[db;,bh"0300000001000000c90000000000000041414141";:]
/ an INTERNAL parse-node subtype (0x41) on an int atom: the reducer would read
/ the payload as a parse-node ko pointer (found via rr on an IPC crash).  db now
/ whitelists the subtype to what serialize emits (0, 0x80, 0x81, 0xc0..0xdc).
.[db;,bh"03000000010000004100000000000000ff8fff7f";:]
/ bd refuses to serialize a 2:-linked C function (nothing meaningful to write)
/ -- exercised in linktest where a .so is available; here just the db side.
/ legitimate values round-trip unchanged: data, lambda, closure, dict, verbs,
/ derived verbs, file verb, and the string->number cast (form) still parses.
(db bd 1 2 3)
(db bd(1;2.5;`a;"xy"))
(db bd{x+y})[3;4]
g:db bd{c:x;{c+y}}[10];g[5;3]
(db bd .((`a;1);(`b;2)))`b
2(db bd(+))/1 2 3 4
(db bd(,/))("ab";"cd")
(db bd(0:))~(0:)
0j$"12345"
0$"00042"
\\
