#include "LAPPDThresReco.h"

LAPPDThresReco::LAPPDThresReco() : Tool() {}

bool LAPPDThresReco::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // Control variables
  // Control variables used in this tool
  // Control variables that you get from the config file, for this tool
  LAPPDThresRecoVerbosity = 0;
  m_variables.Get("LAPPDThresRecoVerbosity", LAPPDThresRecoVerbosity);
  threshold = 15;
  m_variables.Get("threshold", threshold);
  minPulseWidth = 3; // number of bins require for a pulse
  m_variables.Get("minPulseWidth", minPulseWidth);
  printHitsTXT = 1; // 1 print hit information to a txt file
  m_variables.Get("printHitsTXT", printHitsTXT);
  useMaxTime = true; // use the max amplitude time of the pulse as the time of the hit
  m_variables.Get("useMaxTime", useMaxTime);
  signalSpeedOnStrip = 0.567; // speed of the signal on the strip in fraction of speed of light, 0.567 was previous measured data
  m_variables.Get("signalSpeedOnStrip", signalSpeedOnStrip);
  triggerBoardDelay = 0; // arbitrary added to include the electronic delay between two ACDC boards
  m_variables.Get("triggerBoardDelay", triggerBoardDelay);
  savePositionOnStrip = true; // save the position of the hit on the strip, for event display purpose
  m_variables.Get("savePositionOnStrip", savePositionOnStrip);
  useRange = -1; // use the pulse time for hit time, rather than the reconstructed hit time. -1 averaged peak time, 0 low range, 1 high range
  m_variables.Get("useRange", useRange);
  loadPrintMRDinfo = true; // print MRD track information
  m_variables.Get("loadPrintMRDinfo", loadPrintMRDinfo);
  pulseFollowTimeStandard = 20; // 20 bins (2 ns) integral after each pulse
  m_variables.Get("pulseFollowTimeStandard", pulseFollowTimeStandard);
  baselineStart = 50; // start time for independent baseline calculation, 5ns
  m_variables.Get("baselineStart", baselineStart);
  baselineEnd = 100;
  m_variables.Get("baselineEnd", baselineEnd);
  // Control variables in this tool, initialized in this tool
  eventNumber = 0;
  LoadLAPPDMapInfo = false;
  m_variables.Get("LoadLAPPDMapInfo", LoadLAPPDMapInfo);

  // Global Control variables that you get from the config file
  ThresRecoInputWaveLabel = "LAPPDWave";
  m_variables.Get("ThresRecoInputWaveLabel", ThresRecoInputWaveLabel);
  ThresRecoOutputPulseLabel = "LAPPDPulse";
  m_variables.Get("ThresRecoOutputPulseLabel", ThresRecoOutputPulseLabel);
  ThresRecoOutputHitLabel = "LAPPDHit";
  m_variables.Get("ThresRecoOutputHitLabel", ThresRecoOutputHitLabel);
  // Global control variables/objects that you get from other tools
  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);
  if (LAPPDThresRecoVerbosity > 0)
    cout << "LAPPDThresReco: Geometry loaded as AnnieGeometry from Store [ANNIEEvent]" << endl;

  // Data variables
  // Data variables you get from other tools (it not initialized in execute)

  // Data variables you use in this tool
  if (printHitsTXT)
  {
    // recreate myfile
    myfile.open("LAPPDThresRecoHits.txt");
    myfile << "eventNumber"
           << "\t"
           << "thisEventHitNumber"
           << "\t"
           << "stripno"
           << "\t"
           << "ParallelToStripPos"
           << "\t"
           << "TransverseToStripPos"
           << "\t"
           << "HitArivTime"
           << "\t"
           << "HitAmp"
           << "\t"
           << "Pulse1LastTime"
           << "\t"
           << "Pulse2LastTime"
           << "\t"
           << "Pulse1StartTime"
           << "\t"
           << "Pulse2StartTime" << endl;
  }
  if (loadPrintMRDinfo)
  {
    mrdfile.open("LAPPDThresRecoMRDinfo.txt");
    mrdfile << "eventNumber"
            << "\t"
            << "trackNumber"
            << "\t"
            << "trackAngle"
            << "\t"
            << "trackAngleError"
            << "\t"
            << "penetrationDepth"
            << "\t"
            << "trackLength"
            << "\t"
            << "entryPointRadius"
            << "\t"
            << "energyLoss"
            << "\t"
            << "energyLossError"
            << "\t"
            << "trackStartX"
            << "\t"
            << "trackStartY"
            << "\t"
            << "trackStartZ"
            << "\t"
            << "trackStopX"
            << "\t"
            << "trackStopY"
            << "\t"
            << "trackStopZ"
            << "\t"
            << "trackSide"
            << "\t"
            << "trackStop"
            << "\t"
            << "trackThrough"
            << "\t"
            << "fHtrackFitChi2"
            << "\t"
            << "fHtrackFitCov"
            << "\t"
            << "fVtrackFitChi2"
            << "\t"
            << "fVtrackFitCov"
            << "\t"
            << "fHtrackOrigin"
            << "\t"
            << "fHtrackOriginError"
            << "\t"
            << "fHtrackGradient"
            << "\t"
            << "fHtrackGradientError"
            << "\t"
            << "fVtrackOrigin"
            << "\t"
            << "fVtrackOriginError"
            << "\t"
            << "fVtrackGradient"
            << "\t"
            << "fVtrackGradientError"
            << "\t"
            << "particlePID" << endl;
  }

  return true;
}

