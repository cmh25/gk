# gk Reference Manual

gk is a dialect of the k programming language. This reference covers all primitives, builtins, and syntax.

New to gk? Try the [tutorial](tutorial.md) first. If you already know **k3**, see [gk vs k3](k3.md) for dialect differences.

## Contents

- [Types](#types)
  - [Special Values](#special-values)
  - [Atoms, Vectors, and Lists](#atoms-vectors-and-lists)
- [Primitive Verbs](#primitive-verbs)
  - [Monadic](#monadic)
  - [Dyadic](#dyadic)
- [Adverbs](#adverbs)
  - [Each-Prior](#each-prior)
- [Indexing](#indexing)
  - [Index at Depth](#index-at-depth)
  - [Dictionary Indexing](#dictionary-indexing)
- [Assignment](#assignment)
  - [Global Assignment](#global-assignment)
- [Dictionaries](#dictionaries)
  - [Creation](#creation)
  - [Access](#access)
  - [Update](#update)
  - [Dimensional Indexing](#dimensional-indexing)
  - [Elided Index Dimension](#elided-index-dimension)
- [Functions (Lambdas)](#functions-lambdas)
  - [Closures](#closures)
- [Valence](#valence)
  - [What is Valence?](#valence-what)
  - [Valence Zero?](#valence-zero)
  - [Prototypes](#valence-prototypes)
  - [Applying Functions to Empty Vectors](#valence-empty)
  - [Why `[]` Passes `nul`](#valence-empty-brackets)
  - [Minimum Valence is 1](#valence-minimum)
  - [Dictionaries](#valence-dictionaries)
  - [Summary](#valence-summary)
- [Builtins](#builtins)
  - [Math](#math)
  - [Bitwise and Integer](#bitwise-and-integer)
  - [Number Theory](#number-theory)
  - [String](#string)
  - [List](#list)
  - [Conversion and Serialization](#conversion-and-serialization)
  - [Time](#time)
  - [Random](#random)
  - [I/O](#io)
  - [System](#system)
  - [Matrix](#matrix)
- [System Commands](#system-commands)
- [IPC](#ipc)
  - [Quick Start](#quick-start)
  - [Client](#client)
  - [Server](#server)
  - [Handlers](#handlers)
  - [Draining a Batch of Asyncs](#draining-a-batch-of-asyncs)
- [Timer](#timer)
  - [Quick Start](#timer-quick-start)
  - [Surface](#timer-surface)
  - [Set-form Validation](#timer-set-form-validation)
  - [Pass by Reference (sym)](#timer-pass-by-reference)
  - [Tick Output](#timer-tick-output)
  - [Semantics](#timer-semantics)
  - [Why a Builtin and Not `\t`?](#timer-why-builtin)
- [Watches](#watches)
  - [Quick Start](#watch-quick-start)
  - [Surface](#watch-surface)
  - [Which Assignments Fire](#watch-which-assignments)
  - [Re-entrancy](#watch-reentrancy)
  - [Errors](#watch-errors)
  - [Semantics](#watch-semantics)
- [Errors](#errors)
  - [Error Trapping](#error-trapping)
- [Verb Reference](#verb-reference)
  - [Dyadic Conformability](#dyadic-conformability)
  - [Numeric Type Rules](#numeric-type-rules)
  - [Dyadic](#dyadic-1)
    - [`a+x` plus](#d-plus)
    - [`a-x` minus](#d-minus)
    - [`a*x` times](#d-times)
    - [`a%x` divide](#d-divide)
    - [`a&x` min / and](#d-min-and)
    - [`a|x` max / or](#d-max-or)
    - [`a<x` `a>x` `a=x` comparisons](#d-comparisons)
    - [`a~x` match](#d-match)
    - [`a.x` apply](#d-apply)
    - [`a!x` mod / dict](#d-mod-dict)
    - [`a@x` at / index](#d-at-index)
    - [`a?x` find](#d-find)
    - [`a#x` take / reshape](#d-take-reshape)
    - [`a_x` drop / cut](#d-drop-cut)
    - [`a^x` power](#d-power)
    - [`a,x` join](#d-join)
    - [`a$x` cast / pad](#d-cast-pad)
  - [Monadic](#monadic-1)
    - [`+x` flip](#m-flip)
    - [`-x` negate](#m-negate)
    - [`*x` first](#m-first)
    - [`%x` reciprocal](#m-reciprocal)
    - [`&x` where](#m-where)
    - [`|x` reverse](#m-reverse)
    - [`<x` grade up](#m-grade-up)
    - [`>x` grade down](#m-grade-down)
    - [`=x` group](#m-group)
    - [`~x` not](#m-not)
    - [`.x` value](#m-value)
    - [`!x` enumerate / keys](#m-enumerate)
    - [`@x` atom](#m-atom)
    - [`?x` unique](#m-unique)
    - [`#x` count](#m-count)
    - [`_x` floor](#m-floor)
    - [`^x` shape](#m-shape)
    - [`,x` enlist](#m-enlist)
    - [`$x` string from data](#m-string)

## Types

| Type | Code | Example | Description |
|------|------|---------|-------------|
| i32 | 1 | `42` | 32-bit signed integer |
| f64 | 2 | `3.14` | 64-bit IEEE float |
| char | 3 | `"a"` | single character |
| symbol | 4 | `` `name `` | interned string |
| dict | 5 | `.()` | dictionary |
| null | 6 | `nul` | null value |
| verb | 7 | `+`, `{x+y}` | primitive/lambda |
| i64 | 8 | `42j` | 64-bit signed integer |
| f32 | 9 | `3.14e` | 32-bit IEEE float |
| list | 0 | `(1;2.0;"a")` | heterogeneous list |
| i32 vector | -1 | `1 2 3` | homogeneous i32 array |
| f64 vector | -2 | `1 2 3.0` | homogeneous f64 array |
| char vector | -3 | `"abc"` | string |
| symbol vector | -4 | `` `a`b`c `` | symbol list |
| i64 vector | -8 | `1 2 3j` | homogeneous i64 array |
| f32 vector | -9 | `1 2 3.0e` | homogeneous f32 array |

### Special Values

| Value | Type | Description |
|-------|------|-------------|
| `0N` | i32 | null integer |
| `0I` | i32 | positive infinity |
| `-0I` | i32 | negative infinity |
| `0Nj` | i64 | null integer |
| `0Ij` | i64 | positive infinity |
| `-0Ij` | i64 | negative infinity |
| `0n` | f64 | NaN |
| `0i` | f64 | positive infinity |
| `-0i` | f64 | negative infinity |
| `0ne` | f32 | NaN |
| `0ie` | f32 | positive infinity |
| `-0ie` | f32 | negative infinity |
| `nul` | null | null value |

### Atoms, Vectors, and Lists

An **atom** is a single value of any type.

```
  42
42
  3.14
3.14
```

A **vector** is a homogeneous array of atoms (type code < 0).

```
  1 2
1 2
  1.2 3.4
1.2 3.4
```

A **list** is a heterogeneous collection (type 0). Lists can contain atoms, vectors, or other lists.

```
  (1;2.0)
(1;2.0)
  (1;2 3;(`a;"asdf"))
(1
 2 3
 (`a;"asdf"))
```

A homogeneous list is automatically promoted to a vector.
```
  (1;2;3)
1 2 3
```

The `kv` builtin can be used to construct a non-promoted homogeneous list.
```
  kv 1 2 3
(1;2;3)
```

Most dyadic verbs operate element-wise on vectors and recurse into lists.

---

## Primitive Verbs

### Monadic

| Verb | Name | Example | Result |
|------|------|---------|--------|
| `+x` | flip | `+(1 2;3 4)` | `(1 3;2 4)` |
| `-x` | negate | `-3` | `-3` |
| `*x` | first | `*1 2 3` | `1` |
| `%x` | reciprocal | `%4` | `0.25` |
| `&x` | where | `&1 0 1 0` | `0 2` |
| <code>&#124;x</code> | reverse | <code>&#124;1 2 3</code> | `3 2 1` |
| `<x` | grade up | `<3 1 2` | `1 2 0` |
| `>x` | grade down | `>3 1 2` | `0 2 1` |
| `=x` | group | `=0 1 0 1` | `(0 2;1 3)` |
| `~x` | not | `~0 1` | `1 0` |
| `.x` | value | `."1+2"` | `3` |
| `!x` | enumerate / keys | `!5` | `0 1 2 3 4` |
| `@x` | atom | `@1` → `1`, `@1 2` → `0` | is atom? |
| `?x` | unique | `?1 2 1 3` | `1 2 3` |
| `#x` | count | `#1 2 3` | `3` |
| `_x` | floor | `_3.7` | `3` |
| `^x` | shape | `^(1 2;3 4)` | `2 2` |
| `,x` | enlist | `,1` | `,1` |
| `$x` | format | `$42` | `"42"` |

### Dyadic

| Verb | Name | Example | Result |
|------|------|---------|--------|
| `a+x` | plus | `2+3` | `5` |
| `a-x` | minus | `5-3` | `2` |
| `a*x` | times | `3*4` | `12` |
| `a%x` | divide | `10%4` | `2.5` |
| `a&x` | min/and | `3&5` | `3` |
| <code>a&#124;x</code> | max/or | <code>3&#124;5</code> | `5` |
| `a<x` | less | `3<5` | `1` |
| `a>x` | greater | `5>3` | `1` |
| `a=x` | equal | `3=3` | `1` |
| `a~x` | match | `1 2~1 2` | `1` |
| `a.x` | apply | `{x+y}.(1;2)` | `3` |
| `a!x` | mod/dict | `7!3` | `1` |
| `a@x` | at/index | `1 2 3@1` | `2` |
| `a?x` | find | `1 2 3?2` | `1` |
| `a#x` | take/reshape | `3#1 2` | `1 2 1` |
| `a_x` | drop/cut | `2_1 2 3 4` | `3 4` |
| `a^x` | power | `2^10` | `1024.0` |
| `a,x` | join | `1 2,3 4` | `1 2 3 4` |
| `a$x` | cast/pad | `3$"ab"` | `" ab"` |

---

<h2 id="adverbs">Adverbs</h2>

| Adverb | Name | Example | Result |
|--------|------|---------|--------|
| `f'x` | each | `#'"ab""cde"` | `2 3` |
| `a f'x` | each (dyad) | `1 2+'3 4` | `4 6` |
| `f/x` | over/fold | `+/1 2 3` | `6` |
| `a f/x` | each-right | `1 2+/3 4` | `(4 5;5 6)` |
| `f\x` | scan | `+\1 2 3` | `1 3 6` |
| `a f\x` | each-left | `1 2+\3 4` | `(4 5;5 6)` |
| `n f/x` | do n times | `5{x,x}/1` | `1 1 1...` |
| `b f/x` | while | `{x<100}{x*2}/1` | `128` |
| `n f\x` | do n times (scan) | `5{x+1}\0` | `0 1 2 3 4 5` |
| `b f\x` | while (scan) | `{x<10}{x+1}\0` | `0 1 2...10` |

### Each-Prior

```
  ep[-]1 2 3 4 5
1 1 1 1
  ep[,]"abc"
("ba"
 "cb")
```

---

## Indexing

```
  a:1 2 3 4 5
  a@2          / index
3
  a[2]         / same
3
  a@1 3        / multiple indices
2 4
  a[1 3]       / same
2 4
```

### Index at Depth

```
  m:(1 2 3;4 5 6)
  m[0;1]       / row 0, col 1
2
  m[;1]        / all rows, col 1
2 5
```

### Dictionary Indexing

```
  d.a:1 2 3
  d.b:4 5 6
  d[`a]        / by key
1 2 3
  d[;1]        / index each value
2 5
```

---

## Assignment

```
  a:1 2 3      / simple assign
  a[1]:99      / indexed assign
  a
1 99 3
  a+:1         / modify in place
  a
2 100 4
```

### Global Assignment

```
  f:{a::1}     / assign to global from function
```

---

<h2 id="dictionaries">Dictionaries</h2>

A dictionary is a map of symbols to values.

### Creation

```
  d:.()            / empty
  d.a:1; d.b:2     / dot syntax
  d[`c]:3          / bracket syntax
  !d               / keys
`a `b `c
  d[]              / values
1 2 3
```

### Access

```
  d:.();d.a:1;d.b:2
  d.a              / dot
1
  d@`a             / at
1
  d[`a]            / bracket
1
  d[`a`b]          / multiple keys
1 2
```

### Update

```
  d:.();d.a:1;d.b:2;d[`c]:3
  d.a:10           / single
  d[`b`c]:20 30    / multiple
```

### Dimensional Indexing

Dictionaries can be indexed along multiple dimensions.

```
  d:.+(`a`b`c;(0 1 2;3 4 5;6 7 8))
  d[`b;1]
4
```

### Elided Index Dimension

An elided index is the same as the entire index.
```
  d:.+(`a`b`c;(0 1 2;3 4 5;6 7 8))
  d[;1]
1 4 7
  d[;]
(0 1 2
 3 4 5
 6 7 8)
```

---

## Functions (Lambdas)

Can have up to three implicit params: `x` `y` `z`.

```
  f:{x+y+z}    / implicit x,y,z
  f[1;2;3]
6
  f:{[x;y]x+y} / explicit params (same slots as implicit x,y)
  f[2;3]
5
```

### Closures

If a function returns a locally defined function, it is returned as a closure.

```
  f:{i:0;{i+:1;i}}
  g:f`
  h:f`
  g`
1
  g`
2
  h`
1
  h`
2
```

If a function returns a dictionary of functions, any locally defined functions in the dictionary share a closure.

```
  f:{i:j:0;r.inci:{(i+:1;j)};r.incj:{(i;j+:1)};r}
  g:f`
  g.inci`
1 0
  g.inci`
2 0
  g.incj`
2 1
  g.incj`
2 2
```

---

## Valence

<a id="valence-what"></a>
### What is Valence?

Valence is the number of parameters a function expects. 

```
  val {x}
1
  val {x,y}
2
  val {x,y,z}
3
  val {[x;y;z;u;v]x,y,z,u,v}
5
```

If a function is called with fewer than the required number of parameters, the result is a projection.
```
  f:{x,y,z}
  val f
3
  g:f[1;2]
  g
{x,y,z}[1;2]
  val g
1
  g 3
1 2 3
```

If a function is called with more than the required number of parameters, the result is a valence error.
```
  {x}[1;2]
valence error
{x}[1;2]
^
>
```

<a id="valence-zero"></a>
### Valence Zero?

So, what is the valence of `{1}`? It doesn't reference any parameters, so we might be tempted to say its valence is zero. However, there are a couple of problems with that.

1. What is the function's domain if its valence is zero?
2. What is the syntax to execute a valence zero function?

gk takes the position that there is no such thing as a valence zero function. All functions have at least a valence of one (the one parameter may be ignored).

It's arguable that `{1}[]` is an answer to (2), but gk takes the position that `[]` is not empty, but rather the same as `[nul]`.

The reason for this is to be consistent with prototypes.

<a id="valence-prototypes"></a>
### Prototypes

Every empty vector has a prototype—a default atom for its type. The prototype is what you get when you take the first element of an empty vector.

```
  nul~*()
1
  *0#0
0
  *0#0.0
0.0
  *0#""
" "
  *0#`
`
```

<a id="valence-empty"></a>
### Applying Functions to Empty Vectors

gk's rule: **applying a function to an empty vector passes the prototype**.

```
f . v   ≡   f[*v]
```

For any empty vector `v`, `f . v` is equivalent to applying `f` to the prototype of `v`.

```
f . ()      ≡   f[*()]      ≡   f[nul]   ≡   f@nul   ≡   f nul
f . 0#0     ≡   f[*0#0]     ≡   f[0]     ≡   f@0     ≡   f 0
f . 0#0.0   ≡   f[*0#0.0]   ≡   f[0.0]   ≡   f@0.0   ≡   f 0.0
f . ""      ≡   f[*""]      ≡   f[" "]   ≡   f@" "   ≡   f " "
f . 0#`     ≡   f[*0#`]     ≡   f[`]     ≡   f@`     ≡   f `
```

This is consistent with how the same operations work with a vector of one element.
```
f . ,1      ≡   f[*,1]      ≡   f[1]     ≡   f@1     ≡   f 1
```

<a id="valence-empty-brackets"></a>
### Why `[]` Passes `nul`

With this model, `[]` is not a special case—it's just the general instance of the rule:

- `f[]` is `f . ()`
- `f . ()` is `f[*()]`
- `*()` is `nul`
- therefore `f[]` is `f[nul]`

This explains why `{x}[]` returns `nul` rather than being a projection:

```
  nul~{x}[]
1
  {1,x}[]
(1;)
```

The function executes with `x` bound to `nul` (the prototype of `()`), not with `x` unbound.

<a id="valence-minimum"></a>
### Minimum Valence is 1

Since there's no way to invoke a function without providing *some* argument, the minimum valence is 1:

```
  val {5}
1
  val {x}
1
```

Under this model, `{5}` is not a nullary function—it's a unary function that ignores its argument. It maps any input to 5. This aligns with how constant functions work in mathematics, where f(x) = 5 is still a function of one variable.

<a id="valence-dictionaries"></a>
### Dictionaries

Dictionaries are similar to functions. They can be thought of as a function with an explicit domain/range map, instead of a computed one. So, the same rule applies to dictionaries.

```
  d:.+(`a`b`c;1 2 3)
  d[]
1 2 3
  d[nul]
1 2 3
  d . ()
1 2 3
  d[*()]
1 2 3
```

All four are equivalent: `d[]`, `d[nul]`, `d . ()`, and `d[*()]` all pass `nul`, which returns all values from the dictionary.

<a id="valence-summary"></a>
### Summary

gk's position:

- every empty vector has a prototype, `*v`
- `f . v` where `v` is empty passes the prototype: `f . v ≡ f[*v]`
- `f[]` is equivalent to `f . ()`, so `f[]` passes `nul`
- therefore every function has valence ≥ 1
- `{5}` is a function that maps any input to 5

Note: `f[]` is not the same as `f[()]`. The latter passes the empty list itself as an argument.

Other k implementations may take different approaches, and it may be possible to construct other consistent models.

---

## Builtins

### Math

| Builtin | Description | Example |
|---------|-------------|---------|
| `abs x` | absolute value | `abs -5` → `5` |
| `sqrt x` | square root | `sqrt 4` → `2.0` |
| `sqr x` | square | `sqr 3` → `9` |
| `exp x` | e^x | `exp 1` → `2.718...` |
| `log x` | natural log | `log 2.718` → `1.0` |
| `floor x` | floor | `floor 3.7` → `3` |
| `ceil x` | ceiling | `ceil 3.2` → `4` |
| `sin x` | sine | `sin 0` → `0.0` |
| `cos x` | cosine | `cos 0` → `1.0` |
| `tan x` | tangent | `tan 0` → `0.0` |
| `asin x` | arc sine | |
| `acos x` | arc cosine | |
| `atan x` | arc tangent | |
| `atan2[a;x]` | atan2 | |
| `sinh x` `cosh x` `tanh x` | hyperbolic sine/cosine/tangent | `sinh 0` → `0.0` |
| `erf x` `erfc x` | error function / complement | `erf 0` → `0.0` |
| `gamma x` `lgamma x` | gamma / log-gamma | `gamma 5` → `24.0` |
| `rint x` | round to nearest even whole | `rint 2.5` → `2.0` |
| `trunc x` | truncate toward zero | `trunc 2.7` → `2.0` |
| `a round x` | round `x` to the nearest multiple of `a` | `2 round 3.14159` → `4.0` |
| `a dot x` | dot product | `1 2 3 dot 4 5 6` → `32` |
| `mag x` | vector magnitude (Euclidean norm) | `mag 3 4.0` → `5.0` |
| `a choose x` | binomial coefficient | `5 choose 2` → `10.0` |

### Bitwise and Integer

| Builtin | Description | Example |
|---------|-------------|---------|
| `a div x` | integer (floor) division | `7 div 2` → `3` |
| `a and x` | bitwise and | `12 and 10` → `8` |
| `a or x` | bitwise or | `12 or 3` → `15` |
| `a xor x` | bitwise xor | `12 xor 10` → `6` |
| `not x` | bitwise complement | `not 0` → `-1` |
| `a shift x` | shift `x` left by `a` bits (negative `a` shifts right) | `1 shift 8` → `256` |
| `a rot x` | bitwise rotate | |

### Number Theory

| Builtin | Description | Example |
|---------|-------------|---------|
| `prime x` | is `x` prime? | `prime 7` → `1` |
| `factor x` | prime factorization | `factor 12` → `2 2 3` |
| `gcd[a;x]` | greatest common divisor | `gcd[12;18]` → `6` |
| `lcm[a;x]` | least common multiple | `lcm[4;6]` → `12` |
| `modinv[a;m]` | modular inverse of `a` mod `m` | `modinv[3;7]` → `5` |

### String

| Builtin | Description | Example |
|---------|-------------|---------|
| `ic x` | char to int | `ic"A"` → `65` |
| `ci x` | int to char | `ci 65` → `"A"` |
| `ss[a;x]` | string/symbol search — start indices (`x` may use `?[^-]`) | `ss["abcabc";"bc"]` → `1 4` |
| `sm[a;x]` | string match (glob `*?[^-]`) | |
| `ssr[a;x;y]` | search `a` for `x`, replace with `y` (`y` may be a function) | |

### List

| Builtin | Description | Example |
|---------|-------------|---------|
| `a at x` | index `a` at `x`, yielding null for out-of-range instead of `index error` | `1 2 3 at 0 5` → `1 0N` |
| `a in x` | `1` if `a` is an item of `x`, else `0` | `2 in 1 2 3` → `1` |
| `a lin x` | element-wise `in` — for each item of `a` | `2 4 lin 1 2 3` → `1 0` |
| `a dv x` | drop items of `a` equal to value `x` | `1 2 3 2 4 dv 2` → `1 3 4` |
| `a di x` | drop items of `a` at index/indices `x` | `1 2 3 4 5 di 1 3` → `1 3 5` |
| `a sv x` | scalar from vector — decode digits `x` in base `a` | `10 sv 1 2 3 4` → `1234` |
| `a vs x` | vector from scalar — encode `x` in base `a` | `10 vs 1234` → `1 2 3 4` |
| `kv x` | list from vector (non-promoted homogeneous list) | `kv 1 2 3` → `(1;2;3)` |
| `vk x` | vector from list | `vk kv 1 2 3` → `1 2 3` |

### Conversion and Serialization

| Builtin | Description | Example |
|---------|-------------|---------|
| `bd x` | bytes from data — serialize to a byte vector | |
| `db x` | data from bytes — deserialize | `db bd 1 2 3` → `1 2 3` |
| `hb x` | hex string from bytes | |
| `bh x` | bytes from hex string | `db bh hb bd 1 2 3` → `1 2 3` |
| `zb x` | compress bytes | |
| `bz x` | decompress bytes | `db bz zb bd 1 2 3` → `1 2 3` |
| `val x` | valence (argument count) of a function | `val{x+y}` → `2` |
| `md5 x` | MD5 digest (hex string) | `md5"abc"` → `"900150983cd24fb0d6963f7d28e17f72"` |
| `sha1 x` | SHA-1 digest | |
| `sha2 x` | SHA-256 digest | |
| `(iv;key) encrypt x` | AES-256 encrypt (`iv:16 draw 256`, `key:32 draw 256`) | |
| `(iv;key) decrypt x` | AES-256 decrypt | |

`md5` / `sha1` / `sha2` / `encrypt` / `decrypt` accept `x` of type `-3` (string), `4`/`-4` (symbol), or `0` (list).

### Time

Time builtins use an epoch of **2035-01-01** (Julian day `0` is a Monday).

| Builtin | Description | Example |
|---------|-------------|---------|
| `gtime x` | GMT date-time from timestamp → `YYYYMMDD HHMMSS` | `gtime 0` → `20350101 0` |
| `ltime x` | local date-time from timestamp (timezone-dependent) | |
| `lt x` | local timestamp (days since 2035, float) | |
| `jd x` | Julian day number from `YYYYMMDD` | `jd 20231225` → `-4025` |
| `dj x` | `YYYYMMDD` from Julian day number | `dj 0` → `20350101` |
| `sleep x` | sleep `x` milliseconds | |

### Random

| Builtin | Description |
|---------|-------------|
| `a draw x` | `a` random picks from `!x`; `a draw -x` deals without repeats; `a draw 0` draws floats in `(0,1)` |

### I/O

| Builtin | Description |
|---------|-------------|
| `0: x` | read lines from file |
| `a 0: x` | write lines to file |
| `1: x` | read k data from file |
| `a 1: x` | write k data to file |
| `2: x` | read bytes from file |
| `a 2: x` | write bytes to file |

#### Fixed-Width Records

Dyadic `0:` and `1:` with a `(types;widths)` left argument load **fixed-width
records** — `0:` from a text file, `1:` from a binary file. The left argument is a
two-item list `(s;w)`: a character vector `s` naming the type of each field and an
integer vector `w` of the same count giving each field's width. Each line (text)
or packed record (binary) is partitioned into fields; the result is a list of
columns, one per non-skipped field, each of length equal to the number of
records.

A third form takes `(f;b;n)` on the right instead of a bare file `f`, reading only
`n` bytes starting at byte offset `b` — for loading slices of a large file.

**Fixed-width text — `(s;w)0:f`**

| `s` | field | column type |
|-----|-------|-------------|
| `I` | integer | `i32` |
| `J` | integer | `i64` |
| `E` | float | `f32` |
| `F` | float | `f64` |
| `C` | character | char vector |
| `S` | symbol | symbol (whitespace-trimmed) |
| (blank) | skip `w` characters | — |

Each `w` is a width in characters; `+/w` must equal the length of every line (a
`length error` otherwise). Numeric fields are parsed leniently (`0N`/`0I` give the
null/infinity of that type). `J` and `E` are gk extensions to the classic `IFCS`
set.

```
/ a text file of three right-justified columns:
/    1     100      1.5
/    2     200      2.5
("IJE";4 9 9)0:"data.txt"
(1 2
 100 200j
 1.5 2.5e)
```

**Fixed-width binary — `(s;w)1:f`** *(a gk extension; the manual's `1:` is the
K-data load/save below)*

| `s` | on-disk | `w` | column type |
|-----|---------|-----|-------------|
| `c` | char | 1 | char vector |
| `b` | byte / bool | 1 | `i32` |
| `s` | short | 2 | `i32` |
| `i` | int | 4 | `i32` |
| `j` | i64 | 8 | `i64` |
| `e` | f32 | 4 | `f32` |
| `f` | float | 8 | `f64` |
| `C` | char | `w` | char vector of width `w` |
| `S` | char | `w` | symbol (whitespace-trimmed) |
| (blank) | — | `w` | skip `w` bytes |

The fixed-numeric letters (`c b s i j e f`) require the exact `w` shown; the file
is read as packed records of `+/w` bytes. The lowercase letters mirror the
uppercase text letters by width: `i`/`j` = 4-/8-byte integer, `e`/`f` = 4-/8-byte
float (cf. text `I`/`J`/`E`/`F`).

```
/ records of (i32; i64; f32; f64):
("ijef";4 8 4 8)1:"data.bin"
(1 2
 100 200j
 1.5 2.5e
 10.5 20.5)
```

A **single type letter** with no width list loads a whole file (or `(f;b;n)`
slice) as one vector of that type — `c` char, `i` int, `j` i64, `e` f32,
`f` float:

```
"i"1:"ints.bin"               / whole file as an int vector
10 20 30 40 50
"i"1:("ints.bin";4;12)        / 12 bytes from offset 4
20 30 40
"j"1:"longs.bin"
1 2 3000000000j
```

### System

| Builtin | Description |
|---------|-------------|
| `exit x` | exit with code |
| `getenv x` | get environment variable |
| `setenv[a;x]` | set environment variable |
| `del x` | delete file `x` |
| `a rename x` | rename file `a` to `x` |

### Matrix

| Builtin | Description |
|---------|-------------|
| `lu x` | LU decomposition `(+P;L;U)` |
| `ldu x` | LDU decomposition `(+P;L;D;U)` |
| `qr x` | QR decomposition `(Q;R)` |
| `svd x` | singular value decomposition `(U;S;+V)` |
| `inv x` | matrix inverse |
| `det x` | determinant |
| `rref x` | reduced row echelon form |
| `mul[a;x]` | matrix multiply |
| `lsq[a;x]` | least squares solve |

---

## System Commands

| Command | Description |
|---------|-------------|
| `\l file` | load file |
| `\t expr` | time expression |
| `\p n` | set print precision |
| `\v` | list variables |
| `\\` | exit |

In the **interactive** top-level REPL, **`\`** alone (then Enter) prints a **help index** of further `\`-topics, for example `\0` (data), `\+` (verbs), `\'` (adverbs), `\_` (reserved), `\.` (assign / control / debug), `\:` (I/O), `\-` (client/server), `` \` `` (OS), `\?` (commands). At nested prompts (e.g. debug under `\e 1`), a lone `\` is **abort**, not this menu.

---

## IPC

gk has built-in IPC over TCP. A gk process can open a connection to a peer,
send messages synchronously or asynchronously, and act as a server that
receives messages and dispatches them to user-defined handlers.

```
  sync  (h 4:msg)                 async  (h 3:msg)
    client       server             client       server
      | --SYNC--> |                   | --ASYNC-> |
      |           | .m.g msg          |           | .m.s msg
      | <--RSP--- |                   |           | (no reply)
      v           v                   v           v
    result      continue            null        continue
```

### Quick Start

server:
```
  f:{x+y+z}
  \m i 5555            / listen on port 5555, inline
```

client (in another gk):
```
  h:3:("localhost";5555)
  h 4:"2+2"            / sync: -> 4
  h 4:(`g;1 2 3)       / sync: apply server-side g to 1 2 3 -> 6
  h 3:"a::42"          / async: no reply
  3:h                  / close
```

### Client

A connection handle is an `int` returned by `3:(host;port)`. Messages flow in both
directions on one open handle. Handles are persistent until you close
them.

```
h:3:(host;port)    open: host is sym or string
h 3:msg            async: send msg, do not wait for a reply
h 4:msg            sync:  send msg, wait for response
3:h                close handle h
(host;port) 3:msg  async with implicit open (handle is cached, not closed)
(host;port) 4:msg  sync  with implicit open (handle is cached, not closed)
```

`msg` is any K value. It is serialized to gk's binary form and reassembled
on the server.

#### Host forms

All of these open a connection to port 5555 on the local machine:

```
  h:3:("localhost";5555)
  h:3:(`localhost;5555)
  h:3:(`;5555)
  h:3:("127.0.0.1";5555)
```

#### Sync result

`h 4:msg` returns the handler's result value, or raises whatever error
the handler raised. The error surfaces on the client side with its
original message. Errors traverse IPC just like any other k data type.

```
  h 4:"1+`foo"
type error
  h 4:"'\"custom\""
custom
```

To trap remote errors locally:

```
  @[{h 4:x};"1+`foo";:]
(1;"type")
```

#### Async result

`h 3:msg` returns `null` immediately; the call does not wait for the
server handler to run. Server-side errors go through the usual `\e`
path: under `\e 0` they are silently dropped, under `\e 1` the server
drops into a debug sub-REPL (there is no caller to surface them to
either way). Use async for fire-and-forget updates, state mutation,
logging, etc.

#### Implicit open

You can skip the explicit `h:3:(host;port)` step by passing the `(host;port)`
pair directly on the left of `3:` / `4:`:

```
  (`;5555) 4:"2+2"             / -> 4
  (`;5555) 3:(`log;`hello)     / async, no reply
```

The first call opens a connection; subsequent calls to the same
`(host;port)` reuse the same underlying handle. The handle is **not**
closed on return — it stays in gk's conn table until process exit or
until you close it explicitly:

```
  h:3:(`;5555)    / grabs the same handle (dedup)
  3:h             / closes it
```

The return value matches the explicit-handle form: `null` for `3:`,
the remote response (or raised error) for `4:`. Errors from the open
step (e.g. connection refused) are raised just like send errors and can
be trapped with `.[f;x;:]`.

### Server

One gk process can run up to two listeners at the same time: an
inline listener and a forking listener, on different ports.

```
\m i PORT    start inline listener   (PORT 0 stops it)
\m f PORT    start forking listener  (POSIX only; PORT 0 stops it)
\m i         show inline  port (0 if not bound)
\m f         show forking port (0 if not bound)
\m           show both, tagged 'i' / 'f'
```

Equivalent CLI flags at startup:

```
gk -i 5555 -f 5556 [script]
```

#### Inline

All accepted connections are serviced by the single gk process.
Handlers run in the REPL's event loop.

#### Forking

Each newly accepted connection is handed off to a child via `fork()`.
The child serves only that one connection, then exits. The parent
resumes accepting. This gives each peer a fresh, isolated copy of the
gk namespace — useful when handlers mutate state that you don't want
bleeding between clients.

forking mode is **POSIX only**. On Windows, `\m f PORT` returns
`fork: not supported on windows`.

#### Rebind semantics

Binding the same slot to the same port is an error:

```
  \m i 5555
  \m i 5555
bind: Address already in use
```

Rebinding to a different port succeeds atomically. If the new bind
fails, the existing listener is left intact.

The two slots are independent; binding forking while inline is already
listening (or vice versa) works, provided the ports differ.

#### Stopping a listener

```
  \m i 0      / stop inline slot
  \m f 0      / stop forking slot
```

Listeners persist across sub-REPLs (`\\` into a nested REPL does not
close them). Stop them explicitly with `\m X 0`, or let process exit do
it. Client handles opened with `3:(host;port)` are similarly persistent;
close with `3:h`.

### Handlers

Three overridable globals control dispatch:

| global | default    | fires on                  | result        |
|--------|------------|---------------------------|---------------|
| `.m.s` | `{. x}`    | `h 3:msg` (async)         | discarded     |
| `.m.g` | `{. x}`    | `h 4:msg` (sync)          | sent back     |
| `.m.c` | `""`       | peer disconnect           | discarded     |

The default handler for `.m.s` and `.m.g` is `{. x}`:

- `. "!10"` — if `x` is a char vector, treat it as K source and evaluate.
- `. (`g;1 2 3)` — if `x` is a `(fn;args)` pair, apply the function to the args.

So out of the box, a freshly started gk server handles both raw code
strings and typed function calls:

```
  h 4:"!10"           / -> 0 1 2 3 4 5 6 7 8 9
  h 4:(`g;1 2 3)      / -> g[1;2;3]
```

#### `.m.c` is a string, not a lambda

`.m.c` is executed when a connection is closed.
Anything that isn't a non-empty char vector (including lambdas,
symbols, ints, lists, and the default `""`) is silently ignored.

```
  .m.c:"closed+:1"    / bumps a global each time a peer goes away
```

#### Overriding

Assign a new lambda to wrap or replace the default:

```
  .m.g:{hist,:,x; . x}    / log and execute every sync msg
  .m.s:{`0:x}             / async messages just get printed
```

Handler errors on sync calls surface on the client as raised errors.
Handler errors on async calls are dispatched per the server's `\e`
setting: `\e 0` silently drops them, `\e 1` drops the server into a
debug sub-REPL so you can inspect state (IPC dispatch pauses until
you exit the sub-REPL with `\`).

#### `.z.w`

Inside `.m.s`, `.m.g`, and `.m.c`, the global `.z.w` holds the handle of
the peer whose message is being dispatched. Outside handler context
it reads as `0`.

```
  .m.s:{`0:"msg from handle ",($.z.w),"\n"}
  .m.c:"`0:\"peer \",($.z.w),\" disconnected\\n\""
```

### Draining a Batch of Asyncs

Messages on a single conn are dispatched strictly in order, so a sync
on the same conn after a burst of asyncs acts as a barrier — its
response can't come back until every prior async has finished
running:

```
  {(`;5555) 3:x}'msgs    / fire a batch, non-blocking
  (`;5555) 4:"0"         / returns once the whole batch has been handled
```

This is usually what you want instead of `sleep`.

---

## Timer

gk has a built-in recurring timer. You set an interval and a function;
the function fires at the top-level REPL prompt every `interval`
seconds until you turn it off.

<a id="timer-quick-start"></a>
### Quick Start

```
  timer(1;{`0:"tick\n"})    / fire every 1s
  ...
  timer(0;{x})              / interval 0 disables
```

<a id="timer-surface"></a>
### Surface

The timer is a single monadic builtin, `timer`. It has two forms:

```
timer (t;f)        SET. t is seconds (int or float, 0 disables);
                   f is a 1-valence fn called as f[nul] each tick,
                   OR a sym `g referring to a global g (resolved at
                   every tick). Returns nul.
timer x            QUERY (anything that isn't a 2-element value:
                   timer[], timer nul, timer`, timer 1, ...).
                   Returns the current (t;f). Before any set,
                   returns (0.0;{}).
```

Examples:

```
  timer[]
(0.0;{})
  timer(0.5;{!10})
  timer[]
(0.5;{!10})
  timer 0                   / 0 by itself is a query, not a set
(0.5;{!10})
  timer(0;{})               / disable
  timer[]
(0.0;{})
```

<a id="timer-set-form-validation"></a>
### Set-form Validation

A 2-element argument is *always* interpreted as a set attempt, even
when k normalizes it to a typed vector (e.g. `(1;5)` is the int
vector `1 5`, not a mixed list). Bad shapes raise an error rather
than silently falling back to the query path:

```
  timer("a";5)              / interval not numeric
type error
  timer(-1;{x})             / interval negative
domain error
  timer(1;5)                / fn not callable
type error
  timer(1;`undefined)       / sym doesn't resolve
value error
  timer(1;{x+y})            / valence != 1
valence error
  timer 1 2                 / same: 2-element int vector
type error
```

Valence is checked strictly: the fn must have valence exactly 1.
`{x+y}` would just project on `nul` rather than fire, which is never
what you want.

<a id="timer-pass-by-reference"></a>
### Pass by Reference (sym)

The fn can be a sym referring to a global function, mirroring
``` `f . ` ``` (apply by reference). The sym is resolved at *every*
tick, so redefinitions are picked up live:

```
  f:{!3}
  timer(0.5;`f)             / fires {!3}
0 1 2
0 1 2
  f:{!7}                    / next tick fires the new f
0 1 2 3 4 5 6
0 1 2 3 4 5 6
  timer[]
(0.5;`f)                    / query returns the sym, not the fn
```

If the sym stops resolving (deleted, rebound to a non-callable, or
rebound to a non-1-valence fn), the corresponding error prints at
the next tick and the timer stays armed -- restoring the global
resumes ticking on the following interval.

<a id="timer-tick-output"></a>
### Tick Output

Tick handler results follow the REPL's echo rules: a non-nul result
is printed via `kprint`, suppressed under `-q`. Return `nul`
explicitly when you want a silent tick:

```
  timer(1;{!5})
0 1 2 3 4
0 1 2 3 4
...

  timer(1;{!5;nul})         / compute, then return nul
  ...                       / no output
```

<a id="timer-semantics"></a>
### Semantics

- **top-level only.** Ticks fire only at the top-level REPL
  prompt. Sub-REPLs (entered via `\` after an error under `\e 1`,
  or any other recursive REPL), script `load()`, and the wait
  inside an IPC sync call (`h 4:msg`) all suppress firing.
- **re-entrancy guard.** A tick is skipped if the previous handler
  hasn't returned yet. Long-running handlers won't stack.
- **missed ticks are dropped.** The next fire is scheduled
  `interval` seconds from when the previous handler *returned*,
  not from when it was supposed to fire. There is no catch-up.
- **cleared on fork.** An IPC fork child inherits no timer
  schedule. The parent's timer continues unaffected.
- **persists across script load.** A script can install a timer
  and return; the timer keeps firing once the REPL is reached.
  Subsequent `load()` calls do not clear it.
- **errors honor `\e`.** Under `\e 0` the error is printed and the
  timer keeps running. Under `\e 1` the handler error drops you
  into a debug sub-REPL, with the timer suspended until you exit
  it with `\`.

<a id="timer-why-builtin"></a>
### Why a Builtin and Not `\t`?

k3 uses `\t expr` for both timing an expression and setting a
recurring timer (the form is overloaded by argument type). gk
splits these: `\t expr` is the one-shot timer, and `timer (t;f)` is
the recurring one. A builtin keeps the recurring form
introspectable (`timer[]` returns the current state) and lets it
participate in normal k expressions, including passing the
`(t;f)` pair around as data.

---

## Watches

A *watch* binds a global variable to a gk expression that is evaluated
for its side effects every time that variable is assigned. It is gk's
analog of e333j's per-variable `t` attribute:

```
  e333j:   a..t:"b+:1"        gk:   watch(`a;"b+:1")
```

<a id="watch-quick-start"></a>
### Quick Start

```
  a:b:0
  watch(`a;"b+:1")          / run "b+:1" after each assignment to a
  a:1
  b
1
  a:1
  b
2
```

<a id="watch-surface"></a>
### Surface

Watches are a single monadic builtin, `watch`. It has three forms:

```
watch (`name;"expr")   ARM. Run "expr" after each assignment to the
                       global `name` in the current namespace. Arming
                       an already-watched name replaces its expr.
                       Returns nul.
watch (`name;"")       DISARM. An empty string (or nul) as the expr
                       removes the watch. Returns nul.
watch x                QUERY (anything that isn't a 2-element value
                       whose first element is a sym: watch[], watch`,
                       watch nul, ...). Returns a list of
                       (`fqname;"expr") pairs, one per armed watch,
                       where fqname is the fully-qualified name. Empty
                       list if none armed.
```

Examples:

```
  watch(`a;"b+:1")
  watch(`c;"b+:10")
  watch`
((`.k.a;"b+:1")
 (`.k.c;"b+:10"))
  watch(`a;"")              / disarm a
  watch`
,(`.k.c;"b+:10")
```

<a id="watch-which-assignments"></a>
### Which Assignments Fire

Any assignment that changes the watched variable fires its expr, after
the new value is stored (so the expr observes the post-assignment
state):

```
  a:1                       / plain assign
  a+:1                      / amend (and a*:2, a,:x, ...)
  a::1                      / global-assign from inside a function
  .k.a:1                    / fully-qualified, even from another namespace
  a.x:1                     / a member assign still fires a's watch
  @[`a;0;:;9]               / amend by reference (.[`a;...] / @[`a;...]),
  @[`.k.a;0;:;9]            /   qualified or not
```

A watch is keyed by **(namespace, name)**. Different k-tree namespaces
(`.k`, `.foo`, ...) can hold same-named variables with independent
watches; a watch fires only for its own namespace's variable:

```
  b:0
  watch(`a;"b+:1")          / watches .k.a
  \d .foo
  y:0
  watch(`a;"y+:100")        / watches .foo.a, independent of .k.a
  \d .k
  a:1                       / fires .k's watch only
  b
1
  .foo.y
0
  .foo.a:1                  / fires .foo's watch only
  .foo.y
100
```

The expr runs in the watched variable's namespace, so a watch armed in
`.foo` resolves the names in its expr against `.foo` regardless of where
the triggering assignment was made.

<a id="watch-reentrancy"></a>
### Re-entrancy

A watch will not re-fire while its own expr is still running, so a watch
whose expr re-assigns its own variable runs once, not forever:

```
  a:0
  watch(`a;"a+:1")
  a:5
  a
6                           / a:5 fired the expr once -> 6, no loop
```

Distinct watches may chain -- if `a`'s expr assigns `b`, then `b`'s
watch fires in turn. A cycle terminates the moment it would re-enter an
already-firing watch.

<a id="watch-errors"></a>
### Errors

An error in a watch expr is reported like any other top-level error and
honors `\e`: under `\e 0` it is returned (and so silently dropped by the
firing assignment); under `\e 1` it prints and drops into a debug
sub-REPL. The triggering assignment itself always completes -- the new
value is stored before the expr runs.

```
  watch(`a;"b+:`")
  a:2
type error
b+:`
 ^
>
```

<a id="watch-semantics"></a>
### Semantics

- **globals only.** Watches apply to global variables. A plain local
  assignment inside a function does not fire a watch (a `::`
  global-assign does).
- **string expr, not a fn.** The expr is gk source evaluated each time
  via the normal parse+eval path, not a stored lambda. This mirrors
  e333j's `t` attribute and lets the expr reference whatever is in
  scope at fire time.
- **introspectable.** `watch[]` returns the armed set as ordinary data
  you can index and inspect.
- **limitation.** A fully-qualified assign whose path descends through a
  *dictionary variable* in another namespace (e.g. `.foo.a.b:1` where
  `.foo.a` is a dict, not a sub-namespace) does not fire `a`'s watch;
  the in-namespace form (`a.b:1`, run from `.foo`) does.

---

## Errors

| Error | Cause |
|-------|-------|
| `rank` | wrong number of dimensions |
| `length` | mismatched lengths |
| `type` | wrong type |
| `value` | undefined variable, or string not evaluable |
| `range` | value out of the representable range (e.g. integer overflow `0N div -1`) |
| `domain` | argument outside the verb's valid domain (e.g. `7 div 0`, `` `zzz$5 ``) |
| `valence` | wrong number of arguments to a function |
| `index` | index out of bounds |
| `int` | non-integer where an integer is required |
| `parse` | syntax error |
| `stack` | recursion too deep |
| `reserved` | use of a reserved name (single-letter top-level name, `.k`/`.m` namespace) |
| `wsfull` | workspace full — allocation too large or out of memory |

### Error Trapping
 The result is **`(0;result)`** on success or **`(1;error)`** if `f x` errors.

**`@[f;x;:]`** — trap on **`f @ x`**.

**`.[f;x;:]`** — trap on **`f . x`** (one value: **`,0`**, two: **`(1;2)`**, etc.).

```
  @[{1+`a};0;:]
(1;"type")
  .[{1+`a};,0;:]
(1;"type")
  .[{x+y};(1;`a);:]
(1;"type")
  @[{x+1};5;:]
0 6
```

---

## Verb Reference

Common usage and examples. Expand "spec" for detailed type semantics.

<h3 id="dyadic-conformability">Dyadic Conformability</h3>

Dyadic verbs follow these rules:  
(using `+` as an example)

- atom + atom  
  Atoms always conform.
  ```
    1+2
  3
  ```
- atom + vector  
  Result is atom + each element of vector
  ```
    1 + 2 3 4
  3 4 5
  ```
- vector + atom  
  Result is each element of vector + atom
  ```
    2 3 4 + 1
  3 4 5
  ```
- vector + vector  
  Result is element-wise. Vectors must be the same length.
  ```
    1 2 3 + 4 5 6
  5 7 9
  ```
- atom + list  
  Result is atom + each element of list, recursively.
  ```
    1 + (2;3 4;(5;6 7))
  (3
   4 5
   (6
    7 8))
  ```
- list + atom  
  Result is each element of list, recursively, + atom.
  ```
    (2;3 4;(5;6 7)) + 1
  (3
   4 5
   (6
    7 8))
  ```
- vector + list  
  Result is each element of vector + each element of list.
  ```
    1 2 + (3 4;5 6)
  (4 5
   7 8)
  ```
- list + vector  
  Result is each element of list + vector.
  ```
    (3 4;5 6) + 1 2
  (4 5
   7 8)
  ```
- list + list  
  Result is each element of left list + each element of right list.
  ```
    (1 2;3 4) + (5 6;(7 8;9 10))
  (6 8
   (10 11
    13 14))
  ```

<h3 id="numeric-type-rules">Numeric Type Rules</h3>

Arithmetic verbs (`+` `-` `*` `%`):

- **Types:** numeric only — `i32`, `i64`, `f32`, `f64`
- **Promotion:** a dyad's result takes the wider of the two operand types. The
  one non-obvious case is `i64` with `f32` → `f64` (neither type contains the
  other, so the result widens to `f64`):

  | a \ x | `i32` | `i64` | `f32` | `f64` |
  |-------|-------|-------|-------|-------|
  | `i32` | `i32` | `i64` | `f32` | `f64` |
  | `i64` | `i64` | `i64` | `f64` | `f64` |
  | `f32` | `f32` | `f64` | `f32` | `f64` |
  | `f64` | `f64` | `f64` | `f64` | `f64` |

- **Errors:** `type error` if either operand is non-numeric

### Dyadic

<a id="d-plus"></a>
#### `a+x` (plus)

Arithmetic sum of `a` and `x`.

```
  2+3
5
  2+3.0
5.0
  1+1 2 3
2 3 4
  1 2 3+4 5 6
5 7 9
  1.5+1 2 3
2.5 3.5 4.5
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)  
[Numeric Type Rules](#numeric-type-rules)

**Errors:** `type` when an operand is non-numeric; `length` when the vectors differ in length.

</details>

<a id="d-minus"></a>
#### `a-x` (minus)

`a` minus `x`.

```
  5-3
2
  5-3.0
2.0
  10-1 2 3
9 8 7
  5 4 3-1 2 3
4 2 0
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)  
[Numeric Type Rules](#numeric-type-rules)

**Errors:** `type` when an operand is non-numeric; `length` when the vectors differ in length.

</details>

<a id="d-times"></a>
#### `a*x` (times)

Arithmetic product of `a` and `x`.

```
  3*4
12
  2*1 2 3
2 4 6
  2*-1 2 -3
-2 4 -6
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)  
[Numeric Type Rules](#numeric-type-rules)

**Errors:** `type` when an operand is non-numeric; `length` when the vectors differ in length.

</details>

<a id="d-divide"></a>
#### `a%x` (divide)

Arithmetic quotient of `a` and `x`.

```
  10%4
2.5
  12%1 2 3 4
12 6 4 3.0
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)  
[Numeric Type Rules](#numeric-type-rules)

**Errors:** `type` when an operand is non-numeric; `length` when the vectors differ in length. Division by zero yields infinity, not an error.

</details>

<a id="d-min-and"></a>
#### `a&x` (min / and)

Element-wise minimum for ints and floats (same shape rules as `+`). Also defined for char and sym (lexicographic min). On boolean-ish `0`/`1` ints, this is logical **and**.

```
  3&5
3
  0&1
0
  2&3.0
2.0
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)

**Types:** int, float, char, sym (pairwise); mixed int/float promotes to float where applicable.

**Errors:** `type` when an operand is non-numeric; `length` when the vectors differ in length.

</details>

<a id="d-max-or"></a>
#### `a|x` (max / or)

Element-wise maximum for ints and floats; max for char and sym. On boolean-ish `0`/`1` ints, logical **or**.

```
  3|5
5
  0|1
1
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)

**Types:** int, float, char, sym (pairwise); mixed int/float promotes to float where applicable.

**Errors:** `type` when an operand is non-numeric; `length` when the vectors differ in length.

</details>

<a id="d-comparisons"></a>
#### `a<x`, `a>x`, `a=x` (comparisons)

Less, greater, equal — return int `0` or `1`, element-wise with [Dyadic Conformability](#dyadic-conformability).

```
  3<5
1
  5>3
1
  3=3
1
  1 2 3<2 2 2
1 0 0
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)

**Types:** int, float, char, sym where the comparison is defined; mixed-type rules follow the implementation.

**Errors:** `type` for incomparable operands; `length` when the vectors differ in length.

</details>

<a id="d-match"></a>
#### `a~x` (match)

Structural **match** (`1` if `a` and `x` have the same shape and data, else `0`).

```
  1 2~1 2
1
  1 2~1 2.0
0
```

<details>
<summary>spec</summary>

Recurses into nested lists.

**Errors:** none — always returns `0` or `1`.

</details>

<a id="d-apply"></a>
#### `a.x` (apply)

Apply callable `a` to argument list `x` (typically `x` is a list of values).

```
  {x+y} . 1 2
3
```

<details>
<summary>spec</summary>

`a` must be callable with valence matching `n(x)` when `x` is a list of arguments.

**Errors:** `valence` when the argument count doesn't match `a`; otherwise whatever error `a` itself raises.

</details>

<a id="d-mod-dict"></a>
#### `a!x` (mod / dict)

Remainder / modulo for numeric `a` and `x` (C-style remainder for negatives). Dictionary-related forms of `!` are covered under [Dictionaries](#dictionaries).

```
  7!3
1
  7!3.0
1.0
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)

**Errors:** `type` / `int` when an operand is not an integer.

</details>

<a id="d-at-index"></a>
#### `a@x` (at / index)

Select from `a` at indices `x`. `x` is usually int indices (atom or vector).

```
  1 2 3@1
2
  1 2 3@0 2
1 3
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)

**Errors:** `index` when out of range; `type` for a non-integer index.

</details>

<a id="d-find"></a>
#### `a?x` (find)

Membership / lookup: for a vector `a`, result is the index of each item of `x` in `a` (first occurrence), or length of `a` if not found (see implementation for full rules).

```
  1 2 3?2
1
  1 2 3?5
3
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)

**Errors:** none — returns `#a` (the count) when `x` is not found.

</details>

<a id="d-take-reshape"></a>
#### `a#x` (take / reshape)

Take first `a` items of `x`, or reshape `x` when `a` is an integer list (see examples in primitive table and implementation).

```
  3#1 2 3 4
1 2 3
  -2#1 2 3 4
3 4
  2 2#1 2 3 4
(1 2
 3 4)
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)

**Errors:** `type` when the left argument `a` is not an integer.

</details>

<a id="d-drop-cut"></a>
#### `a_x` (drop / cut)

With an int atom `a`, drop the first `a` items of `x`. With an int vector of cut points, splits `x` (see examples).

```
  2_1 2 3 4
3 4
  2 4_!10
(2 3
 4 5 6 7 8 9)
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)

**Errors:** `type` when the left argument `a` is non-numeric.

</details>

<a id="d-power"></a>
#### `a^x` (power)

`a` raised to the power `x` (float result where needed).

```
  2^10
1024.0
  1 2^3 4
1 16.0
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)  
[Numeric Type Rules](#numeric-type-rules)

**Errors:** `domain` when a base is negative and its exponent is not a whole number (gk has no complex numbers); `type` when an operand is non-numeric; `length` when the vectors differ in length.

</details>

<a id="d-join"></a>
#### `a,x` (join)

Concatenate `a` and `x` (join lists/vectors).

```
  1 2,3 4
1 2 3 4
  (1;2),3
1 2 3
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)

**Errors:** none — operands of unlike type are joined into a list.

</details>

<a id="d-cast-pad"></a>
#### `a$x` (cast / pad)

Dyadic `$` pads, truncates, or reshapes strings when `a` is int; other cast combinations exist (see implementation).

```
  3$"ab"
" ab"
  6$"ab"
"    ab"
```

<details>
<summary>spec</summary>

[Dyadic Conformability](#dyadic-conformability)

**Errors:** `domain` for an unrecognized cast specifier (e.g. `` `zzz$5 ``).

</details>

### Monadic

<a id="m-flip"></a>
#### `+x` (flip)

Transposes a matrix.

```
  +(1 2;3 4)
(1 3
 2 4)
  +("asdf";"0123")
("a0"
 "s1"
 "d2"
 "f3")
```

Atoms expand to match the length of other rows:

```
  +(1 2;3;4 5)
(1 3 4
 2 3 5)
```

Noop for atoms and vectors.

```
  +1 2 3
1 2 3
  +5
5
```

<details>
<summary>spec</summary>

**Types:** list of equal-length lists → transposed list. Atoms pass through unchanged.

**Errors:** `length` when the rows have unequal length.

</details>

<a id="m-negate"></a>
#### `-x` (negate)

Arithmetic negation.

```
  -5
-5
  --5
5
```

Note: `-` before a number is a negative literal, not negate. Use a space:

```
  -1 2 3
-1 2 3
  - -1 2 3
1 -2 -3
```

Works on ints and floats; recurses into lists.

<details>
<summary>spec</summary>

[Numeric Type Rules](#numeric-type-rules)

**Errors:** `type` when `x` is non-numeric.

</details>

<a id="m-first"></a>
#### `*x` (first)

First item of `x`.

```
  *1 2 3
1
  *"asdf"
"a"
```

<details>
<summary>spec</summary>

**Types:** any.
- atom → `x` unchanged
- vector / list → first item
- empty vector → the type's fill element (`0`, `0.0`, `" "`)
- empty list `()` → `()`

**Errors:** none — empty aggregates return the type fill or `()`.

</details>

<a id="m-reciprocal"></a>
#### `%x` (reciprocal)

`1 % x` for numeric `x`.

```
  %4
0.25
  %1 2 4
1 0.5 0.25
```

<details>
<summary>spec</summary>

[Numeric Type Rules](#numeric-type-rules)

**Errors:** `type` when `x` is non-numeric.

</details>

<a id="m-where"></a>
#### `&x` (where)

Indices where boolean-ish `x` is non-zero (typically `1`).

```
  &1 0 1 0
0 2
```

<details>
<summary>spec</summary>

**Types:** non-negative int vector (or atom). Returns each index `i` repeated `x[i]` times, so a `0`/`1` mask yields the positions of the `1`s; `&n` on an atom gives `n` copies of `0`.

**Errors:** `type` when `x` is non-integer (char or float).

</details>

<a id="m-reverse"></a>
#### `|x` (reverse)

Reverse a vector (chars, ints, …).

```
  |1 2 3
3 2 1
  |"abc"
"cba"
```

<details>
<summary>spec</summary>

**Types:** vector or list → items in reverse order; atoms are returned unchanged. Reversal is top-level only — nested items are not themselves reversed.

**Errors:** none.

</details>

<a id="m-grade-up"></a>
#### `<x` (grade up)

Permutation indices that would sort `x` ascending.

```
  <3 1 2
1 2 0
```

<details>
<summary>spec</summary>

**Types:** numeric, char, or symbol vector (also a list of atoms). Returns an int vector of indices that would sort `x` ascending.

**Errors:** `rank` when `x` is an atom.

</details>

<a id="m-grade-down"></a>
#### `>x` (grade down)

Permutation indices that would sort `x` descending.

```
  >3 1 2
0 2 1
```

<details>
<summary>spec</summary>

**Types:** numeric, char, or symbol vector (also a list of atoms). Returns an int vector of indices that would sort `x` descending.

**Errors:** `rank` when `x` is an atom.

</details>

<a id="m-group"></a>
#### `=x` (group)

Group indices of equal items.

```
  =0 1 0 1
(0 2
 1 3)
```

<details>
<summary>spec</summary>

**Types:** numeric, char, or symbol vector. Returns a list of index-vectors, one per distinct value, in first-seen order.

**Errors:** `rank` when `x` is an atom.

</details>

<a id="m-not"></a>
#### `~x` (not)

Logical not on `0`/`1` ints (and related cases).

```
  ~0 1
1 0
```

<details>
<summary>spec</summary>

[Numeric Type Rules](#numeric-type-rules) for int boolean vectors.

**Errors:** `type` when `x` is non-numeric.

</details>

<a id="m-value"></a>
#### `.x` (value)

Evaluate a string of code, or apply when `x` has the shape `(f; args)` (see tutorial and IPC docs).

```
  ."1+2"
3
  .(+;1 2)
3
```

<details>
<summary>spec</summary>

**Errors:** `value` when a name is undefined; `parse` on malformed code.

</details>

<a id="m-enumerate"></a>
#### `!x` (enumerate / keys)

With positive int `x`, `0..x-1`. With a dict, key list.

```
  !5
0 1 2 3 4
  d:.();d.a:1;d.b:2;!d
`a `b
```

<details>
<summary>spec</summary>

**Types:**
- non-negative int `n` → `0 1 … n-1` (`!0` is the empty int vector)
- dictionary → its key list

Other types follow [Dictionaries](#dictionaries) and implementation rules.

**Errors:** `domain` when `x` is a negative count.

</details>

<a id="m-atom"></a>
#### `@x` (atom)

`1` if `x` is an atom, `0` if it is not (e.g. a vector).

```
  @1
1
  @1 2
0
```

<details>
<summary>spec</summary>

**Types:** any. Returns int `1` for atoms — including numbers, chars, symbols, and dictionaries — and `0` for vectors and lists (the empty list `()` is `0`).

**Errors:** none.

</details>

<a id="m-unique"></a>
#### `?x` (unique)

Unique items of `x` in first-seen order.

```
  ?1 2 1 3
1 2 3
```

<details>
<summary>spec</summary>

**Types:** vector or list. Returns the distinct items in first-seen order; works on numeric, char, and symbol vectors and on general lists.

**Errors:** none.

</details>

<a id="m-count"></a>
#### `#x` (count)

Number of items at the top level of `x`.

```
  #1 2 3
3
  #(1;2 3)
2
```

<details>
<summary>spec</summary>

**Types:** any. Number of top-level items: `1` for an atom, the length for a vector or list, the number of entries for a dictionary. Empty aggregates return `0`.

**Errors:** none.

</details>

<a id="m-floor"></a>
#### `_x` (floor)

Floor for floats; truncates toward \(-\infty\). Also defined on ints.

```
  _3.7
3
  _-3.2
-4
```

<details>
<summary>spec</summary>

[Numeric Type Rules](#numeric-type-rules)

**Errors:** `type` when `x` is non-numeric.

</details>

<a id="m-shape"></a>
#### `^x` (shape)

Shape vector for nested `x` (length of top level, then row lengths where applicable).

```
  ^(1 2;3 4)
2 2
  ^1 2 3
,3
```

<details>
<summary>spec</summary>

**Types:** any. Returns an int vector of the regular leading dimensions: empty (`!0`) for an atom, `,n` for a vector, and one entry per nested level while the sub-lists stay rectangular — ragged levels are not measured.

**Errors:** none.

</details>

<a id="m-enlist"></a>
#### `,x` (enlist)

Wrap atom `x` as a length-1 vector `,x`.

```
  ,1
,1
```

<details>
<summary>spec</summary>

**Types:** any. Wraps `x` in a length-1 aggregate: an atom becomes a one-element vector, a vector or list becomes a one-element list whose sole item is `x` (so `^,x` gains a leading `1`).

**Errors:** none.

</details>

<a id="m-string"></a>
#### `$x` (string from data)

Format `x` as a string (types vary; see implementation for full rules).

```
  $42
"42"
  $`ab
"ab"
```

<details>
<summary>spec</summary>

**Types:**
- atom (int / float / char / symbol) → its text as a char vector
- char vector → returned unchanged
- vector / list → list of formatted strings, item by item

See also dyadic [`a$x` cast / pad](#d-cast-pad) for width formatting.

**Errors:** none.

</details>

---

