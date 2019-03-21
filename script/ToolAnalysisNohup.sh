#This script can be used to run multiple iterations of the same ToolAnalysis
#configuration on the annie Fermilab machine using nohup.  
#To use this script, make the
#following changes:
#  - Select your Config of choice by defining $CONFIG
#  - Define the directory where all of your wcsim data files are (DATADIR)
#  - Define the basenames of the PMT/LAPPD files in DATADIR, as well as the desired
#    basename of the root tree output files 
#  - In your ToolAnalysis configfiles directory, make a template as follows:
#    In LoadWCSimConfig: InputFile #INPUT_FILE_PMT#
#    In LoadWCSimLAPPDConfig: InputFile #INPUT_FILE_LAPPD#
#    In PhaseIITreeMaker: OutputFile #OUTPUT_FILE_TREE#
#    In ToolsConfig: Each tool's config path has base #INPUT_CONFIG_DIR#
#    In ToolChainConfig: #./configfiles/PhaseIIRecoTruth/ToolsConfigINPUT_TOOLS_CONFIG#  
#    sed is used to replace these with the basenames you define below. 


#Path to WCSim data
DATADIR=/pnfs/annie/persistent/users/moflaher/wcsim/multipmt/tankonly/wcsim_23-02-19_ANNIEp2v6_nodigint_BNB_Water_10k_22-05-17/

#Input and output file basenames
LAPPD_NAMEBASE=wcsim_lappd_0
PMT_NAMEBASE=wcsim_0
OUTPUT_NAMEBASE=PhaseIIRecoTruth_0

#Modify to define which WCSim file indexes to run ToolAnalysis with 
declare -a firstind=(1010) #1011 1012)
declare -a secondind=(1 2) #3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19)

#ToolAnalysis home directory and configuration directory
HOMEDIR=/annie/app/users/wjingbo/ToolAnalysis/
CONFIGDIR=${HOMEDIR}configfiles/PhaseIIRecoTruthJobs/
OUTPUTDIR=${HOMEDIR}/TAOutput/RecoTruth_KDECone/
LOGDIR=${HOMEDIR}/TAOutlog/
#source the ToolAnalysis and Fermilab Cluster UPS setup
source ${HOMEDIR}SetupFNAL.sh

cd ${HOMEDIR}
NUMFIRST=$((${#firstind[@]}-1))
NUMSEC=$((${#secondind[@]}-1))
for l in $(seq 0 $NUMFIRST)
    do
    for k in $(seq 0 $NUMSEC)
        do
            CURRENT_NAME_PMT=${PMT_NAMEBASE}.${firstind[l]}.${secondind[k]}.root
            CURRENT_NAME_LAPPD=${LAPPD_NAMEBASE}.${firstind[l]}.${secondind[k]}.root
            CURRENT_OUTPUT=${OUTPUT_NAMEBASE}.${firstind[l]}.${secondind[k]}.root
            echo $CURRENT_NAME_PMT
            cd ${CONFIGDIR}
            THISJOBCONFIG=${CONFIGDIR}/run${firstind[l]}.${secondind[k]}/
            mkdir ${THISJOBCONFIG}
            cp -r ${CONFIGDIR}/template/* ${THISJOBCONFIG} 
            sed -i "s|#INPUT_FILE_PMT#|${DATADIR}${CURRENT_NAME_PMT}|g" ${THISJOBCONFIG}*Config
            sed -i "s|#INPUT_FILE_LAPPD#|${DATADIR}${CURRENT_NAME_LAPPD}|g" ${THISJOBCONFIG}*Config
            sed -i "s|#OUTPUT_FILE_TREE#|${OUTPUTDIR}${CURRENT_OUTPUT}|g" ${THISJOBCONFIG}*Config
            sed -i "s|#INPUT_CONFIG_DIR#|${THISJOBCONFIG}|g" ${THISJOBCONFIG}*Config
            sed -i "s|#INPUT_TOOLS_CONFIG#|${THISJOBCONFIG}/ToolsConfig|g" ${THISJOBCONFIG}*Config
            cd ${HOMEDIR}
            nohup ./Analyse ${THISJOBCONFIG}/ToolChainConfig  ${LOGDIR}/ourlog_run${firstind[l]}.${secondind[k]}.txt 2>&1 &
    done
done
