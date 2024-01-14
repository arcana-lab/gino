#!/bin/bash

# Set the installation directory
installDir=$NOELLE_INSTALL_DIR ;
if test "$installDir" == "" ; then
  installDir="`git rev-parse --show-toplevel`/install"  ;
fi

mkdir -p ${installDir}/bin ;

for i in `ls scripts/gino*` ; do
  fileName=`basename $i` ;
  cp $i ${installDir}/bin/ ;
  chmod 744 ${installDir}/bin/$fileName ;
done