bool LAPPDThresReco::Execute()
{
  CleanDataObjects();

  m_data->CStore.Get("LAPPDana", LAPPDana);
  if (!LAPPDana)
    return true;

  if (LoadLAPPDMapInfo)
  {
    m_data->Stores["ANNIEEvent"]->Get("LAPPD_IDs", LAPPD_IDs);
    // print
    /*cout << "ThresReco got LAPPD_IDs: ";
    for (int i = 0; i < LAPPD_IDs.size(); i++)
    {
      cout << LAPPD_IDs.at(i) << " ";
    }
    cout << endl;*/
  }
  else
  {
    // get current LAPPD_ID
    m_data->CStore.Get("LAPPD_ID", LAPPD_ID);
  }

  // get the input LAPPD Data waveform;
  m_data->Stores["ANNIEEvent"]->Get(ThresRecoInputWaveLabel, lappdData);

  if (LAPPDThresRecoVerbosity > 1)
    cout << "Start WaveformMaximaFinding" << endl;
  WaveformMaximaFinding(); // find the maxima of the waveforms
  if (LAPPDThresRecoVerbosity > 1)
    cout << "Start FillLAPPDPulse" << endl;
  FillLAPPDPulse(); // find pulses
  if (LAPPDThresRecoVerbosity > 1)
    cout << "Start FillLAPPDHit" << endl;
  FillLAPPDHit(); // find hits
  if (LAPPDThresRecoVerbosity > 1)
    cout << "Reco Finished, start filling" << endl;

  m_data->Stores["ANNIEEvent"]->Set(ThresRecoOutputPulseLabel, lappdPulses);
  m_data->Stores["ANNIEEvent"]->Set(ThresRecoOutputHitLabel, lappdHits);
  m_data->Stores["ANNIEEvent"]->Set("waveformMax", waveformMax);
  m_data->Stores["ANNIEEvent"]->Set("waveformRMS", waveformRMS);
  m_data->Stores["ANNIEEvent"]->Set("waveformMaxLast", waveformMaxLast);
  m_data->Stores["ANNIEEvent"]->Set("waveformMaxNearing", waveformMaxNearing);
  m_data->Stores["ANNIEEvent"]->Set("waveformMaxTimeBin", waveformMaxTimeBin);
  eventNumber++; // operation in this loop finished, increase the event number

  if (LAPPDThresRecoVerbosity > 1)
    cout << "Filling finished, printing to txt" << endl;
  if (printHitsTXT)
    PrintHitsToTXT(); // print hits to a txt file
  if (loadPrintMRDinfo)
  {
    bool gotMRDdata = m_data->CStore.Get("MrdTimeClusters", MrdTimeClusters);
    if (gotMRDdata)
      PrintMRDinfoToTXT();
  }

  return true;
}

void LAPPDThresReco::CleanDataObjects()
{

  lappdData.clear();
  lappdPulses.clear();
  lappdHits.clear();
  MrdTimeClusters.clear();
  waveformRMS.clear();
  waveformMax.clear();
  waveformMaxLast.clear();
  waveformMaxNearing.clear();

  LAPPD_ID = -9999;
  LAPPDana = false;
  pulseFollowTime = pulseFollowTimeStandard;
}

void LAPPDThresReco::FillLAPPDPulse()
{

  // loop over the data and fine pulses
  std::map<unsigned long, vector<Waveform<double>>>::iterator it;
  for (it = lappdData.begin(); it != lappdData.end(); it++)
  {

    // get the waveforms from channel number
    unsigned long channel = it->first;
    channel = channel % 1000 + 1000;
    if ((channel % 1000) % 30 == 5)
      continue;
    Waveform<double> waveforms = it->second.at(0);
    vector<double> wav = *waveforms.GetSamples();
    vector<double> wave = wav;
    if (wave.size() != 256)
    {
      cout << "FillLAPPDPulse: Found a bug waveform at channel " << channel << ", size is " << wave.size() << endl;
      continue;
    }
    if (LAPPDThresRecoVerbosity > 2)
      cout << "FillLAPPDPulse: Found waveform at channel " << channel << ", size is " << wave.size() << endl;
    // flip the waveform, so that the signal is positive
    for (int i = 0; i < wave.size(); i++)
    {
      wave.at(i) = -wave.at(i);
    }

    if (LoadLAPPDMapInfo)
      LAPPD_ID = static_cast<int>((channel - 1000) / 60);

    // cout << "FillLAPPDPulse LAPPD_ID: " << LAPPD_ID << " channel: " << channel << endl;

    // for this channel, find the pulses
    vector<LAPPDPulse> pulses = FindPulses(wave, LAPPD_ID, channel);

    Channel *chan = _geom->GetChannel(channel);
    int stripno = chan->GetStripNum();
    int stripSide = chan->GetStripSide();

    // check, in lappdPulses, is there an element with the strip no, if not, create one vector with lenth 2, set the pulses to element with index stripSide
    if (lappdPulses.find(stripno) == lappdPulses.end())
    {
      vector<vector<LAPPDPulse>> stripPulses;
      stripPulses.resize(2);
      stripPulses.at(stripSide) = pulses;
      lappdPulses[stripno] = stripPulses;
    }
    else
    {
      lappdPulses[stripno].at(stripSide) = pulses;
    }
  }
}

