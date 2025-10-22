#include <vector>

#include "ANNIEconstants.h"
#include "PrintADCTraces.h"

#include "TGraph.h"

PrintADCTraces::PrintADCTraces():Tool(){}

// Tool will loop over hits, extract ADC traces from the RecoADCData, and output those traces to a root file

// output root file structure
// ROOT file:
// ├── chankey/                # directory for each PMT channel
// ├── 332/
// │   ├── wf1 TGraph          # for each pulse, a TGraph
// │   └── wf2 TGraph
// ├── 463/
// │   ├── wf1 TGraph
// │   └── ...
// └── TraceSummary TTree      # metadata
//     ├── chan                # all pulse channel ids
//     ├── run                 # all pulse run numbers 
//     ├── eventTime           # all pulse event times
//     ├── hitT                # all pulse hit times [ns]
//     ├── hitPE               # all pulse hit charges [pe]
//     ├── hitBaseline         # all pulse baselines [adc]
//     └── hitNoise            # all pulse baseline sigma (noise) [adc]


bool PrintADCTraces::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // get config variables

  bool gotVerbosity = m_variables.Get("verbosity", verbosity);
  if (!gotVerbosity) verbosity = 1;

  // range of hit charges to save
  gotPEmin = m_variables.Get("hitPE_min", fhitPEmin);
  if (!gotPEmin) {
    logmessage = "PrintADCTraces: hitPE_min not defined. No selection will be imposed.";
    Log(logmessage, v_error, verbosity);
  } 
  gotPEmax = m_variables.Get("hitPE_max", fhitPEmax);
  if (!gotPEmax) {
    logmessage = "PrintADCTraces: hitPE_max not defined. No selection will be imposed.";
    Log(logmessage, v_error, verbosity);
  } 

  // save only hit times in a certain range (useful for laser peak or beam spill)
  gotTmin = m_variables.Get("hitT_min", fhitTmin);
  if (!gotTmin) {
    logmessage = "PrintADCTraces: hitT_min not defined. No selection will be imposed.";
    Log(logmessage, v_error, verbosity);
  } 
  gotTmax = m_variables.Get("hitT_max", fhitTmax);
  if (!gotTmax) {
    logmessage = "PrintADCTraces: hitT_max not defined. No selection will be imposed.";
    Log(logmessage, v_error, verbosity);
  } 

  // maximum number of total traces saved - this could explode depending on how many events we run over
  bool gotMaxTraces = m_variables.Get("MaxTraces", fMaxTraces);
  if (!gotMaxTraces) {
    logmessage = "PrintADCTraces: MaxTraces not defined. Using default of 10000.";
    Log(logmessage, v_error, verbosity);
    fMaxTraces = 10000;
  } 

  // per individual channel, how many traces do you want?
  bool gotMaxTracesPerChan = m_variables.Get("MaxTracesPerChannel", fMaxTracesPerChan);
  if (!gotMaxTracesPerChan) {
    logmessage = "PrintADCTraces: MaxTracesPerChannel not defined. Setting no limit on traces per channel, will default to filling according to MaxTraces argument.";
    Log(logmessage, v_error, verbosity);
    fMaxTracesPerChan = 0;
  } 

  // name of the output root file containing trace data
  bool gotOutputFilename = m_variables.Get("OutputFilename", filename);
  if (!gotOutputFilename) {
    logmessage = "PrintADCTraces: OutputFilename not defined. Setting output file name to: 'ADCTraces.root'";
    Log(logmessage, v_error, verbosity);
    filename = "ADCTraces.root";
  } 

  // set up output file
  fOutFile = new TFile(filename.c_str(), "RECREATE");

  // summary TTree containing global distribution of all saved ADC traces
  fTraceSummaryTree = new TTree("TraceSummary", "Summary of saved ADC traces");
  fTraceSummaryTree->Branch("chan", &fchan);
  fTraceSummaryTree->Branch("run", &frun);
  fTraceSummaryTree->Branch("eventTime", &feventTime);
  fTraceSummaryTree->Branch("hitT", &fhitT);
  fTraceSummaryTree->Branch("hitPE", &fhitPE);
  fTraceSummaryTree->Branch("hitBaseline", &fhitBaseline);
  fTraceSummaryTree->Branch("hitNoise", &fhitNoise);
  
  return true;
}

