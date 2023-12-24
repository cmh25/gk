#!/bin/bash

ec=0
for t in `cat tests`; do
  echo -n "test$t: "
  ../gk test$t > $t 2>/dev/null
  diff -u res$t $t &> diff$t
  if [ $? -ne 0 ]; then
    ec=$((ec+1))
    echo -e "fail *****"
  else
    echo -e "pass"
    rm -f $t diff$t
  fi
done
echo failed: $ec
exit 0
