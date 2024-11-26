#!/bin/bash

if [[ -z "$1" ]]; then
    echo ""
    echo "##############################"
    echo "Error: No run number provided."
    echo "Usage: $0 <run_number>"
    echo ""
    exit 1
fi

run=$1

output_file="my_inputs.txt"
processed_dir="/pnfs/annie/persistent/processed/processed_EBV2/R${run}/"

find "$processed_dir" -type f -name "Processed*" | sort -t'/' -k2V > "$output_file"

echo "done"
