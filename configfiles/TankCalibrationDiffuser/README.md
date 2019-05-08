# Configure files

***********************
#Description
**********************

Configure files are simple text files for passing variables to the Tools.

Text files are read by the Store class (src/Store) and automatically asigned to an internal map for the relavent Tool to use.


************************
#Useage
************************

Any line starting with a "#" will be ignored by the Store, as will blank lines.

Variables should be stored one per line as follows:


Name Value #Comments 


Note: Only one value is permitted per name and they are stored in a string stream and templated cast back to the type given.

