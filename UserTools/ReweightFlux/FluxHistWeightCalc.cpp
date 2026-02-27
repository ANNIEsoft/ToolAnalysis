#include "WeightCalcCreator.h"
#include "WeightCalc.h"

#include <iostream>
#include <cstdlib>
#include <stdexcept>
//#include "nutools/RandomUtils/NuRandomService.h"
#include "CLHEP/Random/JamesRandom.h"
#include "CLHEP/Random/RandGaussQ.h"
//#include "WeightStructs.h"

#include "TFile.h"
#include "TH1F.h"

namespace evwgh {
  class FluxHistWeightCalc : public WeightCalc
  {
  public:
    FluxHistWeightCalc();
    void Configure(fluxconfig pset);
    std::vector<std::vector<double> > GetWeight(event & e);
    
  private:    
    CLHEP::RandGaussQ *fGaussRandom;
    std::vector<double> fWeightArray;
    int fNmultisims;
    std::string fMode;
    std::string fGenieModuleLabel;

    //         pi+-,k+-,k0,mu+- 
    //         |  numu, numubar, nue, nuebar 
    //         |  |   50MeV bins
    //         |  |   |
    double fCV[4][4][200];
    double fRW[4][4][200];
    
    DECLARE_WEIGHTCALC(FluxHistWeightCalc)
  };
  FluxHistWeightCalc::FluxHistWeightCalc()
  {
  }

  void FluxHistWeightCalc::Configure(fluxconfig pset)
  {    

    //calc config
    fNmultisims = pset.number_of_multisims;
    fMode       = pset.mode;		
    std::string dataInput1 = pset.cv_hist_file;
    std::string dataInput2 = pset.rw_hist_file;

    std::string fw_path = std::getenv("FW_SEARCH_PATH");
    std::string cvfile = fw_path + "/" + dataInput1;
    std::string rwfile = fw_path + "/" + dataInput2;

    std::string ptype[] = {"pi", "k", "k0", "mu"};
    std::string ntype[] = {"numu", "numubar", "nue", "nuebar"};

    TFile fcv(Form("%s",cvfile.c_str()));
    TFile frw(Form("%s",rwfile.c_str()));
    for (int iptyp=0;iptyp<4;iptyp++) {
      for (int intyp=0;intyp<4;intyp++) {
	for (int ibin=0;ibin<200;ibin++) {
	  fCV[iptyp][intyp][ibin]=(dynamic_cast<TH1F*> (fcv.Get(Form("h_%s_%s",ptype[iptyp].c_str(),ntype[intyp].c_str()))))->GetBinContent(ibin+1);
	  fRW[iptyp][intyp][ibin]=(dynamic_cast<TH1F*> (frw.Get(Form("h_%s_%s",ptype[iptyp].c_str(),ntype[intyp].c_str()))))->GetBinContent(ibin+1);
	}
      }
    }
    fcv.Close();
    frw.Close();

    CLHEP::HepRandomEngine* rng=new CLHEP::HepJamesRandom();
    fGaussRandom = new CLHEP::RandGaussQ(rng,0,1);
    fWeightArray.resize(fNmultisims);

    if (fMode.find("multisim") != std::string::npos )
      for (int i=0;i<fNmultisims;i++) fWeightArray[i]=fGaussRandom->shoot(rng,0,1.);
    else
      for (int i=0;i<fNmultisims;i++) fWeightArray[i]=1.;
  }

  std::vector<std::vector<double> > FluxHistWeightCalc::GetWeight(event & e)
  {
    //calculate weight(s) here 
    std::vector<std::vector<double> > weight;

    //within art it is possible to have multiple nu interactions in an event, but here it is always 1
    //leaving vector of vectors to minimize the diff in code
    weight.resize(1);
    for (unsigned int inu=0;inu<1;inu++) {
      weight[inu].resize(fNmultisims);
     
      int ptype=-9999;
      int ntype=-9999;
      int bin=-9999;
      
      if (      e.ptype==211 || e.ptype==-211 ) ptype = 0;
      else if ( e.ptype==321 || e.ptype==-321 ) ptype = 1;
      else if ( e.ptype==130                  ) ptype = 2;
      else if ( e.ptype==13  || e.ptype==-13  ) ptype = 3;
      else {
	throw std::invalid_argument("::Unknown ptype " + e.ptype);
      }
      
      if (      e.ntype==14  ) ntype=0;
      else if ( e.ntype==-14 ) ntype=1;
      else if ( e.ntype==12  ) ntype=2;
      else if ( e.ntype==-12 ) ntype=3;
      else {
	throw std::invalid_argument("::Unknown ptype " + e.ptype);
      }

      //nuray index should match the location of input reweighting histograms
      double enu=e.nenergyn;
      bin=enu/0.05;     
      for (int i=0;i<fNmultisims;i++) {
	double test = 1-(1-fRW[ptype][ntype][bin]/fCV[ptype][ntype][bin])*fWeightArray[i];
	
	// Guards against inifinite weights
	if(std::isfinite(test)){ weight[inu][i] = test;}
	else{weight[inu][i] = 1;}
      }
    }
    return weight;
  }
  REGISTER_WEIGHTCALC(FluxHistWeightCalc)
}
