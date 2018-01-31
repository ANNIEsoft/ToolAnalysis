### Created by Dr. Benjamin Richards (b.richards@qmul.ac.uk)

### Download base image from cern repo on docker hub
FROM cern/cc7-base:latest

### Run the following commands as super user (root):
USER root

#######################
### GCC ENVIRONMENT ###
#######################

### Required package for ROOT
RUN yum install -y \
    wget \
    tar \
    cmake \
    gcc-c++ \
    gcc \
    binutils \
    libX11-devel \
    libXpm-devel \
    libXft-devel \
    libXext-devel \
    make \
    file

### Git and Other Requirments
RUN yum install -y \
    git \
    bzip2-devel \
    python-devel
    
Run git clone https://github.com/ANNIEsoft/ToolAnalysis.git ;     cd ToolAnalysis ;     ./GetToolDAQ.sh ;

Run cd ToolAnalysis; make update; make; 

### Open terminal
CMD ["/bin/bash"]