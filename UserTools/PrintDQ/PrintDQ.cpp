#include "PrintDQ.h"

PrintDQ::PrintDQ():Tool(){}

//------------------------------------------------------------------------------

bool PrintDQ::Initialise(std::string configfile, DataModel &data){
  
    // Get configuration variables and set default values if necessary

    if ( !configfile.empty() ) m_variables.Initialise(configfile);
    m_data = &data;
    
    bool got_verbosity = m_variables.Get("verbosity", verbosity);

    if (!got_verbosity) {
        verbosity = 0;
        logmessage = "Warning (PrintDQ): \"verbosity\" not set in the config, defaulting to 0";
        Log(logmessage, v_warning, verbosity);
    }

    // initialize counts
    totalclusters = 0;
    totalevents = 0;
    totalclusters_in_prompt = 0;
    totalclusters_in_ext = 0;
    totalext_rate_1 = 0;
    totalext_rate_2 = 0;
    totalokay_beam = 0;
    totaltmrd_coinc = 0;
    totalveto_hit = 0;
    totalveto_tmrd_coinc = 0;
    totalhas_track = 0;
    totalhas_lappd = 0;
    totalhas_BRF = 0;
    totaltimezero = 0;
    totalclusters_in_spill = 0;

    return true;
}

//------------------------------------------------------------------------------


bool PrintDQ::Execute()
{

    if (verbosity >= v_debug) {
        std::cout << "PrintDQ: Executing tool..." << std::endl;
    }

    ResetVariables();
    if (verbosity >= v_debug) {
        std::cout << "PrintDQ: Succesfully reset variables" << std::endl;
    }

    if (!LoadStores())         // Load info from store
        return false;
    if (verbosity >= v_debug) {
        std::cout << "PrintDQ: Store info loading successful" << std::endl;
    }

    if (!GrabVariables())      // Assign variable information for more complex objects
        return false;
    if (verbosity >= v_debug) {
        std::cout << "PrintDQ: Tricky variables assigned successfully" << std::endl;
    }

    FindCounts();
    if (verbosity >= v_debug) {
        std::cout << "PrintDQ: Succesfully found counts for that ANNIEEvent" << std::endl;
    }

    return true;
    
}


//------------------------------------------------------------------------------

