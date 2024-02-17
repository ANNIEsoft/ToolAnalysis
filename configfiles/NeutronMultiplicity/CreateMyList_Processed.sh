if [ "$#" -ne 2 ]; then
      echo "Usage: ./CreateMyList.sh DIR RUN"
      echo "Specified input variable must contain the directory where the files are stored, second variable run number"
      exit 1
fi

FILEDIR=$1
RUN=$2

NUMFILES=$(ls -1q ${FILEDIR}/ProcessedRawData_TankAndMRDAndCTC_R${RUN}* | wc -l)

echo "NUMBER OF FILES IN ${FILEDIR}: ${NUMFILES}"
((NUMFILES-=1))

> my_inputs.txt

for p in $(seq 0 $NUMFILES)
do
	echo "${FILEDIR}/ProcessedRawData_TankAndMRDAndCTC_R${RUN}S0p${p}" >> my_inputs.txt
done
