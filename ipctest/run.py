#!/usr/bin/env python3
"""
ipctest/run.py -- run every IPC client/server test pair.

For each pair NN (excluding 10):
  1. spawn ipctest/srvNN.k in the background
  2. wait up to ~2s for either listener (15555 or 15556) to bind
  3. run ipctest/cliNN.k against it with `-q`, with a 30s timeout
  4. kill the server, record pass/fail by client exit code

Pair 10 is self-contained (the server connects to itself) and runs
solo with no client.

Per-run stdout is captured under tempdir as srvNN.out / cliNN.out for
post-mortem inspection of any failure.

Usage (from anywhere):
  python3 ipctest/run.py
  GK=/path/to/gk python3 ipctest/run.py    # override binary
  GK=/tmp/vgk python3 ipctest/run.py       # e.g. valgrind wrapper

Cross-platform: works on POSIX and Windows. Python 3.8+, stdlib only.
"""

import os
import socket
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
TMP  = tempfile.gettempdir()

GK_DEFAULT = os.path.join(ROOT, "gk.exe" if os.name == "nt" else "gk")
GK = os.environ.get("GK", GK_DEFAULT)

PORTS           = (15555, 15556)
WAIT_LISTENER_S = 2.0
TIMEOUT_PAIR_S  = 30
TIMEOUT_SOLO_S  = 10
SETTLE_S        = 0.1   # let server flush before kill

# Pairs whose server uses the forking listener (`\m f`). gk's fork-per-
# connection mode is POSIX-only; on Windows the server prints
# `fork: not supported on windows` and never binds, so these pairs
# can't run there. Skip rather than fail.
FORK_PAIRS = frozenset(["06", "07", "12", "13", "14"])
SKIP_PAIRS = FORK_PAIRS if os.name == "nt" else frozenset()


def listener_up():
    """Return True if anything is listening on either test port."""
    for port in PORTS:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.05):
                return True
        except (ConnectionRefusedError, socket.timeout, OSError):
            continue
    return False


def ports_in_use():
    """Return the subset of PORTS that already have someone listening,
    before we spawn any server. Used as a pre-flight to fail fast with a
    clear message instead of letting the new server's bind error get
    masked by a `wait_for_listener` probe that succeeds against the
    *previous* owner of the port (and a client that then connects to
    the wrong server and hangs)."""
    busy = []
    for port in PORTS:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.05):
                busy.append(port)
        except (ConnectionRefusedError, socket.timeout, OSError):
            continue
    return busy


def wait_for_listener(deadline):
    while time.monotonic() < deadline:
        if listener_up():
            return True
        time.sleep(0.05)
    return False


def kill_proc(proc):
    """Kill a server. On POSIX we kill the whole process group so the
    forking pairs (06, 07, 12-14) don't leak short-lived children
    that haven't quite finished yet. On Windows we just kill the
    tracked process; the fork-mode pairs aren't supported there
    anyway and report `fork: not supported on windows`."""
    if proc.poll() is None:
        try:
            if os.name == "posix":
                os.killpg(os.getpgid(proc.pid), 9)
            else:
                proc.kill()
        except OSError:
            pass
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        pass


def spawn_server(srv_path, sout_fp):
    """Start a server with stdin held open (never closed) so gk's REPL
    doesn't hit EOF and exit -- the cross-platform equivalent of
    `</dev/zero` in the bash version. start_new_session puts the
    server in its own process group on POSIX so kill_proc can take
    out forked children too; it's a no-op on Windows."""
    kw = {}
    if os.name == "posix":
        kw["start_new_session"] = True
    return subprocess.Popen(
        [GK, srv_path],
        cwd=ROOT,
        stdin=subprocess.PIPE,
        stdout=sout_fp,
        stderr=subprocess.STDOUT,
        **kw,
    )