bool PrintDQ::Finalise()
{

    if (verbosity >= v_debug) {
        std::cout << "PrintDQ: Calculating rates" << std::endl;
    }

    // calculate rates
    float_t events_per_cluster = totalclusters / totalevents;
    float_t ext_rate_1 = totalext_rate_1 * 100 / totalevents;
    float_t ext_rate_2 = totalext_rate_2 * 100 / totalevents;
    float_t okay_beam = totalokay_beam * 100 / totalevents;                                    
    float_t has_track = totalhas_track * 100 / totalevents;                                    
    float_t has_lappd = totalhas_lappd * 100 / totalevents;
    float_t has_BRF = totalhas_BRF * 100 / totalevents;
    float_t timezero = totaltimezero * 100 / totalevents;
    float_t tmrd_coinc = totaltmrd_coinc * 100 / totalevents;
    float_t veto_hit = totalveto_hit * 100 / totalevents;
    float_t veto_tmrd_coinc = totalveto_tmrd_coinc * 100 / totalevents;
    float_t clusters_in_spill = totalclusters_in_spill * 100 / totalclusters;                  
    float_t clusters_in_prompt = totalclusters_in_prompt * 100 / totalclusters;
    float_t clusters_in_ext = totalclusters_in_ext * 100 / totalclusters;

    if (verbosity >= v_debug) {
        std::cout << "PrintDQ: Calculating errors" << std::endl;
    }

    // and errors
    float_t er_events_per_cluster = CalculateStatError(totalclusters, totalevents) / 100;   // clusters per event is a true rate, not a %
    float_t er_ext_rate_1 = CalculateStatError(totalext_rate_1, totalevents);
    float_t er_ext_rate_2 = CalculateStatError(totalext_rate_2, totalevents);
    float_t er_okay_beam = CalculateStatError(totalokay_beam, totalevents);
    float_t er_has_lappd = CalculateStatError(totalhas_lappd, totalevents);
    float_t er_has_BRF = CalculateStatError(totalhas_BRF, totalevents);
    float_t er_timezero = CalculateStatError(totaltimezero, totalevents);
    float_t er_tmrd_coinc = CalculateStatError(totaltmrd_coinc, totalevents);
    float_t er_veto_hit = CalculateStatError(totalveto_hit, totalevents);
    float_t er_veto_tmrd_coinc = CalculateStatError(totalveto_tmrd_coinc, totalevents);
    float_t er_has_track = CalculateStatError(totalhas_track, totalevents);
    float_t er_clusters_in_spill = CalculateStatError(totalclusters_in_spill, totalclusters);
    float_t er_clusters_in_prompt = CalculateStatError(totalclusters_in_prompt, totalclusters);
    float_t er_clusters_in_ext = CalculateStatError(totalclusters_in_ext, totalclusters);

    // output
    std::cout << "" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "**************************************" << std::endl;
    std::cout << "Run " << fRunNumber << std::endl;
    std::cout << "**************************************" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "total events:         " << totalevents << std::endl;
    std::cout << "has LAPPD data:       " << totalhas_lappd << "    (" << has_lappd << "% +/- " << er_has_lappd << "%)" << std::endl;
    std::cout << "has BRF fit:          " << totalhas_BRF << "    (" << has_BRF << "% +/- " << er_has_BRF << "%)" << std::endl;
    std::cout << "eventTimeTank = 0:    " << totaltimezero << "    (" << timezero << "% +/- " << er_timezero << "%)" << std::endl;
    std::cout << "beam_ok:              " << totalokay_beam << "    (" << okay_beam << "% +/- " << er_okay_beam << "%)" << std::endl;
    std::cout << "total clusters:       " << totalclusters << "    (" << events_per_cluster << " +/- " << er_events_per_cluster << ")" << std::endl;
    std::cout << "prompt clusters:      " << totalclusters_in_prompt << "    (" << clusters_in_prompt << "% +/- " << er_clusters_in_prompt << "%)" << std::endl;
    std::cout << "spill clusters:       " << totalclusters_in_spill << "    (" << clusters_in_spill << "% +/- " << er_clusters_in_spill << "%)" << std::endl;
    std::cout << "ext clusters:         " << totalclusters_in_ext << "    (" << clusters_in_ext << "% +/- " << er_clusters_in_ext << "%)" << std::endl;
    std::cout << "extended (CC):        " << totalext_rate_1 << "    (" << ext_rate_1 << "% +/- " << er_ext_rate_1 << "%)" << std::endl;
    std::cout << "extended (NC):        " << totalext_rate_2 << "    (" << ext_rate_2 << "% +/- " << er_ext_rate_2 << "%)" << std::endl;
    std::cout << "Tank+MRD coinc:       " << totaltmrd_coinc << "    (" << tmrd_coinc << "% +/- " << er_tmrd_coinc << "%)" << std::endl;
    std::cout << "1 MRD track:          " << totalhas_track << "    (" << has_track << "% +/- " << er_has_track << "%)" << std::endl;
    std::cout << "Tank+Veto coinc:      " << totalveto_hit << "    (" << veto_hit << "% +/- " << er_veto_hit << "%)" << std::endl;
    std::cout << "Tank+MRD+Veto coinc:  " << totalveto_tmrd_coinc << "    (" << veto_tmrd_coinc << "% +/- " << er_veto_tmrd_coinc << "%)" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "" << std::endl;

    return true;
}

//------------------------------------------------------------------------------

