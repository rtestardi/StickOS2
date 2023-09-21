#! /bin/sh

if [ X${1:-} = X-u ]; then
  cp basic3.log basic3.txt
else
  sh basic3.sh ${1:-} | sed 's/\(v[0-9][0-9.]*\)[a-z]!/\1!/' | tr -d '\000' >basic3.log
  diff -a basic3.txt basic3.log
fi
