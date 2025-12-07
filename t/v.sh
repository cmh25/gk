#!/bin/bash

if [ "$CORE" = "" ]; then CORE=k.core; fi
if [ "$CORE" != "k.core" ]; then echo "valgrind tests not available for $CORE core"; exit 0; fi

os=`uname -s`
if [ "$os" = "Darwin" ]; then echo "valgrind tests are linux only"; exit 0; fi
which valgrind &>/dev/null
if [ $? -ne 0 ]; then echo "valgrind not found" && exit 1; fi
file ../gk | grep -q debug_info
if [ $? -ne 0 ]; then echo "valgrind tests only work with debug build (make clean; make gkd)"; exit 1; fi
ec=0
vo="--leak-check=full --show-leak-kinds=all --error-exitcode=1 --errors-for-leak-kinds=all"
for t in `cat tests`; do
  echo -n "t$t: "
  valgrind $vo ../gk t$t >/dev/null 2> v$t
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
