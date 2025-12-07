# tradeoffs related to removal of digraphs

gk got rid of the digraphs for each-prior, each-right, each-left, and force monad:
`': /: \: +:`

```
k3         gk         description
-----      ----       --------------------
a+/:x      a+/x       a plus-each-right x
a+\:x      a+\x       a plus-each-left x
a+/x       +/a,x      a plus-over x
a+\x       +\a,x      a plus-scan x
+/x        +/x        plus-over x
+\x        +\x        plus-scan x
+:/x       {+x}/x     flip-over x
+:\x       {+x}\x     flip-scan x
a+'x       a+'x       a plus-each x
a+':x      a,ep[+]x   a plus-each-prior x
+':x       ep[+]x     plus-each-prior x
n +:/x     n {+x}/x   primitive do over
n +:\x     n {+x}\x   primitive do scan
b +:/x     b {+x}/x   primitive while over
b +:\x     b {+x}\x   primitive while scan
f/[x;y;z]  f/[x;y;z]  over
f\[x;y;z]  f\[x;y;z]  scan
f'[x;y;z]  f'[x;y;z]  each
```

Removal of the digraphs makes the language simpler, but the table above shows some tradeoffs worth discussion.

1. "a plus-over x" and "a plus-scan x" require a join

   The expense of join is a problem on its own, not just particular to this. Potentially, patterns like `+/a,x` could be detected at parse time and handled efficiently without a join.

2. each-prior became a named builtin, ep[f;x]

   No big deal. This has a pretty nice feel to it.

   ```
     ep[,]"asdf"
   ("sa"
    "ds"
    "fd")
     ep[-]1 2 3 4
   1 1 1
   ```

3. Primitive do/while with a primitive verb requires a lambda to force monadic

   Have to use a lambda to force a primitive to be monadic.
   ```
     5{,x}/1
   ,,,,,1
   ```
   No big deal. These lambdas can be detected and specially handled as primitive monads so we don't pay the price of using a lambda.

4. We lost dyad over and dyad scan.
   `+/[a;x]` is each-right
   `+\[a;x]` is each-left.

   No big deal. We can still do dyad over and dyad scan by using three args, but just ignoring the third one.
   ```
     {z;x,y}/["";"asdf";nul]
   "asdf"
     {z;x,y}\["";"asdf";nul]
   (""
    ,"a"
    "as"
    "asd"
    "asdf")
   ```
