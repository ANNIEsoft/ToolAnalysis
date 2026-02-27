/**
 * \file WeightManager.h
 *
 * 
 * \brief Allows to interface to EventWeight calculators
 *
 * @author Marco Del Tutto <marco.deltutto@physics.ox.ac.uk>
 */

#ifndef WEIGHTMANAGER_H
#define WEIGHTMANAGER_H

//#include "fhiclcpp/ParameterSet.h"
//#include "dk2nu/tree/dk2nu.h"
//#include "WeightStructs.h"
#include "Weight_t.h"
#include "MCEventWeight.h"
#include "WeightCalc.h"
#include "WeightCalcFactory.h"

namespace evwgh {
  /**
     \class WeightManager
  */
  class WeightManager {

  public:
    
    /// Default constructor
    WeightManager(const std::string name="WeightManager");
    
    /// Default destructor
    ~WeightManager(){}

    /// Name getter
    const std::string& Name() const;

    /** 
      * @brief Configuration function
      * @param cfg the input parameters for settings
      * @param the enging creator for the random seed (usually passed with *this)
       CONFIGURE FUNCTION: created the weights algorithms in the following way: \n
       0) Looks at the weight_functions fcl parameter to get the name of the calculators   \n
       1) Creates the Calculators requested in step 0, and assigne a different random seed to each one \n
       3) The future call WeightManager::Run will run the calculators           \n
    */
    //size_t Configure(fhicl::ParameterSet const & cfg);
    size_t Configure(std::vector<fluxconfig> p);


    /**
      * @brief Core function (previous call to Configure is needed)
      * @param e the struct event
      * @param inu the index of the simulated neutrino in the event
       CORE FUNCTION: executes algorithms to assign a weight to the event as requested users. \n
       WeightManager::Configure needs to be called first \n
       The execution takes following steps:             \n
       0) Loops over all the previously emplaced calculators \n
       1) For each of them calculates the weights (more weight can be requested per calculator) \n
       3) Returns a map from "calculator name" to vector of weights calculated which is available inside MCEventWeight
     */
    MCEventWeight Run(event &e, const int inu);

    /**
      * @brief Returns the map between calculator name and Weight_t product
      */
    std::map<std::string, Weight_t*> GetWeightCalcMap() { return fWeightCalcMap; }

    /// Reset
    void Reset()
    { _configured = false; }

    void PrintConfig();


  private:

    std::map<std::string, Weight_t*> fWeightCalcMap; ///< A set of custom weight calculators

    bool _configured = false; ///< Readiness flag

    std::string _name; ///< Name

  };
}

#endif

