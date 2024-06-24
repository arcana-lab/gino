#!/bin/bash

testsThatFailed=`cat regression*/errors.txt 2>>/dev/null | awk '{print $1}' | sed 's/.*regression_[0-9]*\///' | sort -u` ;
tmpFile=`mktemp` ;

for failedTest in $testsThatFailed ; do
  n=`cat regression*/errors.txt 2>>/dev/null | grep "\/$failedTest " | wc -l` ;
  echo -n "$failedTest  " >> $tmpFile ;

  spaces=`echo "80 - ${#failedTest}" | bc` ;
  for i in `seq 1 $spaces` ; do
    echo -n " " >> $tmpFile ;
  done

  echo "$n configurations failed" >> $tmpFile;
done
sort -r -g -k 2 $tmpFile ;

rm $tmpFile ;
