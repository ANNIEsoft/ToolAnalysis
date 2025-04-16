#include "ProcessedLAPPDFilter.h"

ProcessedLAPPDFilter::ProcessedLAPPDFilter() : Tool() {}

bool ProcessedLAPPDFilter::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  FilterVerbosity = 0;
  m_variables.Get("FilterVerbosity", FilterVerbosity);
  requirePulsedAmp = 15;
  m_variables.Get("requirePulsedAmp", requirePulsedAmp);
  requirePulsedStripNumber = 7;
  m_variables.Get("requirePulsedStripNumber", requirePulsedStripNumber);

  saveAllLAPPDEvents = false;
  m_variables.Get("saveAllLAPPDEvents", saveAllLAPPDEvents);
  savePMTClusterEvents = false;
  m_variables.Get("savePMTClusterEvents", savePMTClusterEvents);

  MRDDataName = "FilteredMRDData";
  MRDNoVetoDataName = "FilteredMRDNoVetoData";
  m_variables.Get("MRDDataName", MRDDataName);
  m_variables.Get("MRDNoVetoDataName", MRDNoVetoDataName);

  AllLAPPDDataName = "FilteredAllLAPPDData";
  m_variables.Get("AllLAPPDDataName", AllLAPPDDataName);
  PMTClusterDataName = "FilteredPMTClusterData";
  m_variables.Get("PMTClusterDataName", PMTClusterDataName);

  filterType = "MRDtrack"; // MRDtrack and pmt cluster
  // filterType = "PMTCluster"; // pmt cluster only
  m_variables.Get("filterType", filterType);

  RequireNoVeto = false;
  m_variables.Get("RequireNoVeto", RequireNoVeto);
  RequireCherenkovCover = false;
  m_variables.Get("RequireCherenkovCover", RequireCherenkovCover);

  FilteredMRD = new BoostStore(false, 2);        // all events with MRD in it //if gotEventMRD = true;
  FilteredMRDNoVeto = new BoostStore(false, 2);  // all events with MRD but no veto// if gotEventMRD && gotEventMRDNoVeto = true
  FilteredAllLAPPD = new BoostStore(false, 2);   // all events with LAPPD data
  FilteredPMTCluster = new BoostStore(false, 2); // all events with PMT cluster

  gotEventMRD = false;
  gotEventMRDNoVeto = false;
  gotEventPMTCluster = false;

  EventPMTClusterNumber = 0;
  EventMRDNumber = 0;
  EventMRDNoVetoNumber = 0;
  CurrentExeNumber = 0;
  pairedEventNumber = 0;
  PsecDataNumber = 0;
  PMTClusterEventNum = 0;

  passCutEventNumber = 0;

  return true;
}

