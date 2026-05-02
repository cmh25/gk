# ipc

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

## quick start

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

## client

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

### host forms

All of these open a connection to port 5555 on the local machine:

```
  h:3:("localhost";5555)
  h:3:(`localhost;5555)
  h:3:(`;5555)
  h:3:("127.0.0.1";5555)
```

### sync result

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

### async result

`h 3:msg` returns `null` immediately; the call does not wait for the
server handler to run. Server-side errors go through the usual `\e`
path: under `\e 0` they are silently dropped, under `\e 1` the server
drops into a debug sub-REPL (there is no caller to surface them to
either way). Use async for fire-and-forget updates, state mutation,
logging, etc.

### implicit open

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

## server

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

### inline

All accepted connections are serviced by the single gk process.
Handlers run in the REPL's event loop.

### forking

Each newly accepted connection is handed off to a child via `fork()`.
The child serves only that one connection, then exits. The parent
resumes accepting. This gives each peer a fresh, isolated copy of the
gk namespace — useful when handlers mutate state that you don't want
bleeding between clients.

forking mode is **POSIX only**. On Windows, `\m f PORT` returns
`fork: not supported on windows`.

### rebind semantics

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

### stopping a listener

```
  \m i 0      / stop inline slot
  \m f 0      / stop forking slot
```

Listeners persist across sub-REPLs (`\\` into a nested REPL does not
close them). Stop them explicitly with `\m X 0`, or let process exit do
it. Client handles opened with `3:(host;port)` are similarly persistent;
close with `3:h`.

## handlers

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

### `.m.c` is a string, not a lambda

`.m.c` is executed when a connection is closed.
Anything that isn't a non-empty char vector (including lambdas,
symbols, ints, lists, and the default `""`) is silently ignored.

```
  .m.c:"closed+:1"    / bumps a global each time a peer goes away
```

### overriding

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

### `.z.w`

Inside `.m.s`, `.m.g`, and `.m.c`, the global `.z.w` holds the handle of
the peer whose message is being dispatched. Outside handler context
it reads as `0`.

```
  .m.s:{`0:"msg from handle ",($.z.w),"\n"}
  .m.c:"`0:\"peer \",($.z.w),\" disconnected\\n\""
```

## draining a batch of asyncs

Messages on a single conn are dispatched strictly in order, so a sync
on the same conn after a burst of asyncs acts as a barrier — its
response can't come back until every prior async has finished
running:

```
  {(`;5555) 3:x}'msgs    / fire a batch, non-blocking
  (`;5555) 4:"0"         / returns once the whole batch has been handled
```

This is usually what you want instead of `sleep`.
