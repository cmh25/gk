# gk tutorial

A guided tour of the REPL. See the [reference manual](ref.md) for detailed documentation—there, **primitive dyads** are written **`a+x`** style (**`a`** left, **`x`** right); tutorial snippets use normal names like **`f`** and **`x`**, **`y`** at the prompt.

## Start the interpreter

After [building](../README.md#build), from the repository root:

```
./gk
```

On **Linux** and **macOS**, **`rlwrap ./gk`** is often nicer: line editing and history via [rlwrap](https://github.com/hanslub42/rlwrap) (install from your package manager if you do not have it).

There is a two-space input prompt. Type an expression and press **Enter**; gk prints the result. There are no `print` statements—**the value of what you type is what you see**.

Exit the session with **`\\`** (two backslashes then Enter).

At the **top-level** prompt, a single **`\`** then **Enter** prints the built-in help index:

```
  \
\0 data
\+ verbs
\' adverbs
\_ reserved verbs/nouns
\. assign define control debug
\: input/output
\- client/server
\` os commands
\? commands
```

Use `\` plus the key on the left (e.g. `\0` for data types, `\+` for verbs) to open that topic.

## Comments

Anything from `/` to the end of the line is a comment:

```
  2+2    / this is ignored
4
```

## Numbers and text

Integers and floats look familiar. Character data uses **double quotes** (strings). Names that start with **`` ` ``** are **symbols** — short names, often used as keys in dictionaries:

```
  40+2
42
  3.5*2
7.0
  "hello"
"hello"
  `north
`north
```

## Vectors: space-separated

If you write several values **with only spaces between them**, gk makes one **vector** (a simple array). Many verbs work **element-wise** on vectors:

```
  1 2 3 4
1 2 3 4
  1 2 3 + 10 20 30
11 22 33
```

## Lists: parentheses and semicolons

Use **`(a;b;c)`** when you need a **list** that can hold different shapes or types (semicolon separates items):

```
  (1;2.0;"hi")
(1;2.0;"hi")
```

If every item in a list is of the same type, gk promotes it to a vector.
```
  (1;2;3)
1 2 3
  ("a";"b";"c")
"abc"
```

## Useful primitives (first pass)

| Form | Idea |
|------|------|
| `!n` | Integers `0` through `n-1` |
| `#x` | Length of `x` |
| `+/x` | Sum (`/` is “over”, `+` is plus) |
| `*x` | First element |
| `|x` | Reverse |
| `x#y` | Take first `x` items of `y` |
| `x_y` | Drop first `x` items of `y` (no `_` in **names**—only this verb) |

Try:

```
  +/!10
45
  2#1 2 3 4
1 2
```

## Each: `'`

**`f'x`** applies **`f`** to **each** element of **`x`** (and there are other each-patterns; see [ref](ref.md#adverbs)):

```
  {x*x}'1 2 3 4
1 4 9 16
```

## Assignment and functions

**`name:value`** assigns to a name (no `let` or `var`):

```
  xs:1 2 3 4
  xs*2
2 4 6 8
```

**`{…}`** is a function. If you do not declare parameters, **`x`**, **`y`**, and **`z`** are the first, second, and third arguments. You can also name them explicitly in **`[...]`** at the start of the body:

```
  sq:{x*x}
  sq 9
81
  add:{x+y}
  add[3;4]
7
  f:{[x;y;z]x+y*z}
  f[1;2;3]
7
```

## Conditional

**`$[cond;then;else]`** chooses one branch (full rules in [ref](ref.md)):

```
  $[1>2;`no;`yes]
`yes
```

## After an error

Sometimes a failing expression leaves you at a **`>`** prompt instead of the usual two spaces. That is a small **debug** session: you can still type expressions (for example **`x`** and **`y`** inside a function that was running). When you are done looking, **`\`** then **Enter** (**abort**) drops you back to the main REPL. That is not the same as a lone **`\`** at the top level (which opens the help index above).

```
  f:{x+y}
  f[1;`a]
type error
{x+y}
  ^
>  x
1
>  y
`a
>  \
```

## Dictionaries

A **dictionary** maps symbols to values. Build an empty one with **`.()`**, then assign fields (like named columns):

```
  d:.()
  d.name:`ada`bob
  d.score:91 87
  d.score
91 87
```

See [Dictionaries in the reference](ref.md#dictionaries) for indexing, amend, and more.

## Dot: evaluate or apply

With one argument, **`.`** either evaluates a **string** of code, or **applies** when `x` looks like **`(function; arguments)`**:

```
  ."1+2"
3
  .(+;1 2)
3
```

This is also the default idea behind **IPC** handlers (see [ipc.md](ipc.md)).

## Where next

| Doc | When to open it |
|-----|------------------|
| [ref.md](ref.md) | Every primitive, builtin, type, and syntax detail |
| [valence.md](valence.md) | How many args functions take; projections |
| [ipc.md](ipc.md) | Talking to another gk process |
| [timer.md](timer.md) | Periodic callbacks |
| [k3.md](k3.md) | If you already know k3 and want a delta list |