bool ProcessedLAPPDFilter::Execute()
{

  uint64_t ETT = 0;
  bool gotETT = m_data->Stores["ANNIEEvent"]->Get("EventTimeTank", ETT);
  int pTW = 0;
  bool gotpTW = m_data->Stores["ANNIEEvent"]->Get("PrimaryTriggerWord", pTW);
  uint64_t pTT = 0;
  bool gotpTT = m_data->Stores["ANNIEEvent"]->Get("PrimaryTriggerTime", pTT);

  Log("ProcessedLAPPDFilter: Got ETT: " + std::to_string(ETT) + "(" + std::to_string(gotETT) + "), pTW: " + std::to_string(pTW) + "(" + std::to_string(gotpTW) + "), pTT: " + std::to_string(pTT) + "(" + std::to_string(gotpTT) + ")", 5, FilterVerbosity);

  m_data->Stores.at("ANNIEEvent")->Get("DataStreams", DataStreams);

  if (FilterVerbosity > 3)
  {
    for (auto it = DataStreams.begin(); it != DataStreams.end(); ++it)
    {
      cout << "DataStream: " << it->first << " " << it->second << ", ";
    }
    cout << endl;
  }
  CurrentExeNumber += 1;

  if (filterType == "BeamSelection")
  {

    if (CurrentExeNumber % 1000 == 0)
    {
      cout << "ProcessedLAPPDFilter: CurrentExeNumber: " << CurrentExeNumber << ", passCutEventNumber: " << passCutEventNumber << endl;
    }

    // require beam_good
    int beam_good = 0;
    bool gotBeamGood = m_data->Stores["ANNIEEvent"]->Get("beam_good", beam_good);
    if (!gotBeamGood || beam_good != 1)
    {
      Log("ProcessedLAPPDFilter: not good beam" + std::to_string(beam_good), 2, FilterVerbosity);
      return true;
    }
    // require there is PMT cluster
    std::map<double, std::vector<Hit>> *m_all_clusters = nullptr;
    bool get_clusters = m_data->CStore.Get("ClusterMap", m_all_clusters);
    if (!get_clusters)
    {
      std::cout << "ProcessedLAPPDFilter tool: No clusters!" << std::endl;
      return false;
    }
    if (!(static_cast<int>(m_all_clusters->size()) > 0))
    {
      Log("ProcessedLAPPDFilter: No PMT cluster found!", 4, FilterVerbosity);
      return true;
    }
    // require there is a cluster in the prompt window (2us)
    vector<double> clusterTimes;
    std::map<double, std::vector<Hit>>::iterator it_cluster_pair;
    it_cluster_pair = (*m_all_clusters).begin();
    bool loop_map = true;
    while (loop_map)
    {
      std::vector<Hit> cluster_hits = it_cluster_pair->second;
      double thisClusterTime = it_cluster_pair->first;
      clusterTimes.push_back(thisClusterTime);

      it_cluster_pair++;
      if (it_cluster_pair == (*m_all_clusters).end())
        loop_map = false;
    }
    // loop clusterTimes, if there is not any element < 2000, return true
    bool found = false;
    for (auto it = clusterTimes.begin(); it != clusterTimes.end(); ++it)
    {
      if (*it < 2000)
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      Log("ProcessedLAPPDFilter: No cluster in the prompt window!", 2, FilterVerbosity);
      return true;
    }

    // require the brightest cluster in the prompt window have charge balance < 0.2
    std::map<double, double> ClusterChargeBalances;
    std::map<double, double> ClusterMaxPEs;

    bool got_ccb = m_data->Stores.at("ANNIEEvent")->Get("ClusterChargeBalances", ClusterChargeBalances);
    bool got_cmpe = m_data->Stores.at("ANNIEEvent")->Get("ClusterMaxPEs", ClusterMaxPEs);

    double max_charge = 0;
    double max_charge_cluster_time = 0;
    double max_charge_cluster_CB = 1;
    for (auto it = ClusterMaxPEs.begin(); it != ClusterMaxPEs.end(); ++it)
    {
      double cluster_time = it->first;
      double charge_balance = ClusterChargeBalances.at(cluster_time);

      if (it->second > max_charge)
      {
        max_charge = it->second;
        max_charge_cluster_time = cluster_time;
        max_charge_cluster_CB = charge_balance;
      }
    }
    if (max_charge_cluster_CB > 0.2)
    {
      Log("ProcessedLAPPDFilter: The brightest cluster in the prompt window has charge balance > 0.2", 2, FilterVerbosity);
      return true;
    }

    // require there is a MRD track, and the MRD coincidence is true
    int numtracksinev = m_data->Stores["MRDTracks"]->Get("NumMrdTracks", numtracksinev);
    if (numtracksinev < 1)
    {
      Log("ProcessedLAPPDFilter: No MRD track!", 2, FilterVerbosity);
      return true;
    }

    bool pmtmrdcoinc;
    bool gotCoinc = m_data->Stores["RecoEvent"]->Get("PMTMRDCoinc", pmtmrdcoinc);
    if (!gotCoinc || !pmtmrdcoinc)
    {
      Log("ProcessedLAPPDFilter: No PMT MRD coincidence!", 2, FilterVerbosity);
      return true;
    }

    // require there is no veto
    if (RequireNoVeto)
    {
      bool noVeto = false;
      bool gotNoVeto = m_data->Stores.at("RecoEvent")->Get("NoVeto", noVeto);
      if (!gotNoVeto || !noVeto)
      {
        Log("ProcessedLAPPDFilter: There is veto!", 2, FilterVerbosity);
        return true;
      }
    }

    // require the Cherenkov light cover the center LAPPD
    if (RequireCherenkovCover)
    {
      vector<vector<int>> PMT_ChannelKeys = {{462, 428, 406, 412}};
      std::vector<Hit> brightestCluster = m_all_clusters->at(max_charge_cluster_time);
      vector<int> PMTKeys;
      int coveredPMT = 0;
      for (int i = 0; i < brightestCluster.size(); i++)
      {
        int channel_key = brightestCluster.at(i).GetTubeId();
        PMTKeys.push_back(channel_key);
        // check if channel_key is in PMT_ChannelKeys
        for (int j = 0; j < PMT_ChannelKeys.size(); j++)
        {
          if (std::find(PMT_ChannelKeys[j].begin(), PMT_ChannelKeys[j].end(), channel_key) != PMT_ChannelKeys[j].end())
          {
            coveredPMT += 1;
            break;
          }
        }
      }
      if (coveredPMT < 3)
      {
        Log("ProcessedLAPPDFilter: The Cherenkov light doesn't cover the center LAPPD!", 2, FilterVerbosity);
        return true;
      }
    }


    GotANNIEEventAndSave(FilteredPMTCluster, PMTClusterDataName);
    passCutEventNumber += 1;
  }

  if (filterType == "MRDtrack")
  {
    gotEventMRD = false;
    gotEventPMTCluster = false;

    std::map<double, std::vector<Hit>> *m_all_clusters = nullptr;
    bool get_clusters = m_data->CStore.Get("ClusterMap", m_all_clusters);
    if (!get_clusters)
    {
      std::cout << "ProcessedLAPPDFilter tool: No clusters found!" << std::endl;
      return false;
    }
    if ((int)m_all_clusters->size() > 0)
      gotEventPMTCluster = true;

    if (savePMTClusterEvents && gotEventPMTCluster)
    {
      GotANNIEEventAndSave(FilteredPMTCluster, PMTClusterDataName);
      PMTClusterEventNum += 1;
    }

    if (DataStreams["LAPPD"] == false)
    {
      return true;
    }

    

    std::map<uint64_t, uint64_t> LAPPDBeamgate_ns;
    m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns", LAPPDBeamgate_ns);
    PsecDataNumber += LAPPDBeamgate_ns.size();
    m_data->Stores["ANNIEEvent"]->Set("LAPPDBeamgate_ns", LAPPDBeamgate_ns);

    std::vector<std::vector<int>> MrdTimeClusters;
    bool get_clusters_mrd = m_data->CStore.Get("MrdTimeClusters", MrdTimeClusters);
    if (!get_clusters_mrd)
    {
      std::cout << "ProcessedLAPPDFilter tool: No MRD clusters found! Did you run the TimeClustering tool?" << std::endl;
      return false;
    }
    if (MrdTimeClusters.size() > 0)
      gotEventMRD = true;

    bool gotStoreNoVeto = m_data->Stores.at("RecoEvent")->Get("NoVeto", gotEventMRDNoVeto);
    if (!gotStoreNoVeto)
    {
      cout << "The Stage1 Filter doesn't work because it need the Veto information form EventSelector Tool. Please include that." << endl;
      return false;
    }

    if (saveAllLAPPDEvents)
    {
      GotANNIEEventAndSave(FilteredAllLAPPD, AllLAPPDDataName);
    }

    if (gotEventPMTCluster)
    {
      EventPMTClusterNumber += 1;
      if (FilterVerbosity > 0)
        cout << "Got an event with PMT cluster, got event number " << EventPMTClusterNumber << endl;
      if (gotEventMRD)
      {
        GotANNIEEventAndSave(FilteredMRD, MRDDataName);
        EventMRDNumber += 1;
        if (FilterVerbosity > 0)
          cout << "Got an event with PMT cluster, MRD hits, got event number " << EventMRDNumber << endl;
        if (gotEventMRDNoVeto)
        {
          GotANNIEEventAndSave(FilteredMRDNoVeto, MRDNoVetoDataName);
          EventMRDNoVetoNumber += 1;
          if (FilterVerbosity > 0)
            cout << "Got an event with PMT cluster, MRD hits and no veto, got event number " << EventMRDNoVetoNumber << endl;
        }
      }

      if (CurrentExeNumber % 10 == 0)
      {
        cout << "Filter event number: " << CurrentExeNumber << ", PsecDataNumber " << PsecDataNumber << ", LAPPD events with PMT clusters: " << EventPMTClusterNumber << ", also with MRD tracks: " << EventMRDNumber << ", also with MRD tracks and no veto: " << EventMRDNoVetoNumber << ", events with PMT cluster (may not have LAPPD): " << PMTClusterEventNum << endl;
      }

      m_data->Stores["ANNIEEvent"]->Get("RunNumber", currentRunNumber);
    }
  }

  if (filterType == "PMTCluster")
  {
    std::map<uint64_t, uint64_t> LAPPDBeamgate_ns;
    m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns", LAPPDBeamgate_ns);
    PsecDataNumber += LAPPDBeamgate_ns.size();

    gotEventPMTCluster = false;

    std::map<double, std::vector<Hit>> *m_all_clusters = nullptr;
    bool get_clusters = m_data->CStore.Get("ClusterMap", m_all_clusters);
    if (!get_clusters)
    {
      std::cout << "ProcessedLAPPDFilter tool: No clusters found!" << std::endl;
      return false;
    }
    if ((int)m_all_clusters->size() > 0)
      gotEventPMTCluster = true;

    if (gotEventPMTCluster)
    {
      EventPMTClusterNumber += 1;
      if (FilterVerbosity > 0)
        cout << "Got an event with PMT cluster, got event number " << EventPMTClusterNumber << endl;
      GotANNIEEventAndSave(FilteredMRD, MRDDataName);
      if (CurrentExeNumber % 50 == 0)
      {
        cout << "Filter event number: " << CurrentExeNumber << ", PsecDataNumber " << PsecDataNumber << ", events with PMT clusters: " << EventPMTClusterNumber << endl;
      }

      m_data->Stores["ANNIEEvent"]->Get("RunNumber", currentRunNumber);
    }
  }

  /*
    if (filterType == "LAPPDPulsedStripNumber")
    {
      CurrentExeNumber += 1;
      int pulsedStripNumber = 0;
      std::vector<std::vector<double>> LAPPDVectorSide0 = std::vector<std::vector<double>>(30, std::vector<double>(256));
      // loop the vector. For each strip, loop and find maximum, if the maximum > 15, then pulsedStripNumber += 1
      m_data->Stores["ANNIEEvent"]->Get("LAPPDSigVecSide0", LAPPDVectorSide0);
      for (int i = 0; i < 30; i++)
      {
        double max = 0;
        for (int j = 0; j < 256; j++)
        {
          if (LAPPDVectorSide0[i][j] > max)
            max = LAPPDVectorSide0[i][j];
          if (FilterVerbosity > requirePulsedStripNumber)
            cout << "LAPPDVectorSide0[" << i << "][" << j << "] = " << LAPPDVectorSide0[i][j] << endl;
        }
        if (max > requirePulsedAmp)
          pulsedStripNumber += 1;
      }
      if (pulsedStripNumber > requirePulsedStripNumber)
      {
        GotANNIEEventAndSave(FilteredMRD, MRDDataName);
        EventMRDNumber += 1;
      }

      if (CurrentExeNumber % 50 == 0)
      {
        cout << "Filter event number " << CurrentExeNumber << ", pulsedStripNumber more than " << requirePulsedStripNumber << " events " << EventMRDNumber << endl;
      }
    }
  */

  return true;
}

