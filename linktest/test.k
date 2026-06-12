/ Regression test for `f 2:(e;t)` link object code.
/ Driven by linktest/run.py, which compiles linktest/lib.c to a shared
/ object and passes its path in the GKLINKLIB environment variable.
/ Self-checking: exits 1 on the first failed assertion, else prints "OK".
ck:{[got;want] if[~got~want; `0:,"FAIL: ",($got)," != ",$want; exit 1]}

f:getenv"GKLINKLIB"
ans:f 2:("t_answer";0)
neg:f 2:("t_neg";1)
add:f 2:("t_add";2)
s3:f 2:("t_sum3";3)

/ apply forms
ck[ans[];42]              / niladic, bracket
ck[val ans;1]            / valence-0 link reports 1 (no valence-0 fns)
ck[neg 9;-9]             / monadic juxtaposition
ck[neg[10];-10]         / monadic bracket
ck[add[3;4];7]          / dyadic bracket
ck[add . 5 6;11]        / dot apply (spreads)
ck[s3[1;2;3];6]         / valence 3

/ heap returns via the gk_mk* callback (f64 atom, char vector, i32 vector)
half:f 2:("t_half";1)
greet:f 2:("t_greet";0)
iota:f 2:("t_iota";1)
ck[half 7;3.5]
ck[greet[];"gk"]
ck[iota 5;0 1 2 3 4]

/ value semantics
g:add                    / alias
ck[g[8;9];17]
fns:(add;neg)            / stored in a list
ck[(fns 0)[2;5];7]
ck[(fns 1)5;-5]

/ projection (partial application): builds a valence-1 function
ck[(add[3;])4;7]         / trailing hole
ck[(add[;4])3;7]         / leading hole
ck[(add 3)4;7]           / juxtaposition partial
ck[(s3[1;;3])20;24]      / middle hole, valence 3 -> 1
ck[s3[1][2;3];6]         / partial then complete

/ adverbs (first-class)
ck[neg'1 2 3;-1 -2 -3]                 / each
ck[add'[1 2 3;10 20 30];11 22 33]      / each-pair (bracket)
ck[1 2 3 add'10 20 30;11 22 33]        / each-pair (infix)
ck[add/1 2 3 4;10]                     / fold
ck[add\1 2 3 4;1 3 6 10]               / scan

/ errors: @[f;x;:] traps, giving (0;result) or (1;errorstring)
ck[@[{"/no/such.so" 2:("t_add";2)};0;:];(1;"domain")]   / missing file
ck[@[{f 2:("nope";2)};0;:];(1;"domain")]                / missing symbol
ck[@[{f 2:("t_add";`x)};0;:];(1;"type")]                / bad valence type
ck[@[{add[1;2;3]};0;:];(1;"valence")]                   / too many args

`0:,"OK"
\\
