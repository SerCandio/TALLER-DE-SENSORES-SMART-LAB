#include "Temperature_Controler.h"
#include "Thermistor.h"
#include "STEPPER_A4988.h"

#define HEATER_CALOR_R                    10
#define END_STOP_X_MIN                    3


//Sentido de giro
#define CK                                false 
#define CCK                               false

//Definir el SET POINT DE TEMPERATURA o Temperatura que del 'hooter' deseada
#define SET_POINT_TEMPERATURA             260.0f

/*
  Estructuras y definiciones tipo
*/
typedef struct{
  uint8_t appEstadoActual;
  uint8_t appEstadoSiguiente;

}FSM_t;


/*
  Variables globales
*/
static int valPotenciaPWM=0;
static float temperatura;
static bool  tempControllerEnable=false;
static bool temperatureReady=false;
static uint8_t finalCarreraAlcanzado;
static uint8_t finalCarreraAlcanzadoPrev=HIGH;
static bool stepperState=false;                 //Flag para saber si el motor PAP esta rotando o no
static bool modoAutomatico=false;                //Por defecto puede iniciar en manual o en automatico

FSM_t FSM;                          //Variable para la maquina de estados

#define TASK1_POLLRATE_MS       15UL  //Tarea de ............Leer comandos del puerto consola
#define TASK2_POLLRATE_MS       20UL  //Tarea de ...............Leer y Calcular y mostrar la resistencia y Temperatura (toma unos 300ms en tltal) (MODO MAN)
#define TASK3_POLLRATE_MS       50UL  //Tarea de ...............leer pulsadores/switches
#define TASK4_POLLRATE_MS       1000UL  //Tarea de ..............SETEO EXACTO DE TEMPERATURA MEDIANTE CONTROLADOR (con la tempersra actual de la TASK2, y el SP de temperatura pre-establecido)
#define TASK5_POLLRATE_MS       100UL  //Tarea de ..............SECUENCIA AUTOMATICA DE CALENTAMIENTO Y ENROLLADO

#define NUMBER_OF_TASKS         5     //Cantidad de tareas del sistema

/*  
    Directivas para la asignacion de estados
*/
#define ESTADO_DETENIDO                0x01
#define ESTADO_CALENTANDO_HOOTER       0x02
#define ESTADO_EXTRUYENDO              0x03    

//Definir variables de temporizacion de las tareas
static uint32_t timePrev[NUMBER_OF_TASKS]={0};


/*
  Codigo de Tareas o Tasks
  Nota: Acciones Principales del sistema

*/
void Task1(void){
  /*
    TAREA DE ATENCION A ORDENES DE USUARIO DESDE CONSOLA
  */
      
  if(Serial.available()>0){
    char cmd=Serial.read();

    if(cmd=='H' && !tempControllerEnable &&!modoAutomatico){
      /*  
        Solo si esta habilitado el modo manual
      */

      if(valPotenciaPWM>254) valPotenciaPWM=254;
      else                   valPotenciaPWM+=10;

      //Escribir periferico PWM
      analogWrite(HEATER_CALOR_R, valPotenciaPWM);

      //Informar por consola al usuario
      Serial.println("");
      Serial.print("potenccia PWM (resistencia calor) [+]= ");
      Serial.println(valPotenciaPWM);
      Serial.println("");
      

    }else if(cmd=='h' && !tempControllerEnable &&!modoAutomatico){
      /*  
        Solo si esta habilitado el modo manual
      */
      if(valPotenciaPWM<0)   valPotenciaPWM=0;
      else                   valPotenciaPWM-=10;

     //Escribir periferico PWM
      analogWrite(HEATER_CALOR_R,valPotenciaPWM);

      //Informar por consola al usuario
      Serial.println("");
      Serial.print("potenccia PWM (resistencia calor) [-]= ");
      Serial.println(valPotenciaPWM);
      Serial.println("");

    }else if(cmd=='R' && !modoAutomatico){                                   //GIAR EL PAP
      /*  
        Solo si esta habilitado el modo manual
      */
      stepperState=!stepperState;

      if(stepperState==true){
        //Girar el motor PAP a traves del driver A4988
        STEPPER_A4988_Rotate(CK);

        //Informar por consola al usuario
        Serial.println("");
        Serial.print("Girando motor PAP  [A4988]= ");
        CK? Serial.print("sentido CK"):Serial.print("sentido CCK");
        Serial.println("");

      }else{
          //Parar / Detener el PAP 
         STEPPER_A4988_Stop();

         //Informar por consola al usuario
        Serial.println("");
        Serial.print("Deteniendo motor PAP  [A4988]......");
        Serial.println("");

      }

   
    }else if(cmd=='E'){
  
      //Colocar la resistencia de calor a 0 de potencia
      valPotenciaPWM=0;

      //DESHABILITAR el controlador de temperatura
      tempControllerEnable=false;

      //REESTABLECER en modo manual
      modoAutomatico=false;

      //INHABILIATR EL MOTOR PAP
      stepperState=false;

      //Actualizar el  periferico PWM
      analogWrite(HEATER_CALOR_R,valPotenciaPWM);

      //Parar / Detener el PAP 
      STEPPER_A4988_Stop();

      //Informar por consola al usuario
      Serial.println("");
      Serial.print("potenccia PWM (resistencia calor) [OFF]= ");
      Serial.println(valPotenciaPWM);
      Serial.println("");

      //Informar por consola serie al usuario
      Serial.println("");
      modoAutomatico? Serial.print("MODO [AUTOMATICO] ESTABLECIDO"):Serial.print("MODO [MANUAL] ESTABLECIDO");
      Serial.println("");

      //Informar por consola al usuario
      Serial.println("");
      Serial.print("Deteniendo motor PAP  [A4988]......");
      Serial.println("");

    }else if(cmd=='T' && !modoAutomatico){
      /*  
        Solo si esta habilitado el modo manual
      */

      //Habilitar el controlador de temperatura
      tempControllerEnable=!tempControllerEnable;

       //Informar por consola al usuario
      Serial.println("");
      tempControllerEnable? Serial.print("Controlador de temperatura [ON] ==>alcanzando temperatura progamada...."):Serial.print("Controlador de temperatura [DETENIDO]");
      Serial.println("");
      
    }else if(cmd=='M'){
      //Establecer en modo automatico /manual
      modoAutomatico=!modoAutomatico;

      //Informar por consola serie al usuario
      Serial.println("");
      modoAutomatico? Serial.print("MODO [AUTOMATICO] ESTABLECIDO"):Serial.print("MODO [MANUAL] ESTABLECIDO");
      Serial.println("");

    }else if(cmd=='p'){
      //Mostrar solo los parametros actuales del sistema a modo de copia o recordatorio




    }

  }

}

