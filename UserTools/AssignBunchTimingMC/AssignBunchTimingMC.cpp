#include "AssignBunchTimingMC.h"

AssignBunchTimingMC::AssignBunchTimingMC():Tool(){}

//------------------------------------------------------------------------------

bool AssignBunchTimingMC::Initialise(std::string configfile, DataModel &data){
  
    // Get configuration variables and set default values if necessary

    if ( !configfile.empty() ) m_variables.Initialise(configfile);
    m_data = &data;
    
    bool got_verbosity = m_variables.Get("verbosity", verbosity);
    bool got_width     = m_variables.Get("bunchwidth", fbunchwidth);
    bool got_interval  = m_variables.Get("bunchinterval", fbunchinterval);
    bool got_count     = m_variables.Get("bunchcount", fbunchcount);
    bool got_sample    = m_variables.Get("sampletype", fsample);
    bool got_trigger   = m_variables.Get("prompttriggertime", ftriggertime);

    if (!got_verbosity) {
        verbosity = 0;
        logmessage = "Warning (AssignBunchTimingMC): \"verbosity\" not set in the config, defaulting to 0";
        Log(logmessage, v_warning, verbosity);
    }


    // default bunch parameters from MicroBooNE paper: https://doi.org/10.1103/PhysRevD.108.052010

    if (!got_width) {
        fbunchwidth = 1.308;  // ns
        logmessage = ("Warning (AssignBunchTimingMC): \"bunchwidth\" not "
            "set in the config file. Using default: 1.308ns");
        Log(logmessage, v_warning, verbosity);
    }

    if (!got_interval) {
        fbunchinterval = 18.936;  // ns
        logmessage = ("Warning (AssignBunchTimingMC): \"bunchinterval\" not "
            "set in the config file. Using default: 18.936ns");
        Log(logmessage, v_warning, verbosity);
    }

    if (!got_count) {
        fbunchwidth = 81;
        logmessage = ("Warning (AssignBunchTimingMC): \"bunchcount\" not "
            "set in the config file. Using default: 81");
        Log(logmessage, v_warning, verbosity);
    }

    if (!got_sample) {
        fsample = 0;    // assume they are using the Tank samples
        logmessage = ("Warning (AssignBunchTimingMC): \"sampletype\" not "
            "set in the config file. Using default: 0 (GENIE tank samples)");
        Log(logmessage, v_warning, verbosity);
    }

    if (!got_trigger) {
        ftriggertime = 0;    // assume they are using the old WCSim prompt trigger time (t = 0, first particle arrival)
        logmessage = ("Warning (AssignBunchTimingMC): \"prompttriggertime\" not "
            "set in the config file. Using default: 0 (WCSim prompt trigger time = 0)");
        Log(logmessage, v_warning, verbosity);
    }

    
    if (verbosity >= v_message) {
        std::cout<<"------------------------------------"<<"\n";
        std::cout<<"AssignBunchTimingMC: Config settings"<<"\n";
        std::cout<<"------------------------------------"<<"\n";
        std::cout<<"bunch width       = "<<fbunchwidth<<" ns"<<"\n";
        std::cout<<"bunch interval    = "<<fbunchinterval<<" ns"<<"\n";
        std::cout<<"number of bunches = "<<fbunchcount<<"\n";
        std::cout<<"sample type       = "<<(fsample == 0 ? "(0) Tank" : "(1) World")<<"\n";
        std::cout<<"trigger time      = "<<(ftriggertime == 0 ? "(0) prompt trigger starts when first particle arrives (default WCSim)" 
                                                   : "(1) prompt trigger starts at beam dump (modified WCSim)")<<"\n";
    }

    fbunchTimes = new std::map<double, double>;   // set up pointer; Store will handle deletion in Finalize

    return true;
}

//------------------------------------------------------------------------------


bool AssignBunchTimingMC::Execute()
{

    if (verbosity >= v_debug) {
        std::cout << "AssignBunchTimingMC: Executing tool..." << std::endl;
    }

    if (!LoadStores())      // Load info from store
        return false;
    if (verbosity >= v_debug) {
        std::cout << "AssignBunchTimingMC: Store info loading successful" << std::endl;
    }

    fbunchTimes->clear();   // clear map

    BNBtiming();            // grab BNB structure
    if (verbosity >= v_debug) {
        std::cout << "AssignBunchTimingMC: BNB timing successful" << std::endl;
    }

    for (std::pair<double, std::vector<MCHit>>&& apair : *fClusterMapMC) {
        double totalHitTime = 0;
        int hitCount = 0;
        int totalHits = apair.second.size();

        CalculateClusterAndBunchTimes(apair.second, totalHitTime, hitCount, totalHits);

        // store the cluster time in a map (e.g., keyed by cluster identifier)
        fbunchTimes->emplace(apair.first, bunchTime);
    }

    if (verbosity >= v_debug) {
        std::cout << "AssignBunchTimingMC: Assigned bunch time successfully. Writing to Store..." << std::endl;
    }

    // send to store
    m_data->Stores.at("ANNIEEvent")->Set("bunchTimes", fbunchTimes);

    return true;
    
}


