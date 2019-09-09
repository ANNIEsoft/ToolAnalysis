#!/bin/bash

# rpms prerequisits needs root
#yum install make gcc-c++ gcc binutils libX11-devel libXpm-devel libXft-devel libXext-devel git bzip2-devel python-devel

init=1
tooldaq=1
rootflag=0
root6flag=1
boostflag=1
zmq=1
final=1
MrdTrackLib=1
WCSimlib=1
Python=0
Python3=1
Pythia=1
Genie=1
RATEventlib=1
fnalflag=0

while [ ! $# -eq 0 ]
do
    case "$1" in
	--help | -h)
	    helpmenu
	    exit
	    ;;
	
	--no_root | -r)
	    echo "Installing ToolDAQ without root"
	    rootflag=0 
	    ;;
	
	--no_boost | -b)
            echo "Installing ToolDAQ without boost"
            boostflag=0
	    ;;
	
	--FNAL | -f)
            echo "Installing ToolDAQ for FNAL"
            boostflag=0
	    rootflag=0
	    fnalflag=1
            Python=0
	    cp Makefile.FNAL Makefile
	    ;;

	--no_init )
	     echo "Installing ToolDAQ without creating ToolDAQ Folder"
	    init=0;
	    ;;

	--no_zmq )
            echo "Installing ToolDAQ without zmq"
            zmq=0
            ;;

	--no_tooldaq )
	    echo "Installing dependancies without ToolDAQ"
	    tooldaq=0
	    ;;

	--no_final )
            echo "Installing ToolDAQ without compiling ToolAnalysis"
            final=0
            ;;

	--ToolDAQ_ZMQ )
            echo "Installing ToolDAQ & ZMQ"
	    rootflag=0
	    boostflag=0
	    final=0
	    MrdTrackLib=0
	    WCSimlib=0
	    root6flag=0   
	    Python=0
	    Python3=0
	    Pythia=0
	    Genie=0
	    RATEventlib=0
            ;;

	--Boost )
            echo "Installing Boost"
	    init=0
	    tooldaq=0
	    rootflag=0
	    zmq=0
	    final=0
	    MrdTrackLib=0
	    WCSimlib=0
	    root6flag=0
	    Python=0
	    Python3=0
	    Pythia=0
	    Genie=0
	    RATEventlib=0
            ;;
	
	--Root )
            echo "Installing ToolDAQ"
	    init=0
	    tooldaq=0
	    boostflag=0
	    zmq=0
	    final=0
	    root6flag=0
	    MrdTrackLib=0
	    WCSimlib=0
	    Python=0
	    Python3=0
	    Pythia=0
	    Genie=0
	    RATEventlib=0
            ;;

        --Root6 )
            echo "Installing ToolDAQ"
            init=0
            tooldaq=0
            boostflag=0
            zmq=0
            final=0
            MrdTrackLib=0
            WCSimlib=0
	    rootflag=0
	    root6flag=1
	    Python=0
	    Python3=0
	    Pythia=0
	    Genie=0
	    RATEventlib=0
            ;;
	
	--WCSim )
            echo "Installing WCSim libs"
	    init=0
	    tooldaq=0
	    rootflag=0
	    root6flag=0
	    boostflag=0
	    zmq=0
	    final=0
	    Python=0
	    Python3=0
	    Pythia=0
	    Genie=0
	    RATEventlib=0
	    ;;

	--Python )
            echo "Compiling ToolDAQ"
            init=0
            tooldaq=0
            rootflag=0
	    root6flag=0
            boostflag=0
            zmq=0
            MrdTrackLib=0
            WCSimlib=0
            final=0
	    Python=1
	    Python3=0
	    Pythia=0
	    Genie=0
	    RATEventlib=0
            ;;

	--Python3 )
            echo "Compiling ToolDAQ"
            init=0
            tooldaq=0
            rootflag=0
	    root6flag=0
            boostflag=0
            zmq=0
            MrdTrackLib=0
            WCSimlib=0
            final=0
	    Python=0
	    Python3=1
	    Pythia=0
	    Genie=0
	    RATEventlib=0
            ;;

	--Pythia )
            echo "Compiling ToolDAQ"
            init=0
            tooldaq=0
            rootflag=0
	    root6flag=0
            boostflag=0
            zmq=0
            MrdTrackLib=0
            WCSimlib=0
            final=0
            Python=0
	    Python3=0
	    Pythia=1
	    Genie=0
	    RATEventlib=0
            ;;

	--Genie )
            echo "Compiling ToolDAQ"
            init=0
            tooldaq=0
            rootflag=0
	    root6flag=0
            boostflag=0
            zmq=0
            MrdTrackLib=0
            WCSimlib=0
            final=0
	    Python=0
	    Python3=0
	    Pythia=0
	    Genie=1
	    RATEventlib=0
            ;;

	--RATEventlib )
            echo "Compiling ToolDAQ"
            init=0
            tooldaq=0
            rootflag=0
	    root6flag=0
            boostflag=0
            zmq=0
            MrdTrackLib=0
            WCSimlib=0
            final=0
	    Python=0
	    Python3=0
	    Pythia=0
	    Genie=0
	    RATEventlib=1
            ;;



	--Final )
            echo "Compiling ToolDAQ"
	    init=0
	    tooldaq=0
	    rootflag=0
	    root6flag=0
	    boostflag=0
	    zmq=0
	    MrdTrackLib=0
	    WCSimlib=0
	    Python=0
	    Python=0
	    Python3=0
	    Pythia=0
	    Genie=0
	    RATEventlib=0
            
            ;;

    esac
    shift
done

if [ $init -eq 1 ]
then
    
    mkdir ToolDAQ
fi

cd ToolDAQ

