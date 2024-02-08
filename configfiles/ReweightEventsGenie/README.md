# Configure files

***********************
#Description
**********************

Configure files are simple text files for passing variables to the Tools.

Text files are read by the Store class (src/Store) and automatically assigned to an internal map for the relevant Tool to use.


************************
#Usage
************************

Any line starting with a "#" will be ignored by the Store, as will blank lines.

Variables should be stored one per line as follows:


Name Value #Comments 

Values that contain multiple variables are separated by "|" into separate tokens
These tokens are broken up into "key:value" pairs
Syntax matters. Follow example in ReweightEventsGenieConfig

Note: Only one value is permitted per name and they are stored in a string stream and template cast back to the type given.

