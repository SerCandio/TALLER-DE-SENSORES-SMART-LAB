#include "Thermistor.h"

/**
 * Obtiene la resistencia del termistor resolviendo el divisor resistivo.
 * 
 *  'adcval' Valor medido por el convertidor analógico a digital.
 *  retorna  int32_t Resistencia electrica del termistor.
 */
extern float thermistor_get_resistance(const uint8_t AXpin, const float termistorResistor)
{
  int raw;

  uint32_t start=millis(), nSamples=0,sumSamples=0;
  while((millis() - start)<=300)      //Instrumentos de medidia se actualizan a este rate
  {
    // Read (instant-sample) from A (single-ended)
    raw=analogRead(AXpin);     // Read the analog value

    //Accumulate on variable and increment counter
    sumSamples+=(uint32_t) raw;
    nSamples++;
  }

  float Vout = (sumSamples / nSamples) * (5.0f / 1023.0f);   // Convert raw value to output voltage

  // calculamos la resistencia del NTC a partir del valor del ADC
 return termistorResistor * (1.0 / (5.0f / Vout - 1)); 
}

/**
 *  Obtiene la temperatura en grados centigrados a partir de la resistencia
 * actual del componente.
 * 
 * 'resistance' Resistencia actual del termistor.
 *  float Temperatura en grados centigrados.
 */

extern float thermistor_get_temperature(const float resistance)
{
  // variable de almacenamiento temporal, evita realizar varias veces el calculo de log
  //float T_1;
 
  // calculamos logaritmo natural, se almacena en variable para varios calculos
 // temp = log(resistance);
 
  // resolvemos la ecuacion de STEINHART-HART
  // http://en.wikipedia.org/wiki/Steinhart–Hart_equation
 // temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
 //1/T = (1/T₀) + (1/β) * ln(R/R₀)

  float T_1=(1.0f/298.15f) + (1.0f/3950.0f) * log(resistance/100000.0f);
 
  // convertir el resultado de kelvin a centigrados y retornar
  return (1.0f/T_1) - 273.15f;
}