void Task2(void){

  // Calcular  la resistencia electrica en ohms del termistor usando la lectura del ADC
  float resistencia = thermistor_get_resistance(CONFIG_THERMISTOR_ADC_PIN, CONFIG_THERMISTOR_RESISTOR);

  // Calcular la TEMEPRATURA segun la resistencia del termistor
  temperatura = thermistor_get_temperature(resistencia);

  if(!modoAutomatico)
  {
    Serial.println("TASK 2===>>>>");

    // Imprimir resistencia y temperatura al monitor serial (a modo de testeo en el modo manual)
    Serial.print("Resistencia del NTC [kohm]: ");
    Serial.print(resistencia/1000, 2);
    Serial.print(" ==>Temperatura [°C]: ");
    Serial.println(temperatura, 1);
  }

}


void Task3(void){
  /*
    TAREA DE LECTURA DE SWITCHES Y PULSADORES
  */
    //Lectura del pin digital
  finalCarreraAlcanzado=digitalRead(END_STOP_X_MIN);

  //Retardo de debouncing
  delay(10);

  if(finalCarreraAlcanzado!=finalCarreraAlcanzadoPrev){

    Serial.println("TASK 3===>>>>");
 
    if(finalCarreraAlcanzado==LOW){
      //Hacer algo.....

      //Notificar al usuario
      Serial.println("");
      Serial.println("FINAL X_MIN [presionado]");
      Serial.println("");

    }else{  //finalCarreraAlcanzado==HIGH

      //Notificar al usuario
      Serial.println("");
      Serial.println("FINAL X_MIN [soltado]");
      Serial.println("");
    }

  }

  //Salvar el estadon del switch X_MIN
  finalCarreraAlcanzadoPrev=finalCarreraAlcanzado;

}

void Task4(void){

  /*
    TAREA DE CONTROL CASI exacto de la temperartura del sistema mediante controlador
    Nota: 'temperatura' es la variable que contiene la temperatura actual, que ya se calcula en 'Task 2'
          El controlador sera llamaddo cada  'TASK4_POLLRATE_MS'

    OBS: Debe habilitarse el flag 'tempControllerEnable' si se esta en modo MAN. 
         La variable 'temperatureReady' indica si ya se llego al SP de temperatura requerido

    OBS: El controaldor de temperatura siempre debe de correr
  */

  if(tempControllerEnable==true){
    Serial.println("TASK 4===>>>>");
    Serial.print("Controlador de temperatura [ON]......");
    TEMPERATURE_Controller(SET_POINT_TEMPERATURA , temperatura, HEATER_CALOR_R, &temperatureReady);
    temperatureReady? Serial.println("SP de Temperatura Listo OK!!!"):Serial.println("SP de Temperatura en proceso....");
  }

}

