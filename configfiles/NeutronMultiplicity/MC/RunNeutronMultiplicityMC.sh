#!/bin/sh

filelist="./pmt-files-gst.txt"
filelist_genie="./genie-files-gst.txt"
filelist_offset="./offset-gst.txt"

let i=0
while read -r file && read -r filegenie <&3 && read -r offset <&6
do
	echo "$file"
	echo "$filegenie"
	echo "$offset"
	#Change into ToolAnalysis directory
	cd ../../../

	#Change config files
	sed -i "7s#.*#InputFile /pnfs/annie/persistent/users/mnieslon/wcsim/output/tankonly/wcsim_ANNIEp2v7_beam/pmt-files/${file}#" configfiles/NeutronMultiplicity/MC/LoadWCSimConfig
	#sed -i "7s#.*#InputFile ${filelappd}#" configfiles/NeutronMultiplicity/MC/LoadWCSimLAPPDConfig
	sed -i "6s#.*#Filename NeutronMultiplicity_MCBeam_${i}#" configfiles/NeutronMultiplicity/MC/NeutronMultiplicityConfig
	sed -i "7s#.*#FilePattern ${filegenie}#" configfiles/NeutronMultiplicity/MC/LoadGenieEventConfig
	sed -i "11s#.*#EventOffset ${offset}#" configfiles/NeutronMultiplicity/MC/LoadGenieEventConfig

	#Run Analysis
	./Analyse ./configfiles/NeutronMultiplicity/MC/ToolChainConfig

	#Change back into original directory
	cd configfiles/NeutronMultiplicity/MC

	#Increment
        i=$((i+1))

done < $filelist 3<$filelist_genie 6<$filelist_offset

