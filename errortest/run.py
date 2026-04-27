#!/usr/bin/env python3
r"""
errortest/run.py -- automate the manual error-handling / subrepl tests.

Each errortest/z?? file contains:

  <K source block>            <- loaded by `./gk errortest/z??`
  \\                          <- exits the load
                              <- (blank + `\` delimit source from sessions)
  \
  cmh@host:~/gk$ rlwrap ./gk errortest/z??
  gk-v1.0.0 Copyright ...
  ...interleaved output and echoed user input at `>  ` subrepl prompts...

  cmh@host:~/gk$ rlwrap ./gk errortest/z??
  ...second transcript, different user inputs...

For each session we:
  1. parse the invocation command (after `cmh@...$ `)
  2. find every subrepl prompt line matching `^(>+)  (.+)$` and treat
     the content after the prompt as echoed user input (the pipe won't
     echo it, so it must be replayed via stdin AND stripped from the
     expected output before comparison)
  3. run `./gk errortest/z??` with those inputs piped on stdin
  4. diff actual output against the normalized expected output
     (banner year normalized, echoed inputs stripped).

Usage:
  ./run.py                # run every errortest/z?? file
  ./run.py z00 z05        # run specific files
  ./run.py -v z00         # show diffs for failures
"""

import argparse, difflib, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
GK   = os.path.join(ROOT, "gk.exe" if os.name == "nt" else "gk")

# A subrepl prompt line: `>+  INPUT\n` at the start of a line.
# Top-level `  ` prompts are never used in z?? files (the `./gk FILE`
# invocation exits after loading; there is no interactive top level).
PROMPT_INPUT = re.compile(r"^(>+)  (.+)$", re.MULTILINE)

# Session header: `cmh@host:cwd$ CMD`.
SESSION_HDR = re.compile(r"^[A-Za-z0-9_\-\.]+@[^\n]*\$ (.+)$", re.MULTILINE)

# gk banner line, with variable version (X.Y.Z) and copyright year range.
# Both are normalized to placeholders before diffing so transcripts survive
# version bumps and year rollovers without regeneration.
BANNER_RE = re.compile(r"gk-v\d+\.\d+\.\d+ Copyright \(c\) 2023-\d{4} Charles Hall")
BANNER_NORM = "gk-vX.Y.Z Copyright (c) 2023-YYYY Charles Hall"

# gk used to print load-frame offsets as `... +N` (no space); the
# current format is `... + N` (one space). Collapse both to a single
# canonical form so this harmless cosmetic change doesn't show up as a
# test failure.
PLUS_FIX = re.compile(r"(\.\.\. )\+(\d)")


def parse_file(path):
    """Return (source_text, [(cmd, expected_output_text), ...]).

    source_text is everything before the first session header.
    Each expected_output_text is everything between one session header
    and the next (or EOF), with leading/trailing blank lines trimmed.
    """
    with open(path) as fp:
        body = fp.read()
    headers = list(SESSION_HDR.finditer(body))
    if not headers:
        return body, []
    source = body[: headers[0].start()]
    sessions = []
    for i, m in enumerate(headers):
        cmd = m.group(1).strip()
        start = m.end() + 1  # skip newline after header
        end = headers[i + 1].start() if i + 1 < len(headers) else len(body)
        expected = body[start:end]
        # Strip any trailing blank lines between sessions.
        expected = expected.rstrip("\n") + "\n"
        sessions.append((cmd, expected))
    return source, sessions


def strip_invocation(cmd):
    """Strip `rlwrap` wrapper if present and return argv list."""
    parts = cmd.split()
    if parts and parts[0] == "rlwrap":
        parts = parts[1:]
    # Accept `./gk` invocation from repo root.
    return parts


def extract_inputs(expected):
    """Return list of echoed user inputs from subrepl prompt lines."""
    return [m.group(2) for m in PROMPT_INPUT.finditer(expected)]


