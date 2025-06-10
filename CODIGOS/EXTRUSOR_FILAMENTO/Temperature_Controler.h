#ifndef Temperature_Controler_h
#define Temperature_Controler_h

// the #include statment and code go here...
#include "Arduino.h"

/*
  Global Constants
*/

/*
    heatingRate = (Tf - T0) / deltaTiempo

    T0=27 °C
    Tf=373.9°C
    deltaTiempo=593.0 segundos
*/

static const float heatingRate = 0.585f;  // Heating rate (°C/s) - adjust based on your system
static const int pwmIncrement = 25;  // PWM increment value
static const int pwmMax = 250;  // Maximum PWM value
static const int minPwm = 100;  // Minimum PWM value to start heating
static const float temperatureTolerance = 0.5f;  // Temperature tolerance (°C)

/*
  Global Variables
*/
static unsigned long lastHeatStartTime = 0;
static unsigned long heatingTime = 0;
static int pwmValue = 0;
static uint8_t ledState=LOW;

/*
  Function prototypes
*/
extern void TEMPERATURE_Controller(const float desiredTemperature , const float currentTemperature, const uint8_t pinPWM, bool *pTemperatureReady);

#endif