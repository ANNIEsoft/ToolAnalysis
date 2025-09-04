#!/bin/bash
if [ $# -eq 0 ] || [ ! -f "configfiles/$1/ToolsConfig" ]; then
	DIRS=( $(find -L UserTools -maxdepth 1 -mindepth 1 -type d -exec basename {} \; | grep -v -e Factory -e template -e InactiveTools) )
	declare -a TOOLS;
	# filter out python tools as they do not need to go into Unity/Factory
	for DIR in "${DIRS[@]}"; do
		#echo "checking ${DIR}"
		if [ -f "UserTools/${DIR}/${DIR}.cpp" ]; then
			TOOLS+=( "${DIR}" )
		fi
	done
	
	# hack to handle examples which do not follow standard folder structure... should we just move them?
	for FILE in `ls UserTools/Examples/Example*.h`; do
		cp ${FILE} include/
		TOOLNAME=$(basename ${FILE%%.h})
		TOOLS+=( "${TOOLNAME}" )
	done
	
	echo "${TOOLS[@]}"
	#ls UserTools/*/*.cpp | sed 's/.cpp$//' | xargs -n1 basename | grep -v -e Factory -e template -e InactiveTools
else
	#echo "getting tools needed for toolchain $1" >&2
        # parse into bash array
	declare -a TOOLS;
	while read -r line; do
		if [ -z "${line}" ]; then continue; fi
		NC=$(echo ${line:0:1})
		if [ "${NC}" == "#" ]; then continue; fi
		TOOL=$(echo "${line}" | cut -d' ' -f 2)
		TOOLS+=( "${TOOL}" )
		#echo "adding tool ${TOOL}"  >&2
	done < <(cat "./configfiles/$1/ToolsConfig")
	echo "${TOOLS[@]}"
fi
