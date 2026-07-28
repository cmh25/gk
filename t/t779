/ hostile db/0: input must be REJECTED, not crash.  Four bugs, all found by the
/ 2026-07-25 width/sentinel audit, all reachable from untrusted input (db x,
/ 2:/1:/6: file read, and an unauthenticated IPC frame via db_buf).
/ Errors are trapped with @[f;x;:] so one file can cover all four.

/ 1. dict blob whose VALUES slot is an int vector (4 bytes/elt) with the same
/ COUNT as the keys: dict.c walks it 8 bytes/elt, so k_(pv[i]) did
/ ++((ko*)(b(48)&x))->r -- a +1 write at an attacker-chosen address.
d:3 0 0 0 0 0 0 0 128 0 0 0 0 0 0 0 3 0 0 0 0 0 0 0 252 255 255 255 0 0 0 0 0 0 0 0 2 0 0 0 0 0 0 0 97 0 98 0 255 255 255 255 0 0 0 0 0 0 0 0 2 0 0 0 0 0 0 0 16 0 0 0 0 0 0 31 1 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0
@[{db ci x};d;:]

/ A dict's values slot must be a plain general list, not a 0x81 argument
/ plist with the same T=0 storage tag. Mutate that subtype in an empty dict.
d2:ic bd[.()]
d2[48]:129
@[{db ci x};d2;:]

/ 2. 0xd9 projection with an ATOM in slot 1; kprint_ walks that slot as a
/ general list, so merely DISPLAYING the decoded value segfaulted.
p:3 0 0 0 0 0 0 0 217 0 0 0 0 0 0 0 2 0 0 0 0 0 0 0 1 0 0 0 198 0 0 0 0 0 0 0 8 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 65 65 65 65
@[{db ci x};p;:]

/ A serialized lambda's scope slot is null or a captured-bindings dict.
/ Replacing it with a named builtin used to pass the base-tag-only checks;
/ applying the result then treated the builtin's inline payload as a scope.
l:bd[{x}]
bi:bd[abs]
c:(59#l),(4_bi),71_l
@[db;c;:]

/ Structural function wrappers must pin callable slots, not merely the outer
/ list subtype.  Replace each valid serialized verb with an ordinary int of
/ the same wire width; these used to reach fe/fc, where a noun's low byte could
/ be mistaken for a primitive opcode.  Composition also validates every member.
ni:bd[7]
d0:bd[(5+/)]
c:(40#d0),4_ni
@[db;c;:]
p:bd[+[1;]]
c:(24#p),(4_ni),40_p
@[db;c;:]
c5:ic[bd[(+/-)]]
c5[105]:0
@[{db ci x};c5;:]
d7:bd[(3{x+1}/)]
c:(40#d7),4_ni
@[db;c;:]

/ Adverb wrappers accept only the three parser adverb bytes.  A forged suffix
/ used to reach unchecked strchr subtraction in the apply path.
da:ic[bd[(5+/)]]
da[(#da)-1]:120
@[{db ci x};da;:]

/ 3. unbounded nesting: 20 wire bytes buy one C frame, so a small blob blew the
/ stack (serialize has a depth guard, deserialize had none).
u:0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0
v:3 0 0 0,(120000#u),1 0 0 0 0 0 0 0 0 0 0 0 7 0 0 0
@[{db ci x};v;:]

/ 4. 0: fixed-width: the field widths are range-checked individually but their
/ SUM accumulated in a u32, so 1431655770+1431655768+1431655768 wrapped to 10,
/ passed the row-length gate on a 10-byte row, and indexed ~1.4GB past it.
"z779"0:,"0123456789"
@[{("I  ";1431655770 1431655768 1431655768)0:x};"z779";:]
/ same file, honest widths: still works
("I  ";2 4 4)0:"z779"
del"z779"

/ positive control: legitimate nesting still round-trips
db bd ,,,1
\\