void PrintDQ::FindCounts() {

    // total beam triggers (UBT only)
    int trigger_index = -1; // Default to -1 if not found
    if (!fGroupedTriggerWord.empty()) {
        for (size_t k = 0; k < fGroupedTriggerWord.size(); k++) {
            if ((fGroupedTriggerWord)[k] == 14) {
                trigger_index = static_cast<int>(k);
                break;
            }
        }
    }

    if (trigger_index != -1) {    // There is a beam trigger
        Log("PrintDQ: We found an undelayed beam trigger!", v_debug, verbosity);

        totalevents++;


        if (fBeamok == 1) {
            totalokay_beam++;     // all subsequent rates are based on the number of "good" beam triggers
            Log("PrintDQ: beam_ok = 1 for this event", v_debug, verbosity);
        }


        // events with eventTimeTank == 0 (weird issue where ADV waveforms don't get timestamps)
        if (fEventTimeTank == 0){
            totaltimezero++;
            Log("PrintDQ: eventTimeTank = 0 for this event", v_debug, verbosity);
        }


        // Extended readout rates (1 = charge-based, 2 = forced, random)
        if (fExtended == 1){
            totalext_rate_1++;
            Log("PrintDQ: ext = 1 for this event", v_debug, verbosity);
        }
        if (fExtended == 2){
            totalext_rate_2++;
            Log("PrintDQ: ext = 2 for this event", v_debug, verbosity);
        }


        // Does the event have LAPPDs?
        if (fHasLAPPD == 1) {
            totalhas_lappd++;
            Log("PrintDQ: ext = 2 for this event", v_debug, verbosity);
        }


        // coincidences
        if (fNoVeto == 0) {
            totalveto_hit++;              // opposite logic to NoVeto
            Log("PrintDQ: Tank+Veto coincidence for this event", v_debug, verbosity);
        }
        if (fPMTMRDCoinc == 1) {
            totaltmrd_coinc++;            // Tank + MRD
            if (fNoVeto == 0) {
                totalveto_tmrd_coinc++;   // Veto + Tank + MRD
                Log("PrintDQ: Tank+Veto+MRD coincidence for this event", v_debug, verbosity);
            }
            // find MRD tracks
            // Search through the MRD cluster array to see if any of them had a single track. If thats the case (and there was TankMRDCoinc), we can say there was a track
            if (std::find(fNumClusterTracks.begin(), fNumClusterTracks.end(), 1) != fNumClusterTracks.end()) {
                totalhas_track++;
                Log("PrintDQ: Tank+MRD coincidence + 1 MRD Track for this event", v_debug, verbosity);
            } 
        }


        // Does the event have a usable BRF signal (+ fit)
        if (fBRFFirstPeakFit != 0) {
            totalhas_BRF++;
            Log("PrintDQ: BRF fit found for this event", v_debug, verbosity);
        } 


        // find total number of clusters, and how many of the prompt clusters are in the beam spill
        Log("*************** Looking at clusters *****************", v_debug, verbosity);
        for (size_t j = 0; j < fClusterTime.size(); j++) {                      // loop over all clusters
            Log("######################################################", v_debug, verbosity);

            totalclusters++;

            std::ostringstream ct_message;
            ct_message << "PrintDQ: PMT Cluster found, with time " << fClusterTime.at(j) << " ns";
            Log(ct_message.str(), v_debug, verbosity);

            if (fClusterTime[j] < 2000) {                                    // cluster is in prompt window
                totalclusters_in_prompt++;
                Log("PrintDQ: cluster is in the prompt window (<2us)", v_debug, verbosity);

                if (fClusterTime[j] > 190 && fClusterTime[j] < 1750) {    // based on expected beam spill structure from data 
                    totalclusters_in_spill++;
                    Log("PrintDQ: cluster is in the beam spill (190:1750ns)", v_debug, verbosity);
                }  
            }
            else {
                totalclusters_in_ext++;                                         // clusters in the ext window

                Log("PrintDQ: cluster is in the ext window (>2us)", v_debug, verbosity);
            }
        }
        Log("*****************************************************", v_debug, verbosity);
        
    }

}


//------------------------------------------------------------------------------

float PrintDQ::CalculateStatError(float numerator, float denominator) {

    // return 0 if denom/numerator = 0 to avoid errors
    if (denominator == 0 || numerator == 0) {
        return 0;
    }

    // Calculate stat error: sqrt( (dN/N)^2 + (dD/D)^2 ) * (N/D), where N/D is the rate
    float relative_error_numerator = sqrt(numerator) / numerator;
    float relative_error_denominator = sqrt(denominator) / denominator;
    float total_relative_error = sqrt((relative_error_numerator * relative_error_numerator) +
                                      (relative_error_denominator * relative_error_denominator));

    // The error in the ratio is: ratio * total_relative_error
    float ratio = 100 * numerator / denominator;   // (needs to be multiplied by 100 as we are expressing them as %'s)
    return ratio * total_relative_error;
}


//------------------------------------------------------------------------------

bool PrintDQ::LoadStores()
{
    // grab necessary information from Stores, and calculate other necessary parameters

    bool get_run = m_data->Stores["ANNIEEvent"]->Get("RunNumber", fRunNumber);
    if (!get_run) {
        Log("PrintDQ: RunNumber not found in the ANNIEEvent!", v_debug, verbosity);
    }

    bool get_ext = m_data->Stores["ANNIEEvent"]->Get("TriggerExtended",fExtended);
    if (!get_ext) {
        Log("PrintDQ: no Extended Variable in the ANNIEEvent!", v_debug, verbosity);
    }
    
    bool get_ETT = m_data->Stores["ANNIEEvent"]->Get("EventTimeTank", fEventTimeTank);
    if (!get_ETT) {
        Log("PrintDQ: no EventTimeTank in the ANNIEEvent!", v_debug, verbosity);
    }

    bool get_veto = m_data->Stores["RecoEvent"]->Get("NoVeto", fNoVeto);
    if (!get_veto) {
        Log("PrintDQ: NoVeto not present in the RecoEvent! Are you sure you ran the EventSelector tool?", v_debug, verbosity);
    }

    bool get_coinc = m_data->Stores["RecoEvent"]->Get("PMTMRDCoinc", fPMTMRDCoinc);
    if (!get_coinc) {
        Log("PrintDQ: PMTMRDCoinc not present in the RecoEvent! Are you sure you ran the EventSelector tool?", v_debug, verbosity);
    }

    bool get_lappd = m_data->Stores["ANNIEEvent"]->Get("DataStreams", fDataStreams);
    if (!get_lappd) {
        Log("PrintDQ: DataStreams (used for hasLAPPD) not present in the ANNIEEvent!", v_debug, verbosity);
    }

    bool get_BRF = m_data->Stores["ANNIEEvent"]->Get("BRFFirstPeakFit", fBRFFirstPeakFit);
    if (!get_BRF) {
        Log("PrintDQ: BRFFirstPeakFit not present in the ANNIEEvent! Are you sure you ran the FitRWMWaveform tool?", v_debug, verbosity);
    }

    bool get_beam = m_data->Stores["ANNIEEvent"]->Get("beam_good", fBeamok);
    if (!get_beam) {
        Log("PrintDQ: no beam_good in the ANNIEEvent!", v_debug, verbosity);
    }
    
    // Grouped Triggers
    bool get_GT = m_data->Stores["ANNIEEvent"]->Get("GroupedTrigger", fGroupedTrigger);
    if (!get_GT) {
        Log("PrintDQ: no GroupedTrigger in the ANNIEEvent!", v_debug, verbosity);
    }
    
    // PMT clusters (need the ClusterFinder tool)
    bool get_Clusters = m_data->CStore.Get("ClusterMap", fClusterMap);
    if (!get_Clusters) {
        Log("PrintDQ: no ClusterMap in the CStore! Did you run the ClusterFinder tool?", v_debug, verbosity);
    }

    // MRD Tracks (TimeClustering and FindMRDTracks tools)
    bool get_mrdclusters = m_data->CStore.Get("MrdTimeClusters", fMrdTimeClusters);
    if (!get_mrdclusters) {
        Log("PrintDQ: No MRD clusters found! Did you run the TimeClustering tool?", v_debug, verbosity);
    }

    return true;

}


