#ifndef ANNIEALGORITHMS_H
#define ANNIEALGORITHMS_H

#include <iostream>
#include <limits>
#include <vector>
#include <sstream>

double FindPulseMax(std::vector<double> *theWav, double &themax, int &maxbin, double &themin, int &minbin);
std::string GetStdoutFromCommand(std::string command);

// Computes the sample mean and sample variance for a std::vector of numerical
// values. Based on http://tinyurl.com/mean-var-onl-alg.
template<typename ElementType> void ComputeMeanAndVariance(
  const std::vector<ElementType>& data, double& mean, double& var,
  size_t sample_cutoff = std::numeric_limits<size_t>::max(), size_t sample_start=0)
{
  if ( data.empty() || sample_cutoff == 0 || (data.size()-sample_start) <= 0) {
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
  double m2 = 0.;
  mean = 0.;

  for (int lcount=sample_start; lcount<data.size(); ++lcount) {
    const ElementType x = data.at(lcount);
    ++num_samples;
    double delta = x - mean;
    mean += delta / num_samples;
    double delta2 = x - mean;
    m2 += delta * delta2;
    if (num_samples == sample_cutoff) break;
  }

  var = m2 / (num_samples - 1);
  return;
}

// helper function: to_string with a precision
// particularly useful for printing doubles and floats in the Log function
namespace anniealgorithms{
	template <typename T>
	std::string toString(const T a_value, const int n = 2){
	    std::ostringstream out;
	    out.precision(n);
	    out << std::fixed << a_value;
	    return out.str();
	}
}
#endif
