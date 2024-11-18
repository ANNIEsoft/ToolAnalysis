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
  m_data->Stores["ANNIEEvent"]->Get("RWMRawWaveform", RWMRawWaveform);
  m_data->Stores["ANNIEEvent"]->Get("BRFRawWaveform", BRFRawWaveform);

  uint64_t WaveformTime = 0;
  m_data->Stores["ANNIEEvent"]->Get("EventTimeTank", WaveformTime);

  if (printToRootFile && ToBePrintedRWMWaveforms.size() < maxPrintNumber)
  {

    ToBePrintedRWMWaveforms.emplace(WaveformTime, RWMRawWaveform);
    ToBePrintedBRFWaveforms.emplace(WaveformTime, BRFRawWaveform);
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
  // fit the first peak, and the average of peaks of std::vector<uint16_t> BRFRawWaveform.
  // find the size of the vector, in the first 10 bin interval, find the bin with highest value, as the first peak
  // for each 10 bin interval, find the highest value bin position in each interval, and minus the beginning of the interval, and average them.
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

  // Find the first peak in the first 10 bin interval
  for (int i = 0; i < 10 && i < max_bin_BRF; ++i)
  {
    if (BRFRawWaveform[i] > maximum)
    {
      maximum = BRFRawWaveform[i];
      first_peak = i;
    }
  }

  // Find the highest value bin position in each 10 bin interval and calculate the average
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

  if (interval_count > 0)
  {
    average /= interval_count;
  }

  // Logging results
  Log("FitRWMWaveform: FitBRF(): First peak at bin = " + std::to_string(first_peak), v_debug, verbosityFitRWMWaveform);
  Log("FitRWMWaveform: FitBRF(): Average peak position = " + std::to_string(average), v_debug, verbosityFitRWMWaveform);

  BRFFirstPeak = first_peak * bin_size;
  BRFAveragePeak = average * bin_size;

  BRFFirstPeakFit = 0;
  // now, we use a simple Gaussian fit to find the first peak in the first 10 bins (before understanding the Booster RF properties)
  // first, in the first 10 bins, find the maximum value and its position, if the maximum value is smaller than 3050, return
  // then check the side bins. check from the max bin to zero, find the first bin with value less than 2920, or until the first bin, save this bin as fitting start bin
  //  then check from the max bin to the 10th bin, find the first bin with value less than 3020. check if the next bin has value less than this bin, if yes, set next bin as the fitting end, if not, set this bin as fitting end.
  //  in the fitting range, fit the waveform with a Gaussian function, and find the peak position.
  //  save the peak position as BRFFirstPeakFit

  if (maximum < 3050)
    return;

  int fit_start_bin = 0;
  for (int i = first_peak; i >= 0; --i)
  {
    if (BRFRawWaveform[i] < 2920)
    {
      fit_start_bin = i;
      break;
    }
  }

  int fit_end_bin = first_peak;
  for (int i = first_peak; i < 10 && i < max_bin_BRF; ++i)
  {
    if (BRFRawWaveform[i] < 3020)
    {
      if (BRFRawWaveform[i + 1] < BRFRawWaveform[i])
      {
        fit_end_bin = i + 1;
      }
      else
      {
        fit_end_bin = i;
      }
      break;
    }
  }

  if (fit_end_bin <= fit_start_bin)
    return;

  std::vector<double> x_vals, y_vals;
  for (int i = fit_start_bin; i <= fit_end_bin; ++i)
  {
    x_vals.push_back(i * bin_size);
    y_vals.push_back(BRFRawWaveform[i]);
  }

  // Find the bin with the minimum value between the 5th and 15th bins
  auto min_it = std::min_element(BRFRawWaveform.begin() + 4, BRFRawWaveform.begin() + 15);
  int bin_minimum = std::distance(BRFRawWaveform.begin(), min_it);

  // Find the bin with the maximum value between bin_minimum + 2 and bin_minimum + 4
  auto max_it = std::max_element(BRFRawWaveform.begin() + bin_minimum + 2, BRFRawWaveform.begin() + bin_minimum + 5);
  int bin_maximum = std::distance(BRFRawWaveform.begin(), max_it);

  // Calculate the half height
  double half_height = (BRFRawWaveform[bin_minimum] + BRFRawWaveform[bin_maximum]) / 2.0;

  // Perform linear interpolation
  const int intervals_per_bin = 2000;
  double min_difference = std::numeric_limits<double>::max();
  int min_difference_bin = -1;

  for (int bin = bin_minimum; bin < bin_maximum; ++bin)
  {
    for (int interval = 0; interval < intervals_per_bin; ++interval)
    {
      double fraction = interval / static_cast<double>(intervals_per_bin);
      double interpolated_value = BRFRawWaveform[bin] + fraction * (BRFRawWaveform[bin + 1] - BRFRawWaveform[bin]);
      double difference = std::abs(interpolated_value - half_height);

      if (difference < min_difference)
      {
        min_difference = difference;
        min_difference_bin = bin * intervals_per_bin + interval;
      }
    }
  }

  double BRFRisingHalfLinearFit = min_difference_bin; // in ps
  BRFFirstPeakFit = BRFRisingHalfLinearFit; 
}
