# coding: utf-8
import pandas as pd
import sys

# Get input file name
if (len(sys.argv) != 2):
    print(" @@@@@ MISSING FILE TO EXTRACT PARTS FROM: fitbyeye_r{RUN}_RNN.txt !! @@@@@ ")
    exit(-1)

INFILENAME = sys.argv[-1]
RUN = INFILENAME.split('_')[1]
print("run: " + RUN)

# open output file
OUTFILENAME = "my_inputs_" + RUN + "_filtered.txt" 
out_f = open(OUTFILENAME, 'a')

# open input file
df = pd.read_csv("/Users/juhe/annie/analysis/playground/" + INFILENAME, header=None, delimiter='_', names=['partnum', 'misc'])
print("num of fits: " + str(len(df)))

# drop duplicate part files
df = df.drop(columns=['misc'])
df.drop_duplicates(inplace=True)
print("num of parts: " + str(len(df)))

# write out filtered list
for idx in range(len(df)):
    print("ProcessedRawData_TankAndMRDAndCTC_R2630S0" + df.iloc[idx,0])
    out_f.write("/pnfs/annie/persistent/users/mnieslon/data/processed_hits_improved/R2630/ProcessedRawData_TankAndMRDAndCTC_R2630S0" + df.iloc[idx,0] + "\n")

# close file
out_f.close()