void LAPPDThresReco::FillLAPPDHit()
{
  int numberOfHits = 0;
  std::map<unsigned long, vector<vector<LAPPDPulse>>>::iterator it2;
  for (it2 = lappdPulses.begin(); it2 != lappdPulses.end(); it2++)
  {
    unsigned long stripno = it2->first;
    vector<vector<LAPPDPulse>> pulses = it2->second;
    vector<LAPPDHit> lHits = FindHit(pulses);
    lappdHits[stripno] = lHits;
    numberOfHits += lHits.size();
  }
  if (LAPPDThresRecoVerbosity > 1)
    cout << "LAPPDThresReco found hits: " << numberOfHits << endl;
}

void LAPPDThresReco::PrintHitsToTXT()
{
  int thisEventHitNumber = 0;

  // print all hits in this event to a txt file, column was separated by tab
  // print: event number, hit number, hit strip number, hit loacl parallel position x, hit local transverse position y, hit time, hit amplitude
  std::map<unsigned long, vector<LAPPDHit>>::iterator it3;
  for (it3 = lappdHits.begin(); it3 != lappdHits.end(); it3++)
  {
    unsigned long stripno = it3->first;
    vector<LAPPDHit> lHits = it3->second;
    for (int i = 0; i < lHits.size(); i++)
    {
      LAPPDHit hit = lHits.at(i);
      vector<double> positionOnLAPPD = hit.GetLocalPosition();
      double pulse1StartTime = hit.GetPulse1StartTime();
      double pulse2StartTime = hit.GetPulse2StartTime();
      double pulse1LastTime = hit.GetPulse1LastTime();
      double pulse2LastTime = hit.GetPulse2LastTime();
      // print the data in this order:myfile<<eventNumber<<" "<<thisEventHitNumber<<" "<<stripno<<" "<<positionOnLAPPD.at(0)<<" "<<positionOnLAPPD.at(1)<<" "<<hit.GetTime()<<" "<<hit.GetCharge()<<endl; , but seperate by tab
      myfile << eventNumber << "\t" << thisEventHitNumber << "\t" << stripno << "\t" << positionOnLAPPD.at(0) << "\t" << positionOnLAPPD.at(1) << "\t" << hit.GetTime() << "\t" << hit.GetCharge() << "\t" << pulse1LastTime << "\t" << pulse2LastTime << "\t" << pulse1StartTime << "\t" << pulse2StartTime << endl;

      thisEventHitNumber++;
    }
  }
}

void LAPPDThresReco::PrintMRDinfoToTXT()
{
  int fNumClusterTracks = 0;
  for (int i = 0; i < (int)MrdTimeClusters.size(); i++)
    fNumClusterTracks += this->LoadMRDTrackReco(i);
  if (LAPPDThresRecoVerbosity > 1)
    cout << "LAPPDThresReco found MRD tracks: " << fNumClusterTracks << endl;
  for (int i = 0; i < fMRDTrackAngle.size(); i++)
  {
    mrdfile << eventNumber << "\t" << i << "\t" << fMRDTrackAngle.at(i) << "\t" << fMRDTrackAngleError.at(i) << "\t" << fMRDPenetrationDepth.at(i) << "\t" << fMRDTrackLength.at(i) << "\t" << fMRDEntryPointRadius.at(i) << "\t" << fMRDEnergyLoss.at(i) << "\t" << fMRDEnergyLossError.at(i) << "\t" << fMRDTrackStartX.at(i) << "\t" << fMRDTrackStartY.at(i) << "\t" << fMRDTrackStartZ.at(i) << "\t" << fMRDTrackStopX.at(i) << "\t" << fMRDTrackStopY.at(i) << "\t" << fMRDTrackStopZ.at(i) << "\t" << fMRDSide.at(i) << "\t" << fMRDStop.at(i) << "\t" << fMRDThrough.at(i) << "\t" << fHtrackFitChi2.at(i) << "\t" << fHtrackFitCov.at(i) << "\t" << fVtrackFitChi2.at(i) << "\t" << fVtrackFitCov.at(i) << "\t" << fHtrackOrigin.at(i) << "\t" << fHtrackOriginError.at(i) << "\t" << fHtrackGradient.at(i) << "\t" << fHtrackGradientError.at(i) << "\t" << fVtrackOrigin.at(i) << "\t" << fVtrackOriginError.at(i) << "\t" << fVtrackGradient.at(i) << "\t" << fVtrackGradientError.at(i) << "\t" << fparticlePID.at(i) << endl;
  }
}

bool LAPPDThresReco::Finalise()
{

  // close txt file
  myfile.close();
  mrdfile.close();

  return true;
}

