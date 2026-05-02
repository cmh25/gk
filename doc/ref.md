# gk Reference Manual

gk is a dialect of the k programming language. This reference covers all primitives, builtins, and syntax.

New to gk? Try the [tutorial](tutorial.md) first. If you already know **k3**, see [gk vs k3](k3.md) for dialect differences.

**Notation:** **Primitive verbs** are written with **`x`** for the monadic argument and **`a`** and **`x`** for dyadic left and right (e.g. **`a+x`**). **User-defined functions** are usually named **`f`**, **`g`**, … with parameters **`x`**, **`y`**, **`z`** or explicit lists like **`[x;y]`**—not the same letters as the primitive slots.

## Types

| Type | Code | Example | Description |
|------|------|---------|-------------|
| int | 1 | `42` | 32-bit signed integer |
| float | 2 | `3.14` | 64-bit IEEE float |
| char | 3 | `"a"` | single character |
| symbol | 4 | `` `name `` | interned string |
| list | 0 | `(1;2.0;"a")` | heterogeneous list |
| int vector | -1 | `1 2 3` | homogeneous int array |
| float vector | -2 | `1 2 3.0` | homogeneous float array (prints like `1 2 3.0`, not `1.0 2.0 3.0`) |
| char vector | -3 | `"abc"` | string |
| symbol vector | -4 | `` `a`b`c `` | symbol list |
| dict | 5 | `.()` | dictionary |
| null | 6 | `nul` | null value |
| verb | 7 | `+`, `{x+y}` | primitive/lambda |

### Special Values

| Value | Type | Description |
|-------|------|-------------|
| `0N` | int | null integer |
| `0I` | int | positive infinity |
| `-0I` | int | negative infinity |
| `0n` | float | NaN |
| `0i` | float | positive infinity |
| `-0i` | float | negative infinity |
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
  kv(1;2;3)
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

### String

| Builtin | Description | Example |
|---------|-------------|---------|
| `ic x` | char to int | `ic"A"` → `65` |
| `ci x` | int to char | `ci 65` → `"A"` |
| `ss[a;x]` | string search | `ss["abcabc";"bc"]` → `1 4` |
| `sm[a;x]` | string match (regex) | |

### I/O

| Builtin | Description |
|---------|-------------|
| `0: x` | read lines from file |
| `a 0: x` | write lines to file |
| `1: x` | read k data from file |
| `a 1: x` | write k data to file |
| `2: x` | read bytes from file |
| `a 2: x` | write bytes to file |

### System

| Builtin | Description |
|---------|-------------|
| `exit x` | exit with code |
| `sleep x` | sleep x milliseconds |
| `getenv x` | get environment variable |
| `setenv[a;x]` | set environment variable |

### Matrix

| Builtin | Description |
|---------|-------------|
| `lu x` | LU decomposition |
| `qr x` | QR decomposition |
| `svd x` | SVD decomposition |
| `inv x` | matrix inverse |
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

## Errors

| Error | Cause |
|-------|-------|
| `type` | wrong type |
| `length` | mismatched lengths |
| `rank` | wrong number of dimensions |
| `index` | index out of bounds |
| `value` | undefined variable |
| `domain` | invalid domain |
| `parse` | syntax error |
| `stack` | recursion too deep |

### Error Trapping

**`@[f;x;h]`** — trap on **`f x`** (unary application). With **`h`** set to **`:`**, the result is **`(0;value)`** on success or **`(1;errstr)`** if `f x` errors.

**`.[f;args;h]`** — trap on applying **`f`** to the arguments in list **`args`** (one value: **`,0`**, two: **`(1;2)`**, etc.). Same **`:`** handler convention as **`@[f;x;:]`**.

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

- **Types:** int and float only
- **Promotion:** (dyadic) int + float -> float
- **Errors:** `type error` if operand is non-numeric

### Dyadic

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

</details>

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

</details>

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

</details>

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

</details>

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

</details>

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

</details>

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

</details>

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

</details>

#### `a.x` (apply)

Apply callable `a` to argument list `x` (typically `x` is a list of values).

```
  {x+y}.(1;2)
3
  (+).(1;2)
3
```

<details>
<summary>spec</summary>

`a` must be callable with valence matching `n(x)` when `x` is a list of arguments.

</details>

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

**Types:** primarily int and float combinations for modulo; see implementation for vector cases.

</details>

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

**Errors:** `index` when out of range.

</details>

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

</details>

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

</details>

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

</details>

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

</details>

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

</details>

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

</details>

### Monadic

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

**Errors:**
- `length error` — rows have different lengths

</details>

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

</details>

#### `*x` (first)

First item of `x` (for a vector, `*1 2 3` is `1`).

```
  *1 2 3
1
  *(1;2;3)
1
```

<details>
<summary>spec</summary>

Returns the first item of `x` for vectors and lists (see implementation for empty aggregates).

</details>

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

</details>

#### `&x` (where)

Indices where boolean-ish `x` is non-zero (typically `1`).

```
  &1 0 1 0
0 2
```

<details>
<summary>spec</summary>

**Types:** int vectors of `0`/`1` (see implementation for other cases).

</details>

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

Recurses into lists element-wise where applicable.

</details>

#### `<x` (grade up)

Permutation indices that would sort `x` ascending.

```
  <3 1 2
1 2 0
```

<details>
<summary>spec</summary>

**Types:** numeric and char vectors (see implementation).

</details>

#### `>x` (grade down)

Permutation indices that would sort `x` descending.

```
  >3 1 2
0 2 1
```

<details>
<summary>spec</summary>

**Types:** numeric and char vectors (see implementation).

</details>

#### `=x` (group)

Group indices of equal items.

```
  =0 1 0 1
(0 2
 1 3)
```

<details>
<summary>spec</summary>

**Types:** vectors of groupable types (see implementation).

</details>

#### `~x` (not)

Logical not on `0`/`1` ints (and related cases).

```
  ~0 1
1 0
```

<details>
<summary>spec</summary>

[Numeric Type Rules](#numeric-type-rules) for int boolean vectors.

</details>

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

**Errors:** `value` / `type` when `x` is not evaluable in the chosen mode.

</details>

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

Other types follow [Dictionaries](#dictionaries) and implementation rules.

</details>

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

</details>

#### `?x` (unique)

Unique items of `x` in first-seen order.

```
  ?1 2 1 3
1 2 3
```

<details>
<summary>spec</summary>

Recursion into nested structures follows the implementation.

</details>

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

</details>

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

</details>

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

**Errors:** `type` when `x` is an atom in some cases (see implementation).

</details>

#### `,x` (enlist)

Wrap atom `x` as a length-1 vector `,x`.

```
  ,1
,1
```

<details>
<summary>spec</summary>

</details>

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

</details>

---

---

## Comparison with k3

See `doc/digraphs.md` for differences in adverb syntax.

Key differences:
- Lexical scope (not dynamic)
- No digraphs (`:` suffix)
- `ep[f]x` for each-prior
- Closures supported

