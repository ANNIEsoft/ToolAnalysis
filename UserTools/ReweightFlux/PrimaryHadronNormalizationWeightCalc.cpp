//Sanford-Wang xsec fit for neutral kaon
//
//    This code is intended to generate weights for neutrinos originating from K0 decays
//    utilizing a Sanford-Wang fit of world data  
//    
//    A this code is adopted from the MiniBooNE flux paper and the MiniBooNE reweighting framework 
//    along with code written by Raquel Castillo, Zarko, MiniBooNE (Steve Brice and Mike S), and Athula 
//
//    Current person adding comments and functions is Joseph Zennamo (jaz8600@fnal.gov)
//

#include "WeightCalcCreator.h"
#include "WeightCalc.h"

#include "CLHEP/Random/JamesRandom.h"
#include "CLHEP/Random/RandGaussQ.h"

#include <vector>
#include "TH1.h"
#include "TArrayD.h"
#include "TMatrixD.h"
#include "TRandom3.h"
#include "TLorentzVector.h"
#include <iostream>
#include <stdexcept>
#include "TFile.h"
#include "TTree.h"
#include <TROOT.h>
#include <TChain.h>

using namespace std;

namespace evwgh {
  class PrimaryHadronNormalizationWeightCalc : public WeightCalc
  {
  public:
    PrimaryHadronNormalizationWeightCalc();
    void Configure(fluxconfig pset);
    std::pair< bool, double > MiniBooNEWeightCalc(event & e, double rand);
    std::pair< bool, double > MicroBooNEWeightCalc(event & e, double rand);
    virtual std::vector<std::vector<double> > GetWeight(event & e);
    std::vector< double > MiniBooNERandomNumbers(std::string);
    
  private:
    CLHEP::RandGaussQ *fGaussRandom;
    std::vector<double> ConvertToVector(TArrayD const* array);
    std::string fGenieModuleLabel;
    std::vector<std::string> fParameter_list;
    float fParameter_sigma;
    int fNmultisims;
    int fprimaryHad;
    std::string fWeightCalc;
    double fScaleFactor;
    //TFile* file;
    std::vector< double > fWeightArray; 
    std::string fMode;
    double fSeed;
    bool fUseMBRands;
       
     DECLARE_WEIGHTCALC(PrimaryHadronNormalizationWeightCalc)
  };
  PrimaryHadronNormalizationWeightCalc::PrimaryHadronNormalizationWeightCalc()
  {
  }

  void PrimaryHadronNormalizationWeightCalc::Configure(fluxconfig pset)
  {

    // Here we do all our fhicl file configureation
//    fGenieModuleLabel= genie_module_label;

    fParameter_list		=   pset.parameter_list;
    fParameter_sigma		=   pset.parameter_sigma;
    fNmultisims			=   pset.number_of_multisims;
    fprimaryHad			=   pset.PrimaryHadronGeantCode;
    fWeightCalc                 =   pset.weight_calculator;
    fMode                       =   pset.mode;
    fScaleFactor                =   pset.scale_factor;
    fSeed 			=   pset.random_seed;
    fUseMBRands                 =   pset.use_MiniBooNE_random_numbers;
    //Prepare random generator
    CLHEP::HepRandomEngine* rng=new CLHEP::HepJamesRandom();
    rng->setSeed(long(fSeed),0);
    fGaussRandom = new CLHEP::RandGaussQ(rng,0,1);
    
    //
    // This is the meat of this, select random numbers that will be the 
    //   the reweighting
    //
    if(fUseMBRands){//fParameter_list
      fWeightArray = PrimaryHadronNormalizationWeightCalc::MiniBooNERandomNumbers(fParameter_list.at(0));
    }//Use MiniBooNE Randoms
    else{
      fWeightArray.resize(2*fNmultisims);
           
      for (unsigned int i=0;i<fWeightArray.size();i++) {
	if (fMode.find("multisim") != std::string::npos ){
	  fWeightArray[i]=fGaussRandom->shoot(rng,0,1.);
	}	
	else{
	  fWeightArray[i] = 1.;
	}
      }
    }//Use LArSoft Randoms

  }// End Configure