if [ $tooldaq -eq 1 ]
then

git clone https://github.com/ToolDAQ/ToolDAQFramework.git
fi

if [ $zmq -eq 1 ]
then
    git clone https://github.com/ToolDAQ/zeromq-4.0.7.git
    
    cd zeromq-4.0.7
    
    ./configure --prefix=`pwd`
    make -j8
    make install
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
    
    cd ../
fi

if [ $boostflag -eq 1 ]
then
    
    
    wget http://downloads.sourceforge.net/project/boost/boost/1.66.0/boost_1_66_0.tar.gz
    tar zxf boost_1_66_0.tar.gz
    rm -rf boost_1_66_0.tar.gz
    
    cd boost_1_66_0
    
    mkdir install
    
    ./bootstrap.sh --prefix=`pwd`/install/  > /dev/null 2>/dev/null
    ./b2 install iostreams
    
    export LD_LIBRARY_PATH=`pwd`/install/lib:$LD_LIBRARY_PATH
    cd ../
fi

if [ $rootflag -eq 1 ]
then
    
    wget https://root.cern.ch/download/root_v5.34.34.source.tar.gz
    tar zxvf root_v5.34.34.source.tar.gz
    rm -rf root_v5.34.34.source.tar.gz
    cd root
    
    ./configure --enable-rpath
    make -j8
    
    source ./bin/thisroot.sh
    
    cd ../
    
fi

if [ $root6flag -eq 1 ]
then

 cd ../
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    cd -

  #  wget https://root.cern.ch/download/root_v6.14.04.Linux-centos7-x86_64-gcc4.8.tar.gz
   # tar zxvf root_v6.14.04.Linux-centos7-x86_64-gcc4.8.tar.gz
    #rm -rf root_v6.14.04.Linux-centos7-x86_64-gcc4.8.tar.gz
   # cd root

    #./configure --enable-rpath
    #make -j8

    #source ./bin/thisroot.sh

    wget https://root.cern.ch/download/root_v6.06.08.source.tar.gz
    tar zxf root_v6.06.08.source.tar.gz
    rm -rf root_v6.06.08.source.tar.gz 
    cd root-6.06.08
    mkdir install 
    cd install
    cmake ../ -Dcxx14=OFF -Dcxx11=ON -Dgdml=ON -Dxml=ON -Dmt=ON -Dkrb5=ON -Dmathmore=ON -Dx11=ON -Dimt=ON -Dtmva=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -Dpythia6=ON
    make -j8
    make install
    source bin/thisroot.sh


    cd ../../

fi



if [ $WCSimlib -eq 1 ]
then

    cd ../
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    cd -
    git clone https://github.com/ANNIEsoft/WCSimLib.git
    cd WCSimLib
    #if [ $fnalflag -eq 1 ]; then
    #  make -f Makefile.FNAL
    #else
      make -f Makefile_ROOT6
     # make  # run twice, some sort of circular dependancy, works with ROOT 5.34
    

    cd ../

fi


if [ $MrdTrackLib -eq 1 ]
then
    
    cd ../
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    cd -
    git clone https://github.com/ANNIEsoft/MrdTrackLib.git
    cd MrdTrackLib
    #if [ $fnalflag -eq 1 ]; then
      cp Makefile.FNAL Makefile
    #fi
    make
    cd ../

fi

#cd ../


if [ $Python -eq 1 ]
then
    if [ $fnalflag -eq 0 ]; then
      source Setup.sh
      pip install numpy pandas tensorflow sklearn root_numpy
    fi

fi

if [ $Python3 -eq 1 ]
then
    cd ../
    if [ $fnalflag -eq 0 ]; then
	source Setup.sh
	pip3.6 install numpy pandas tensorflow matplotlib sklearn root_numpy
    fi
    cd -
    
fi

if [ $Pythia -eq 1 ]
then
    
    cd ../
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    cd -
    cvs -d :pserver:anonymous@log4cpp.cvs.sourceforge.net:/cvsroot/log4cpp -z3 co log4cpp
    cd log4cpp/
    ./autogen.sh
    ./configure --prefix=`pwd`
    make
    make install
    cd ../
    git clone https://github.com/GENIE-MC-Community/Pythia6Support.git
    cd Pythia6Support
    ./build_pythia6.sh 
    export PYTHIA6_DIR=/ToolAnalysis/ToolDAQ/Pythia6Support/v6_424/
    cd ..

fi

if [ $Genie -eq 1 ]
then
    
    cd ../
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    cd -
    wget https://github.com/GENIE-MC/Generator/archive/R-3_00_04.tar.gz
    tar zxf R-3_00_04.tar.gz
    rm -rf R-3_00_04.tar.gz
    cd Generator-R-3_00_04/
    mkdir install
    export GENIE=`pwd`
    ./configure --prefix=/ToolAnalysis/ToolDAQ/Generator-R-3_00_04/install/ --disable-lhapdf5 --with-pythia6-inc=/ToolAnalysis/ToolDAQ/Pythia6Support/v6_424/inc/ --with-pythia6-lib=/ToolAnalysis/ToolDAQ/Pythia6Support/v6_424/lib/ --with-log4cpp-inc=/ToolAnalysis/ToolDAQ/log4cpp/include/ --with-log4cpp-lib=/ToolAnalysis/ToolDAQ/log4cpp/lib/
    make -j8
    
    cd ../
    
fi


if [ $RATEventlib -eq 1 ]
then
    
    cd ../
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    cd -
    git clone https://github.com/ANNIEsoft/RATEventLib.git
    cd RATEventLib/
    make
    cd ../

fi




if [ $final -eq 1 ]
then
    
    cd ..
    echo "current directory"
    echo `pwd`
    make clean
    make 
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
fi
