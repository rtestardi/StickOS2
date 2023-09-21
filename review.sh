#! /bin/sh

shopt -s expand_aliases
if git.cmd --version >/dev/null 2>&1; then
  alias git=git.cmd
fi

echo "did you run test.sh and test3.sh?"
echo

if [ -d /cygdrive/c/tmp ]; then
  TEMP=/cygdrive/c/tmp
elif [ -d /cygdrive/c/temp ]; then
  TEMP=/cygdrive/c/temp
elif [ -d /temp ]; then
  TEMP=/temp
elif [ -d /tmp ]; then
  TEMP=/tmp
else
  echo "TEMP directory not found"
  exit 1
fi

# get current branch
branch=`git branch | grep \* | sed 's!\* !!'`

# find modified files
modified=`git diff --name-only --diff-filter=M ${1:-origin/$branch} --`

# find added files
added=`git diff --name-only --diff-filter=A ${1:-origin/$branch} --`

# find removed files
removed=`git diff --name-only --diff-filter=D ${1:-origin/$branch} --`

# find binary files
binary=`git diff ${1:-origin/$branch} -- | grep "Binary files" | sed 's!.* a/!!; s! .*!!'`

# verify all non-binary modified and added files have proper line endings
BAD=
if [ "X$CSAARCH" != "X${CSAARCH%*linux*}" ]; then
  CONV=dos2unix
else
  CONV=unix2dos
fi
for f in $modified $added; do
  if echo "$binary" | grep "^$f$" >/dev/null; then
    continue
  fi
  rm -f $TEMP/f
  cp $f $TEMP/f
  $CONV $TEMP/f >/dev/null 2>&1
  if cmp $f $TEMP/f; then
    :
  else
    echo "BAD LINE ENDINGS in $f"
    echo
    BAD="$BAD $f"
  fi
done

# create review.txt
(
echo "Origin:" `grep url .git/config | sed 's!.*= !!'`
echo "Branch: $branch"
# summary
echo "modified files:"
echo "$modified" | sed 's!^!  !'
if [ -n "$added" ]; then
  echo "added files:"
  echo "$added" | sed 's!^!  !'
fi
if [ -n "$removed" ]; then
  echo "removed files:"
  echo "$removed" | sed 's!^!  !'
fi
echo

# primary diff
git fetch origin
git diff -b ${1:-origin/$branch} --

# look for bad line endings
echo
echo "************************** BAD LINE ENDINGS ****************************"
echo $BAD

# look for tabs
echo
echo "********************************* TABS *********************************"
grep "	" $modified $added /dev/null | grep -vE "vcproj|vssettings|Makefile" | dos2unix

# look for XXX
echo
echo "********************************* XXX *********************************"
grep "^[+].*XXX" $modified $added /dev/null | dos2unix

# finally, diff the whitespace
echo
echo "****************************** WHITESPACE ******************************"
git diff -b ${1:-origin/$branch} -- >$TEMP/diff1.txt
git diff ${1:-origin/$branch} -- >$TEMP/diff2.txt
diff $TEMP/diff1.txt $TEMP/diff2.txt
) >$TEMP/review.txt 2>&1

# create review.tar.gz archive
rm -f $TEMP/review.tar.gz
tar -cvf $TEMP/review.tar $modified $added >/dev/null
gzip $TEMP/review.tar

# echo the summary
awk '!length{exit}{print}' <$TEMP/review.txt
echo

echo "see $TEMP/review.txt, $TEMP/review.tar.gz"
echo

echo "see also:"
echo "  coding conventions in convents.txt"
echo

