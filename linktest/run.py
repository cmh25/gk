#!/usr/bin/env python3
"""
linktest/run.py -- exercise `f 2:(e;t)` link object code end to end.

Steps:
  1. compile linktest/lib.c into a shared object (.so / .dylib / .dll)
  2. run linktest/test.k with `-q`, passing the library's path in the
     GKLINKLIB environment variable (the script reads it via getenv)
  3. record pass/fail by the client's exit code (test.k exits 1 on the
     first failed assertion, 0 on success)

Unlike errortest/ipctest this needs a C compiler at run time. If none is
found the test SKIPs (it does not fail) so `make test` stays robust on
machines without a toolchain.

Usage (from anywhere):
  python3 linktest/run.py
  GK=/path/to/gk python3 linktest/run.py    # override the gk binary
  CC=clang python3 linktest/run.py          # override the compiler

Python 3.8+, stdlib only.
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

GK_DEFAULT = os.path.join(ROOT, "gk.exe" if os.name == "nt" else "gk")
GK = os.environ.get("GK", GK_DEFAULT)

TIMEOUT_S = 30


def is_msvc(exe):
    """MSVC-style driver (cl / clang-cl): different flags, /LD for a DLL."""
    base = os.path.basename(exe).lower()
    return base in ("cl", "cl.exe", "clang-cl", "clang-cl.exe")


def find_cc():
    """Locate a C compiler. Honor $CC (first token, in case it carries
    flags); otherwise try the usual names. On Windows that includes MSVC's
    `cl` (present once vcvars64.bat has been run), so no mingw/clang is
    required. Returns the executable path or None."""
    env_cc = os.environ.get("CC")
    if env_cc:
        exe = env_cc.split()[0]
        return shutil.which(exe) or (exe if os.path.isfile(exe) else None)
    names = ("cl", "clang-cl", "clang", "cc", "gcc") if os.name == "nt" \
            else ("cc", "gcc", "clang")
    for name in names:
        p = shutil.which(name)
        if p:
            return p
    return None


def lib_name():
    if sys.platform == "darwin":
        return "libgklinktest.dylib"
    if os.name == "nt":
        return "gklinktest.dll"
    return "libgklinktest.so"


def compile_lib(cc, src, out, workdir, implib=None):
    """Build the test object as a shared library. Plain flags only -- the
    test functions are trivial, so we skip the main build's -flto/-march
    tuning. MSVC `cl`/`clang-cl` use /LD (build a DLL); everything else uses
    -shared. The gk_* ABI the .so calls is resolved at load time against gk on
    POSIX, but a Windows DLL must link gk's import library (implib) at build
    time. Compiled in workdir so scratch lands in the temp dir."""
    if is_msvc(cc):
        cmd = [cc, "/nologo", "/O2", "/LD", src]
        if implib:
            cmd.append(implib)
        cmd.append("/Fe:" + out)
    else:
        cmd = [cc, "-O2", "-shared"]
        if os.name != "nt":
            cmd += ["-fPIC"]
        if sys.platform == "darwin":
            # macOS won't leave gk_* undefined in a -shared object by default;
            # defer them to load-time lookup against the host gk (which exports
            # them via -export_dynamic).
            cmd += ["-undefined", "dynamic_lookup"]
        cmd += ["-o", out, src]
    return subprocess.run(cmd, cwd=workdir, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT)


def main():
    if not (os.path.isfile(GK) and os.access(GK, os.X_OK)):
        print(f"error: {GK} not found or not executable -- run `make` first",
              file=sys.stderr)
        return 2

    cc = find_cc()
    if not cc:
        print("link: SKIP (no C compiler found; set $CC to enable)")
        print()
        print("summary: 0 passed, 0 failed, 1 skipped")
        return 0

    test = os.path.join("linktest", "test.k")
    implib = os.path.join(ROOT, "gk.lib")  # Windows: gk's import lib (if built)
    implib = implib if os.path.isfile(implib) else None

    with tempfile.TemporaryDirectory() as td:
        # lib.c does #include "gk.h"; copy both into the build dir and compile
        # there, so the self-contained header resolves with no -I into the gk
        # tree (which would risk shadowing system headers via the unistd shim).
        shutil.copy(os.path.join(ROOT, "gk.h"), os.path.join(td, "gk.h"))
        src = os.path.join(td, "lib.c")
        shutil.copy(os.path.join(HERE, "lib.c"), src)
        lib = os.path.join(td, lib_name())
        cres = compile_lib(cc, src, lib, td, implib)
        if cres.returncode != 0:
            print("link: FAIL (compile)")
            sys.stdout.write(cres.stdout.decode("utf-8", "replace"))
            print()
            print("summary: 0 passed, 1 failed")
            return 1

        env = dict(os.environ, GKLINKLIB=lib)
        try:
            res = subprocess.run(
                [GK, "-q", test],
                cwd=ROOT,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                env=env,
                timeout=TIMEOUT_S,
            )
        except subprocess.TimeoutExpired:
            print("link: FAIL (timed out)")
            print()
            print("summary: 0 passed, 1 failed")
            return 1

    out = res.stdout.decode("utf-8", "replace")
    ok = res.returncode == 0 and "OK" in out
    if ok:
        print("link: OK")
        print()
        print("summary: 1 passed, 0 failed")
        return 0
    print(f"link: FAIL (rc={res.returncode})")
    sys.stdout.write(out)
    print()
    print("summary: 0 passed, 1 failed")
    return 1


if __name__ == "__main__":
    sys.exit(main())
