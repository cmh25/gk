# gk

gk is an implementation of the k programming language, originally invented by [Arthur Whitney](https://en.wikipedia.org/wiki/Arthur_Whitney_(computer_scientist)).

It's loosely based on k3, but with some changes suggested by [Stevan Apter](https://nsl.com/).

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

## gk differences from k3

### verbs
The verbs are all like k3. However, there is no "force monadic" (ex: #:'). The valence of primitive verbs (including composition) is always determined from context. A few examples:
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
The adverbs are similar to k3, but `/: \: ':` are gone. The only adverbs are `/ \ '`. How they operate depends on the context and the valence of the modified verb. Here's a quick synopsis:
```
      monad: each      f'x
 infix dyad: each      x f'y
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

eachprior is ep[f;x], ex: ep[-;3 2 1]
```

### slide
slide is an enhanced version of eachprior from k3 (aka eachpair).
```
_[n;f;a]
```

The first argument is a positive or negative integer. Its sign indicates the order the arguments are passed to the modified verb. Its absolute value indicates the number of steps the sliding window moves each time arguments are taken. The second argument is a verb. The third argument is a vector or list. These examples should make clear how it works:
```
  _[1;,;"abcd"]
("ab"
 "bc"
 "cd")
  _[-1;,;"abcd"]
("ba"
 "cb"
 "dc")
  _[2;,;"abcd"]
("ab"
 "cd")
  _[-2;,;"abcd"]
("ba"
 "dc")
```
The case of `_[-1;f;a]` in gk is equivalent to `f': a` in k3. gk also has a builtin for eachprior.
```
  ep[,]"asdf"
("sa"
 "ds"
 "fd")
```

### no underscore in names
Underscores are not allowed in variable names. It makes things cleaner since there is no longer a need to have space around the drop/cut verb.
```
  a:1
  b:1 2 3
  a_b
2 3
```
Most of the reserved names from k3 are still there, but with the _ removed.
```
  draw[10;10]
7 2 7 6 6 3 6 2 5 8
  1 2 3 dv 2
1 3
  2 vs 5
1 0 1
```
Some single-letter names have changed.
```
k3     gk
---    ----
 _n    nul
 _P    .z.P
 _T    .z.T
 _f    .z.f
 _h    .z.h
 _i    .z.i
 _t    .z.t
```

### $ replaces : in cond
```
  $[0;`a;`b]
`b
  $[1;`a;`b]
`a
```
