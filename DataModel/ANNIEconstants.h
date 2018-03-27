#ifndef ANNIECONSTANTS_H
#define ANNIECONSTANTS_H

/// @brief The impedance to assume when computing charge values for
/// calibrated ADC hits
constexpr double ADC_IMPEDANCE = 50.; // Ohm

/// @brief Multiplying by this constant converts ADC counts to Volts
constexpr double ADC_TO_VOLT = 2.415 / std::pow(2., 12);

#endif
