#!/bin/bash

# rpms prerequisits needs root
#yum install make gcc-c++ gcc binutils libX11-devel libXpm-devel libXft-devel libXext-devel git bzip2-devel python-devel

init=1
tooldaq=1
rootflag=1
boostflag=1
zmq=1
final=1

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


	--b1 )
            echo "Installing ToolDAQ part1"
	    final=0
	    rootflag=0
	    boostflag=0
            ;;

	--b2 )
            echo "Installing ToolDAQ part2"
            init=0
	    zmq=0
	    tooldaq=0
	    rootflag=0
	    final=0
            ;;
	
	--b3 )
            echo "Installing ToolDAQ part3"
            init=0
	    zmq=0
	    tooldaq=0
            final=0
            boostflag=0
            ;;
	
	--b4 )
            echo "Installing ToolDAQ part4"
            init=0
	    zmq=0
	    tooldaq=0
            rootflag=0
            boostflag=0
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
    cd root
    
    ./configure --enable-rpath
    make -j8
    
    source ./bin/thisroot.sh
    
    cd ../
    
fi

cd ../

if [ $final -eq 1 ]
then
    
    echo "current directory"
    echo `pwd`
    make clean
    make 
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
fi
