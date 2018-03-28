#ifndef ANNIECONSTANTS_H
#define ANNIECONSTANTS_H

/// @brief The impedance (in Ohms) to assume when computing charge values for
/// calibrated ADC hits
constexpr double ADC_IMPEDANCE = 50.; // Ohm

/// @brief Multiplying by this constant converts ADC counts to Volts
constexpr double ADC_TO_VOLT = 2.415 / std::pow(2., 12);

/// @brief The number of nanoseconds per ADC sample
constexpr unsigned int NS_PER_ADC_SAMPLE = 2; // ns

/// @brief A dummy value to use to initialize integers
constexpr int BOGUS_INT = -9999;

#endif
