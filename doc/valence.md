# valence in gk

## what is valence?

Valence is the number of parameters a function expects. 

```
  val {x}
1
  val {x,y}
2
  val {x,y,z}
3
  val {[a;b;c;d;e]a,b,c,d,e}
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

## valence zero?

So, what is the valence of `{1}`? It doesn't reference any parameters, so we might be tempted to say its valence is zero. However, there are a couple of problems with that.

1. What is the function's domain if its valence is zero?
2. What is the syntax to execute a valence zero function?

gk takes the position that there is no such thing as a valence zero function. All functions have at least a valence of one (the one parameter may be ignored).

It's arguable that `{1}[]` is an answer to (2), but gk takes the position that `[]` is not empty, but rather the same as `[nul]`.

The reason for this is to be consistent with prototypes.

## prototypes

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

## applying functions to empty vectors

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

## why `[]` passes `nul`

With this model, `[]` is not a special case—it's just the general instance of the rule:

- `f[]` is `f . ()`
- `f . ()` is `f[*()]`
- `*()` is `nul`
- therefore `f[]` is `f[nul]`

This explains why `{x}[]` returns `nul` rather than being a projection:

```
  {x}[]
nul
  {1,x}[]
(1;)
```

The function executes with `x` bound to `nul` (the prototype of `()`), not with `x` unbound.

## minimum valence is 1

Since there's no way to invoke a function without providing *some* argument, the minimum valence is 1:

```
  val {5}
1
  val {x}
1
```

Under this model, `{5}` is not a nullary function—it's a unary function that ignores its argument. It maps any input to 5. This aligns with how constant functions work in mathematics, where f(x) = 5 is still a function of one variable.

## dictionaries

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

## summary

gk's position:

- every empty vector has a prototype, `*v`
- `f . v` where `v` is empty passes the prototype: `f . v ≡ f[*v]`
- `f[]` is equivalent to `f . ()`, so `f[]` passes `nul`
- therefore every function has valence ≥ 1
- `{5}` is a function that maps any input to 5

Note: `f[]` is not the same as `f[()]`. The latter passes the empty list itself as an argument.

Other k implementations may take different approaches, and it may be possible to construct other consistent models.
