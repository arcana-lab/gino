#!/bin/bash

function cleanTests {
  cd $1 ;

  rm -f *.txt ;

  for i in `ls`; do
    if ! test -d $i ; then
      continue ;
    fi

    cd $i ;
    make clean ;
    rm -f GINO_APIs.* *_utils.cpp Makefile *.log *.dot ;
    cd ../ ;
  done

  cd ../ ;
}

./scripts/add_symbolic_link.sh ;

cleanTests regression
cleanTests performance

# Remove speedup info on performance tests
cd performance ;
rm -f *.txt ;
cd ../ ;
