#!/bin/bash

ec=0
for t in `cat tests`; do
  echo -n "t$t: "
  ../gk t$t > $t 2>e$t
  rc=$?
  diff -u r$t $t &> d$t
  if [ $rc -ne 0 -o $? -ne 0 ]; then
    ec=$((ec+1))
    echo -e "fail *****"
  else
    echo -e "pass"
    rm -f $t d$t e$t
  fi
done
echo failed: $ec
exit 0
