if [ "$#" -ne 1 ]; then
      echo "Usage: ./CreateProcessedList.sh RUN"
      echo "Specified input variable must contain the run number"
      exit 1
fi

RUN=$1
DIR=/pnfs/annie/persistent/processed/processed_EBV2/

NUMFILES=$(ls -1q ${DIR}/R${RUN}/ProcessedData_PMTMRDLAPPD_R${RUN}* | wc -l)

echo "NUMBER OF FILES IN ${DIR}${RUN}: ${NUMFILES}"

rm my_inputs.txt

for p in $(seq 0 $(($NUMFILES -1 )))
do
	echo "${DIR}/R${RUN}/ProcessedData_PMTMRDLAPPD_R${RUN}S0p${p}" >> my_inputs.txt
done