def run_pair(n, results):
    srv = os.path.join("ipctest", f"srv{n}.k")
    cli = os.path.join("ipctest", f"cli{n}.k")
    sout_path = os.path.join(TMP, f"srv{n}.out")
    cout_path = os.path.join(TMP, f"cli{n}.out")

    with open(sout_path, "wb") as sout, open(cout_path, "wb") as cout:
        srv_proc = spawn_server(srv, sout)
        try:
            ok_listener = wait_for_listener(time.monotonic() + WAIT_LISTENER_S)
            if not ok_listener:
                results.append((n, False, "no listener bound"))
                print(f"pair {n}: FAIL (no listener bound)")
                return
            try:
                cli_rc = subprocess.run(
                    [GK, "-q", cli],
                    cwd=ROOT,
                    stdout=cout,
                    stderr=subprocess.STDOUT,
                    timeout=TIMEOUT_PAIR_S,
                ).returncode
            except subprocess.TimeoutExpired:
                results.append((n, False, "client timed out"))
                print(f"pair {n}: FAIL (client timed out)")
                return
            time.sleep(SETTLE_S)
        finally:
            kill_proc(srv_proc)

    if cli_rc == 0:
        results.append((n, True, None))
        print(f"pair {n}: OK")
    else:
        results.append((n, False, f"client rc={cli_rc}"))
        print(f"pair {n}: FAIL (client rc={cli_rc})")


def run_solo(n, results):
    srv = os.path.join("ipctest", f"srv{n}.k")
    sout_path = os.path.join(TMP, f"srv{n}.out")
    rc = -1
    with open(sout_path, "wb") as sout:
        try:
            rc = subprocess.run(
                [GK, srv],
                cwd=ROOT,
                stdout=sout,
                stderr=subprocess.STDOUT,
                timeout=TIMEOUT_SOLO_S,
            ).returncode
        except subprocess.TimeoutExpired:
            results.append((n, False, "timed out"))
            print(f"pair {n}: FAIL (timed out)")
            return
    if rc == 0:
        results.append((n, True, "solo"))
        print(f"pair {n}: OK (solo)")
    else:
        results.append((n, False, f"rc={rc}"))
        print(f"pair {n}: FAIL (rc={rc})")


def dump_failures(results):
    failed = [n for (n, ok, _) in results if not ok]
    if not failed:
        return
    print()
    print(f"failed pairs: {' '.join(failed)}")
    print(f"capture dir:  {TMP}")
    for n in failed:
        for kind in ("srv", "cli"):
            p = os.path.join(TMP, f"{kind}{n}.out")
            if not os.path.exists(p):
                continue
            print(f"=== {p} ===")
            try:
                with open(p, "rb") as fp:
                    sys.stdout.write(fp.read().decode("utf-8", errors="replace"))
            except OSError:
                pass


def main():
    if not os.path.isfile(GK) and not os.access(GK, os.X_OK):
        print(f"error: {GK} not found or not executable -- run `make` first",
              file=sys.stderr)
        return 2

    busy = ports_in_use()
    if busy:
        names = ", ".join(str(p) for p in busy)
        print(f"error: port(s) {names} already in use -- a leftover gk?",
              file=sys.stderr)
        print("       try: pkill -9 -x gk    (or lsof -nP -iTCP:%d -sTCP:LISTEN)" % busy[0],
              file=sys.stderr)
        return 2

    results = []
    pairs = ["01", "02", "03", "04", "05", "06", "07", "08", "09",
             "11", "12", "13", "14", "15", "16"]
    nskip = 0
    for n in pairs:
        if n in SKIP_PAIRS:
            print(f"pair {n}: SKIP (forking listener unsupported on windows)")
            nskip += 1
            continue
        run_pair(n, results)
    run_solo("10", results)

    npass = sum(1 for (_, ok, _) in results if ok)
    nfail = len(results) - npass
    print()
    if nskip:
        print(f"summary: {npass} passed, {nfail} failed, {nskip} skipped")
    else:
        print(f"summary: {npass} passed, {nfail} failed")
    dump_failures(results)
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