def strip_inputs(expected):
    """Remove echoed input (and its trailing newline) from each subrepl
    prompt line, because the pipe doesn't echo. After stripping, a
    prompt like `>  :\n` becomes `>  ` (no newline), which matches what
    the pipe-driven gk actually emits.

    Also strip the top-level `  \\\\\\n` exit line: in a transcript the
    user types `\\\\` at the top-level prompt to quit, but the pipe
    driver reaches EOF instead, which terminates the same way. The
    prompt (`  `) stays; only the echoed `\\\\` is removed."""
    def repl(m):
        return m.group(1) + "  "
    expected = re.sub(r"^(>+)  (.+)\n", repl, expected, flags=re.MULTILINE)
    expected = re.sub(r"^  \\\\\n", "  ", expected, flags=re.MULTILINE)
    return expected


def normalize_banner(text):
    return BANNER_RE.sub(BANNER_NORM, text)


def normalize_plus(text):
    return PLUS_FIX.sub(r"\1+ \2", text)


def run_session(cmd_parts, stdin_text):
    """Invoke gk with the given argv (after stripping rlwrap) and the
    given stdin. Returns captured combined stdout+stderr."""
    # `cmd_parts` is like ["./gk", "errortest/z00"]. Run from ROOT.
    p = subprocess.run(
        cmd_parts,
        input=stdin_text,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=ROOT,
        text=True,
        timeout=5,
    )
    return p.stdout


# A trailing shell prompt line terminates a session transcript but isn't
# gk output. Strip it (and any preceding blank line) before comparing.
TRAILING_SHELL = re.compile(
    r"\n?[A-Za-z0-9_\-\.]+@[^\n]*\$\s*\n?\s*\Z"
)


def compare(expected_raw, actual_raw):
    """Compare after normalizing banner + stripping echoed input.
    Both sides are rstripped so a trailing interactive-REPL prompt
    (`  ` with no newline, emitted when the pipe reaches EOF after the
    load) doesn't spuriously fail the match."""
    exp = normalize_plus(normalize_banner(strip_inputs(expected_raw)))
    exp = TRAILING_SHELL.sub("", exp)
    act = normalize_plus(normalize_banner(actual_raw))
    return exp.rstrip() == act.rstrip(), exp, act


def run_one(path, verbose=False):
    """Run every session in path. Return (#pass, #fail)."""
    rel = os.path.relpath(path, ROOT)
    source, sessions = parse_file(path)
    if not sessions:
        print(f"  {rel}: SKIP (no sessions)")
        return 0, 0
    p = f = 0
    for i, (cmd, expected) in enumerate(sessions):
        argv = strip_invocation(cmd)
        inputs = extract_inputs(expected)
        stdin_text = "".join(s + "\n" for s in inputs)
        try:
            actual = run_session(argv, stdin_text)
        except subprocess.TimeoutExpired:
            print(f"  {rel} session {i+1}: TIMEOUT")
            f += 1
            continue
        ok, exp_norm, act_norm = compare(expected, actual)
        if ok:
            p += 1
        else:
            f += 1
            print(f"  {rel} session {i+1}: FAIL  ({cmd})")
            if verbose:
                diff = difflib.unified_diff(
                    exp_norm.splitlines(keepends=True),
                    act_norm.splitlines(keepends=True),
                    fromfile="expected(normalized)",
                    tofile="actual",
                    n=3,
                )
                sys.stdout.writelines(diff)
                if not exp_norm.endswith("\n") or not act_norm.endswith("\n"):
                    sys.stdout.write("\n")
    status = "OK" if f == 0 else "FAIL"
    print(f"  {rel}: {status}  ({p} pass, {f} fail, {len(sessions)} total)")
    return p, f


def main():
    ap = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("files", nargs="*",
        help="z?? file names (default: all errortest/z??)")
    ap.add_argument("-v", "--verbose", action="store_true",
        help="show unified diff for each failing session")
    args = ap.parse_args()

    if not os.path.isfile(GK):
        print(f"error: {GK} not found -- run `make` first", file=sys.stderr)
        return 2

    if args.files:
        paths = [os.path.join(HERE, f) for f in args.files]
    else:
        paths = sorted(
            os.path.join(HERE, n)
            for n in os.listdir(HERE)
            if re.fullmatch(r"z\d{2}", n)
        )

    total_pass = total_fail = 0
    print(f"running {len(paths)} errortest file(s)")
    for path in paths:
        p, f = run_one(path, verbose=args.verbose)
        total_pass += p
        total_fail += f
    print(f"\nsummary: {total_pass} passed, {total_fail} failed")
    return 0 if total_fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
