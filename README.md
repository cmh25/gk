# gk

This is an implementation of the k programming language, originally invented by [Arthur Whitney](https://en.wikipedia.org/wiki/Arthur_Whitney_(computer_scientist)).

It's loosely based on k32, but with some changes suggested by [Stevan Apter](https://nsl.com/).

Some other notable open source implementations of k:

https://github.com/JohnEarnest/ok

https://codeberg.org/ngn/k

https://github.com/kevinlawler/kona

## build instructions
linux/mac:
```
make
make test
```
win: (requires visual studio cli)
```
make.bat
```

## gk differences from k32

### verbs
The verbs are all like k32. However, there is no "force monadic" (ex: #:'). The valence of primitive verbs (including composition) is always determined from context. A few examples:
```
  #'(1 2;3 4 5)
2 3
  !'2 3 4
(0 1
 0 1 2
 0 1 2 3)
  *'("ab";"cd";"ef")
"ace"
  (%-)'1 2 3
-1 -0.5 -0.3333333
```
### adverbs
The adverbs are similar to k32, but `/: \: ':` are gone. The only adeverbs are `/ \ '`. How they operate depends on the context and the valence of the modified verb. Here's a quick synopsis:
```
      monad: each      f'x
       dyad: eachprior f'x
 infix dyad: slide     n f'x
prefix dyad: each      f'[x;y]
      other: each      f'[x;y;z]

      monad: scanm     f\x
      monad: do        n f\x
      monad: while     b f\x
 infix dyad: eachleft  x f\y
prefix dyad: scand     f\x
      other: scan      f\[x;y;z]

      monad: overm     f/x
      monad: do        n f/x
      monad: while     b f/x
 infix dyad: eachright x f/y
prefix dyad: overd     f/x
      other: over      f/[x;y;z]
```

### slide adverb
The slide adverb is an enhanced version of eachpair (aka eachprior). It only takes effect in the context of infix notation.

The left argument is a positive or negative integer. Its sign indicates the order the arguments are passed to the modified verb. Its absolute value indicates the width of the sliding window. These examples should make clear how it works:
```
  1 ,' "abcd"
("ab"
 "bc"
 "cd")
  -1 ,' "abcd"
("ba"
 "cb"
 "dc")
  2 ,' "abcd"
("ab"
 "cd")
  -2 ,' "abcd"
("ba"
 "dc")
```
So `-1 f' a` in gk is equivalent to `f': a` in k32. gk gives up the k32 behavior of prepending the left argument of eachprior to the result in order to have a much more flexible adverb.

I believe its a good tradeoff since I've never found a good use for something like this in k32:
```
  `a-':1 2 3
(`a;1;1)
```
However, you can still do that in gk if you need to for some reason. Ex:
```
  `a,-1-'1 2 3
(`a;1;1)
```

### no underscore in names
Underscores are not allowed in variable names. It makes things cleaner since there is no longer a need to have space around the drop/cut verb.
```
  a:1
  b:1 2 3
  a_b
2 3
```
Most of the reserved names from k32 are still there, but with the _ removed.
```
  draw[10;10]
7 2 7 6 6 3 6 2 5 8
  1 2 3 dv 2
1 3
  2 vs 5
1 0 1
```
I didn't want to reserve single-letter names, so some have changed.
```
k32    gk
---    ---
 _n    nul
 _t    ts
 _T    TS
 _f    .z.s
```

### $ replaces : in cond
```
  a:0;$[a;`a;`b]
`b
  a:1;$[a;`a;`b]
`a
```
