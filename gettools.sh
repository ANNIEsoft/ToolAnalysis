#!/bin/bash
TOOLCHAIN="${1##./configfiles/}"
#echo "get tools for toolchain '${TOOLCHAIN}'"
if [ $# -eq 0 ] || [ ! -f "configfiles/$TOOLCHAIN/ToolsConfig" ]; then
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
	
	#echo "${TOOLS[@]}"
	UNIQUE_TOOLS=( $(for ATOOL in "${TOOLS[@]}"; do echo "${ATOOL}"; done | sort -u) )
	echo "${UNIQUE_TOOLS[@]}"
	#ls UserTools/*/*.cpp | sed 's/.cpp$//' | xargs -n1 basename | grep -v -e Factory -e template -e InactiveTools
else
	#echo "getting tools needed for toolchain $TOOLCHAIN" >&2
        # parse into bash array
	declare -a TOOLS;
	while read -r line; do
		if [ -z "${line}" ]; then continue; fi
		NC=$(echo ${line:0:1})
		if [ "${NC}" == "#" ]; then continue; fi
		TOOL=$(echo "${line}" | cut -d' ' -f 2)
		TOOLS+=( "${TOOL}" )
		#echo "adding tool ${TOOL}"  >&2
	done < <(cat "./configfiles/$TOOLCHAIN/ToolsConfig")
	#echo "${TOOLS[@]}"
	UNIQUE_TOOLS=( $(for ATOOL in "${TOOLS[@]}"; do echo "${ATOOL}"; done | sort -u) )
	echo "${UNIQUE_TOOLS[@]}"
fi
