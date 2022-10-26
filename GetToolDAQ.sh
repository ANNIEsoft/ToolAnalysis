#!/bin/bash

# rpms prerequisits needs root
#yum install make gcc-c++ gcc binutils libX11-devel libXpm-devel libXft-devel libXext-devel git bzip2-devel python-devel

BASEDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

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
PlotWaveforms=0
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
	    MrdTrackLib=0
	    WCSimlib=1
	    Python=0
	    Python3=0
	    Pythia=0
	    Genie=0
	    RATEventlib=0
	    ;;
	    
	--MrdTrackLib )
	    echo "Installing MRDTrackLib"
	    init=0
	    tooldaq=0
	    rootflag=0
	    root6flag=0
	    boostflag=0
	    zmq=0
	    final=0
	    MrdTrackLib=1
	    WCSimlib=0
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
	
        --PlotWaveforms )
            echo "Building PlotWaveformslib"
            init=0
            tooldaq=0
            rootflag=0
            root6flag=0
            boostflag=0
            zmq=0
            final=0
            MrdTrackLib=1
            WCSimlib=0
            Python=0
            Python3=0
            Pythia=0
            Genie=0
            RATEventlib=0
            PlotWaveforms=1
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
    cd ${BASEDIR}
    mkdir ToolDAQ
fi

if [ $tooldaq -eq 1 ]
then
    cd ${BASEDIR}/ToolDAQ
    git clone https://github.com/ToolDAQ/ToolDAQFramework.git
    # ANNIE currently uses an old version
    cd ToolDAQFramework
    git checkout a06f13d09845c4f0fb679946f3c385dae406e2fe
fi

if [ $zmq -eq 1 ]
then
    
    cd ${BASEDIR}/ToolDAQ
    git clone https://github.com/ToolDAQ/zeromq-4.0.7.git
    
    cd zeromq-4.0.7
    
    ./configure --prefix=`pwd`
    make -j8
    make install
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
      
fi

if [ $boostflag -eq 1 ]
then
    
    cd ${BASEDIR}/ToolDAQ
    wget http://downloads.sourceforge.net/project/boost/boost/1.66.0/boost_1_66_0.tar.gz
    tar zxf boost_1_66_0.tar.gz
    rm -rf boost_1_66_0.tar.gz
    
    cd boost_1_66_0
    
    mkdir install
    
    ./bootstrap.sh --prefix=`pwd`/install/  > /dev/null 2>/dev/null
    ./b2 install iostreams
    
    export LD_LIBRARY_PATH=`pwd`/install/lib:$LD_LIBRARY_PATH
    
fi

if [ $root6flag -eq 1 ]
then
    
    cd ${BASEDIR}
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    
    cd ${BASEDIR}/ToolDAQ
    wget https://root.cern/download/root_v6.24.06.source.tar.gz
    tar -xzf root_v6.24.06.source.tar.gz
    cd root-6.24.06
    mkdir install
    cd install
    cmake ../ -DCMAKE_CXX_STANDARD=14 -Dgdml=ON -Dxml=ON -Dmt=ON -Dmathmore=ON -Dx11=ON -Dimt=ON -Dtmva=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -Dpythia6=ON -Dfftw3=ON
    make -j8
    make install
    source bin/thisroot.sh

fi

if [ $WCSimlib -eq 1 ]
then

    cd ${BASEDIR}
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    
    cd ${BASEDIR}/ToolDAQ
    git clone https://github.com/ANNIEsoft/WCSimLib.git
    cd WCSimLib
    make -f Makefile_ROOT6
    
fi


if [ $MrdTrackLib -eq 1 ]
then
    
    cd ${BASEDIR}
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    
    cd ${BASEDIR}/ToolDAQ
    git clone https://github.com/ANNIEsoft/MrdTrackLib.git
    cd MrdTrackLib
    cp Makefile.FNAL Makefile
    make
    
fi

if [ $Python3 -eq 1 ]
then
    
    cd ${BASEDIR}
    
    source Setup.sh
    pip3 install numpy==1.23.4
    pip3 install pandas==1.5.0
    pip3 install matplotlib==3.6.1
    pip3 install scipy==1.9.2
    pip3 install scikit-learn==1.1.2
    pip3 install seaborn==0.12.0
    pip3 install uproot==4.3.7
    pip3 install xgboost==1.6.2
    pip3 install tensorflow==2.10.0
    # set tensorflow verbosity to suppress info messages about not having a GPU or maximal acceleration
    # https://stackoverflow.com/questions/35911252/disable-tensorflow-debugging-information/42121886#42121886
    echo "export TF_CPP_MIN_LOG_LEVEL=2" >> ${BASEDIR}/Setup.sh
    
    cd ${BASEDIR}/UserTools
    mkdir -p InactiveTools
    mkdir -p ImportedTools
    cd ImportedTools
    git clone https://github.com/ToolFramework/ToolPack.git
    cd ToolPack
    ./Import.sh PythonScript
    
fi

if [ $Pythia -eq 1 ]
then
    
    cd ${BASEDIR}
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    
    cd ${BASEDIR}/ToolDAQ
    mkdir fsplit && cd fsplit
    wget https://gist.githubusercontent.com/marc1uk/c0e32d955dd1c06ef69d80ce643018ad/raw/10e592d42737ecc7dca677e774ae66dcb5a3859d/fsplit.c
    gcc fsplit.c -o fsplit
    export PATH=$PWD:$PATH
    echo "export PATH=$PWD:\$PATH" >> ${BASEDIR}/Setup.sh
    
    cd ${BASEDIR}/ToolDAQ
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
    
fi

if [ $Genie -eq 1 ]
then
    
    cd ${BASEDIR}
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    
    cd ${BASEDIR}/ToolDAQ
    wget https://github.com/GENIE-MC/Generator/archive/R-3_00_04.tar.gz
    tar zxf R-3_00_04.tar.gz
    rm -rf R-3_00_04.tar.gz
    cd Generator-R-3_00_04/
    mkdir install
    export GENIE=`pwd`
    ./configure --prefix=/ToolAnalysis/ToolDAQ/Generator-R-3_00_04/install/ --disable-lhapdf5 --with-pythia6-inc=/ToolAnalysis/ToolDAQ/Pythia6Support/v6_424/inc/ --with-pythia6-lib=/ToolAnalysis/ToolDAQ/Pythia6Support/v6_424/lib/ --with-log4cpp-inc=/ToolAnalysis/ToolDAQ/log4cpp/include/ --with-log4cpp-lib=/ToolAnalysis/ToolDAQ/log4cpp/lib/
    make -j8
    
fi


if [ $RATEventlib -eq 1 ]
then
    
    cd ${BASEDIR}
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    
    cd ${BASEDIR}/ToolDAQ
    git clone https://github.com/ANNIEsoft/RATEventLib.git
    cd RATEventLib/
    make
    
fi

if [ $PlotWaveforms -eq 1 ]
then

    cd ${BASEDIR}
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi

    cd ${BASEDIR}/UserTools/PlotWaveforms
    make

fi

if [ $final -eq 1 ]
then
    
    cd ${BASEDIR}
    if [ $fnalflag -eq 1 ]; then
      source SetupFNAL.sh
    else
      source Setup.sh
    fi
    
    make clean
    make 
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
    
fi
