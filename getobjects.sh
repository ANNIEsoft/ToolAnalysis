#!/bin/bash
if [ $# -ne 0 ]; then
	#echo "getting objects needed for tools $*" >&2
	declare -a OBJS;
	#echo "looping over $# tools" >&2
	for TOOL in "$@"; do
		#echo "searching for objects for tool ${TOOL}" >&2
		OBJ=$(ls -1 UserTools/${TOOL}/*.cpp 2>/dev/null | sed 's/.cpp$/.o/')
		if [ ! -z "${OBJ}" ]; then
			OBJS+=( "${OBJ}" )
		fi
		#echo "adding obj ${OBJ}"  >&2
		
		# hack for examples as they don't follow normal folder structure
		if ! ls UserTools/${TOOL}/${TOOL}.cpp &> /dev/null && ls UserTools/Examples/${TOOL}.cpp &> /dev/null; then
			OBJS+=( "UserTools/Examples/${TOOL}.o" )
		fi
	done
	#OBJS+=( "UserTools/MyFactory/MyFactory.o" )
	echo "${OBJS[@]}"
fi
