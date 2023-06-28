#!/bin/sh

if [ "$#" -ne 1 ]; then
      echo "Usage: ./RunNeutronMultiplicityFiltered.sh RUNLIST"
      echo "Specified input variable must contain the path to a file specifying all runs"
      exit 1
fi

RUNLIST=$1

while read -r run
do
	echo "$run"
	
	#Change into TA directory
	cd ../../

	#Filtered event files need to be in the directory of ToolAnalysis
	echo FilteredEvents_R${run}_NeutrinoCandidate > my_inputs.txt
        cp my_inputs.txt configfiles/NeutronMultiplicity

	#Adapt config file
	sed -i "6s#.*#Filename  NeutronMultiplicity_R${run}_Filtered#" configfiles/NeutronMultiplicity/NeutronMultiplicityConfig

	#Run analysis
	./Analyse configfiles/NeutronMultiplicity/ToolChainConfig

	#Change back into previous directory
	cd configfiles/NeutronMultiplicity/

done < $RUNLIST

