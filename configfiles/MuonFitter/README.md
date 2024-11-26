#MuonFitter Config

***********************
#Description
**********************

Date created: 2024-10-02
Date last updated: 2024-11-26
The MuonFitter toolchain makes an attempt to fit muons using hit information.

The Tool has 2 modes. The first mode is pre-reconstruction. It takes input information from the ANNIEEvent and generates a text file containing hit information for the RNN. It is advisable to include minimal tools in this ToolChain, as the same data must be re-analysed with ToolAnalysis later.

This text file produced in this step is then processed by a standalone python script (Fit_data.py), which outputs fitted information into a new text file.

The second mode is reconstruction. In this mode the Tool reads information from the ANNIEEvent, along with both text files from the previous two steps (the first mode and the python script) and reconstructs the vertex based on the fitted paths. The resulting muon fit information is passed into the DataModel.

The Toolchain included in this directory is the minimal working Toolchain for MuonFitter to function. All Tools in the ToolChain MUST be included in order for MuonFitter to function. It is recommended to include PhaseIITreeMaker after MuonFitter in RecoMode 1 to generate an ntuple with reconstructed variables from MuonFitter.

More detailed instructions are below.

************************
#Usage
************************

To generate (train) a model:
============================

1. Run a ToolChain containing the MuonFitter Tool configured in "RecoMode 0". This will generate 2 files: ev_ai_eta_R{RUN}.txt with a {RUN} number corresponding to the WCSim run number and true_track_len.txt. You should not include any Tools further along the ToolChain for this step.

2. Run Data_prepare.py with the generated ev_ai_eta file and true_track_len file. This will generate data files to train the model for the next step.

3. Run RNN_train.py with the data files generated from the previous step. This will generate model.pth to be used as the model for data analysis.

Previously generated models and data are stored in /pnfs/annie/persistent/simulations/models/MuonFitter/ which may be used.

Please update any paths in the Tool configuration and Fit_data.py accordingly, or copy the appropriate model files to the configfiles/MuonFitter/RNNFit directory.

To analyse data:
================

1. First, run a ToolChain containing the MuonFitter Tool configured in "RecoMode 0". This will generate a file: ev_ai_eta_R{RUN}.txt with a {RUN} number corresponding to the WCSim run number. You should not include any Tools further along the ToolChain for this step.

2. Second, run "python3 Fit_data.py ev_ai_eta_R{RUN}.txt". This will apply the fitting and generate another textfile to be ran in ToolAnalysis: tanktrackfitfile_r{RUN}_RNN.txt. Again: please update any paths such that all files and models are available.

NOTE: the script RNNFit/rnn_fit.sh can act as a helper to process multiple ev_ai_eta_R{RUN}.txt files. Be sure to update the path in the script to point to your ev_ai_eta text files from step 1.

3. Finally, run a ToolChain containing the MuonFitter Tool configured in "RecoMode 1". Please set the paths for the ev_ai_eta_R{RUN}.txt and tanktrackfitfile_r{RUN}_RNN.txt in the MuonFitter config file accordingly. You may include any downstream tools you desire for further analysis. See the UserTools/MuonFitter/README.md for short descriptions of information saved to the DataModel and how to access them.

Storage for intermediate files:
===============================

Intermediate files can be stored in directories in /pnfs/annie/persistent/simulations/models/MuonFitter/
truetanktracklength/ - would contain true_track_len.txt
tanktrackfit/ - would contain tanktrackfitfile_r{RUN}_RNN.txt
ev_ai_eta/ - would contain ev_ai_eta_R{RUN}.txt

These directories are currently empty as of 11/26/2024 and will be filled upon Production level generation of ntuples using MuonFitter.
