# Configure files

***********************
#Description
**********************

This configuration is used to run the reconstruction chain on Simulated data
generated using WCSim. The true muon vertex information is given to the
extended vertex fitter as the fit seed.  Minuit is ran once and the position
with the highest figure of merit is accepted.
 
************************
#Useage
************************

Any line starting with a "#" will be ignored by the Store, as will blank lines.

Variables should be stored one per line as follows:


Name Value #Comments 


Note: Only one value is permitted per name and they are stored in a string stream and templated cast back to the type given.

