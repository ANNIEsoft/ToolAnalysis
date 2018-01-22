#!/bin/bash

# rpms prerequisits needs root
#yum install make gcc-c++ gcc binutils libX11-devel libXpm-devel libXft-devel libXext-devel git bzip2-devel python-devel

rootflag=1
boostflag=1

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
	
	esac
    shift
done


mkdir ToolDAQ
cd ToolDAQ

git clone https://github.com/ToolDAQ/ToolDAQFramework.git
git clone https://github.com/ToolDAQ/zeromq-4.0.7.git

cd zeromq-4.0.7

./configure --prefix=`pwd`
make
make install

export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH

if [ $boostflag -eq 1 ]
then

    cd ../
    wget http://downloads.sourceforge.net/project/boost/boost/1.60.0/boost_1_60_0.tar.gz
    tar zxf boost_1_60_0.tar.gz
    
    cd /boost_1_60_0
    
    mkdir install
    
    ./bootstrap.sh --prefix=`pwd`/install/  > /dev/null 2>/dev/null
    ./b2 install iostreams
fi

export LD_LIBRARY_PATH=`pwd`/install/lib:$LD_LIBRARY_PATH

cd ../ToolDAQFramework

make clean
make

cd ../

export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH

if [ $rootflag -eq 1 ]
then
    
    wget https://root.cern.ch/download/root_v5.34.34.source.tar.gz
    tar zxvf root_v5.34.34.source.tar.gz
    cd root
    
    ./configure --prefix=`pwd` --enable-rpath
    make
    make install
    
    source ./bin/thisroot.sh

    cd ../
fi

echo "current directory"
echo `pwd`
make clean
make

export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
