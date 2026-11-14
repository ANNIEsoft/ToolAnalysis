#include "FitRWMWaveform.h"

FitRWMWaveform::FitRWMWaveform() : Tool() {}

bool FitRWMWaveform::Initialise(std::string configfile, DataModel &data)
{

  if (configfile != "")
    m_variables.Initialise(configfile);

  m_data = &data; 

  m_variables.Get("verbosityFitRWMWaveform", verbosityFitRWMWaveform);
  m_variables.Get("printToRootFile", printToRootFile);

  maxPrintNumber = 100;
  m_variables.Get("maxPrintNumber", maxPrintNumber);

  output_filename = "RWMBRFWaveforms.root";
  m_variables.Get("output_filename", output_filename);

  return true;
}

bool FitRWMWaveform::Execute()
{

  Log("FitRWMWaveform: Execute()", v_debug, verbosityFitRWMWaveform);
 
  RWMRawWaveform.clear();
  BRFRawWaveform.clear();
  BESRawWaveform.clear();

  m_data->Stores["ANNIEEvent"]->Get("RWMRawWaveform", RWMRawWaveform);
  m_data->Stores["ANNIEEvent"]->Get("BRFRawWaveform", BRFRawWaveform);
  m_data->Stores["ANNIEEvent"]->Get("BESRawWaveform", BESRawWaveform);

  uint64_t WaveformTime = 0;
  m_data->Stores["ANNIEEvent"]->Get("EventTimeTank", WaveformTime);

  if (printToRootFile && ToBePrintedRWMWaveforms.size() < maxPrintNumber)
  {

    ToBePrintedRWMWaveforms.emplace(WaveformTime, RWMRawWaveform);
    ToBePrintedBRFWaveforms.emplace(WaveformTime, BRFRawWaveform);
    ToBePrintedBESWaveforms.emplace(WaveformTime, BESRawWaveform);

    Log("FitRWMWaveform: Execute(): Added RWM and BRF waveforms to be printed to root file", v_debug, verbosityFitRWMWaveform);
  }

  RWMRisingStart = 0;
  RWMRisingEnd = 0;
  RWMHalfRising = 0;
  RWMFHWM = 0;
  RWMFirstPeak = 0;

  BRFFirstPeak = 0;
  BRFAveragePeak = 0;
  BRFFirstPeakFit = 0;

  // Fit the RWM waveform, find the rising start, rising end and half rising time
  FitRWM();

  FitBRF();

  m_data->Stores["ANNIEEvent"]->Set("RWMRisingStart", RWMRisingStart);
  m_data->Stores["ANNIEEvent"]->Set("RWMRisingEnd", RWMRisingEnd);
  m_data->Stores["ANNIEEvent"]->Set("RWMHalfRising", RWMHalfRising);
  m_data->Stores["ANNIEEvent"]->Set("RWMFHWM", RWMFHWM);
  m_data->Stores["ANNIEEvent"]->Set("RWMFirstPeak", RWMFirstPeak);

  m_data->Stores["ANNIEEvent"]->Set("BRFFirstPeak", BRFFirstPeak);
  m_data->Stores["ANNIEEvent"]->Set("BRFAveragePeak", BRFAveragePeak);
  m_data->Stores["ANNIEEvent"]->Set("BRFFirstPeakFit", BRFFirstPeakFit);

  return true;
}
bool FitRWMWaveform::Finalise()
{
  if (printToRootFile)
  {
    TFile *fOutput_tfile = new TFile(output_filename.c_str(), "recreate");

    // Loop ToBePrintedRWMWaveforms and ToBePrintedBRFWaveforms, fill each waveform to a histogram and save it to the root file
    // Use the RWM+event number + key as the name of the histogram
    int RWMCount = 0;
    for (const auto &kv : ToBePrintedRWMWaveforms)
    {
      if (kv.second.empty()) continue;
      const auto &key = kv.first;
      const auto &val = kv.second;
      TH1D *hRWM = new TH1D(Form("RWM_%d_%lu", RWMCount, key), Form("RWM_%d_%lu", RWMCount, key), val.size(), 0, val.size());
      for (int i = 0; i < val.size(); i++)
      {
        hRWM->SetBinContent(i + 1, val[i]); // Note the 1-based index
      }
      hRWM->Write();
      delete hRWM;
      RWMCount++;
    }

    int BRFCount = 0;
    for (const auto &kv : ToBePrintedBRFWaveforms)
    {
      if (kv.second.empty()) continue;
      const auto &key = kv.first;
      const auto &val = kv.second;
      TH1D *hBRF = new TH1D(Form("BRF_%d_%lu", BRFCount, key), Form("BRF_%d_%lu", BRFCount, key), val.size(), 0, val.size());
      for (int i = 0; i < val.size(); i++)
      {
        hBRF->SetBinContent(i + 1, val[i]); // Note the 1-based index
      }
      hBRF->Write();
      delete hBRF;
      BRFCount++;
    }

    int BESCount = 0;
    for (const auto &kv : ToBePrintedBESWaveforms)
    {
      if (kv.second.empty()) continue;
      const auto &key = kv.first;
      const auto &val = kv.second;
      TH1D *hBES = new TH1D(Form("BES_%d_%lu", BESCount, key), Form("BES_%d_%lu", BESCount, key), val.size(), 0, val.size());
      for (int i = 0; i < val.size(); i++)
      {
        hBES->SetBinContent(i + 1, val[i]); // Note the 1-based index
      }
      hBES->Write();
      delete hBES;
      BESCount++;
    }

    fOutput_tfile->Close();
    delete fOutput_tfile;
  }

  return true;
}

