#!/bin/bash

# rpms prerequisits needs root
#yum install make gcc-c++ gcc binutils libX11-devel libXpm-devel libXft-devel libXext-devel git bzip2-devel python-devel

rootflag=1
boostflag=1
zeromqflag=1
upsflag=0
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

	--ups | -u)
            echo "Installing ToolDAQ for FNAL UPS"
            if [ -z "$BOOST_DIR" ]; then
                echo "The boost product has not been set up."
                exit 1
            elif [ -z "$ROOT_DIR" ]; then
                echo "The root product has not been set up."
                exit 2
            elif [ -z "$ZEROMQ_DIR" ]; then
                echo "The zeromq product has not been set up."
                exit 3
            elif [ -z "$PYTHON_DIR" ]; then
                echo "The python product has not been set up."
                exit 4
            fi
            boostflag=0
	    rootflag=0
	    zeromqflag=0
            upsflag=1
	    cp Makefile.UPS Makefile
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
    
    mkdir -p ToolDAQ
fi

cd ToolDAQ

if [ $b1 -eq 1 ]
then
    git clone https://github.com/ToolDAQ/ToolDAQFramework.git

    if [ $zeromqflag -eq 1 ]
    then
        git clone https://github.com/ToolDAQ/zeromq-4.0.7.git
    
        cd zeromq-4.0.7

        ./configure --prefix=`pwd`
        make
        make install
    
        export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
    
        cd ../
    fi
fi

if [ $boostflag -eq 1 ]
then
    
    
    wget http://downloads.sourceforge.net/project/boost/boost/1.60.0/boost_1_60_0.tar.gz
    tar zxf boost_1_60_0.tar.gz
    
    cd boost_1_60_0
    
    mkdir install
    
    ./bootstrap.sh --prefix=`pwd`/install/  > /dev/null 2>/dev/null
    ./b2 install iostreams
    
    export LD_LIBRARY_PATH=`pwd`/install/lib:$LD_LIBRARY_PATH
    cd ../
fi

if [ $b3 -eq 1 ]
then
    
    cd ToolDAQFramework

    if [ $upsflag -eq 1 ]
    then
        patch Makefile ../../FrameworkMakefileUPS.patch
    fi
    
    make clean
    make
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
    cd ../
    
fi

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

cd ../

if [ $b5 -eq 1 ]
then
    
    echo "current directory"
    echo `pwd`
    make clean
    make
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
fi