bool ProcessedLAPPDFilter::Finalise()
{
  FilteredMRD->Close();
  FilteredMRDNoVeto->Close();
  FilteredAllLAPPD->Close();
  FilteredPMTCluster->Close();

  FilteredMRD->Delete();
  FilteredMRDNoVeto->Delete();
  FilteredAllLAPPD->Delete();
  FilteredPMTCluster->Delete();

  if (filterType == "BeamSelection")
  {
    cout << " ProcessedLAPPDFilter: Finalise" << endl;
    cout << "Current exe number: " << CurrentExeNumber << endl;
    cout << "Got number of Event: " << passCutEventNumber << endl;
  }

  if (filterType == "MRDtrack")
  {
    cout << "Current Run " << currentRunNumber << endl;
    cout << "Filter got " << CurrentExeNumber << " events in total" << endl; // this is the total number of events
    cout << "Got " << PsecDataNumber << " psec data objects on all LAPPD" << endl;
    cout << "Got " << EventPMTClusterNumber << " events with PMT clusters" << endl;
    cout << "Got " << EventMRDNumber << " events with PMT clusters and MRD hits" << endl;
    cout << "Got " << EventMRDNoVetoNumber << " events with PMT clusters and MRD hits and no veto" << endl;
    cout << "Got " << PMTClusterEventNum << " events with PMT clusters and saved" << endl;
    cout << "Saved " << MRDDataName << " successfully" << endl;
    cout << "Saved " << MRDNoVetoDataName << " successfully" << endl;
    cout << "Saved " << AllLAPPDDataName << " successfully" << endl;
    cout << "Saved " << PMTClusterDataName << " successfully" << endl;

    std::ofstream file("FilterStat.txt", std::ios::app);
    file << currentRunNumber << " " << CurrentExeNumber << " " << EventPMTClusterNumber << " " << EventMRDNumber << " " << EventMRDNoVetoNumber << " "
         << "\n";
    file.close();
  }
  if (filterType == "PMTCluster")
  {
    cout << "Current Run " << currentRunNumber << endl;
    cout << "Filter got " << CurrentExeNumber << " events in total" << endl; // this is the total number of events
    cout << "Got " << EventPMTClusterNumber << " events with PMT clusters" << endl;
    cout << "Saved " << MRDDataName << " successfully" << endl;
  }
  if (filterType == "LAPPDTiming")
  {
    cout << "Filter got " << CurrentExeNumber << " events in total" << endl; // this is the total number of events
    cout << "Got " << EventMRDNumber << " events in beamgate window" << endl;
    cout << "Saved " << MRDDataName << " successfully" << endl;
  }

  if (filterType == "LAPPDPulsedStripNumber")
  {
    cout << "Filter got " << CurrentExeNumber << " events in total" << endl; // this is the total number of events
    cout << "Got " << EventMRDNumber << " events with more than 5 pulsed strips" << endl;
    cout << "Saved " << MRDDataName << " successfully" << endl;
  }

  return true;
}

