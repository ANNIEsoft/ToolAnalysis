#ifndef WEIGHTMANAGER_CXX
#define WEIGHTMANAGER_CXX

#include <sstream>
#include <map>
#include <set>
#include "WeightManager.h"
#include "CLHEP/Random/Random.h"

namespace evwgh {

  WeightManager::WeightManager(const std::string name)
    : _name(name)
  {
    _configured = false;
  }

  const std::string& WeightManager::Name() const
  { return _name; }

  size_t WeightManager::Configure(std::vector<fluxconfig> p) 
  {
       // Loop over all the functions and register them
    for (int i = 0; i < p.size(); i++) 
    {
      std::string func_type = p[i].type;
      WeightCalc* wcalc=WeightCalcFactory::Create(func_type+"WeightCalc");//BREAKS
      if ( wcalc == NULL ) {
	std::cerr<< "Function " << p[i].title << " requested in fcl file has not been registered!" << std::endl;
	throw std::exception();
      } 
      if ( fWeightCalcMap.find(p[i].title)!= fWeightCalcMap.end() ) {//fix up
	std::cerr<< "Function " << p[i].title << " has been requested multiple times in fcl file!" << std::endl;
	throw std::exception();
      }

      //mf::LogInfo("") << "Configuring weight calculator " << p[i].type;
  
      // Create random engine for each rw function (name=*ifunc) (and seed it with random_seed set in the fcl)

      wcalc->SetName(p[i].type);
      wcalc->Configure(p[i]);//what is this doing??? there is no Configure definition in WeightCalc class
	//I think it goes straight to the calculator.
      Weight_t* winfo=new Weight_t();
      winfo->fWeightCalcType=func_type;
      winfo->fWeightCalc=wcalc;
      winfo->fNmultisims=(p[i]).number_of_multisims;
  
      std::pair<std::string, Weight_t*> pwc(p[i].title,winfo);
      fWeightCalcMap.insert(pwc);
    }

    _configured = true;
    std::cout << "Called" << std::endl;

    return fWeightCalcMap.size();
  }


 
  // 
  // CORE FUNCTION
  //
  MCEventWeight WeightManager::Run(event & e, const int inu)
  {     

    if (!_configured) {
      std::cerr<< "WeightManager was not configured!"<<std::endl;
      throw std::exception();
    }
    //
    // Loop over all functions ang calculate weights
    //
    MCEventWeight mcwgh;
    for (auto it = fWeightCalcMap.begin() ;it != fWeightCalcMap.end(); it++) {

      auto const & weights = it->second->GetWeight(e);
      if(weights.size() == 0){
        std::vector<double> empty;
        std::pair<std::string, std::vector <double> > pr("empty",empty);
        mcwgh.fWeight.insert(pr);
      } 
      else{
        std::pair<std::string, std::vector<double> > 
          pr(it->first+"_"+it->second->fWeightCalcType,
            weights[inu]);
        mcwgh.fWeight.insert(pr);
      }
    }

    return mcwgh;
  }



  void WeightManager::PrintConfig() {
    
    return; 
  }

}

#endif
