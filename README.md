<img src="gk.png" alt="gk" width="200">

**gk** is an interpreter for an array programming language in the tradition of [k](https://en.wikipedia.org/wiki/K_(programming_language)) (Arthur Whitney).

▶ **[Try gk in your browser](https://cmh25.github.io/gk/)** — a full REPL running entirely in WebAssembly, no install.

If you know **APL**, **J**, or **q**, much of the feel will be familiar. If you are new to array languages, work through the **[tutorial](doc/tutorial.md)** first, then use the [reference manual](doc/ref.md) for complete detail.

The design of gk is inspired by k3, with additional ideas from [Stevan Apter](https://nsl.com/). Other open-source **k**-like languages are listed in [implementations](doc/implementations.md).

## Documentation

| Doc | What it is |
|-----|----------------|
| [doc/tutorial.md](doc/tutorial.md) | Short hands-on REPL tour for newcomers |
| [doc/ref.md](doc/ref.md) | Language reference (types, primitives, builtins, syntax) |
| [doc/v3.md](doc/v3.md) | v3 changes and migration from v2 |
| [doc/v2.md](doc/v2.md) | v2 changes and migration from v1 |
| [doc/v1.md](doc/v1.md) | v1 changes and migration from v0 |
| [doc/k3.md](doc/k3.md) | If you already know k3: main dialect differences |
| [doc/digraphs.md](doc/digraphs.md) | k3 digraphs removed |

## Build

From the repository root, on **Linux**, **macOS**, or **Windows**:

```
make
make test
```

**Note:** On **Windows**, use a **Visual Studio Developer Command Prompt** so the MSVC toolchain is on `PATH`.

## A taste of the REPL

```
$ ./gk
gk-v3.0.0 Copyright (c) 2023-2026 Charles Hall

  1+2
3
  1+2 3 4                 / scalar + vector
3 4 5
  1 2 3+4 5 6             / vector + vector
5 7 9

  !10                     / first n, 0-based
0 1 2 3 4 5 6 7 8 9
  +/!10                   / + over: sum
45

  s:"asdfasdfasdf"        / assign string to variable
  |s                      / reverse
"fdsafdsafdsa"
  <s                      / grade up: indices that sort ascending
0 4 8 2 6 10 3 7 11 1 5 9
  s@<s                    / index by grade up: sorted ascending
"aaadddfffsss"
  ?s@<s                   / unique of sorted
"adfs"

  f:{x+y*z}               / function definition
  f[1;2;3]                / function execution
7

  \\                      / exit
```
