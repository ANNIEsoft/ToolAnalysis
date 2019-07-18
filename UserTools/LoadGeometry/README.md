# LoadGeometry

LoadGeometry

## Data

The LoadGeometry tool takes in the specifications for all ANNIE detector
subsystems and creates an instance of the Geometry class.  After loading
the detector geometry's information, the geometry instance is saved to
the ANNIEEvent store with the 'AnnieGeometry' key.

## Writing a geometry file ##

An example of how to write a geometry file can be found in ./configfiles/LoadGeometry/FullMRDGeometry.csv


The line following LEGEND_LINE specifies the name of each data type.  The comma-separated
lines between DATA_START and DATA_END are the data for the geometry being loaded.

If you're writing a geometry file and things won't load for some reason, here are
some common pitfalls:
  - Make sure there are no commas in any strings in your data lines.  The parser
    will split the string and your data line will now have more entries than your
    legend can identify with.
  - Make sure your data line reader is reading in the data into the correct 
    variable type.  Refer to any of the ParseDataEntry() methods for how to handle this.
  - If you're seeing duplicate entries of variables of the same type, confirm that
    the labels defined in the vectors of the header file match the ones used in the
    .cpp file to pair data entries with variables via the legend line.  
    You may just have a typo in one of the three places.

## Configuration

Describe any configuration variables for LoadGeometry.

```
DetectorGeoFile string
Specifies what CSV file to use to load the MRD specifications into the Geometry class.
String should be a full file path to the CSV file.

FACCMRDGeoFile string
Specifies what CSV file to use to load the MRD specifications into the Geometry class.
String should be a full file path to the CSV file.

TankPMTGeoFile string
Specifies what CSV file to use to load the Tank PMT specifications into the Geometry class.
String should be a full file path to the CSV file.

LAPPDGeoFile string
Specifies what CSV file to use to load the LAPPD detector/channel 
specifications into the Geometry class.
String should be a full file path to the CSV file.
```
