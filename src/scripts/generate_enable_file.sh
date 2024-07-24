#!/bin/bash

# Set the installation directory
installDir=$GINO_INSTALL_DIR ;
if test "$installDir" == "" ; then
  installDir="`git rev-parse --show-toplevel`/install"  ;
fi
installDir="`realpath ${installDir}/`" ;
mkdir -p $installDir ;

# Set the enable file
enableFile="../enable" ;
echo "#!/bin/bash" > ${enableFile} ;
echo "" >> ${enableFile} ;

# Enable NOELLE
noelleEnablePath="`noelle-config --prefix`/../";
echo "source `realpath $noelleEnablePath`/enable" >> ${enableFile};

# Enable GINO
echo "GINO_HOME=${installDir}" >> ${enableFile} ;
echo "export PATH=\${GINO_HOME}/bin:\$PATH" >> ${enableFile} ;
echo "export LD_LIBRARY_PATH=\${GINO_HOME}/lib:\$LD_LIBRARY_PATH" >> ${enableFile} ;