//------------------------------------------------------------------------------

void PrintDQ::ResetVariables() {

    // initialize to 0; if we dont find them in the Store, we want their counts to be 0
    fRunNumber = 0;
    fExtended = 0;
    fEventTimeTank = 0;
    fNoVeto = 0;
    fPMTMRDCoinc = 0;
    fHasLAPPD = 0;
    fBRFFirstPeakFit = 0;
    fBeamok = 0;

    fClusterTime.clear();
    fGroupedTrigger.clear();
    fGroupedTriggerTime.clear();
    fGroupedTriggerWord.clear();
    fNumClusterTracks.clear();
    fDataStreams.clear();

}


//------------------------------------------------------------------------------

bool PrintDQ::GrabVariables() {
    
    // HasLAPPD
    Log("PrintDQ: Seeing if DataStreams has LAPPDs", v_debug, verbosity);
    if (fDataStreams["LAPPD"] == 1)
        fHasLAPPD = 1;
    else
        fHasLAPPD = 0;

    // load grouped trigger info
    Log("PrintDQ: Accessing GroupedTriggers", v_debug, verbosity);
    for (std::map<uint64_t, uint32_t>::iterator it = fGroupedTrigger.begin(); it != fGroupedTrigger.end(); ++it)
    {
        uint64_t key = it->first;
        uint32_t value = it->second;

        fGroupedTriggerTime.push_back(key);
        fGroupedTriggerWord.push_back(value);
    }

    // load cluster time info
    Log("PrintDQ: Accessing pairs in the cluster map", v_debug, verbosity);
    for (std::pair<double, std::vector<Hit>>&& apair : *fClusterMap) {
        double thisClusterTime = apair.first;                // grab avg hit time of the cluster (clusterTime)
        fClusterTime.push_back(thisClusterTime);
    }

    // load MRD clusters and tracks
    Log("PrintDQ: Accessing MRD Clusters", v_debug, verbosity);
    int SubEventID;
    for (int i = 0; i < (int)fMrdTimeClusters.size(); i++) {

        SubEventID = i;

        // Check for valid Track criteria
        bool get_tracks = m_data->Stores["MRDTracks"]->Get("MRDTracks", ftheMrdTracks);
        bool get_numtracks = m_data->Stores["MRDTracks"]->Get("NumMrdTracks", fnumtracksinev);

        int NumClusterTracks = 0;
        Log("PrintDQ: Finding MRD Tracks", v_debug, verbosity);
        for (int tracki = 0; tracki < fnumtracksinev; tracki++) {
            
            NumClusterTracks += 1;
            BoostStore *thisTrackAsBoostStore = &(ftheMrdTracks->at(tracki));
            int TrackEventID = -1;
            thisTrackAsBoostStore->Get("MrdSubEventID", TrackEventID);
            if (TrackEventID != SubEventID)
                continue;

            // If we're here, this track is associated with this cluster

            NumClusterTracks += 1;
        }

        // get the track info
        int ThisMRDClusterTrackNum = NumClusterTracks;
        fNumClusterTracks.push_back(ThisMRDClusterTrackNum);

    }
    
    return true;
}


// ************************************************************************* //

// done
