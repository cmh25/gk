#!/bin/bash

os=`uname -s`
if [ "$os" = "Darwin" ]; then echo "valgrind tests are linux only"; exit 0; fi
which valgrind &>/dev/null
if [ $? -ne 0 ]; then echo "valgrind not found" && exit 1; fi
ec=0
for t in `cat tests`; do
  echo -n "test$t: "
  valgrind --leak-check=full ../gk test$t >/dev/null 2> v$t
  grep -q "ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)" v$t
  if [ $? -ne 0 ]; then
    ec=$((ec+1))
    echo -e "fail *****"
  else
    echo -e "pass"
    rm -f $t v$t
  fi
done
echo failed: $ec
exit 0
