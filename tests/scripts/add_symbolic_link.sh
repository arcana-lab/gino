#!/bin/bash

function linkParUtils {
  cd $1 ;

  for i in `ls`; do
    if ! test -d "$i" ; then
      continue ;
    fi
    cd $i ;
    echo "  $i" ;
    if ! test -f gino_runtime_def.cpp ; then
      ln -s ${rootDir}/src/runtime/gino_runtime_def.cpp ;
      ln -s ${rootDir}/src/runtime/gino_runtime_decl.c ;
    fi
    if ! test -f Makefile ; then
      ln -s ../../scripts/Makefile ;
    fi
    cd ../ ;
  done

  cd ../ ;
}


rootDir="`git rev-parse --show-toplevel`" ;

echo "Adding symbolic links for regression tests" ;
linkParUtils regression
echo "Adding symbolic links for performance tests" ;
linkParUtils performance