vector<LAPPDPulse> LAPPDThresReco::FindPulses(vector<double> wave, int LAPPD_ID, int channel)
{
  // the wave pass to here must be all positive, not the default negative signals
  std::vector<LAPPDPulse> pulses;

  // loop the waveform, find the pulses with amplitude larger than threshold and last for bin number > minPulseWidth
  bool inPulse = false;
  double currentSig = threshold;

  int pulseStart = 0;
  int pulseSize = 0;  // number of bins of the pulse
  double peakBin = 0; // which bin is the peak
  double peakAmp = 0; // the peak amplitude
  double Q = 0;

  vector<double> binNumbers;
  vector<double> amplitudes;

  double baselineGet = 0;
  for (int i = baselineStart; i < baselineEnd; i++)
  {
    baselineGet += wave.at(i);
  }
  baselineGet = baselineGet / (baselineEnd - baselineStart);

  for (int i = 1; i < wave.size(); i++)
  {
    currentSig = wave.at(i);
    if (wave.at(i) > threshold)
    { // if the signal is larger than threshold
      if (!inPulse)
      {
        inPulse = true;
        pulseStart = i - 1;
        pulseSize = 1;
        peakBin = i;
        peakAmp = currentSig;
        Q = currentSig / 50000. * (1e-10); // same as LAPPDFindPeak
      }
      else
      {
        if (inPulse)
        {
          pulseSize++;
          if (currentSig > peakAmp)
          {
            peakAmp = currentSig;
            peakBin = i;
          }
          Q += currentSig / 50000. * (1e-10);
        }
      }
      binNumbers.push_back(static_cast<double>(i));
      amplitudes.push_back(currentSig);
    }
    else
    { // if the signal is smaller than threshold
      if (inPulse)
      {
        inPulse = false; // exit the pulse
        if (pulseSize > minPulseWidth)
        { // if the pulse is long enough
          double peakBinGaus = GaussianFit(binNumbers, amplitudes);
          // use a linear interpolation to find the half peak time, from wave.at(pulseStart) to wave.at(peakBin), divide each bin 0.1 ns to 100 parts, 1ps per bin,find the bin number that the amplitude is half of the peakAmp
          double startBin = pulseStart;
          if (pulseStart > 10)
            startBin = pulseStart - 10;
          else
            startBin = 1;
          double targetAmp = peakAmp / 2;
          int stopBin = wave.size() - 1;
          if (peakBin < wave.size() - 1)
            stopBin = peakBin;

          double halfPeakBin = startBin;
          for (int j = startBin; j < stopBin; j++)
          {
            halfPeakBin = j - 1;
            if (wave.at(j) > targetAmp)
            {
              break;
            }
          }
          int halfPeak_ps = 0;
          double halfHeightAmp = 0;
          for (int j = 1; j < 100; j++)
          {
            double interpAmp = wave.at(halfPeakBin) + (wave.at(halfPeakBin + 1) - wave.at(halfPeakBin)) * (j / 100.0);
            if (interpAmp > targetAmp)
            {
              double prevInterAmp = wave.at(halfPeakBin) + (wave.at(halfPeakBin + 1) - wave.at(halfPeakBin)) * ((j - 1) / 100.0);
              if (abs(interpAmp - targetAmp) < abs(prevInterAmp - targetAmp))
              {
                halfPeak_ps = j;
                halfHeightAmp = interpAmp;
                break;
              }
              else
              {
                halfPeak_ps = j - 1;
                halfHeightAmp = prevInterAmp;
                break;
              }
            }
          }
          if (LAPPDThresRecoVerbosity > 0)
            cout << "LAPPDThresReco: thres: " << threshold << " thres Start: " << pulseStart << " pulseStart Amp: " << wave.at(pulseStart) << " halfPeakBin: " << halfPeakBin << " halfPeakBin Amp: " << wave.at(halfPeakBin) << " peakBin: " << peakBin << " peakBin Amp: " << wave.at(peakBin) << " half height time: " << (halfPeakBin * (25. / 256.) + halfPeak_ps * 0.001 * 25 / 25.6) << " halfHeightAmp: " << halfHeightAmp << endl;

          // from the peak bin, find half end time
          double halfEndBin = peakBin;
          for (int j = peakBin; j < wave.size() - 1; j++)
          {
            halfEndBin = j - 1;
            if (wave.at(j) < targetAmp)
            {
              break;
            }
          }
          int halfEnd_ps = 0;
          for (int j = 1; j < 100; j++)
          {
            double interpAmp = wave.at(halfEndBin) + (wave.at(halfEndBin + 1) - wave.at(halfEndBin)) * (j / 100.0);
            if (interpAmp < targetAmp)
            {
              double prevInterAmp = wave.at(halfEndBin) + (wave.at(halfEndBin + 1) - wave.at(halfEndBin)) * ((j - 1) / 100.0);
              if (abs(interpAmp - targetAmp) < abs(prevInterAmp - targetAmp))
              {
                halfEnd_ps = j;
                break;
              }
              else
              {
                halfEnd_ps = j - 1;
                break;
              }
            }
          }

          int startFollowTime = i;
          int stopFollowTime = i + static_cast<int>(pulseFollowTime);
          if (stopFollowTime > 256)
          {
            stopFollowTime = 256;
            pulseFollowTime = 256 - i;
          }

          double integralFollowCharge = 0;
          for (int j = startFollowTime; j < stopFollowTime; j++)
          {
            integralFollowCharge += wave.at(j);
          }

          if (LAPPDThresRecoVerbosity > 1)
            cout << "inserting pulse on LAPPD ID =" << LAPPD_ID << " at time: " << peakBin * (25. / 256.) << "(" << peakBinGaus << ") with peakAmp: " << peakAmp << " from " << pulseStart << " to " << pulseStart + pulseSize << endl;
          if (useMaxTime)
          {
            LAPPDPulse thisPulse(LAPPD_ID, channel, peakBin * (25. / 256.), Q, peakAmp, pulseStart, pulseStart + pulseSize);
            thisPulse.SetHalfHeightTime(halfPeakBin * (25. / 256.) + halfPeak_ps * 0.001 * 25 / 25.6);
            thisPulse.SetHalfEndTime(halfEndBin * (25. / 256.) + halfEnd_ps * 0.001 * 25 / 25.6);
            thisPulse.SetBaseline(baselineGet);
            thisPulse.SetPulseFollowTime(pulseFollowTime);
            thisPulse.SetPulseFollowCharge(integralFollowCharge);
            pulses.push_back(thisPulse);
          }
          else
          {
            LAPPDPulse thisPulse(LAPPD_ID, channel, peakBinGaus * (25. / 256.), Q, peakAmp, pulseStart, pulseStart + pulseSize);
            thisPulse.SetHalfHeightTime(halfPeakBin * (25. / 256.) + halfPeak_ps * 0.001 * 25 / 25.6);
            thisPulse.SetHalfEndTime(halfEndBin * (25. / 256.) + halfEnd_ps * 0.001 * 25 / 25.6);
            thisPulse.SetBaseline(baselineGet);
            thisPulse.SetPulseFollowTime(pulseFollowTime);
            thisPulse.SetPulseFollowCharge(integralFollowCharge);
            pulses.push_back(thisPulse);
          }
          // tube ID: LAPPD_ID
          // channel: this channel
          // peakBin: the bin number of the peak in the 256 bins (use 256 bins for 25 ns, might be wrong)
          // Q: charge of the pulse
          // peakAmp: the peak amplitude
          // pulseStart: the bin number of the start of the pulse
          // pulseStart+pulseSize: the bin number of the end of the pulse

          // clean the bin numbers and amplitudes vectors
          binNumbers.clear();
          amplitudes.clear();
        }
        else
        {
          pulseStart = 0;
          pulseSize = 0;
          peakAmp = currentSig;
          Q = 0;
        }
      }
    }
  }
  return pulses;
}

