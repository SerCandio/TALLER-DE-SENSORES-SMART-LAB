#ifndef Thermistor_h
#define Thermistor_h

// the #include statment and code go here...
#include "Arduino.h"

// configurar el pin utilizado para la medicion de voltaje del divisor resistivo del NTC
#define CONFIG_THERMISTOR_ADC_PIN         A13

// configurar el valor de la resistencia que va en serie con el termistor NTC en ohms
#define CONFIG_THERMISTOR_RESISTOR        4700.0f

/*
  Global Constants
*/


/*
  Global Variables
*/


/*
  Function prototypes
  Access  Public
*/
extern float thermistor_get_temperature(const float resistance);
extern float thermistor_get_resistance(const uint8_t AXpin, const float termistorResistor);

#endif