// Ensure that ROOT (via rootcint) generates all of the dictionary entries
// needed to write recoANNIE output classes to a ROOT file
#ifdef __MAKECINT__
#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedefs;
#pragma link C++ class std::vector< std::vector<unsigned short> >+;
#pragma link C++ class annie::RecoPulse+;
#pragma link C++ class annie::RawChannel+;
#pragma link C++ class annie::RawCard+;
#pragma link C++ class annie::RawReadout+;
#pragma link C++ class annie::RawReader+;
#pragma link C++ class std::vector<annie::RecoPulse>+;
#pragma link C++ class std::map<int, std::vector<annie::RecoPulse> >+;
#pragma link C++ class std::map<int, std::map<int, std::vector<annie::RecoPulse> > >+;
#pragma link C++ class std::map<int, std::map<int, std::map<int, std::vector<annie::RecoPulse> > >+;
#pragma link C++ class annie::RecoReadout+;
#pragma link C++ class annie::BeamStatus+;
#pragma link C++ class IFBeamDataPoint+;
#pragma link C++ class std::map<unsigned long long, IFBeamDataPoint>+;
#pragma link C++ class std::map<std::string, std::map<unsigned long long, IFBeamDataPoint> >+;
#pragma link C++ class std::pair<unsigned long long, unsigned long long>+;
#pragma link C++ class std::map<int, std::pair<unsigned long long, unsigned long long> >+;
#endif