vector<LAPPDHit> LAPPDThresReco::FindHit(vector<vector<LAPPDPulse>> pulses)
{
  vector<LAPPDHit> lHits;

  // loop the pulses, find the pairs of pulses with the same strip number and different side, then do the reco
  vector<LAPPDPulse> side0 = pulses.at(0);
  vector<LAPPDPulse> side1 = pulses.at(1);
  if (side0.size() == 0 || side1.size() == 0)
    return lHits;

  // use the vector with less number of pulse to do reco, incase some noise pulses are inserted
  if (side0.size() < side1.size())
  { // if side 0 has fewer pulses
    for (int i = 0; i < side0.size(); i++)
    {
      if (side1.size() == 0)
        break;
      LAPPDPulse pulse0;
      LAPPDPulse pulse1;
      pulse0 = side0.at(i);
      int bestMatchIndex = -1;
      double prob = 0;
      for (int j = 0; j < side1.size(); j++)
      {
        pulse1 = side1.at(j);
        // if the higher pulse peak amplitude is larger than 50% of the lower one, don't match
        if ((pulse0.GetPeak() < pulse1.GetPeak() * 0.5) || (pulse0.GetPeak() * 0.5 > pulse1.GetPeak()))
          continue;

        // if the peak time of the pulse is 2ns away from the peak time of the other pulse, don't match
        if (abs(pulse1.GetTime() - pulse0.GetTime()) > 20)
          continue;

        // just use a simple inverse product as the prob, the pulse finding is very rough anyway
        double thisProb = 1 / (abs(pulse1.GetTime() - pulse0.GetTime()) * abs(pulse1.GetPeak() - pulse0.GetPeak()));
        if (thisProb > prob)
        {
          prob = thisProb;
          bestMatchIndex = j;
        }
      }
      // if found the bestMatch, pair to make a hit, if not, skip
      if (bestMatchIndex != -1)
      {
        pulse1 = side1.at(bestMatchIndex);
        pulse0 = side0.at(i);
        LAPPDHit hit = MakeHit(pulse0, pulse1);
        lHits.push_back(hit);
        side1.erase(side1.begin() + bestMatchIndex);
      }
    }
  }
  else
  { // if side 1 has fewer pulses
    for (int i = 0; i < side1.size(); i++)
    {
      if (side0.size() == 0)
        break;
      LAPPDPulse pulse0;
      LAPPDPulse pulse1;
      pulse1 = side1.at(i);
      int bestMatchIndex = -1;
      double prob = 0;
      for (int j = 0; j < side0.size(); j++)
      {
        pulse0 = side0.at(j);
        // if the higher pulse peak amplitude is larger than 50% of the lower one, don't match
        if ((pulse0.GetPeak() < pulse1.GetPeak() * 0.5) || (pulse0.GetPeak() * 0.5 > pulse1.GetPeak()))
          continue;

        // if the peak time of the pulse is 2ns away from the peak time of the other pulse, don't match
        if (abs(pulse1.GetTime() - pulse0.GetTime()) > 20)
          continue;

        // just use a simple inverse product as the prob, the pulse finding is very rough anyway
        double thisProb = 1 / (abs(pulse1.GetTime() - pulse0.GetTime()) * abs(pulse1.GetPeak() - pulse0.GetPeak()));
        if (thisProb > prob)
        {
          prob = thisProb;
          bestMatchIndex = j;
        }
      }
      // if found the bestMatch, pair to make a hit, if not, skip
      if (bestMatchIndex != -1)
      {
        pulse0 = side0.at(bestMatchIndex);
        pulse1 = side1.at(i);
        LAPPDHit hit = MakeHit(pulse0, pulse1);
        lHits.push_back(hit);
        side0.erase(side0.begin() + bestMatchIndex);
      }
    }
  }

  return lHits;
}

