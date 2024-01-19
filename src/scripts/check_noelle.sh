#!/bin/bash

if ! command -v noelle-config &> /dev/null ; then
  echo "NOELLE is not found." ;
  echo "Please install NOELLE before compiling GINO";
  exit 1;
fi

# Fetch the root of the current GINO git repository
rootGitRepo="`git rev-parse --show-toplevel`" ;

# Fetch the installation directory of NOELLE that is used in the environment
noelleInstallDir="`noelle-config --prefix`";

# Specify to CMake which NOELLE to use for compiling GINO
echo "include_directories(\"${noelleInstallDir}/include\")" >> ${rootGitRepo}/src/scripts/DependencesCMake.txt ;