bool ProcessedLAPPDFilter::GotANNIEEventAndSave(BoostStore *BS, string savePath)
{
  std::map<unsigned long, std::vector<Hit>> *AuxHitsOriginal = nullptr;
  std::map<unsigned long, std::vector<Hit>> *HitsOriginal = nullptr;
  m_data->Stores["ANNIEEvent"]->Get("AuxHits", AuxHitsOriginal);
  m_data->Stores["ANNIEEvent"]->Get("Hits", HitsOriginal);

  std::map<unsigned long, std::vector<Hit>> *AuxHitsCopy = nullptr;
  if (!m_data->Stores["ANNIEEvent"]->Get("AuxHits", AuxHitsOriginal))
  {
    std::cerr << "Warning: Could not retrieve AuxHits from ANNIEEvent or AuxHitsOriginal is nullptr. Creating empty AuxHits map." << std::endl;
    AuxHitsCopy = new std::map<unsigned long, std::vector<Hit>>();
  }
  else
  {
    AuxHitsCopy = new std::map<unsigned long, std::vector<Hit>>(*AuxHitsOriginal);
  }
  std::map<unsigned long, std::vector<Hit>> *HitsCopy = nullptr;
  if (!m_data->Stores["ANNIEEvent"]->Get("Hits", HitsOriginal))
  {
    std::cerr << "Warning: Could not retrieve Hits from ANNIEEvent or HitsOriginal is nullptr. Creating empty Hits map." << std::endl;
    HitsCopy = new std::map<unsigned long, std::vector<Hit>>();
  }
  else
  {
    HitsCopy = new std::map<unsigned long, std::vector<Hit>>(*HitsOriginal);
  }

  BS->Set("AuxHits", AuxHitsCopy);
  BS->Set("Hits", HitsCopy);

  std::map<unsigned long, std::vector<Hit>> TDCData;
  BeamStatus BeamStatus;
  uint64_t CTCTimestamp, RunStartTime;
  std::map<std::string, bool> DataStreams;
  uint32_t EventNumber, TriggerWord;
  uint64_t EventTimeTank;
  TimeClass EventTimeMRD;
  std::map<std::string, int> MRDLoopbackTDC;
  std::string MRDTriggerType;
  std::map<unsigned long, std::vector<int>> RawAcqSize;
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> RecoADCData, RecoAuxADCData;
  int PartNumber, RunNumber, RunType, SubrunNumber, TriggerExtended;
  TriggerClass TriggerData;
  m_data->Stores["ANNIEEvent"]->Get("TDCData", TDCData);
  BS->Set("TDCData", TDCData);
  m_data->Stores["ANNIEEvent"]->Get("DataStreams", DataStreams);
  BS->Set("DataStreams", DataStreams);
  m_data->Stores["ANNIEEvent"]->Get("BeamStatus", BeamStatus);
  BS->Set("BeamStatus", BeamStatus);
  m_data->Stores["ANNIEEvent"]->Get("RunStartTime", RunStartTime);
  BS->Set("RunStartTime", RunStartTime);
  m_data->Stores["ANNIEEvent"]->Get("EventNumber", EventNumber);
  BS->Set("EventNumber", EventNumber);
  m_data->Stores["ANNIEEvent"]->Get("TriggerWord", TriggerWord);
  BS->Set("TriggerWord", TriggerWord);
  m_data->Stores["ANNIEEvent"]->Get("EventTimeMRD", EventTimeMRD);
  BS->Set("EventTimeMRD", EventTimeMRD);

  m_data->Stores["ANNIEEvent"]->Get("EventTimeTank", EventTimeTank);
  Log("ProcessedLAPPDFilter: Got EventTimeTank: " + std::to_string(EventTimeTank), 1, FilterVerbosity);
  BS->Set("EventTimeTank", EventTimeTank);
  Log("ProcessedLAPPDFilter: Set EventTimeTank: " + std::to_string(EventTimeTank), 1, FilterVerbosity);

  m_data->Stores["ANNIEEvent"]->Get("MRDLoopbackTDC", MRDLoopbackTDC);
  BS->Set("MRDLoopbackTDC", MRDLoopbackTDC);
  m_data->Stores["ANNIEEvent"]->Get("MRDTriggerType", MRDTriggerType);
  BS->Set("MRDTriggerType", MRDTriggerType);
  m_data->Stores["ANNIEEvent"]->Get("RawAcqSize", RawAcqSize);
  BS->Set("RawAcqSize", RawAcqSize);
  m_data->Stores["ANNIEEvent"]->Get("RecoADCData", RecoADCData);
  BS->Set("RecoADCData", RecoADCData);
  m_data->Stores["ANNIEEvent"]->Get("RecoAuxADCData", RecoAuxADCData);
  BS->Set("RecoAuxADCData", RecoAuxADCData);
  m_data->Stores["ANNIEEvent"]->Get("PartNumber", PartNumber);
  BS->Set("PartNumber", PartNumber);
  m_data->Stores["ANNIEEvent"]->Get("RunNumber", RunNumber);
  BS->Set("RunNumber", RunNumber);
  m_data->Stores["ANNIEEvent"]->Get("RunType", RunType);
  BS->Set("RunType", RunType);
  m_data->Stores["ANNIEEvent"]->Get("SubrunNumber", SubrunNumber);
  BS->Set("SubrunNumber", SubrunNumber);
  m_data->Stores["ANNIEEvent"]->Get("TriggerExtended", TriggerExtended);
  BS->Set("TriggerExtended", TriggerExtended);
  m_data->Stores["ANNIEEvent"]->Get("TriggerData", TriggerData);
  BS->Set("TriggerData", TriggerData);
  m_data->Stores["ANNIEEvent"]->Get("CTCTimestamp", CTCTimestamp);
  BS->Set("CTCTimestamp", CTCTimestamp);

  std::map<uint64_t, uint32_t> GroupedTrigger;
  m_data->Stores["ANNIEEvent"]->Get("GroupedTrigger", GroupedTrigger);
  BS->Set("GroupedTrigger", GroupedTrigger);

  int PrimaryTriggerWord;
  m_data->Stores["ANNIEEvent"]->Get("PrimaryTriggerWord", PrimaryTriggerWord);
  BS->Set("PrimaryTriggerWord", PrimaryTriggerWord);

  uint64_t primaryTrigTime;
  m_data->Stores["ANNIEEvent"]->Get("PrimaryTriggerTime", primaryTrigTime);
  BS->Set("PrimaryTriggerTime", primaryTrigTime);

  bool NCExtended, CCExtended;
  m_data->Stores["ANNIEEvent"]->Get("NCExtended", NCExtended);
  BS->Set("NCExtended", NCExtended);
  m_data->Stores["ANNIEEvent"]->Get("CCExtended", CCExtended);
  BS->Set("CCExtended", CCExtended);

  string TriggerType;
  m_data->Stores["ANNIEEvent"]->Get("TriggerType", TriggerType);
  BS->Set("TriggerType", TriggerType);

  int CTCWordExtended;
  m_data->Stores["ANNIEEvent"]->Get("CTCWordExtended", CTCWordExtended);
  BS->Set("CTCWordExtended", CTCWordExtended);

  std::map<uint64_t, PsecData> LAPPDDataMap;
  std::map<uint64_t, uint64_t> LAPPDBeamgate_ns;
  std::map<uint64_t, uint64_t> LAPPDTimeStamps_ns; // data and key are the same
  std::map<uint64_t, uint64_t> LAPPDTimeStampsRaw;
  std::map<uint64_t, uint64_t> LAPPDBeamgatesRaw;
  std::map<uint64_t, uint64_t> LAPPDOffsets;
  std::map<uint64_t, int> LAPPDTSCorrection;
  std::map<uint64_t, int> LAPPDBGCorrection;
  std::map<uint64_t, int> LAPPDOSInMinusPS;

  m_data->Stores["ANNIEEvent"]->Get("LAPPDDataMap", LAPPDDataMap);
  BS->Set("LAPPDDataMap", LAPPDDataMap);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns", LAPPDBeamgate_ns);
  BS->Set("LAPPDBeamgate_ns", LAPPDBeamgate_ns);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStamps_ns", LAPPDTimeStamps_ns);
  BS->Set("LAPPDTimeStamps_ns", LAPPDTimeStamps_ns);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStampsRaw", LAPPDTimeStampsRaw);
  BS->Set("LAPPDTimeStampsRaw", LAPPDTimeStampsRaw);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgatesRaw", LAPPDBeamgatesRaw);
  BS->Set("LAPPDBeamgatesRaw", LAPPDBeamgatesRaw);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDOffsets", LAPPDOffsets);
  BS->Set("LAPPDOffsets", LAPPDOffsets);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTSCorrection", LAPPDTSCorrection);
  BS->Set("LAPPDTSCorrection", LAPPDTSCorrection);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBGCorrection", LAPPDBGCorrection);
  BS->Set("LAPPDBGCorrection", LAPPDBGCorrection);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDOSInMinusPS", LAPPDOSInMinusPS);
  BS->Set("LAPPDOSInMinusPS", LAPPDOSInMinusPS);

  uint64_t beamInfoTime = 0;
  int64_t timeDiff = -9999;
  double defaultVal = -9999.;
  int beam_good = 0;
  double E_TOR860, E_TOR875, THCURR, BTJT2, HP875, VP875, HPTG1, VPTG1, HPTG2, VPTG2, BTH2T2;
  double B_BRRMPL, B_BRRMPQ, B_BRRMPS, B_BRRMP; 
  int bunch_rotation_on;
  m_data->Stores["ANNIEEvent"]->Get("BeamInfoTime", beamInfoTime);
  BS->Set("BeamInfoTime", beamInfoTime);
  m_data->Stores["ANNIEEvent"]->Get("BeamInfoTimeToTriggerDiff", timeDiff);
  BS->Set("BeamInfoTimeToTriggerDiff", timeDiff);
  m_data->Stores["ANNIEEvent"]->Get("beam_E_TOR860", E_TOR860);
  BS->Set("beam_E_TOR860", E_TOR860);
  m_data->Stores["ANNIEEvent"]->Get("beam_E_TOR875", E_TOR875);
  BS->Set("beam_E_TOR875", E_TOR875);
  m_data->Stores["ANNIEEvent"]->Get("beam_THCURR", THCURR);
  BS->Set("beam_THCURR", THCURR);
  m_data->Stores["ANNIEEvent"]->Get("beam_BTJT2", BTJT2);
  BS->Set("beam_BTJT2", BTJT2);
  m_data->Stores["ANNIEEvent"]->Get("beam_HP875", HP875);
  BS->Set("beam_HP875", HP875);
  m_data->Stores["ANNIEEvent"]->Get("beam_VP875", VP875);
  BS->Set("beam_VP875", VP875);
  m_data->Stores["ANNIEEvent"]->Get("beam_HPTG1", HPTG1);
  BS->Set("beam_HPTG1", HPTG1);
  m_data->Stores["ANNIEEvent"]->Get("beam_VPTG1", VPTG1);
  BS->Set("beam_VPTG1", VPTG1);
  m_data->Stores["ANNIEEvent"]->Get("beam_HPTG2", HPTG2);
  BS->Set("beam_HPTG2", HPTG2);
  m_data->Stores["ANNIEEvent"]->Get("beam_VPTG2", VPTG2);
  BS->Set("beam_VPTG2", VPTG2);
  m_data->Stores["ANNIEEvent"]->Get("beam_BTH2T2", BTH2T2);
  BS->Set("beam_BTH2T2", BTH2T2);
  m_data->Stores["ANNIEEvent"]->Get("beam_B_BRRMPL", B_BRRMPL);
  BS->Set("beam_B_BRRMPL", B_BRRMPL);
  m_data->Stores["ANNIEEvent"]->Get("beam_B_BRRMPQ", B_BRRMPQ);
  BS->Set("beam_B_BRRMPQ", B_BRRMPQ);
  m_data->Stores["ANNIEEvent"]->Get("beam_B_BRRMPS", B_BRRMPS);
  BS->Set("beam_B_BRRMPS", B_BRRMPS);
  m_data->Stores["ANNIEEvent"]->Get("beam_B_BRRMP", B_BRRMP);
  BS->Set("beam_B_BRRMP", B_BRRMP);
  m_data->Stores["ANNIEEvent"]->Get("bunch_rotation_on", bunch_rotation_on);
  BS->Set("bunch_rotation_on", bunch_rotation_on);
  m_data->Stores["ANNIEEvent"]->Get("beam_good", beam_good);
  BS->Set("beam_good", beam_good);

  std::vector<uint16_t> RWMRawWaveform;
  std::vector<uint16_t> BRFRawWaveform;

  m_data->Stores["ANNIEEvent"]->Get("RWMRawWaveform", RWMRawWaveform);
  BS->Set("RWMRawWaveform", RWMRawWaveform);
  m_data->Stores["ANNIEEvent"]->Get("BRFRawWaveform", BRFRawWaveform);
  BS->Set("BRFRawWaveform", BRFRawWaveform);

  std::map<uint64_t, uint64_t> LAPPDBG_PPSBefore;
  std::map<uint64_t, uint64_t> LAPPDBG_PPSAfter;
  std::map<uint64_t, uint64_t> LAPPDBG_PPSDiff;
  std::map<uint64_t, int> LAPPDBG_PPSMissing;
  std::map<uint64_t, uint64_t> LAPPDTS_PPSBefore;
  std::map<uint64_t, uint64_t> LAPPDTS_PPSAfter;
  std::map<uint64_t, uint64_t> LAPPDTS_PPSDiff;
  std::map<uint64_t, int> LAPPDTS_PPSMissing;

  m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSBefore", LAPPDBG_PPSBefore);
  BS->Set("LAPPDBG_PPSBefore", LAPPDBG_PPSBefore);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSAfter", LAPPDBG_PPSAfter);
  BS->Set("LAPPDBG_PPSAfter", LAPPDBG_PPSAfter);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSDiff", LAPPDBG_PPSDiff);
  BS->Set("LAPPDBG_PPSDiff", LAPPDBG_PPSDiff);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSMissing", LAPPDBG_PPSMissing);
  BS->Set("LAPPDBG_PPSMissing", LAPPDBG_PPSMissing);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSBefore", LAPPDTS_PPSBefore);
  BS->Set("LAPPDTS_PPSBefore", LAPPDTS_PPSBefore);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSAfter", LAPPDTS_PPSAfter);
  BS->Set("LAPPDTS_PPSAfter", LAPPDTS_PPSAfter);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSDiff", LAPPDTS_PPSDiff);
  BS->Set("LAPPDTS_PPSDiff", LAPPDTS_PPSDiff);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSMissing", LAPPDTS_PPSMissing);
  BS->Set("LAPPDTS_PPSMissing", LAPPDTS_PPSMissing);

  BS->Save(savePath);
  if (FilterVerbosity > 2)
    cout << "Saved to " << savePath << " successfully" << endl;
  BS->Delete();

  // removd and clean the data pointers from here
  /* std::map<unsigned long, std::vector<Hit>> *AuxHitsOriginal = nullptr;
 std::map<unsigned long, std::vector<Hit>> *HitsOriginal = nullptr;
 m_data->Stores["ANNIEEvent"]->Get("AuxHits", AuxHitsOriginal);
 m_data->Stores["ANNIEEvent"]->Get("Hits", HitsOriginal);
 auto AuxHitsCopy = new std::map<unsigned long, std::vector<Hit>>(*AuxHitsOriginal);
 auto HitsCopy = new std::map<unsigned long, std::vector<Hit>>(*HitsOriginal);
 BS->Set("AuxHits", AuxHitsCopy);
 BS->Set("Hits", HitsCopy);

  AuxHitsCopy->clear();
  HitsCopy->clear();
  delete AuxHitsCopy;
  delete HitsCopy;*/

  return true;
}