LAPPDHit LAPPDThresReco::MakeHit(LAPPDPulse pulse0, LAPPDPulse pulse1)
{
  // Always use pulse1 - pulse0
  double deltaT = pulse1.GetTime() - pulse0.GetTime();
  double averageAmp = (pulse1.GetPeak() + pulse0.GetPeak()) / 2;
  double averageQ = (pulse1.GetCharge() + pulse0.GetCharge()) / 2;

  if (LAPPDThresRecoVerbosity > 1)
  {
    cout << "Making Hit: " << endl;
    cout << "Pulse0: " << endl;
    cout << "Channel = " << pulse0.GetChannelID() << endl;
    pulse0.Print();
    cout << "Pulse1: " << endl;
    cout << "Channel = " << pulse1.GetChannelID() << endl;
    pulse1.Print();
    cout << "DeltaT: " << deltaT << endl;
    cout << "AverageAmp: " << averageAmp << endl;
    cout << "AverageQ: " << averageQ << endl;
  }
  Channel *chan0 = _geom->GetChannel(pulse0.GetChannelID());
  int stripno0 = chan0->GetStripNum();
  Channel *chan1 = _geom->GetChannel(pulse1.GetChannelID());
  int stripno1 = chan1->GetStripNum();
  if (stripno0 != stripno1)
  {
    if (LAPPDThresRecoVerbosity > 1)
      cout << "Error: the two pulses are not on the same strip!" << endl;
    LAPPDHit hit;
    return hit;
  }
  else
  {
    // double stripWidth = 4.62+2.29; //mm
    // double LAPPDEdgeWidth = 14.40s; //mm
    // double transversePos = LAPPDEdgeWidth + stripWidth*(stripno0-0.5);

    double stripWidth = 4.62 + 2.29;          // mm
    double LAPPDEdgeWidth = 14.40 + 4.62 / 2; // mm
    double transversePos = LAPPDEdgeWidth + stripWidth * (stripno0);

    // use the center position of the strip in mm
    if (LAPPDThresRecoVerbosity > 1)
    {
      cout << "making hit from pulse0 on channel " << pulse0.GetChannelID() << ", strip " << stripno0 << endl;
      cout << "making hit from pulse1 on channel " << pulse1.GetChannelID() << ", strip " << stripno1 << endl;
      cout << "transversePos: " << transversePos << endl;
    }
    double t0 = pulse0.GetTime() + triggerBoardDelay / 169.982;
    double t1 = pulse1.GetTime();

    // double parallelPos = (t0 - t1 + 1.14555)*0.5 *signalSpeedOnStrip * 100;
    double parallelPos = (t0 - t1 + 1.37724) * 0.5 * signalSpeedOnStrip * 100;
    // position from the edge of the start of pulse0 (side 0) in mm

    double averageTime = (t0 + t1) * 0.5;
    double averageTimeLow = (pulse0.GetLowRange() + pulse1.GetLowRange()) * 0.5;
    double averageTimeHi = (pulse0.GetHiRange() + pulse1.GetHiRange()) * 0.5;

    double arrivalTime = t0 - (t0 - t1 + 1) * 0.5;

    if (LAPPDThresRecoVerbosity > 1)
      cout << "Position on LAPPD: " << parallelPos << " " << transversePos << endl;

    vector<double> positionOnLAPPD = {parallelPos, transversePos};
    vector<double> positionInTank = {0 + transversePos / 1000 - 0.1, -0.2255, 2.951}; // need to get this from the geometry
    if (savePositionOnStrip)
    {
      if (parallelPos < 100 && parallelPos > -100)
        positionInTank[1] = -0.2255 + parallelPos / 1000;
    }

    int tubeID = pulse0.GetTubeId();
    double pulse1LastTime = pulse0.GetHiRange() - pulse0.GetLowRange();
    double pulse2LastTime = pulse1.GetHiRange() - pulse1.GetLowRange();
    double pulse1StartTime = pulse0.GetLowRange();
    double pulse2StartTime = pulse1.GetLowRange();
    // print tubeID
    if (LAPPDThresRecoVerbosity > 1)
    {
      cout << "TubeID: " << tubeID << endl;
      cout << "average charge is " << averageQ << ", q/amplitude is " << averageQ / averageAmp << endl;
      cout << "average amplitude is " << averageAmp << ", amplitude/7 is " << averageAmp / 7 << endl;
    }
    if (useRange == -1)
    {
      LAPPDHit hit(tubeID, averageTime, averageAmp, positionInTank, positionOnLAPPD, pulse1LastTime, pulse2LastTime, pulse1StartTime, pulse2StartTime, pulse0, pulse1);
      return hit;
    }
    else if (useRange == 0)
    {
      LAPPDHit hit(tubeID, averageTimeLow, averageAmp, positionInTank, positionOnLAPPD, pulse1LastTime, pulse2LastTime, pulse1StartTime, pulse2StartTime, pulse0, pulse1);
      return hit;
    }
    else if (useRange == 1)
    {
      LAPPDHit hit(tubeID, averageTimeHi, averageAmp, positionInTank, positionOnLAPPD, pulse1LastTime, pulse2LastTime, pulse1StartTime, pulse2StartTime, pulse0, pulse1);
      return hit;
    }
    else
    {
      cout << "Error: useRange should be -1, 0 or 1" << endl;
      return LAPPDHit();
    }

    //*********************
    // NOTE: we are actually using the average AMPLITUDE here, rather than the charge
    //*********************

    // return hit;
  }
}