  std::vector<std::vector<double> > PrimaryHadronNormalizationWeightCalc::GetWeight(event & e)
  {

    
    //Create a vector of weights for each neutrino 
    std::vector< std::vector<double> > weight;

    //in art event possible to have multiple interactions, but here treat one event at a time
    weight.resize(1);
       
    // Let's start by iterating through each of the neutrino interactions 
    for(unsigned int inu = 0; inu < 1; inu++){

      // First let's check that the parent of the neutrino we are looking for is 
      //  the particle we intended it to be, if not set all weights to 1
      // 
      
      if (e.tptype != fprimaryHad){	
	weight[inu].resize(fNmultisims);
	std::fill(weight[inu].begin(), weight[inu].end(), 1);
	continue; //now move on to the next neutrino
      }// Hadronic parent check
      
  
      //Let's make a weights based on the calculator you have requested 
      if(fMode.find("multisim") != std::string::npos){       
	for (unsigned int i = 0;  int(weight[inu].size()) < fNmultisims; i++) {
	  if(fWeightCalc.find("MicroBooNE") != std::string::npos){
	    
	    //
	    //This way we only have to call the WeightCalc once
	    // 
	    std::pair<bool, double> test_weight =
	      MicroBooNEWeightCalc(e, fWeightArray[i]);
	    
	    if(test_weight.first){
	      weight[inu].push_back(test_weight.second);
	    }
	  }
	  if(fWeightCalc.find("MiniBooNE") != std::string::npos){

	    //
	    //This way we only have to call the WeightCalc once
	    // 
	    std::pair<bool, double> test_weight =
	      MiniBooNEWeightCalc(e, fWeightArray[i]);
	    
	    if(test_weight.first){
	      weight[inu].push_back(test_weight.second);
	    }
	  }
	}//Iterate through the number of universes      
      } // make sure we are multisiming

        

    }//Iterating through each neutrino 

    

    return weight;
  }

  //////////////////////////////
  ////////       Auxilary Functions
  //////////////////////////////

  //// 
  //   Use the MiniBooNE Implementation to determine the weight 
  ////
  std::pair< bool, double> PrimaryHadronNormalizationWeightCalc::MiniBooNEWeightCalc(event& e, double rand){
    
    // We need to guard against unphysical parameters 
    bool parameters_pass = false;

    double weight = rand+1;

    if(weight > 0) parameters_pass = true;
    else{parameters_pass = false;}
    
    std::pair<bool, double> output(parameters_pass, weight);
    
    return output; 

  }// Done with the MiniBooNE function


  //// 
  //   Use the MicroBooNE Implementation to determine the weight 
  ////
  std::pair<bool, double> PrimaryHadronNormalizationWeightCalc::MicroBooNEWeightCalc(event& e, double rand){
    
    throw std::invalid_argument(" MicroBooNE Weight Calc is Not Configured.");

    double weight = 1;
    bool parameters_pass = true;

    std::pair<bool, double> output(parameters_pass, weight);
    
    return output; 

  }// Done with the MicroBooNE function


  //// 
  //  This converts TArrayD to std::vector< double > 
  ///
  std::vector<double> PrimaryHadronNormalizationWeightCalc::ConvertToVector(TArrayD const* array) {
    std::vector<double> v(array->GetSize());
    std::copy(array->GetArray(), array->GetArray() + array->GetSize(),
	      v.begin());
    return v;
  } // ConvertToVector()

  

