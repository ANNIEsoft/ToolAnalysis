#!/bin/bash

# rpms prerequisits needs root
#yum install make gcc-c++ gcc binutils libX11-devel libXpm-devel libXft-devel libXext-devel git bzip2-devel python-devel

rootflag=1
boostflag=1
b1=1
b3=1
b5=1

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
	    cp Makefile.FNAL Makefile
	    ;;

	--b1 )
            echo "Installing ToolDAQ for FNAL"
	    b3=0
	    b5=0
	    rootflag=0
	    boostflag=0
            ;;

	--b2 )
            echo "Installing ToolDAQ for FNAL"
            b1=0
	    b3=0
	    rootflag=0
	    b5=0
            ;;
	
	--b3 )
            echo "Installing ToolDAQ for FNAL"
            b1=0
	    b5=0
	    rootfalg=0
	    boostflag=0
            ;;
	
	--b4 )
            echo "Installing ToolDAQ for FNAL"
            b1=0
	    b3=0
            b5=0
            boostflag=0
            ;;
	
	--b3 )
            echo "Installing ToolDAQ for FNAL"
            b1=0
            b3=0
            rootfalg=0
            boostflag=0
            ;;

    esac
    shift
done

if [ $b1 -eq 1 ]
then
    
    mkdir ToolDAQ
fi

cd ToolDAQ

if [ $b1 -eq 1 ]
then
    git clone https://github.com/ToolDAQ/ToolDAQFramework.git
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
    
    cd boost_1_66_0
    
    mkdir install
    
    ./bootstrap.sh --prefix=`pwd`/install/  > /dev/null 2>/dev/null
    ./b2 install iostreams
    
    export LD_LIBRARY_PATH=`pwd`/install/lib:$LD_LIBRARY_PATH
    cd ../
fi

if [ $b3 -eq 1 ]
then
    
    cd ToolDAQFramework
    
    make clean
    make -j8
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
    cd ../
    
fi

if [ $rootflag -eq 1 ]
then
    
    wget https://root.cern.ch/download/root_v5.34.34.source.tar.gz
    tar zxvf root_v5.34.34.source.tar.gz
    cd root
    
    ./configure --enable-rpath
    make -j8
    
    source ./bin/thisroot.sh
    
    cd ../
    
fi

cd ../

if [ $b5 -eq 1 ]
then
    
    echo "current directory"
    echo `pwd`
    make clean
    make -j8
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
fi
