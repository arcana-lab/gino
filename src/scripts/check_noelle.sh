#!/bin/bash

if ! command -v noelle-config &> /dev/null ; then
  echo "NOELLE is not found." ;
  echo "Please install NOELLE before compiling GINO";
  exit 1;
fi