double LAPPDThresReco::GaussianFit(const vector<double> &xData, const vector<double> &yData)
{
  TGraph graph(xData.size(), &xData[0], &yData[0]);
  TF1 f("gaus", "gaus", xData.front(), xData.back());
  graph.Fit(&f, "Q");       // "Q" for quiet mode, no output
  return f.GetParameter(1); // 返回高斯拟合的峰值位置
}

void LAPPDThresReco::CleanMRDRecoInfo()
{
  fMRDTrackAngle.clear();
  fMRDTrackAngleError.clear();
  fMRDPenetrationDepth.clear();
  fMRDTrackLength.clear();
  fMRDEntryPointRadius.clear();
  fMRDEnergyLoss.clear();
  fMRDEnergyLossError.clear();
  fMRDTrackStartX.clear();
  fMRDTrackStartY.clear();
  fMRDTrackStartZ.clear();
  fMRDTrackStopX.clear();
  fMRDTrackStopY.clear();
  fMRDTrackStopZ.clear();
  fMRDSide.clear();
  fMRDStop.clear();
  fMRDThrough.clear();
  fHtrackFitChi2.clear();
  fHtrackFitCov.clear();
  fVtrackFitChi2.clear();
  fVtrackFitCov.clear();
  fHtrackOrigin.clear();
  fHtrackOriginError.clear();
  fHtrackGradient.clear();
  fHtrackGradientError.clear();
  fVtrackOrigin.clear();
  fVtrackOriginError.clear();
  fVtrackGradient.clear();
  fVtrackGradientError.clear();
  fparticlePID.clear();
}

int LAPPDThresReco::LoadMRDTrackReco(int SubEventID)
{

  // Check for valid track criteria
  m_data->Stores["MRDTracks"]->Get("MRDTracks", theMrdTracks);
  m_data->Stores["MRDTracks"]->Get("NumMrdTracks", numtracksinev);
  // Loop over reconstructed tracks

  Position StartVertex;
  Position StopVertex;
  double TrackLength = -9999;
  double TrackAngle = -9999;
  double TrackAngleError = -9999;
  double PenetrationDepth = -9999;
  Position MrdEntryPoint;
  double EnergyLoss = -9999; // in MeV
  double EnergyLossError = -9999;
  double EntryPointRadius = -9999;
  bool IsMrdPenetrating;
  bool IsMrdStopped;
  bool IsMrdSideExit;
  double HtrackFitChi2 = -9999;
  double HtrackFitCov = -9999;
  double VtrackFitChi2 = -9999;
  double VtrackFitCov = -9999;
  double HtrackOrigin = -9999;
  double HtrackOriginError = -9999;
  double HtrackGradient = -9999;
  double HtrackGradientError = -9999;
  double VtrackOrigin = -9999;
  double VtrackOriginError = -9999;
  double VtrackGradient = -9999;
  double VtrackGradientError = -9999;
  int particlePID = -9999;

  int NumClusterTracks = 0;
  for (int tracki = 0; tracki < numtracksinev; tracki++)
  {
    BoostStore *thisTrackAsBoostStore = &(theMrdTracks->at(tracki));
    int TrackEventID = -1;
    // get track properties that are needed for the through-going muon selection
    thisTrackAsBoostStore->Get("MrdSubEventID", TrackEventID);
    if (TrackEventID != SubEventID)
      continue;

    // If we're here, this track is associated with this cluster
    thisTrackAsBoostStore->Get("StartVertex", StartVertex);
    thisTrackAsBoostStore->Get("StopVertex", StopVertex);
    thisTrackAsBoostStore->Get("TrackAngle", TrackAngle);
    thisTrackAsBoostStore->Get("TrackAngleError", TrackAngleError);
    thisTrackAsBoostStore->Get("PenetrationDepth", PenetrationDepth);
    thisTrackAsBoostStore->Get("MrdEntryPoint", MrdEntryPoint);
    thisTrackAsBoostStore->Get("EnergyLoss", EnergyLoss);
    thisTrackAsBoostStore->Get("EnergyLossError", EnergyLossError);
    thisTrackAsBoostStore->Get("IsMrdPenetrating", IsMrdPenetrating); // bool
    thisTrackAsBoostStore->Get("IsMrdStopped", IsMrdStopped);         // bool
    thisTrackAsBoostStore->Get("IsMrdSideExit", IsMrdSideExit);
    thisTrackAsBoostStore->Get("HtrackFitChi2", HtrackFitChi2);
    thisTrackAsBoostStore->Get("HtrackFitCov", HtrackFitCov);
    thisTrackAsBoostStore->Get("VtrackFitChi2", VtrackFitChi2);
    thisTrackAsBoostStore->Get("VtrackFitCov", VtrackFitCov);
    TrackLength = sqrt(pow((StopVertex.X() - StartVertex.X()), 2) + pow(StopVertex.Y() - StartVertex.Y(), 2) + pow(StopVertex.Z() - StartVertex.Z(), 2)) * 100.0;
    EntryPointRadius = sqrt(pow(MrdEntryPoint.X(), 2) + pow(MrdEntryPoint.Y(), 2)) * 100.0; // convert to cm
    PenetrationDepth = PenetrationDepth * 100.0;
    thisTrackAsBoostStore->Get("HtrackOrigin", HtrackOrigin);
    thisTrackAsBoostStore->Get("HtrackOriginError", HtrackOriginError);
    thisTrackAsBoostStore->Get("HtrackGradient", HtrackGradient);
    thisTrackAsBoostStore->Get("HtrackGradientError", HtrackGradientError);
    thisTrackAsBoostStore->Get("VtrackOrigin", VtrackOrigin);
    thisTrackAsBoostStore->Get("VtrackOriginError", VtrackOriginError);
    thisTrackAsBoostStore->Get("VtrackGradient", VtrackGradient);
    thisTrackAsBoostStore->Get("VtrackGradientError", VtrackGradientError);
    thisTrackAsBoostStore->Get("particlePID", particlePID);

    // Push back some properties
    fMRDTrackAngle.push_back(TrackAngle);
    fMRDTrackAngleError.push_back(TrackAngleError);
    fMRDTrackLength.push_back(TrackLength);
    fMRDPenetrationDepth.push_back(PenetrationDepth);
    fMRDEntryPointRadius.push_back(EntryPointRadius);
    fMRDEnergyLoss.push_back(EnergyLoss);
    fMRDEnergyLossError.push_back(EnergyLossError);
    fMRDTrackStartX.push_back(StartVertex.X());
    fMRDTrackStartY.push_back(StartVertex.Y());
    fMRDTrackStartZ.push_back(StartVertex.Z());
    fMRDTrackStopX.push_back(StopVertex.X());
    fMRDTrackStopY.push_back(StopVertex.Y());
    fMRDTrackStopZ.push_back(StopVertex.Z());
    fMRDStop.push_back(IsMrdStopped);
    fMRDSide.push_back(IsMrdSideExit);
    fMRDThrough.push_back(IsMrdPenetrating);
    fHtrackFitChi2.push_back(HtrackFitChi2);
    fHtrackFitCov.push_back(HtrackFitCov);
    fVtrackFitChi2.push_back(VtrackFitChi2);
    fVtrackFitCov.push_back(VtrackFitCov);
    fHtrackOrigin.push_back(HtrackOrigin);
    fHtrackOriginError.push_back(HtrackOriginError);
    fHtrackGradient.push_back(HtrackGradient);
    fHtrackGradientError.push_back(HtrackGradientError);
    fVtrackOrigin.push_back(VtrackOrigin);
    fVtrackOriginError.push_back(VtrackOriginError);
    fVtrackGradient.push_back(VtrackGradient);
    fVtrackGradientError.push_back(VtrackGradientError);
    fparticlePID.push_back(particlePID);
    NumClusterTracks += 1;
  }
  return NumClusterTracks;
}

