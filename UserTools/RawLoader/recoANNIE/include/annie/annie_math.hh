// Provides routines used in recoANNIE for computing special mathematical
// functions.
//
// Based on C source files taken from www.mymathlib.com
// Ported to C++ by Steven Gardiner <sjgardiner@ucdavis.edu>
#pragma once

// standard library includes
#include <cfloat>
#include <cmath>

namespace annie_math {

  double Incomplete_Beta_Function(double x, double a, double b);
  double Beta_Function(double a, double b);

}
