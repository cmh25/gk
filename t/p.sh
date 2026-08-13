#!/usr/bin/env bash
for i in p001 p002 p003 p004 p005 p006 p007 p008; do
  echo "$i ========================="
  ../gk $i
  echo
done
exit 0