void FitRWMWaveform::FitRWM()
{
  Log("FitRWMWaveform: FitRWM()", v_debug, verbosityFitRWMWaveform);
  if (RWMRawWaveform.size() == 0)
  {
    Log("FitRWMWaveform: FitRWM(): RWMRawWaveform is empty", v_message, verbosityFitRWMWaveform);
    return;
  }
  int threshold = 50;
  int bin_size = 2; // 2ns per bin
  int max_bin_RWM = RWMRawWaveform.size();

  // RWM
  RWMRisingStart = 300;
  for (int i = 300; i < max_bin_RWM; ++i)
  {
    if (RWMRawWaveform[i] > 400 && RWMRawWaveform[i] - RWMRawWaveform[i - 1] > threshold)
    {
      RWMRisingStart = i - 1;
      break;
    }
  }

  RWMRisingEnd = RWMRisingStart;
  for (int i = RWMRisingStart; i < RWMRisingStart + 60 && i < max_bin_RWM; ++i)
  {
    if (RWMRawWaveform[i] > RWMRawWaveform[RWMRisingEnd])
    {
      RWMRisingEnd = i;
    }
  }
  if (!(RWMRisingStart > 200 && RWMRisingStart < 600) || !(RWMRisingEnd > 200 && RWMRisingEnd < 600))
  {
    Log("FitRWMWaveform: FitRWM(): RWMRisingStart or RWMRisingEnd out of range, found at " + std::to_string(RWMRisingStart) + " and " + std::to_string(RWMRisingEnd), v_message, verbosityFitRWMWaveform);
    return;
  }

  Log("FitRWMWaveform: FitRWM(): Found rising start and end at " + std::to_string(RWMRisingStart) + " and " + std::to_string(RWMRisingEnd) + " with rising start value " + std::to_string(RWMRawWaveform[RWMRisingStart]), v_debug, verbosityFitRWMWaveform);
  double RWMBottom = std::accumulate(RWMRawWaveform.begin(), RWMRawWaveform.begin() + RWMRisingStart, 0.0) / RWMRisingStart;
  double RWMTop = std::accumulate(RWMRawWaveform.begin() + RWMRisingEnd + 50, RWMRawWaveform.begin() + RWMRisingEnd + 400, 0.0) / 350;
  double RWMHalf = (RWMBottom + RWMTop) / 2;

  int best_bin_RWM = RWMRisingStart;
  double best_diff_RWM = std::abs(RWMRawWaveform[RWMRisingStart] - RWMHalf);

  for (int i = RWMRisingStart; i <= RWMRisingEnd; ++i)
  {
    double diff = std::abs(RWMRawWaveform[i] - RWMHalf);
    if (diff < best_diff_RWM)
    {
      best_bin_RWM = i;
      best_diff_RWM = diff;
    }
  }

  int best_interval_RWM = 0;
  double interval_size = 1.0 / 400;
  double min_diff_RWM = std::numeric_limits<double>::max();
  Log("FitRWMWaveform: FitRWM(): Found best bin at " + std::to_string(best_bin_RWM), v_debug, verbosityFitRWMWaveform);

  for (int i = -200; i < 200; ++i)
  {
    // Log("FitRWMWaveform: FitRWM(): Interpolating value at " + std::to_string(i),v_debug, verbosityFitRWMWaveform);
    double interpolated_value = RWMRawWaveform[best_bin_RWM] + (RWMRawWaveform[best_bin_RWM + (i < 0 ? -1 : 1)] - RWMRawWaveform[best_bin_RWM]) * (std::abs(i) * interval_size);
    double diff = std::abs(interpolated_value - RWMHalf);
    if (diff < min_diff_RWM)
    {
      min_diff_RWM = diff;
      best_interval_RWM = i;
    }
  }

  double RWMHalfTimeInPs = (best_bin_RWM + best_interval_RWM * interval_size) * 10 + RWMRisingStart * 2000;

  RWMHalfRising = RWMHalfTimeInPs;
  Log("FitRWMWaveform: FitRWM(): Found RWMHalfRising = " + std::to_string(RWMHalfRising), v_debug, verbosityFitRWMWaveform);
  // finding the falling half
  // Finding the falling end
  int RWMFallingStart = RWMRisingEnd;
  for (int i = RWMRisingEnd; i < max_bin_RWM; ++i)
  {
    if (RWMRawWaveform[i] < 1000)
    {
      RWMFallingStart = i;
      break;
    }
  }
  Log("FitRWMWaveform: FitRWM(): Found falling start", v_debug, verbosityFitRWMWaveform);
  int bin_close = RWMFallingStart;
  double best_diff_fall = std::abs(RWMRawWaveform[RWMFallingStart] - RWMHalf);

  for (int i = RWMFallingStart - 20; i <= RWMFallingStart + 20 && i < max_bin_RWM; ++i)
  {
    double diff = std::abs(RWMRawWaveform[i] - RWMHalf);
    if (diff < best_diff_fall)
    {
      bin_close = i;
      best_diff_fall = diff;
    }
  }
  if (!(bin_close < 950))
  {
    Log("FitRWMWaveform: FitRWM(): falling bin out of range, found at " + std::to_string(bin_close), v_message, verbosityFitRWMWaveform);
    return;
  }
  Log("FitRWMWaveform: FitRWM(): Found falling close", v_debug, verbosityFitRWMWaveform);

  best_interval_RWM = 0;
  min_diff_RWM = std::numeric_limits<double>::max();

  for (int i = -200; i < 200; ++i)
  {
    double interpolated_value = RWMRawWaveform[bin_close] + (RWMRawWaveform[bin_close + (i < 0 ? -1 : 1)] - RWMRawWaveform[bin_close]) * (std::abs(i) * interval_size);
    double diff = std::abs(interpolated_value - RWMHalf);
    if (diff < min_diff_RWM)
    {
      min_diff_RWM = diff;
      best_interval_RWM = i;
    }
  }
  Log("FitRWMWaveform: FitRWM(): Found falling end", v_debug, verbosityFitRWMWaveform);
  double RWMHalfEnd = (bin_close * 2000) + (best_interval_RWM * 10);
  RWMFHWM = RWMHalfEnd - RWMHalfRising;

  int RWMFirstPeakBin = -1;
  for (int i = 0; i < RWMRawWaveform.size(); ++i)
  {
    if (RWMRawWaveform[i] > 1600)
    {
      RWMFirstPeakBin = i;
      break;
    }
  }

  // change the unit from bin number to ns or ps
  RWMRisingStart = RWMRisingStart * 2;
  RWMRisingEnd = RWMRisingEnd * 2;
  RWMFirstPeak = RWMFirstPeakBin * 2;
}

