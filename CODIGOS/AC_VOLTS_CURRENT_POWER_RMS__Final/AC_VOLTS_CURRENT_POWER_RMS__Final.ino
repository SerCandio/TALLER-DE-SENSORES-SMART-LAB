#include <LiquidCrystal_I2C.h>
#include <TimerOne.h>
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

//Crear el objeto lcd  dirección  0x3F y 16 columnas x 2 filas
LiquidCrystal_I2C lcd(0x27,16,2);  //

#define TASK1_POLLRATE_MS       10UL  //Tarea de lectura de sensores

#define NUMBER_OF_TASKS         2     //Cantidad de tareas del sistema

//Definir pines I/O
#define ADS1115_READY_PIN       2  //Pin de ALRT/RDY

//Definir factor de escala del sensor SCT013_30A
#define  SCT013_FACTOR          30.0f //30A/1V

//Definir variables de temporizacion de las tareas
static uint32_t timePrev[NUMBER_OF_TASKS]={0};

//Definir otras variables de trabajo
static float powerRMS, currentRMS, voltageRMS;
static char LCD_BUFFER[16]="";

//Variables de interrupcion
static volatile float energyWh=0.000f;
static volatile bool adsNewData=false;
static volatile uint8_t ledState=LOW;

/*  
  ISR's
*/
void ADS1115_New_Data_Ready(void){
    adsNewData=true;
}

void TIMER_ONE_1_second_Elapsed(void){
  //Acumulacion de kWh
  energyWh+=powerRMS * (1.0f/3600.0f);

    //Parpadeo del LED
  ledState=!ledState;
  digitalWrite(LED_BUILTIN, ledState);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  //Iniciar pines I/O
  pinMode(ADS1115_READY_PIN, INPUT);

  //Habilitar interrupcion por flanco de bajada
  attachInterrupt(digitalPinToInterrupt(ADS1115_READY_PIN), ADS1115_New_Data_Ready, FALLING);

  //Iniciar TIMER 1 a 1 segundo de interrupcion
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(TIMER_ONE_1_second_Elapsed); // Do something every 1 sec....
  
   //Power up, delay &I2C , Iniciar Periferico I2C como Maestro
  //      at ....kHz
  delay(100);
  Wire.begin();
  Wire.setClock(I2C_BIT_RATE_400KHZ);

  //Iniciar puerto serie
  Serial.begin(115200);

  while(!Serial);

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

  // Inicializar el LCD
  lcd.init();
  
  //Encender la luz de fondo.
  lcd.backlight();

  // Escribimos el Mensaje en el LCD.
  lcd.setCursor(0, 0);
  lcd.print("ENERGY MTR [AC]");
  lcd.setCursor(0, 1);
  lcd.print("  by UPN :)");
  delay(2000);

  //Limpiar pantalla
  lcd.clear();

  // Start continuous conversions.
  ads.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, /*continuous=*/true);

  //Iniciar tiempo en milisegundos de ejecucion de tareas
  Task_Init();
 
}

void loop() {

  if((millis() - timePrev[0])>=TASK1_POLLRATE_MS){ //Lectura de sensores..........leer corriente toma 300ms aprox
    timePrev[0]=millis();                         //.............................leeer voltaje toma unos 20ms

    unsigned long Nsamples;

    // Obtener la corriente y voltaje AC RMS
    currentRMS = getCorriente_RMS(&Nsamples);
    voltageRMS = voltageSensor.getRmsVoltage();
    //Calcular la potencia AC RMS
    powerRMS   =voltageRMS * currentRMS;

   //Imprimir en pantalla : CONSOLA SERIAL
    Serial.print("VRMS [AC]: ");
    Serial.print(voltageRMS, 2);
    Serial.print(" V");
    Serial.print(" => IRMS [AC]: "); //Valor convertido
    Serial.print(currentRMS, 2);
    Serial.print(" A");
    Serial.print(" (# samples=");
    Serial.print(Nsamples, DEC);
    Serial.print(")");
    Serial.print(" =>POTENCIA [AC]: ");
    Serial.print(powerRMS, 1);
    Serial.print(" W");

    if(energyWh>1000.0f)   //Ya se tienen 1kWh?
    {
      Serial.print(" =>ENERGIA [AC]:");
      Serial.print(energyWh/1000.0f,1);
      Serial.println(" kWh");

    }else{
      Serial.print(" =>ENERGIA [AC]:");
      Serial.print(energyWh, 3);
      Serial.println(" Wh");
    }

    //=======MOSTRAR EN LA PANTALLA LCD16x2==============
    //Imprimir POTENCIA TOTAL  AC RMS en la pantalla
    lcd.setCursor(1, 0);
    lcd.print("P=");
    lcd.print(powerRMS, 1);
    lcd.print(" W");

    lcd.setCursor(1, 1);

    //Imprimir consumo kWh /Wh en pantalla
    if(energyWh>1000.0f){  //Ya se tienen 1kWh? (100Wh?)
     lcd.print("[ ");
     lcd.print(energyWh/1000.0f, 1);
     lcd.print(" kWh");
     lcd.print(" ]");

    }else{
     lcd.print("[ ");
     lcd.print(energyWh, 2);
     lcd.print(" Wh");
     lcd.print(" ]");
    }

  }

  //-------------------------------------------------------------------------------------------

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
  Sub-Functions code
*/

static void Task_Init(void){
    //Iniciar el arreglo de temporizacion de tareas
    for(int i=0; i<NUMBER_OF_TASKS; i++){
      timePrev[i]=millis();
    }
}

static float getCorriente_RMS(unsigned long *pNsamples)
{
 float voltsADC=0.0f,corriente,sumSamplesX2=0.0f,currentRMS;
 unsigned long nSamples=0;
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

  *pNsamples=nSamples;

  /*
    RMS_Value=sqrt( sum(Xi^2)/N)
  */
  currentRMS=sqrt(sumSamplesX2 / ((float) nSamples));
  return currentRMS;
}
