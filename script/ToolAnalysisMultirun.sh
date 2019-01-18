#This script can be used to run multiple iterations of the same ToolAnalysis
#configuration on multiple input datafiles.  To use this script, make the
#following changes:
#  - Select your Config of choice by defining $CONFIG
#  - Define the directory where all of your wcsim data files are (DATADIR)
#  - Define the basenames of the PMT/LAPPD files, as well as the desired
#    basename of the root tree output files made by PhaseIITreeMaker
#  - Go into your config directory, and put the placeholders as follows:
#    In LoadWCSimConfig: InputFile #INPUT_FILE_PMT#
#    In LoadWCSimLAPPDConfig: InputFile #INPUT_FILE_LAPPD#
#    In PhaseIITreeMaker: OutputFile #OUTPUT_FILE_TREE#
#    sed is used to replace these with the basenames you define below. 


#Choose your 
CONFIG=PhaseIIReco
DATADIR=/AnnieShare/p2r_data/

LAPPD_NAMEBASE=wcsim_lappd_0
PMT_NAMEBASE=wcsim_0
OUTPUT_NAMEBASE=RecoGridSeed_5LAPPD_0

#These will stay fixed if you're using this on the docker
HOMEDIR=/ToolAnalysis/
CONFIGDIR=/ToolAnalysis/configfiles/${CONFIG}/
cd ${HOMEDIR}
source ${HOMEDIR}Setup.sh

declare -a firstind=(1 2)
declare -a secondind=(0 1 2 3 4 5 6 7 8 9)
NUMFIRST=$((${#firstind[@]}-1))
NUMSEC=$((${#secondind[@]}-1))
for l in $(seq 0 $NUMFIRST)
    do
    for k in $(seq 0 $NUMSEC)
        do
        if (($k == 0)); then
            if (($l == 0)); then
                 echo "THE FIRST"
                 echo $l
                 echo $k
                CURRENT_NAME_PMT=${PMT_NAMEBASE}.${firstind[l]}.${secondind[k]}.root
                CURRENT_NAME_LAPPD=${LAPPD_NAMEBASE}.${firstind[l]}.${secondind[k]}.root
                CURRENT_OUTPUT=${OUTPUT_NAMEBASE}.${firstind[l]}.${secondind[k]}.root
                echo $CURRENT_NAME_PMT
                cd ${CONFIGDIR}
                sed -i "s|#INPUT_FILE_PMT#|${DATADIR}${CURRENT_NAME_PMT}|g" ${CONFIGDIR}*Config
                sed -i "s|#INPUT_FILE_LAPPD#|${DATADIR}${CURRENT_NAME_LAPPD}|g" ${CONFIGDIR}*Config
                sed -i "s|#OUTPUT_FILE_TREE#|${HOMEDIR}${CURRENT_OUTPUT}|g" ${CONFIGDIR}*Config
                cd ${HOMEDIR}
                ./Analyse $CONFIG
                continue
             fi
        fi
        if (($k == 0)); then
            prevl=$((l-1))
        else
            prevl=$l
        fi
        prevk=$((k-1))
        PREV_NAME_PMT=${PMT_NAMEBASE}.${firstind[$prevl]}.${secondind[$prevk]}.root
        PREV_NAME_LAPPD=${LAPPD_NAMEBASE}.${firstind[$prevl]}.${secondind[$prevk]}.root
        PREV_OUTPUT=${OUTPUT_NAMEBASE}.${firstind[$prevl]}.${secondind[$prevk]}.root
        CURRENT_NAME_PMT=${PMT_NAMEBASE}.${firstind[l]}.${secondind[k]}.root
        CURRENT_NAME_LAPPD=${LAPPD_NAMEBASE}.${firstind[l]}.${secondind[k]}.root
        CURRENT_OUTPUT=${OUTPUT_NAMEBASE}.${firstind[l]}.${secondind[k]}.root
        echo $CURRENT_NAME_PMT
        cd ${CONFIGDIR}
        sed -i "s|${DATADIR}${PREV_NAME_PMT}|${DATADIR}${CURRENT_NAME_PMT}|g" ${CONFIGDIR}*Config
        sed -i "s|${DATADIR}${PREV_NAME_LAPPD}|${DATADIR}${CURRENT_NAME_LAPPD}|g" ${CONFIGDIR}*Config
        sed -i "s|${HOMEDIR}${PREV_OUTPUT}|${HOMEDIR}${CURRENT_OUTPUT}|g" ${CONFIGDIR}*Config
        cd ${HOMEDIR}
        ./Analyse $CONFIG
    done
done
