#! /bin/sh

if [ X${1:-} = X-u ]; then
  cp basic.log basic.txt
else
  sh basic.sh ${1:-} | sed 's/\(v[0-9][0-9.]*\)[a-z]!/\1!/' >basic.log
  diff -ab basic.txt basic.log
fi