void FitRWMWaveform::FitBRF()
{
  double maximum = 0;
  double first_peak = 0;
  double average = 0;
  int bin_size = 2; // 2ns per bin

  if (BRFRawWaveform.size() == 0)
  {
    Log("FitRWMWaveform: FitBRF(): BRFRawWaveform is empty", v_message, verbosityFitRWMWaveform);
    return;
  }

  int max_bin_BRF = BRFRawWaveform.size();

  // Find the highest peak in the first 10 bins
  for (int i = 0; i < 10 && i < max_bin_BRF; ++i)
  {
    if (BRFRawWaveform[i] > maximum)
    {
      maximum = BRFRawWaveform[i];
      first_peak = i;
    }
  }

  // Find the average position of the highest peaks in each 10-bin block
  int interval_count = 0;
  for (int i = 0; i <= max_bin_BRF - 10; i += 10)
  {
    double max = 0;
    int max_bin = 0;
    for (int j = i; j < i + 10; ++j)
    {
      if (BRFRawWaveform[j] > max)
      {
        max = BRFRawWaveform[j];
        max_bin = j;
      }
    }
    average += (max_bin - i);
    ++interval_count;
  }
  if (interval_count > 0) average /= interval_count;

  Log("FitRWMWaveform: FitBRF(): First peak at bin = " + std::to_string(first_peak), v_debug, verbosityFitRWMWaveform);
  Log("FitRWMWaveform: FitBRF(): Average peak position = " + std::to_string(average), v_debug, verbosityFitRWMWaveform);

  BRFFirstPeak = first_peak * bin_size;
  BRFAveragePeak = average * bin_size;

  // ---------------------------------------------------------------------------
  // BRFFirstPeakFit: 50% time of the first full rising edge, in ps
  //   1. find the valleys in the first 60 samples
  //   2. merge valleys that are only 1-2 samples apart
  //   3. keep one valley per RF cycle (valleys < 8 samples apart: keep the deeper one)
  //   4. first full edge = first kept valley -> highest sample before the next kept valley
  //   5. 50% crossing between the two samples around the halfway level
  // Leaves BRFFirstPeakFit = 0 if no edge can be found.
  // ---------------------------------------------------------------------------
  BRFFirstPeakFit = 0;

  const int searchEnd    = 60;     // look for valleys in the first 60 samples
  const int minValleyGap = 3;      // valleys closer than this are the same valley
  const int sameCycleGap = 8;      // valleys closer than this are in the same RF cycle (~9.4 samples per cycle)
  const double samplePs  = 2000.0; // 2 ns per sample

  const std::vector<uint16_t> &w = BRFRawWaveform;
  if ((int)w.size() <= searchEnd)
  {
    Log("FitRWMWaveform: FitBRF(): BRF waveform too short", v_debug, verbosityFitRWMWaveform);
    return;
  }

  // 1. all valleys (local minima) in the first 60 samples
  std::vector<int> valleys;
  if (w[1] < w[0] && w[1] < w[2] && w[1] < w[3]) valleys.push_back(1);   // valley right at the start
  for (int s = 2; s < searchEnd - 1; s++)
  {
    if (w[s] <= w[s - 1] && w[s] < w[s - 2] && w[s] <= w[s + 1] && w[s] < w[s + 2])
      valleys.push_back(s);
  }

  // 2. merge valleys that are only 1-2 samples apart (flat bottoms)
  std::vector<int> merged;
  for (int v : valleys)
  {
    if (merged.empty() || v - merged.back() >= minValleyGap) merged.push_back(v);
  }
  if (merged.size() < 2)
  {
    Log("FitRWMWaveform: FitBRF(): fewer than 2 valleys found", v_debug, verbosityFitRWMWaveform);
    return;
  }

  // 3. one valley per RF cycle
  std::vector<int> cycleStart;
  if (merged[1] - merged[0] < sameCycleGap)
  {
    if (w[merged[0]] < w[merged[1]]) cycleStart.push_back(merged[0]);
  }
  else
  {
    cycleStart.push_back(merged[0]);
  }
  for (size_t i = 1; i + 1 < merged.size(); i++)
  {
    if (merged[i] - merged[i - 1] < sameCycleGap)
    {
      if (w[merged[i]] < w[merged[i - 1]] && w[merged[i]] < w[merged[i + 1]]) cycleStart.push_back(merged[i]);
    }
    else
    {
      cycleStart.push_back(merged[i]);
    }
  }
  if (cycleStart.size() < 3)
  {
    Log("FitRWMWaveform: FitBRF(): fewer than 3 RF cycles found", v_debug, verbosityFitRWMWaveform);
    return;
  }

  // 4. first full edge: first cycle valley -> highest sample before the next cycle valley
  int edgeStart = cycleStart[0];
  int peak = edgeStart;
  for (int k = edgeStart; k < cycleStart[1]; k++)
  {
    if (w[k] > w[peak]) peak = k;
  }

  double lo = w[edgeStart];
  double hi = w[edgeStart];
  int hiIdx = edgeStart;
  for (int k = edgeStart; k <= peak; k++)
  {
    if (w[k] < lo) lo = w[k];
    if (w[k] > hi) { hi = w[k]; hiIdx = k; }
  }

  // 5. 50% crossing between the last sample below halfway and the next one
  double halfway = lo + (hi - lo) / 2.0;
  int below = -1;
  for (int k = hiIdx; k >= edgeStart; k--)
  {
    if (w[k] < halfway) { below = k; break; }
  }
  if (below < 0 || below + 1 > hiIdx)
  {
    Log("FitRWMWaveform: FitBRF(): no 50% crossing found", v_debug, verbosityFitRWMWaveform);
    return;
  }

  double crossing = below + (halfway - w[below]) / ((double)w[below + 1] - w[below]);   // in samples
  BRFFirstPeakFit = crossing * samplePs;                                                  // in ps

  Log("FitRWMWaveform: FitBRF(): BRFFirstPeakFit = " + std::to_string(BRFFirstPeakFit) + " ps", v_debug, verbosityFitRWMWaveform);
}
