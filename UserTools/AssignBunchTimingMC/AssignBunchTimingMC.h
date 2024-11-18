#ifndef AssignBunchTimingMC_H
#define AssignBunchTimingMC_H

#include <string>
#include <iostream>
#include <random>

#include "Tool.h"
#include "Hit.h"


/**
 * \class AssignBunchTimingMC
 *
 * A tool to assign BNB bunch structure to GENIE events
*
* $Author: S.Doran $
* $Date: 2024/10/09 $
* Contact: doran@iastate.edu
*/
class AssignBunchTimingMC: public Tool {

    public:

        AssignBunchTimingMC(); ///< Simple constructor
        bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
        bool Execute(); ///< Execute function used to perform Tool purpose.
        bool Finalise(); ///< Finalise function used to clean up resources.

        bool LoadStores(); ///< Loads all relevant information from the store, away from the Execute function
        void BNBtiming(); ///< Calculates the appropriate BNB timing to apply to the clusters
        void CalculateClusterAndBunchTimes(std::vector<MCHit> const &mchits, double &totalHitTime, int &hitCount, int &totalHits); ///< Loops through the MCHits, finds the cluster times (avg hit time), and calculates the new bunch timing

    private:

        std::map<unsigned long, std::vector<MCHit>> *fMCHitsMap    = nullptr;  ///< All of the MCHits keyed by channel number
        std::map<double, std::vector<MCHit>>        *fClusterMapMC = nullptr;  ///< All MC clusters
        double TrueNuIntxVtx_T;                                                ///< true neutrino interaction time in ns, from GenieInfo store

        std::map<double, double> *fbunchTimes                      = nullptr;  ///< Bunch-realistic timing from the cluster times; 
                                                                               ///  map linking updated bunch time to normal cluster time

        std::default_random_engine generator;                                  ///< Random number generator for bunch number

        double fbunchwidth;       ///< BNB intrinsic bunch width in ns
        double fbunchinterval;    ///< BNB bunch spacing in ns
        int fbunchcount;          ///< number of BNB bunches per spill
        int fsample;              ///< GENIE Tank or WORLD samples
        int ftriggertime;         ///< whether the samples used the default WCSim prompt trigger = 0 (when particles enter the volume), or the adjusted prompt trigger based on the start of the beam dump

        double new_nu_time;       ///< offset needed to make the cluster times beam realistic
        int bunchNumber;          ///< randomly assigned bunch number
        double jitter;            ///< random jitter based on the intrinsic bunch width
        double bunchTime;         ///< individual bunch time assigned for a specific cluster

        /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
        int verbosity;
        int v_error=0;            // STOP THE SHOW
        int v_warning=1;          // this and below the show will go on
        int v_message=2;
        int v_debug=3;
        std::string logmessage;

};


#endif