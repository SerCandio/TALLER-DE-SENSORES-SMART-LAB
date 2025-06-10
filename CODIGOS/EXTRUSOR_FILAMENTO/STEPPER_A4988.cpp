#include "STEPPER_A4988.h"

extern void STEPPER_A4988_Initialize(void){
  //Configurar los Pines I/O del driver A4988 hacia el motor PAP
  pinMode(STEP_PIN,OUTPUT); 
  digitalWrite(STEP_PIN, LOW);
  pinMode(DIR_PIN,OUTPUT);

  //Inhabilitar el driver A4988 (pin EN como entrada)
  STEPPER_A4988_Disable();

  //Configurar el periferico Timer (es el que accionara el motor PAP mediante el driver A4988)
  Timer1.initialize(TICK_STEPPER_US);
  Timer1.attachInterrupt(toogleSTEP_PAP); 
  Timer1.stop();                    //Inicialmente detener el Timer1

}

extern void STEPPER_A4988_Rotate(const bool direction){
  //Habilitar el motor A4988
  STEPPER_A4988_Enable();

  //Establecer la direccion de giro
  if(direction==true)     digitalWrite(DIR_PIN,HIGH); 
  else                    digitalWrite(DIR_PIN,LOW); 

  //Activar el envio de pulsos (desde la interrupcion)
  Timer1.start();

}

extern void STEPPER_A4988_Stop(void){
   //Desactivar el envio de pulsos (desde la interrupcion)
  Timer1.stop();

  //Deshabilitar el pin ENABLE
  STEPPER_A4988_Disable();
  
}

static void STEPPER_A4988_Enable(void){
   //Habilitar el EN  a LOW (para poder hacer giros)
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);
  
}

static void STEPPER_A4988_Disable(void){
  //Deshabilitar el EN............ya se tiene un pull up en la placa
  pinMode(EN_PIN, INPUT);

}

/*
  ISR's subrutinas de interrupcion
*/
void toogleSTEP_PAP(void){
  stepPinState=!stepPinState;
  digitalWrite(STEP_PIN, stepPinState); 
}