//------------------------------------------------------------------------------
bool PrintADCTraces::Execute()
{
    Log("PrintADCTraces: Execute()", v_debug, verbosity);

    // ***********************************
    // load stores

    bool goodAnnieEvent = m_data->Stores.count("ANNIEEvent");
    if (!goodAnnieEvent) {
        logmessage = "PrintADCTraces: no ANNIEEvent store! Aborting!";
        Log(logmessage, v_error, verbosity);
        return false;    
    }

    // need this for the nQ -> PE conversion
    bool gotSPEmap = m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap",fChannelKeyToSPEMap);
    if (!gotSPEmap) {
        logmessage = "PrintADCTraces: could not find ChannelNumToTankPMTSPEChargeMap! Aborting!";
        Log(logmessage, v_error, verbosity);
        return false;    
    }

    // ADCPulse class information (contains the traces)
    bool got_recoadc = m_data->Stores["ANNIEEvent"]->Get("RecoADCData",fRecoADCData);
    if (!got_recoadc) {
        logmessage = "PrintADCTraces: could not find RecoADCData! Aborting!";
        Log(logmessage, v_error, verbosity);
        return false;    
    }

    // TGraph title will be <run_number>_<p_file>_<eventtimetank>_<hitT>_<hitPE>
    int fRunNumber = 9999;
    int fPartFileNumber = 9999;
    uint64_t fWaveformTime = 0;

    bool gotRunNumber = m_data->Stores["ANNIEEvent"]->Get("RunNumber", fRunNumber);
    bool gotPartFile = m_data->Stores["ANNIEEvent"]->Get("PartNumber", fPartFileNumber);
    bool gotTimestamp = m_data->Stores["ANNIEEvent"]->Get("EventTimeTank", fWaveformTime);

    if (!gotPartFile || !gotTimestamp || !gotRunNumber) {
        logmessage = "PrintADCTraces: Error retrieving ";
        if (!gotRunNumber) logmessage += "RunNumber ";
        if (!gotPartFile) logmessage += "PartNumber ";
        if (!gotTimestamp) logmessage += "EventTimeTank ";
        logmessage += "from ANNIEEvent! Setting default (nonsense) values.";
        Log(logmessage, v_error, verbosity);
    }

    // ***********************************
    // write out ADC traces

    fOutFile->cd();  // go to root of output file

    // loop through RecoADCData and fill the graphs with the ADC traces
    for (auto& chan_pair : fRecoADCData) {
        unsigned long chankey = chan_pair.first;
        auto& minibuffers = chan_pair.second;

        int& chanGraphCount = graphsPerChannel[chankey];

        // make/get directory
        std::string chanStr = std::to_string(chankey);
        TDirectory* dir = fOutFile->GetDirectory(chanStr.c_str());
        if (!dir) dir = fOutFile->mkdir(chanStr.c_str());
        dir->cd();

        for (size_t mb_i = 0; mb_i < minibuffers.size(); ++mb_i) {

          // if the max trace limit is reached, stop looping over the minibuffers
          if ((fMaxTraces != 0 && totalGraphs >= fMaxTraces) ||
              (fMaxTracesPerChan != 0 && chanGraphCount >= fMaxTracesPerChan)) {
              break;
          }

          const auto& pulsevec = minibuffers.at(mb_i);

            for (const auto& pulse : pulsevec) {

                double hitT = pulse.peak_time();           // interpolated hit time [ns]
                double hitQ = pulse.charge();              // charge [nQ]
                double hitBaseline = pulse.baseline();     // baseline [adc]
                double hitNoise = pulse.sigma_baseline();  // noise [adc]

                // need to convert from nQ -> PE using SPE conversion map
                auto spe_it = fChannelKeyToSPEMap.find(chankey);
                double hitPE = -9999;
                if (spe_it != fChannelKeyToSPEMap.end() && spe_it->second > 0) {
                    hitPE = hitQ / spe_it->second;
                } else {
                    logmessage = "PrintADCTraces: Missing or invalid SPE value for channel " + std::to_string(chankey) +
                                ". Cannot convert to PE. Using placeholder -9999.";
                    Log(logmessage, v_warning, verbosity);
                }

                // apply hitT and hitPE cuts (skip pulse if it doesn't pass)
                bool passTime = (!gotTmin || hitT >= fhitTmin) && (!gotTmax || hitT <= fhitTmax);
                bool passPE   = (!gotPEmin || hitPE >= fhitPEmin) && (!gotPEmax || hitPE <= fhitPEmax);
                if (!passTime || !passPE) continue;

                // construct TGraph title: <run_number>_<p_file>_<eventtimetank>_<hitT>_<hitPE>
                std::stringstream grTitle;
                grTitle << fRunNumber << "_" << fPartFileNumber << "_" << fWaveformTime
                        << "_" << std::fixed << std::setprecision(2) << hitT
                        << "_" << std::fixed << std::setprecision(2) << hitPE;

                // grab trace
                const auto& xpts = pulse.GetTraceXPoints();
                const auto& ypts = pulse.GetTraceYPoints();

                if (xpts.empty() || ypts.empty()) continue;

                // create and write to TGraph
                TGraph gr(xpts.size(), xpts.data(), ypts.data());
                gr.SetName(grTitle.str().c_str());
                gr.SetTitle(grTitle.str().c_str());
                gr.Write();

                // write to summary TTree
                fchan = chankey;
                frun = fRunNumber;
                feventTime = fWaveformTime;
                fhitT = hitT;
                fhitPE = hitPE;
                fhitBaseline = hitBaseline;
                fhitNoise = hitNoise;
                fTraceSummaryTree->Fill();

                ++totalGraphs;
                ++chanGraphCount;
            }
        }
    }

    logmessage = "PrintADCTraces: Total graphs written this event: " + std::to_string(totalGraphs);
    Log(logmessage, v_debug, verbosity);

    return true;
}

//------------------------------------------------------------------------------
bool PrintADCTraces::Finalise()
{
    if (fOutFile) {

        if (fTraceSummaryTree) {          // write summary to root file
            fOutFile->cd();
            fTraceSummaryTree->Write();
            fTraceSummaryTree = nullptr;
        }

        fOutFile->cd();
        fOutFile->Write();   // write all contents to file
        fOutFile->Close();   // close the ROOT file
        delete fOutFile;     // clean up the pointer
        fOutFile = nullptr;
    }

    Log("PrintADCTraces: Finished and closed output ROOT file.", v_debug, verbosity);
    
    logmessage = "PrintADCTraces: Total graphs written across all events: " + std::to_string(totalGraphs);
    Log(logmessage, v_debug, verbosity);
    for (const auto& entry : graphsPerChannel) {
        logmessage = "  Channel " + std::to_string(entry.first) +
                    " → " + std::to_string(entry.second) + " graphs";
        Log(logmessage, v_debug, verbosity);
    }

    return true;
}

//------------------------------------------------------------------------------

// done
