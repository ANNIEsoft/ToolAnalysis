#!/bin/bash
# Author: Steven Doran

echo ""
echo "Please make sure you have done the following:

		- Using the following toolchain:
				* LoadANNIEEvent
				* LoadGeometry
				* FitRWMWaveform
		
		- Enable 'printToRootFile 1' in the FitRWMWaveform tool
		- Per 20 part files, ensure 'maxPrintNumber 20000' (you can expect ~15,000 waveforms per 20 part files)

        - Set the correct configurations in this script:
                * pro_dir   (processed files directory)
                * step_size (number of files to execute per toolchain execution)
                * user      (name of present user)
                * TA_folder (name of ToolAnalysis directory)
                * toolchain (name of toolchain to be executed) 
                * offload   (scratch or persistent area where you can copy large root files to - /exp/annie/app/ only allocates ~50 GB of space)

        - Have a temporary tmp/ folder bind mounted for singularity in your /exp/annie/app/user/<user>/ area
                * example: singularity shell -B/pnfs:/pnfs,/exp/annie/app/users/$user/temp_directory:/tmp, ...

"
sleep 5

# Check if the user provided a run argument
if [[ -z "$1" ]]; then
    echo ""
	echo "##############################"
	echo "Error: No run number provided."
    echo "Usage: $0 <run_number>"
	echo ""
    exit 1
fi


#
#
#
# ********************************************************************* #
run=$1

pro_dir="/pnfs/annie/persistent/processed/processed_EBV2/R${run}/"
step_size=20

user="<user>"

TA_folder="ToolAnalysis/"
toolchain="AmBeWaveform"

offload="/pnfs/annie/scratch/users/${user}/AmBe_dump/"

# ********************************************************************* #
#
#
#


echo ""
echo "Generating AmBe waveforms for Run ${run}..."
echo ""
echo "User set to $user"
echo ""

# Check if the run was processed
if [[ ! -d "$pro_dir" ]]; then
    echo "Error: Directory $pro_dir does not exist."
    echo ""
	exit 1
fi

# Get the list of raw files and count them
pro_files=($(ls "$pro_dir" | grep "^ProcessedData_PMT_R${run}S0p"))
num_pro_files=${#pro_files[@]}

if (( num_pro_files <= step_size )); then
    start_indices=("0")
    end_indices=($(($num_pro_files - 1)))
else
    start_indices=("0")
    end_indices=()
    
    for (( i=step_size; i<num_pro_files; i+=step_size )); do
        start_indices+=("$i")
        end_indices+=("$((i - 1))")
    done
    end_indices+=("$((num_pro_files - 1))")
fi

echo ""
echo "Start indices: ${start_indices[@]}"
echo "End indices: ${end_indices[@]}"
echo ""


# run the toolchain over each step

for (( idx=0; idx<${#start_indices[@]}; idx++ )); do
    p_start=${start_indices[$idx]}
    p_end=${end_indices[$idx]}

    echo ""
    echo "Running over [${p_start},${p_end}]"
    echo ""

	my_files="my_inputs.txt"
    rm -f "$my_files"

    # Create the input file based on the start + end index
    for p in $(seq "$p_start" "$p_end"); do
        echo "${pro_dir}/ProcessedData_PMT_R${run}S0p${p}" >> "$my_files"
    done

	cp $my_files /exp/annie/app/users/$user/$TA_folder/configfiles/$toolchain/.

	echo ""
    cat /exp/annie/app/users/$user/$TA_folder/configfiles/$toolchain/$my_files
    echo ""
    echo ""
    echo ""

	sleep 3

    # Enter the Singularity environment and execute commands
    singularity shell -B/pnfs:/pnfs,/exp/annie/app/users/$user/temp_directory:/tmp,/exp/annie/data:/exp/annie/data,/exp/annie/app:/exp/annie/app /cvmfs/singularity.opensciencegrid.org/anniesoft/toolanalysis:latest << EOF

    cd /exp/annie/app/users/$user/$TA_folder

    source Setup.sh

    ./Analyse ./configfiles/$toolchain/ToolChainConfig

    exit

EOF

	echo ""
	ls -lrth /exp/annie/app/users/$user/$TA_folder
	echo ""

    # copy large root files from /annie/app area to some persistent or scratch area
	mkdir -p $offload/$run

    # setup transfer
    source /cvmfs/fermilab.opensciencegrid.org/products/common/etc/setup
    setup ifdhc v2_5_4

    echo ""
    echo "Copying ProcessedData Files..."
    echo ""

	ifdh cp /exp/annie/app/users/$user/$TA_folder/RWMBRFWaveforms.root $offload/$run/AmBeWaveforms_${run}_p${p_start}_p${p_end}.root
    sleep 1
	rm -rf /exp/annie/app/users/$user/$TA_folder/RWMBRFWaveforms.root

	ls -lrth $offload/$run
	echo ""

	sleep 5


echo ""
echo "done"
echo ""

done
