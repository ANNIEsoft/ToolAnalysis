#ifndef ANNIEALGORITHMS_H
#define ANNIEALGORITHMS_H

#include <iostream>
#include <limits>
#include <vector>

double FindPulseMax(std::vector<double> *theWav, double &themax, int &maxbin, double &themin, int &minbin);

// Computes the sample mean and sample variance for a std::vector of numerical
// values. Based on http://tinyurl.com/mean-var-onl-alg.
template<typename ElementType> void ComputeMeanAndVariance(
  const std::vector<ElementType>& data, double& mean, double& var,
  size_t sample_cutoff = std::numeric_limits<size_t>::max())
{
  if ( data.empty() || sample_cutoff == 0) {
    mean = std::numeric_limits<double>::quiet_NaN();
    var = mean;
    return;
  }
  else if (data.size() == 1 || sample_cutoff == 1) {
    mean = data.front();
    var = 0.;
    return;
  }

  size_t num_samples = 0;
  double mean_x2 = 0.;
  mean = 0.;

  for (const ElementType& x : data) {
    ++num_samples;
    double delta = x - mean;
    mean += delta / num_samples;
    double delta2 = x - mean;
    mean_x2 = delta * delta2;
    if (num_samples == sample_cutoff) break;
  }

  var = mean_x2 / (num_samples - 1);
  return;
}

#endif