  std::vector< double > PrimaryHadronNormalizationWeightCalc::MiniBooNERandomNumbers(std::string pdg){

    if(pdg.compare("kminus") == 0){	 
      return {0.485954,-0.382708,-0.619274,1.077375,0.411452,0.551908,0.635836,-0.543290,0.369422,0.470114,0.538983,0.570427,0.530630,-0.428246,-0.767926,1.334734,1.033043,-0.360176,-0.225328,0.006105,0.334265,-0.304865,0.862233,0.145530,0.554744,-0.760917,1.149572,-0.877181,0.172956,-0.953126,1.920878,0.533393,1.412819,1.681677,-0.258286,1.069040,1.364980,-0.462723,-0.733373,0.757764,-0.211294,0.708839,3.298563,-0.914120,-0.586635,0.611965,-0.010409,-0.191123,-0.391530,0.473109,1.681666,-0.746085,0.602197,0.585621,-0.225831,1.189155,1.635702,0.979641,0.883413,0.770045,-0.781704,-0.382347,-0.899799,1.067063,-0.447751,2.410961,0.088843,-0.193447,0.897620,1.266334,0.560346,-0.773275,-0.364997,0.462723,1.749176,-0.909991,-0.685728,-0.896205,-0.852861,-0.125273,-0.097924,1.105909,-0.487878,0.094114,0.010752,1.439521,0.181166,-0.654181,0.123835,0.943598,0.216553,0.907248,-0.771738,-0.211872,0.303527,0.017761,0.383881,0.336015,-0.136930,0.727760,0.140366,-0.843689,1.757605,0.540486,1.515992,0.956636,0.830987,-0.214134,2.277036,-0.852957,0.187510,-0.642041,0.301105,0.207995,0.767048,-0.531694,3.052191,-0.213061,-0.034913,0.227312,-0.014922,-0.003751,0.985104,-0.088778,0.505535,-0.489052,2.328104,-0.258378,1.958725,-0.470344,0.984920,-0.061716,0.455421,-0.904859,-0.933595,0.682175,1.209555,1.047647,0.030922,0.999211,-0.985830,-0.389337,-0.012363,0.334219,-0.677769,0.048236,-0.150761,-0.112678,1.199551,1.983950,0.238357,-0.422593,2.112656,0.982946,0.142781,1.066485,2.020814,0.040829,2.045658,0.340465,1.282572,0.251887,0.298940,1.099826,0.640799,0.355690,0.788948,0.842331,-0.663275,-0.386013,0.380339,0.765388,0.384544,-0.643724,1.352459,0.026127,-0.453935,-0.640569,0.245124,1.490990,-0.279186,0.334147,-0.841383,-0.535157,-0.886807,1.421820,-0.365169,0.088894,-0.556115,-0.181417,0.570433,-0.334761,0.757692,0.068408,0.550732,0.550837,0.680631,0.657485,-0.199186,0.391019,0.560141,0.697788,0.107260,0.639215,0.198765,0.728133,0.212960,-0.627896,0.085343,0.010382,0.698879,-0.253625,0.626085,1.062297,-0.444220,0.634274,-0.534689,-0.162186,2.027973,0.507620,0.411923,-0.254447,-0.353287,0.098904,-0.233489,1.464386,0.328624,-0.903123,-0.083267,0.011711,-0.416383,0.483916,-0.497622,0.615212,-0.302939,0.435946,1.634435,1.841398,-0.669166,0.334890,-0.100038,0.499025,1.161366,0.144218,-0.548081,0.298398,0.374883,0.833335,1.480787,1.867065,-0.456855,0.140891,-0.569633,0.809960,0.896814,0.578200,-0.729039,-0.603689,-0.825915,1.387789,0.667061,-0.548053,0.775041,-0.756463,0.124885,0.439567,0.314940,-0.285602,0.800734,0.716464,1.157755,-0.318706,-0.012150,-0.051774,-0.308139,-0.517193,1.954867,0.491131,0.842546,-0.488717,-0.246186,0.412078,0.849025,-0.073675,0.722678,-0.202229,-0.919868,-0.699883,0.443090,-0.827387,1.582996,-0.856883,-0.394946,0.457144,0.850673,-0.032298,1.422950,1.304929,0.891750,1.901457,-0.651919,-0.060445,1.515280,0.413349,0.660211,0.392589,1.555834,-0.635835,-0.238329,0.037544,-0.300856,-0.481522,0.169079,-0.642659,-0.856194,1.158330,-0.870598,-0.066441,1.129805,-0.254827,1.601069,-0.014019,0.191258,2.233400,0.557749,1.003591,-0.540638,0.794074,1.487177,-0.172623,0.840908,-0.804978,2.089968,0.217379,-0.216967,-0.797429,-0.305230,1.237313,0.388457,1.154858,-0.820848,-0.858535,1.566498,0.391111,0.663486,0.412742,-0.819770,1.956315,0.271417,-0.208325,2.028396,0.696490,2.530880,0.834930,0.099952,-0.116662,0.809565,-0.573339,0.920607,-0.299079,-0.257701,-0.247686,0.185276,-0.978236,1.273666,-0.158775,-0.880955,1.245106,0.588537,-0.942230,1.158663,0.231305,0.603616,0.242274,-0.043631,0.636299,-0.793642,1.195719,0.878402,0.480012,-0.233907,0.494735,-0.602212,0.996000,0.194724,0.576695,0.886453,0.900541,-0.012229,-0.443644,0.493025,2.283338,0.120574,1.794557,1.013778,0.845813,1.062205,-0.075708,1.426607,0.426875,-0.031632,0.135827,0.103752,0.954121,0.284053,0.561090,0.008042,1.203907,0.010080,-0.568988,1.556905,1.404532,1.183801,0.511867,0.175464,0.350645,1.493974,-0.542150,1.583202,-0.137263,0.674281,0.159910,0.252258,0.426246,0.671904,-0.648624,0.535380,-0.819279,-0.958406,1.179373,-0.549603,-0.293049,-0.066901,-0.663800,0.467610,1.644068,-0.516822,0.683260,0.854684,-0.087984,1.541755,0.213475,-0.514917,0.046665,-0.809903,0.024998,1.468014,-0.857450,-0.758848,0.161576,1.373694,-0.853534,-0.069261,-0.556716,2.639745,-0.700894,0.057280,0.166924,0.070571,-0.397138,1.282697,-0.055739,-0.673459,-0.673037,-0.424991,2.411393,-0.108931,1.075593,1.498227,0.685934,-0.937651,-0.635114,0.478928,0.706245,0.924369,0.046778,-0.375192,0.446994,0.308518,-0.313821,-0.671087,0.226404,-0.126132,0.990539,-0.372478,0.585923,-0.120795,0.213825,0.695501,-0.255351,0.126357,-0.290747,1.234939,0.174878,0.154907,0.979435,-0.644357,-0.462287,0.315847,-0.822281,0.310644,-0.560140,0.942479,-0.120564,0.260698,0.195464,0.274368,-0.293841,-0.391365,-0.355048,-0.333344,0.546859,0.103730,0.093892,-0.380284,0.607666,0.676950,1.826112,0.505071,0.930216,-0.852279,-0.579640,-0.245427,-0.478658,0.099475,-0.608540,-0.403428,0.006447,0.045420,0.216830,1.513583,0.116735,-0.684740,-0.847627,0.287627,0.210442,0.459089,0.328155,0.510654,-0.371108,-0.844208,0.244240,2.609581,1.741043,2.105421,1.253111,1.227972,-0.875333,-0.302641,1.900995,1.478203,-0.231209,1.468125,-0.903698,-0.681683,-0.163732,0.233067,-0.089218,0.582424,1.322296,0.289012,-0.952659,-0.474243,0.803216,-0.246675,2.211893,1.132527,0.517176,-0.236182,0.938336,-0.133958,-0.277700,0.438302,-0.361490,-0.275013,1.874751,0.160067,0.739595,0.466803,1.184491,1.086286,-0.845033,1.833224,-0.348334,1.314513,-0.064424,0.412590,0.607351,-0.353495,-0.192026,0.499231,0.374958,-0.630351,-0.458890,0.233087,-0.334652,0.724007,0.605022,0.413049,-0.598796,-0.194107,-0.553407,0.169797,1.145567,2.041925,-0.496643,-0.725948,1.019628,0.845826,0.117279,-0.875811,0.903093,-0.280161,1.333478,-0.539288,1.888353,1.259450,0.834282,-0.564065,1.155913,0.681516,0.320847,1.127141,1.324826,0.762393,-0.441028,0.289657,-0.132819,-0.868773,0.478780,-0.064913,0.762686,1.530980,1.494079,-0.134916,-0.870221,1.020687,-0.540283,-0.890393,1.825655,0.655298,0.483348,-0.711200,-0.212819,0.713310,-0.725137,0.852145,0.346267,-0.427106,0.560934,0.778887,-0.496997,-0.352196,-0.575681,0.663910,1.470271,-0.252677,-0.049441,0.452788,-0.825744,-0.903067,1.192515,0.750360,0.355459,1.529093,1.100835,-0.175934,-0.856883,-0.468477,1.223443,-0.326517,-0.609128,-0.491251,-0.537234,0.902031,-0.297687,1.267757,-0.010789,0.829796,0.383527,0.704763,0.435062,0.225010,-0.081606,-0.066742,0.953316,-0.793427,0.084408,0.084422,-0.250519,1.416102,0.342070,0.956403,2.195523,1.121197,-0.806316,0.680651,-0.936797,1.437882,1.546319,0.870415,-0.562849,-0.111811,0.544329,-0.605433,0.273396,0.052478,-0.302193,1.082098,0.916940,0.603170,1.440985,1.524955,-0.395513,0.246977,0.116232,1.811553,-0.767197,-0.129033,0.522576,-0.423122,1.320546,1.960345,1.178735,1.090967,0.091103,1.882009,-0.959328,-0.682958,0.498713,0.496454,0.186654,0.187967,-0.575457,-0.698994,-0.076465,0.251872,0.349849,-0.509349,1.569349,-0.297359,0.580842,-0.243785,-0.771802,-0.614593,0.995835,0.301917,-0.326022,-0.111218,0.580927,-0.281811,1.586152,-0.603893,-0.922746,-0.220183,-0.251996,-0.659626,-0.028953,1.004683,0.951058,1.289689,1.346810,0.642364,-0.408623,-0.590826,0.122698,-0.061767,-0.871992,1.446526,-0.075964,-0.283087,-0.802560,0.121178,0.952207,0.456088,-0.246728,-0.208724,2.210635,0.116273,0.706253,1.849033,-0.511477,-0.005910,1.847413,-0.622531,-0.309240,-0.314865,0.513723,1.081899,0.936048,-0.042338,-0.220452,-0.296856,-0.268358,-0.598930,2.050804,0.603518,-0.800383,0.564759,0.685342,0.477127,0.339420,1.430570,0.860360,-0.250911,1.874131,1.114833,1.125767,0.103596,0.065485,0.594759,0.973693,0.518245,-0.002110,-0.805770,-0.499040,1.722260,-0.404931,-0.110481,-0.164673,-0.251186,1.985062,1.372231,0.197970,-0.022618,-0.693363,0.366875,0.581784,-0.670171,0.024985,-0.134638,-0.176136,-0.384338,1.634065,0.342787,-0.798273,-0.513500,-0.774284,-0.097819,1.511165,1.620654,0.291855,-0.724669,0.247146,0.959061,-0.427156,0.923338,-0.028926,1.042628,2.099464,-0.286950,-0.385460,-0.049862,1.054684,0.928966,0.249655,0.281056,1.382060,0.857661,-0.343517,1.506828,-0.959172,-0.124366,-0.489924,0.676800,-0.586546,-0.464349,1.007664,-0.322935,-0.195707,1.547385,-0.297580,0.349853,-0.133914,0.761553,-0.016576,2.601623,1.455785,0.698006,0.656991,-0.243445,-0.215170,0.490333,-0.154588,-0.558213,1.693885,0.335409,-0.006041,-0.609431,1.165079,0.053293,0.254207,2.504871,-0.483226,0.756510,-0.174161,0.735393,0.835039,-0.801043,-0.600323,0.107373,0.284233,0.420272,0.771192,1.243451,-0.628239,0.828007,1.809307,0.033507,0.961301,1.010319,1.067495,0.044991,-0.801353,1.003003,0.219000,0.471627,0.154050,-0.758148,-0.367455,-0.424451,2.082598,0.534316,1.142272,1.851958,-0.234453,-0.788603,0.877851,-0.418392,-0.090123,-0.739638,-0.824404,1.512357,0.615090,1.445712,1.402064,0.971894,-0.257647,-0.406229,-0.190765,-0.283388,0.264380,0.581676,1.208547,-0.659633,1.049734,0.848053,-0.183837,0.448615,-0.771785,0.351748,0.437955,-0.737505,1.746906,0.271743,-0.642094,2.185358,0.950766,0.128992,-0.116919,-0.370705,0.358176,-0.293917,1.407221,-0.634740,0.518634,-0.942018,0.102610,1.299826,1.067187,0.285565,0.289850,0.615126,-0.144886,0.002711,0.433162,0.864342,-0.339304,0.841081,-0.977361,-0.599510,0.235111,-0.168198,-0.707863,1.402829,0.153504,-0.040421,0.092493,-0.366944,1.065918,-0.835604,-0.032133,-0.844362,-0.549546,-0.822767,-0.520208,0.050140,-0.487056,0.813188,0.604268};			
    }
    else{
      throw std::invalid_argument(" Normalization MiniBooNE random numbers are only configured for negatively charged kaons");
    }
    
  }

  REGISTER_WEIGHTCALC(PrimaryHadronNormalizationWeightCalc)
}

