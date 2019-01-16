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
#include <Math/GenVector/PxPyPzE4D.h>
#include <Math/GenVector/LorentzVector.h>
//DataModel includes
#include "ANNIEGeometry.h"

using namespace ROOT::Math;

class DigitBuilderDoE: public Tool {


 public:

  DigitBuilderDoE();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

 private:
  // Sets branch addresses in TChain to link properly to initialized variables
  void LinkChain();
 
 	/// Push true neutrino vertex to "RecoVertex"
 	/// \param[in] bool savetodisk: save object to disk if savetodisk=true
 	void PushTrueVertex(bool savetodisk);

  TChain* doechain;

	std::string doefilename;
	std::string doetreename;
  int NumEvents;
	int EventNum;
  std::vector<RecoDigit>* fDigitList;
	void ClearDigitList() {fDigitList->clear();}
	RecoVertex* fMuonStartVertex = nullptr; 	 ///< true muon start vertex
	RecoVertex* fMuonStopVertex = nullptr; 	 ///< true muon stop vertex

  //Digit Hit information
  std::vector< ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >>* DigitVertices; //X,Y,Z,T in vec
	std::vector<std::string>* DigitWhichDet;
	std::vector<double>* DigitCharges;
	std::vector<int>* DigitIdArray;
	TLorentzVector *MuonStartVertex;
	TLorentzVector *MuonStopVertex;
	TVector3 *MuonDirection;	
  double* TrackLengthInMrd;

  int verbosity=1;
	/// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;

};


#endif
