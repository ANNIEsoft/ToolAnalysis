#ifndef ANNIECONSTANTS_H
#define ANNIECONSTANTS_H

#include <math.h>

/// @brief The impedance (in Ohms) to assume when computing charge values for
/// calibrated ADC hits
constexpr double ADC_IMPEDANCE = 50.; // Ohm

/// @brief Multiplying by this constant converts ADC counts to Volts
const double ADC_TO_VOLT = 2.415 / pow(2., 12);

/// @brief The number of nanoseconds per ADC sample
constexpr unsigned int NS_PER_ADC_SAMPLE = 2; // ns

/// @brief A dummy value to use to initialize integers
constexpr int BOGUS_INT = -9999;

/// @brief The format code for a multievent binary BoostStore
constexpr int BOOST_STORE_BINARY_FORMAT = 0;
constexpr int BOOST_STORE_ASCII_FORMAT = 1;
constexpr int BOOST_STORE_MULTIEVENT_FORMAT = 2;

// Used to convert from seconds and nanoseconds to milliseconds when
// creating timestamps in ms while interacting with the Intensity Frontier
// beam database via the BeamChecker and BeamFetcher tools
constexpr unsigned long long THOUSAND = 1000ull;
constexpr unsigned long long MILLION = 1000000ull;
constexpr unsigned long long BILLION = 1000000000ull;

#endif
