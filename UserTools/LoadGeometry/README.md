# LoadGeometry

LoadGeometry

## Data

The LoadGeometry tool takes in the specifications for all ANNIE detector
subsystems and creates an instance of the Geometry class.  After loading
the detector geometry's information, the geometry instance is saved to
the ANNIEEvent store with the 'AnnieGeometry' key.



## Configuration

Describe any configuration variables for LoadGeometry.

```
DimensionsGeoFile string
Specifies what CSV file to use to load the MRD specifications into the Geometry class.
String should be a full file path to the CSV file.

FACCMRDGeoFile string
Specifies what CSV file to use to load the MRD specifications into the Geometry class.
String should be a full file path to the CSV file.

TankPMTGeoFile string
Specifies what CSV file to use to load the Tank PMT specifications into the Geometry class.
String should be a full file path to the CSV file.
```
