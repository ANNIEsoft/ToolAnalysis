if [ "$#" -ne 1 ]; then
      echo "Usage: ./CreateMyList.sh RUN"
      echo "Specified input variable must contain the run number"
      exit 1
fi

RUN=$1
DIR=/pnfs/annie/persistent/raw/raw/

NUMFILES=$(ls -1q ${DIR}${RUN}/RAWDataR${RUN}* | wc -l)

echo "NUMBER OF FILES IN ${DIR}${RUN}: ${NUMFILES}"

rm my_files.txt

for p in $(seq 0 $(($NUMFILES -1 )))
do
	echo "${DIR}${RUN}/RAWDataR${RUN}S0p${p}" >> my_files.txt
done
