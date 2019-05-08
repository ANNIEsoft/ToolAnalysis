# Configure files

***********************
#Description
**********************

This configuration is used to run the reconstruction chain on Simulated data
generated using WCSim. Minuit is ran multiple times using each point on a 
position grid as the fitter seed.  The result with the best figure of merit
is accepted as the best fit.

************************
#Useage
************************

Any line starting with a "#" will be ignored by the Store, as will blank lines.

Variables should be stored one per line as follows:


Name Value #Comments 


Note: Only one value is permitted per name and they are stored in a string stream and templated cast back to the type given.

