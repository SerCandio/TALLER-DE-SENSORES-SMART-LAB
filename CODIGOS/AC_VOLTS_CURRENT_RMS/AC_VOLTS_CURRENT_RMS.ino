#include <ZMPT101B.h>

#define SENSITIVITY 467.25f

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

//Definir las velocidades I2C
#define I2C_BIT_RATE_400KHZ             400000
#define I2C_BIT_RATE_100KHZ             100000

//Objetos de Librerias
Adafruit_ADS1115 ads;

// ZMPT101B sensor output connected to analog pin A0
// and the voltage source frequency is 50 Hz.
ZMPT101B voltageSensor(A0, 60.0);

#define TASK1_POLLRATE_MS       10UL  //Tarea de lectura -promediado de sensor

#define NUMBER_OF_TASKS         1     //Cantidad de tareas del sistema

//Definir pines I/O
#define ADS1115_READY_PIN       2  //Pin de ALRT/RDY

//Definir factor de escala del sensor SCT013_30A
#define  SCT013_FACTOR          30.0f //30A/1V

//Definir variables de temporizacion de las tareas
static uint32_t timePrev[NUMBER_OF_TASKS]={0};

//Variable de interrupcion
volatile bool adsNewData=false;

/*  
  ISR's
*/
void ADS1115_New_Data_Ready(void){
    adsNewData=true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  //Iniciar pines I/O
  pinMode(ADS1115_READY_PIN, INPUT);

  //Habilitar interrupcion por flanco de bajada
  attachInterrupt(digitalPinToInterrupt(ADS1115_READY_PIN), ADS1115_New_Data_Ready, FALLING);
  
   //Power up, delay &I2C , Iniciar Periferico I2C como Maestro
  //      at ....kHz
  delay(100);
  Wire.begin();
  Wire.setClock(I2C_BIT_RATE_400KHZ);

  //Iniciar puerto serie
  Serial.begin(9600);

  // Change the sensitivity value based on value you got from the calibrate
  // example.
  voltageSensor.setSensitivity(SENSITIVITY);

  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115.");
    return;
  }

  // Set gain to ±2.048 -->> 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  /*  
    Note: SCT013 sensor (30A /1V RMS) has a maximum value of 1.4142V
  */
  ads.setGain(GAIN_TWO);

  // Set sample rate to .... samples per second (Fs).....aprox to 500Hz
  ads.setDataRate(RATE_ADS1115_860SPS);

  Serial.println("ADS1115 initialized with GAIN_TWO (+-2.048V) and Fs= 475SPS, AIN0 (P) and AIN1 (N), continuous mode");
  Serial.println("");

  // Start continuous conversions.
  ads.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, /*continuous=*/true);

  //Iniciar tiempo en milisegundos de ejecucion de tareas
  Task_Init();
 
}

void loop() {

  if((millis() - timePrev[0])>=TASK1_POLLRATE_MS){
      timePrev[0]=millis();

    // Obtener la corriente AC RMS
    float currentRMS = getCorriente_RMS();
    float voltageRMS = voltageSensor.getRmsVoltage();

   //Imprimir en pantalla
    Serial.print(" VRMS [AC]: ");
    Serial.print(voltageRMS, 2);
    Serial.print(" V");
    Serial.print(" => IRMS [AC]: "); //Valor convertido
    Serial.print(currentRMS, 2);
    Serial.println(" A");

  }


}

/*
  Sub-Functions code
*/

static void Task_Init(void){
    //Iniciar el arreglo de temporizacion de tareas
    for(int i=0; i<NUMBER_OF_TASKS; i++){
      timePrev[i]=millis();
    }
}

static float getCorriente_RMS(void)
{
 float voltsADC=0.0f,corriente,sumSamplesX2=0.0f,currentRMS;
 uint64_t nSamples=0;
 unsigned long start=millis();

 while ( (millis() - start)<=300 ) //Instrumentos de medidia (pantallas) se actualizan a este rate de 300 ms
 {
   if(!adsNewData) continue;

    voltsADC=ads.computeVolts(ads.getLastConversionResults()); // Read Differential sample from A0 (A+) respect to A1(A-)
    corriente=voltsADC*SCT013_FACTOR;
    //Acumular valores X^2 en una varable suma : suma=suma+Xi^2;
    sumSamplesX2+=sq(corriente);
    nSamples++;

    adsNewData=false;

  }

  /*
    RMS_Value=sqrt( sum(Xi^2)/N)
  */
  currentRMS=sqrt(sumSamplesX2 / ((float) nSamples));
  return currentRMS;
}