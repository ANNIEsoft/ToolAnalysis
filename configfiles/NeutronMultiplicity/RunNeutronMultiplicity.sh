#!/bin/sh

if [ "$#" -ne 1 ]; then
      echo "Usage: ./RunNeutronMultiplicity.sh RUNLIST"
      echo "Specified input variable must contain the path to a file specifying all runs"
      exit 1
fi

RUNLIST=$1
cut=CBStrict

while read -r run
do
	echo "$run"
        ./CreateMyList_Processed.sh /pnfs/annie/persistent/processed/processed_hits/R${run} ${run}

	#Change into the ToolAnalysis directory first
	cd ../../

	#Run analysis from here
	sed -i "6s#.*#Filename  NeutronMultiplicity_R${run}_${cut}#" configfiles/NeutronMultiplicity/NeutronMultiplicityConfig
	sed -i "4s#.*#Method ${cut}#" configfiles/NeutronMultiplicity/FindNeutronsConfig
	./Analyse ./configfiles/NeutronMultiplicity/ToolChainConfig

	#Change back
	cd configfiles/NeutronMultiplicity/
done < $RUNLIST