void LAPPDThresReco::WaveformMaximaFinding()
{
  // loop the lappdData map, for each value, find the maxima and RMS, and put them into waveformMax and waveformRMS
  // use the strip number + 30* strip side as the key
  if (LAPPDThresRecoVerbosity > 0)
    cout << "WaveformMaximaFinding: Start finding maxima and RMS of the waveforms" << endl;

  std::map<unsigned long, vector<Waveform<double>>>::iterator it;
  for (it = lappdData.begin(); it != lappdData.end(); it++)
  {
    unsigned long channel = it->first;
    channel = channel % 1000 + 1000;
    if (channel == 1005 || channel == 1035)
      continue;
    Channel *chan = _geom->GetChannel(channel);
    int stripno = chan->GetStripNum();
    int stripSide = chan->GetStripSide();
    Waveform<double> waveforms = it->second.at(0);
    vector<double> wav = *waveforms.GetSamples();

    vector<double> wave = wav;
    if (wave.size() != 256)
    {
      cout << "WaveformMaximaFinding: Found a bug waveform at channel " << channel << ", size is " << wav.size() << endl;
      continue;
    }
    if (LAPPDThresRecoVerbosity > 2)
      cout << "WaveformMaximaFinding: Found a waveform at channel " << channel << ", size is " << wave.size() << ",side is " << stripSide << ", stripno is " << stripno << endl;
    for (int i = 0; i < wave.size(); i++)
    {
      wave.at(i) = -wave.at(i);
    }
    double max = wave.at(0);
    double rms = wave.at(0) * wave.at(0);
    bool maxIsLast = 0;
    double nearingMin = 0;
    int binOfMax = -1;
    for (int i = 1; i < wave.size() - 1; i++)
    {
      if (wave.at(i) > max)
      {
        if (wave.at(i + 1) > 0.8 * wave.at(i) && wave.at(i - 1) > 0.8 * wave.at(i))
        {
          max = wave.at(i);
          maxIsLast = 1;
          binOfMax = i;
        }
        if (wave.at(i + 1) > wave.at(i - 1))
        {
          nearingMin = wave.at(i - 1);
        }
        else
        {
          nearingMin = wave.at(i + 1);
        }
      }

      rms += (wave.at(i)) * (wave.at(i));
    }

    rms = sqrt(rms / wave.size());
    int LAPPD_ID = static_cast<int>((channel - 1000) / 60);
    int key = stripno + 30 * stripSide + LAPPD_ID * 60;
    if (waveformMax[key].size() == 0)
    {
      waveformMax[key].resize(2);
      waveformRMS[key].resize(2);
      waveformMaxLast[key].resize(2);
      waveformMaxNearing[key].resize(2);
      waveformMaxTimeBin[key].resize(2);
    }
    waveformMax[key].at(stripSide) = max;
    waveformRMS[key].at(stripSide) = rms;
    waveformMaxLast[key].at(stripSide) = maxIsLast;
    waveformMaxNearing[key].at(stripSide) = nearingMin;
    waveformMaxTimeBin[key].at(stripSide) = binOfMax;
  }
}