//------------------------------------------------------------------------------

bool AssignBunchTimingMC::Finalise()
{
  return true;
}

//------------------------------------------------------------------------------




bool AssignBunchTimingMC::LoadStores()
{
    // grab necessary information from Stores

    bool get_MCClusters = m_data->CStore.Get("ClusterMapMC", fClusterMapMC);
    if (!get_MCClusters) {
        Log("AssignBunchTimingMC: no ClusterMapMC in the CStore! Are you sure you ran the ClusterFinder tool?", v_error, verbosity);
        return false;
    }

    bool get_AnnieEvent = m_data->Stores.count("ANNIEEvent");
    if (!get_AnnieEvent) {
        Log("AssignBunchTimingMC: no ANNIEEvent store!", v_error, verbosity);
        return false;
    }

    bool get_MCHits = m_data->Stores.at("ANNIEEvent")->Get("MCHits", fMCHitsMap);
    if (!get_MCHits) {
        Log("AssignBunchTimingMC: no MCHits in the ANNIEEvent!", v_error, verbosity);
        return false;
    }
    
    bool get_neutrino_vtxt = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_T",TrueNuIntxVtx_T);   // grab neutrino interaction time
    if (!get_neutrino_vtxt) {
        Log("AssignBunchTimingMC: no GENIE neutrino interaction time info in the ANNIEEvent! Are you sure you ran the LoadGENIEEvent tool?", v_error, verbosity);
        return false;
    }

    return true;
}


//------------------------------------------------------------------------------


void AssignBunchTimingMC::BNBtiming()
{
    // Determined from GENIE samples (as of Oct 2024)
    const double tank_time = 67.0;    // Tank neutrino arrival time: 67ns
    const double world_time = 33.0;   // WORLD neutrino arrival time: 33ns
    
    if (ftriggertime == 0) {
        new_nu_time = (fsample == 0) ? (TrueNuIntxVtx_T - tank_time) : (TrueNuIntxVtx_T - world_time);
    } else if (ftriggertime == 1) {
        new_nu_time = (fsample == 0) ? -tank_time : -world_time;
    }

    // seed random number generator with the current time
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    generator.seed(seed);
    if (verbosity >= v_debug) {
        std::cout << "AssignBunchTimingMC: Random seed selected: " << seed << std::endl;
    }


    // per event, assign BNB structure (random bunch number + jitter based on intrinsic bunch width + new_nu_time)
    bunchNumber = rand() % fbunchcount;
    std::normal_distribution<double> distribution(0, fbunchwidth); 
    jitter = distribution(generator);

    if (verbosity >= v_message) {
        std::string logmessage = "AssignBunchTimingMC: bunchNumber = " + std::to_string(bunchNumber) + " | t0 = " + std::to_string(new_nu_time);
        Log(logmessage, v_debug, verbosity);
    }

}


//------------------------------------------------------------------------------


void AssignBunchTimingMC::CalculateClusterAndBunchTimes(std::vector<MCHit> const &mchits, double &totalHitTime, int &hitCount, int &totalHits)
{

    // loop over the hits to get their times
    for (auto mchit : mchits) { 
        double hitTime = mchit.GetTime();
        totalHitTime += hitTime;
        hitCount++;
        if (verbosity >= v_debug) {
            std::string logmessage = "AssignBunchTimingMC: (" + std::to_string(hitCount) + "/" + std::to_string(totalHits) + ") MCHit time = " + std::to_string(hitTime);
            Log(logmessage, v_debug, verbosity);
        }
    }

    // find nominal cluster time (average hit time)
    double clusterTime = (hitCount > 0) ? totalHitTime / hitCount : -9999;
    if (verbosity >= v_debug) {
        std::string logmessage = "AssignBunchTimingMC: ClusterTime = " + std::to_string(clusterTime);
        Log(logmessage, v_debug, verbosity);
    }

    // calculate BunchTime
    bunchTime = fbunchinterval * bunchNumber + clusterTime + jitter + new_nu_time ;
    if (verbosity >= v_debug) {
        std::string logmessage = "AssignBunchTimingMC: bunchTime = " + std::to_string(bunchTime);
        Log(logmessage, v_debug, verbosity);
    }

}


//------------------------------------------------------------------------------

// done
