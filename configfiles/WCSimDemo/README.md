# Configure files

***********************
#Description
**********************

This toolchain demonstrates loading and reading WCSim events. It loads WCSim hits from PMTs and LAPPDs (using LoadWCSim and LoadWCSimLAPPD tools), then retrieve hits from the ANNIEEvent in the WCSimDemo tool and prints some event information.


************************
#Useage
************************

The LoadWCSimConfig file must be given the path to a WCSim file, usually named with the form "wcsim_0.XXX.YYY.root".

The LoadWCSimLAPPDConfig file must be given the path to a WCSim LAPPD file, usually named with the form "wcsim_lappd_0.XXX.YYY.root"

# on naming conventions:
XXX is the grid job number, and will match the file number for upstream genie and dirt propagated files
e.g. "wcsim_0.4500.root" would be generated from genie file "gntp.4500.ghep.root", with corresponding dirt propagated file annie_tank_flux.4500.root.
YYY is a further splitting added for later jobs using some of Vincent's genie files, as the time taken for WCSim to process each genie file would take too long on the grid. In this case the upstream job number is divided into ten - the genie file "gntp.4500.ghep.root" would generate 10 wcsim files named "wcsim_0.450.0.root - wcsim_0.450.9.root"