void Task5(void){
  /*
    TAREA DE REALIZACION DE SECUENCIA AUTOMATICA DE CALENTAMIENTO Y ENROLLAMIENTO

    Nota: Para que inicie la secuencia basada en maquinas de estado FSM, debe estar habilitado el modo automatico (tecla 'M')
          Y se debe de haber accionado el final de carrera que esta en el pin 'END_STOP_X_MIN' que hace cambiar a la variable 'finalCarreraAlcanzado'
  */

  //Ejecutar la FSM solo si el modo automatico esta habilitado
  if(modoAutomatico==true){

    switch(FSM.appEstadoActual){
      case ESTADO_DETENIDO:
      /*  
        Acciones del estado DETENIDO
      */
      //Colocar la resistencia de calor a 0 de potencia
      valPotenciaPWM=0;

      //DESHABILITAR el controlador de temperatura
      tempControllerEnable=false;

      //INHABILIATR EL MOTOR PAP
      stepperState=false;

      //Actualizar el  periferico PWM
      analogWrite(HEATER_CALOR_R, valPotenciaPWM);

      //Parar / Detener el PAP A4988
      STEPPER_A4988_Stop();

      /*  
        Condiciones del estado DETENIDO
      */
      if(finalCarreraAlcanzado==LOW){ //Se presiono el Final de carrera
        FSM.appEstadoSiguiente=ESTADO_CALENTANDO_HOOTER;
        Serial.print("Task 5 ==>>");
        Serial.println("ESTADO SIGUIENTE: CALENTANDO_HOOTER...");
      }else{
        FSM.appEstadoSiguiente=ESTADO_DETENIDO;

      }

        break;
      
      case ESTADO_CALENTANDO_HOOTER:
      /*  
        Acciones del estado CALENTANDO_HOOTER
      */
      //Habilitar el controlador de temperatura
      tempControllerEnable=true;

      /*  
        Condiciones del estado CALENTANDO_HOOTER
      */
      if(temperatureReady==true){ //Si la Temperatura de la resistencia de calor ya esta lista
        FSM.appEstadoSiguiente=ESTADO_EXTRUYENDO;
        Serial.print("Task 5 ==>>");
        Serial.println("ESTADO SIGUIENTE: EXTRUYENDO...");

      }else{
        FSM.appEstadoSiguiente=ESTADO_CALENTANDO_HOOTER;

      }

        break;

      case ESTADO_EXTRUYENDO:
       /*  
        Acciones del estado EXTRUYENDO
      */
      stepperState=true;
      //Girar el motor PAP a traves del driver A4988
      STEPPER_A4988_Rotate(CK);

       /*  
        Condicion  del estado EXTRUYENDO
      */
      if(finalCarreraAlcanzado==HIGH){  //Se solto el Final de carrera ???
        FSM.appEstadoSiguiente=ESTADO_DETENIDO;
        Serial.print("Task 5 ==>>");
        Serial.println("ESTADO SIGUIENTE: DETENIDO...");

      }else{
        FSM.appEstadoSiguiente=ESTADO_EXTRUYENDO;

      }
        break;

    }

    //Transferir el sigueinte estado al estado actual
    FSM.appEstadoActual=FSM.appEstadoSiguiente;

  }//Fin de condicional del modo automatico

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
  ISR's subrutinas de interrupcion
*/


//////////////////////////////////////////////////////////////
void setup() {
  //Pines I/O.....resistenbcia de caloR y Final de carrera + LED el placa
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(HEATER_CALOR_R, OUTPUT);
  analogWrite(HEATER_CALOR_R, 0);
  pinMode(END_STOP_X_MIN, INPUT_PULLUP);
  
  //Iniciar el driver del motor PAP A4988
  STEPPER_A4988_Initialize();

  //Iniciar consola del puerto serie
  Serial.begin(9600);

  //Mensaje
  Serial.println("RESET......\r\n");

  //Iniciar Maquina de estados
  FSM.appEstadoActual=ESTADO_DETENIDO;

  //Iniciar el temporizador de tareas
  Task_Init();

}

void loop() {

  //Actualizar los Task Scheduler...........
  if((millis() - timePrev[0])>=TASK1_POLLRATE_MS){
    timePrev[0]=millis();
    Task1();
  }

  if((millis() - timePrev[1])>=TASK2_POLLRATE_MS){
    timePrev[1]=millis();
    Task2();
  }

  if((millis() - timePrev[2])>=TASK3_POLLRATE_MS){
    timePrev[2]=millis();
    Task3();
  }

  if((millis() - timePrev[3])>=TASK4_POLLRATE_MS){
    timePrev[3]=millis();
    Task4();
  }

  if((millis() - timePrev[4])>=TASK5_POLLRATE_MS){
    timePrev[4]=millis();
    Task5();
  }

}

/*
  Codigo de Subfunciones de ayuda

*/

/**
  Inicializador de la temporizacion de las tareas

 */
void Task_Init(void){
    //Iniciar el arreglo de temporizacion de tareas
    for(int i=0; i<NUMBER_OF_TASKS; i++){
      timePrev[i]=millis();
    }
}

