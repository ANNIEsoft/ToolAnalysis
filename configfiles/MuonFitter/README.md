#MuonFitter Config

***********************
#Description
**********************

Date created: 2024-10-02
The MuonFitter toolchain makes an attempt to fit muons using hit information.

The Tool has 2 modes. The first mode is pre-reconstruction. It takes input information from the ANNIEEvent and generates a text file containing hit information for the RNN. It is advisable to include minimal tools in this ToolChain, as the same data must be re-analysed with ToolAnalysis later.

This text file produced in this step is then processed by a standalone python script (Fit_data.py), which outputs fitted information into a new text file.

The second mode is reconstruction. In this mode the Tool reads information from the ANNIEEvent, along with both text files from the previous two steps (the first mode and the python script) and reconstructs the vertex based on the fitted paths. The resulting muon fit information is passed into the DataModel.

More detailed instructions are below.

************************
#Usage
************************

To generate (train) a model:
============================

1. Data_prepare.py

2. RNN_train.py

NOTE: Data_prepare.py requires input files that do not yet exist on the ANNIE gpvms; as a result the model cannot at present be re-trained.
Previously generated models are stored in /pnfs/annie/persistent/simulations/models/MuonFitter/ which may be used.

Please update any paths in the Tool configuration and Fit_data.py accordingly, or copy the appropriate model files to the configfiles/MuonFitter/RNNFit directory.

To analyse data:
================

1. First, run a ToolChain containing the MuonFitter Tool configured in "RecoMode 0". This will generate a file: ev_ai_eta_R{RUN}.txt with a {RUN} number corresponding to the WCSim run number. You should not include any Tools further along the ToolChain for this step.

2. Second, run "python3 Fit_data.py ev_ai_eta_R{RUN}.txt". This will apply the fitting and generate another textfile to be ran in ToolAnalysis: tanktrackfitfile_r{RUN}_RNN.txt. Again: please update any paths such that all files and models are available.

NOTE: the script RNNFit/rnn_fit.sh can act as a helper to process multiple ev_ai_eta_R{RUN}.txt files. Be sure to update the path in the script to point to your ev_ai_eta text files from step 1.

3. Finally, run a ToolChain containing the MuonFitter Tool configured in "RecoMode 1". Please set the paths for the ev_ai_eta_R{RUN}.txt and tanktrackfitfile_r{RUN}_RNN.txt in the MuonFitter config file accordingly. You may include any downstream tools you desire for further analysis. See the UserTools/MuonFitter/README.md for short descriptions of information saved to the DataModel and how to access them.
