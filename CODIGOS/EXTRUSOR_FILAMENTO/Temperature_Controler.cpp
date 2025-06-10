#include "Temperature_Controler.h"


extern void TEMPERATURE_Controller(const float desiredTemperature , const float currentTemperature, const uint8_t pinPWM, bool *pTemperatureReady){

  if (currentTemperature < desiredTemperature - temperatureTolerance) {
    //Clear flag for temperature Ready calculation
    //*pTemperatureReady=false;

    //Hacer TOOGLE de LED si aun no se alcanza la temperatura
   // ledState=!ledState;
   // digitalWrite(LED_BUILTIN, ledState);

    // Calculate required heating time
    heatingTime =( (desiredTemperature - currentTemperature) / heatingRate ) * 1000;  // Convert to milliseconds

    // Start heating if not already heating
    if (lastHeatStartTime == 0) {
      lastHeatStartTime = millis();
      pwmValue = minPwm;  // Start with minimum PWM value
    }

    // Increment PWM value if heating time is not elapsed
    if (millis() - lastHeatStartTime < heatingTime) {
      pwmValue = min(pwmValue + pwmIncrement, pwmMax);
    } else {
      pwmValue = minPwm;  // Maintain minimum PWM value if heating time is elapsed
      lastHeatStartTime = millis();  // Reset heating time
    }
  } else if (currentTemperature > desiredTemperature + temperatureTolerance) {
     //Clear flag for temperature Ready calculation
    //*pTemperatureReady=false;

    //Hacer TOOGLE de LED si aun no se alcanza la temperatura
   // ledState=!ledState;
    //digitalWrite(LED_BUILTIN, ledState);

    // Decrement PWM value if temperature is above setpoint
    pwmValue = max(pwmValue - pwmIncrement, 0);
    lastHeatStartTime = 0;  // Stop heating time calculation
  } else {
    // Maintain current PWM value if temperature is within tolerance
    lastHeatStartTime = 0;  // Stop heating time calculation

    //Set flag for temperature Ready calculation
    //*pTemperatureReady=true;

    //APAGAR el LED si ya se alcanzo la temperatura
   //ledState=LOW;
   // digitalWrite(LED_BUILTIN, ledState);
  }

////////////////////////////////////////////////////////////////////////////////////

  if((desiredTemperature - currentTemperature)<=10.0f){
    //Set flag for temperature Ready calculation
    *pTemperatureReady=true;

    //APAGAR el LED si ya se alcanzo la temperatura
    ledState=LOW;
    digitalWrite(LED_BUILTIN, ledState);

  }else{

     //Clear flag for temperature Ready calculation
    *pTemperatureReady=false;

    //Hacer TOOGLE de LED si aun no se alcanza la temperatura
    ledState=!ledState;
    digitalWrite(LED_BUILTIN, ledState);

  }

 //Actualizar el valor del duty PWM
  analogWrite(pinPWM, pwmValue);  

}
