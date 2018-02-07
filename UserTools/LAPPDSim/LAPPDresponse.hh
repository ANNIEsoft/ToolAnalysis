#ifndef LAPPDRESPONSE_HH
#define LAPPDRESPONSE_HH

//#include "LAPPDpulse.hh"
//#include "LAPPDpulseCluster.hh"
#include "TObject.h"
#include "TH1.h"
#include "TRandom3.h"

//class LAPPDresponse : public TObject {
class LAPPDresponse {

 public: 

  LAPPDresponse();

  ~LAPPDresponse();

  void AddSinglePhotonTrace(double trans, double para, double time);
	
  TH1D* GetTrace(int CHnumber, int parity, double starttime, double samplesize, int numsamples, double thenoise);

  int FindStripNumber(double trans);

  double StripCoordinate(int stripnumber);

  //  LAPPDpulseCluster* GetPulseCluster() {return _pulseCluster;}

 private: 



  //relevant to a particular event
  double _freezetime;

  //input parameters and distributions
  TH1D* _templatepulse;
  TH1D* _PHD;
  TH1D* _pulsewidth; 

  //output responses
  TH1D** StripResponse_neg;
  TH1D** StripResponse_pos;

  //  LAPPDpulseCluster* _pulseCluster;

  //randomizer
  TRandom3* mrand;	

  //useful functions
  int FindNearestStrip(double trans);
  double TransStripCenter(int CHnum);
  
  //  ClassDef(LAPPDresponse,0)

};

#endif
