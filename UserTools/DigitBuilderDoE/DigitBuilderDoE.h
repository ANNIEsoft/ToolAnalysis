#ifndef DigitBuilderDoE_H
#define DigitBuilderDoE_H

#include "Tool.h"

#include <string>
#include <iostream>

//ROOT includes
#include <TROOT.h>
#include <TMath.h>
#include <TChain.h>
#include <TFile.h>
#include <TRandom3.h>
#include <TLorentzVector.h>
#include <TVector3.h>
#include <TBox.h>
#include <Math/Vector4D.h>
#include <Math/Vector3D.h>
//#include <Math/PxPyPzE4D.h>
//#include <Math/LorentzVector.h>

//DataModel includes
#include "ANNIEGeometry.h"
#include "Position.h"
#include "Direction.h"

using namespace ROOT::Math;

class DigitBuilderDoE: public Tool {


 public:

  DigitBuilderDoE();
  ~DigitBuilderDoE();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  static DigitBuilderDoE* Instance();

 private:
  /// Reset the digit list and start muon vertex
  void Reset();
  // Sets branch addresses in TChain to link properly to initialized variables
  void LinkChain();
 
 	/// Push true neutrino vertex to "RecoVertex"
 	/// \param[in] bool savetodisk: save object to disk if savetodisk=true
 	void PushTrueVertex(bool savetodisk);
 	void PushRecoDigits(bool savetodisk);


  TChain* doechain;

	std::string doefilename;
	std::string doetreename;
  std::string fPhotodetectorConfiguration;
  double fHistoricOffset;

	TRandom3 frand;  ///< Random number generator

  uint32_t NumEvents;
	uint32_t EventNum;
  uint64_t MCEventNum;
  std::vector<RecoDigit>* fDigitList;
	RecoVertex* fMuonStartVertex = nullptr; 	 ///< true muon start vertex
	//RecoVertex* fMuonStopVertex = nullptr; 	 ///< true muon stop vertex

  //Digit Hit information
  std::vector<ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >>* DigitVertices; //X,Y,Z,T in vec
	std::vector<std::string>* DigitWhichDet;
	std::vector<double>* DigitCharges;
	std::vector<int>* DigitIdArray;
	TLorentzVector *MuonStartVertex;
	TLorentzVector *MuonStopVertex;
	TVector3 *MuonDirection;	
  double TrackLengthInMrd;

  int verbosity=1;
	/// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;

};


#endif
