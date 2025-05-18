#define TASK1_POLLRATE_MS       2UL  //Tarea de lectura -promediado de sensor

#define NUMBER_OF_TASKS         2     //Cantidad de tareas del sistema

//Definir variables de temporizacion de las tareas
static uint32_t timePrev[NUMBER_OF_TASKS]={0};


void setup() {
 
   //Iniciar puerto serie
  Serial.begin(115200);



  

    //Iniciar tiempo en milisegundos de ejecucion de tareas
  Task_Init();

}

void loop() {
  if((millis() - timePrev[0])>=TASK1_POLLRATE_MS){
    timePrev[0]=millis();

    int16_t rawADC=analogRead(A0);

    Serial.print("rawADC:");
    Serial.println(rawADC, DEC);

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

static float getVoltaje_RMS(void){

}
