#MuonFitter Config

***********************
#Description
**********************

Date created: 2024-10-02
The MuonFitter toolchain makes an attempt to fit muons using hit information.

Configure files are simple text files for passing variables to the Tools.

Text files are read by the Store class (src/Store) and automatically assigned to an internal map for the relevant Tool to use.

MuonFitter has 2 functionalities. The first functionality is pre-reconstruction. It takes input information and outputs a text file providing information to be fitted. This text file is used in a script to generate an additional text file that contains the fitted information.

The second functionality is reconstruction. It takes both text files and reconstructs the vertex based on the fitted paths.

Therefore, in order to properly use this Tool in a ToolChain: The ToolChain must be ran twice on the same set of data. Instructions are below.

************************
#Usage
************************

Any line starting with a "#" will be ignored by the Store, as will blank lines.

Variables should be stored one per line as follows:


Name Value #Comments 

Note: Only one value is permitted per name and they are stored in a string stream and template cast back to the type given.


To generate a model, run the following scripts in order:

1. Data_prepare.py

2. RNN_train.py

WARNING: currently, Data_prepare.py requires input files that do not exist on the ANNIE gpvms. These scripts were created and used to create models outside of the ANNIE gpvms and ToolAnalysis container. These scripts are to be considered DEPRECATED until further notice.

Please update any paths such that all files and models are available or copy all model files to configfiles/MuonFitter/RNNFit directory. All model files are currently located at /pnfs/annie/persistent/simulations/models/MuonFitter/

To run the Tool:

1. First, run in "RecoMode 0". This will generate a file: ev_ai_eta_R{RUN}.txt with a {RUN} number corresponding to the WCSim run number. You do not need any Tools further along the ToolChain for this step. This text file is all you need.

2. Second, run "python3 Fit_data.py ev_ai_eta_R{RUN}.txt". This will apply the fitting and generate another textfile to be ran in ToolAnalysis: tanktrackfitfile_r{RUN}_RNN.txt. ALSO: please update any paths such that all files and models are available.

3. Finally, run in "RecoMode 1". This is running the ToolChain for real. Please set the paths for the ev_ai_eta_R{RUN}.txt and tanktrackfitfile_r{RUN}_RNN.txt accordingly so they can be read in with the corresponding data file. See the README.md in UserTools/MuonFitter/ for short descriptions of information saved to the DataModel and how to access them.
