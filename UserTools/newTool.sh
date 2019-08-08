#!/bin/bash

if [ $1 != "" ]
then
    mkdir $1
    more template/MyTool.h | sed s:MyTool:$1: | sed s:MYTOOL_H:$1_H: > ./$1/$1.h
    more template/MyTool.cpp | sed s:MyTool:$1: | sed s:MyTool\(\):$1\(\): > ./$1/$1.cpp
    more template/README.md | sed s:MyTool:$1: | sed s:MyTool\(\):$1\(\): > ./$1/README.md
    echo "#include \"$1.h\"" >>Unity.h

    while read line
    do
	if [ "$line" == "return ret;" ]
	then
	    echo "if (tool==\""$1"\") ret=new "$1";" >>Factory/Factory.cpp.tmp
	fi
	echo $line >>Factory/Factory.cpp.tmp
    done < Factory/Factory.cpp
    mv Factory/Factory.cpp.tmp Factory/Factory.cpp
else

echo "Error no name given"

fi
