setup_genie_2_8_6(){
  setup genie v2_8_6d -q e9:debug
  setup genie_xsec v2_8_6 -q default
  setup genie_phyopt v2_8_6 -q dkcharmtau
  setup -f Linux64bit+2.6-2.12 -q debug:e10 xerces_c v3_1_3
  export XERCESROOT=/grid/fermiapp/products/larsoft/xerces_c/v3_1_3/Linux64bit+2.6-2.12-e10-debug
  export ROOT_INCLUDE_PATH=${ROOT_INCLUDE_PATH}:${GENIE}/../include/GENIE
  export ROOT_LIBRARY_PATH=${ROOT_LIBRARY_PATH}:${GENIE}/../lib
}

setup_genie_2_12(){
  if [ -z "$GVERS"    ]; then export GVERS="v2_12_0"             ; fi
  if [ -z "$GQAUL"    ]; then export GQUAL="e10:debug:r6"         ; fi
  if [ -z "$XSECQUAL" ]; then export XSECQUAL="DefaultPlusMECWithNC" ; fi
  setup genie        ${GVERS}a -q ${GQUAL}
  setup genie_phyopt ${GVERS} -q dkcharmtau
  # do phyopt before xsec in case xsec has its own UserPhysicsOptions.xml
  setup genie_xsec   ${GVERS} -q ${XSECQUAL}
  if [ $? -ne 0 ]; then
    # echo "$b0: looking for genie_xec ${GVERS}a -q ${XSECQUAL}"
    # might have a letter beyond GENIE code's
    setup genie_xsec   ${GVERS}a -q ${XSECQUAL}
  fi
  
  setup -f Linux64bit+2.6-2.12 -q debug:e10 xerces_c v3_1_3  # xerces needed for gdml parsing? (needed for geant4 gdml...)
  export XERCESROOT=/grid/fermiapp/products/larsoft/xerces_c/v3_1_3/Linux64bit+2.6-2.12-e10-debug
  export ROOT_INCLUDE_PATH=${ROOT_INCLUDE_PATH}:${GENIE}/../include/GENIE
  export ROOT_LIBRARY_PATH=${ROOT_LIBRARY_PATH}:${GENIE}/../lib
  
  #others from setup_setup used for genie file generation... needed?
  #setup pandora v01_01_00a -q debug:e7:nu
  #setup dk2nu v01_01_03a -q debug:e7    ## or version: setup dk2nu v01_03_00c -q debug:e9:r5  ?
  #setup dk2nu v01_03_00c -q debug:e9:r5  ## disable! sets genie version 2.10 up!
  #setup cstxsd v4_0_0b -q e7
  #setup boost v1_57_0 -q debug:e7
}

#source /grid/fermiapp/products/common/etc/setup
export PRODUCTS=${PRODUCTS}:/grid/fermiapp/products/larsoft
setup_genie_2_12
