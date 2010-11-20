#!/bin/sh

# Convert vanilla xlife files into ASCII PBM files.

is_pattern_file() {
  first=`head -1 $1 | grep -i '^#p'`
  if [ -n "$first" ]; then
    echo 1
  else
    echo 0
  fi
}

pattern_width() {
  first=`grep -v '^#' $1 | head -1`
  char=`echo "$first" | wc -c`
  echo `expr $char - 1`
}

pattern_height() {
  echo `grep -v '^#' $1 | grep -v '^$' | wc -l`
}

for f in `echo *.life`; do
  if [ `is_pattern_file $f` = 1 ]; then
    echo converting $f...
    out=convert/`basename $f .life`.pbm
    w=`pattern_width $f`
    h=`pattern_height $f`
    echo P1 > $out
    echo "$w $h" >> $out
    grep -v ^# $f | tr -c '*\n' 0 | tr \* 1 >> $out
  else
    echo skipping $f.
  fi
done

exit 0

