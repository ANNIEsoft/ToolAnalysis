#!/bin/sh

filelist="/home/jhe/annie/analysis/flist.txt"

while read -r file
do
  echo "python3 Fit_data.py ${file}"
  python3 Fit_data.py ${file}
done < $filelist
