#!/usr/bin/env bash
# Usage: t.sh [stdin]
#   (no arg)  run each test as a file argument   (file mode)
#   stdin     feed each test on stdin            (REPL mode)
# REPL-mode bugs (e.g. top-level :return, RETURN leaking from a called
# function) don't show in file mode, so the suite can be run both ways.
# Prompts go to stderr (captured in e$t); stdout is compared to r$t.

mode=$1
ec=0
for t in `cat tests`; do
  echo -n "t$t: "
  if [ "$mode" = stdin ]; then
    ../gk < t$t > $t 2>e$t
  else
    ../gk t$t > $t 2>e$t
  fi
  rc=$?
  ../ndiff r$t $t &> d$t
  if [ $rc -ne 0 -o $? -ne 0 ]; then
    ec=$((ec+1))
    echo -e "fail *****"
  else
    echo -e "pass"
    rm -f $t d$t e$t
  fi
done
echo failed: $ec
exit $ec
