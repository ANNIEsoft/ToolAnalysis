// Ensure that ROOT (via rootcint) generates all of the dictionary entries
// needed to read ROOT files created using the IF beam database

// standard library includes
#include <map>

// ToolAnalysis includes
#include "IFBeamDataPoint.h"

#ifdef __MAKECINT__
#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedefs;
#pragma link C++ class IFBeamDataPoint+;
#pragma link C++ class std::map<unsigned long long, IFBeamDataPoint>+;
#pragma link C++ class std::map<std::string, std::map<unsigned long long, IFBeamDataPoint> >+;
#pragma link C++ class std::pair<unsigned long long, unsigned long long>+;
#pragma link C++ class std::map<int, std::pair<unsigned long long, unsigned long long> >+;
#endif
