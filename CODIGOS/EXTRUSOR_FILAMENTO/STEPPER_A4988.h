
#ifndef STEPPER_A4988_h
#define STEPPER_A4988_h

// the #include statment and code go here...
#include "Arduino.h"
#include <TimerOne.h>


//Defines
//Defines pins numbers...............ver documento "RAMPS_1-4manual.pdf"
#define STEP_PIN                          26
#define DIR_PIN                           28
#define EN_PIN                            24

//Definir la velocidad de toogleo del tren de pulsos hacia el motor
#define TICK_STEPPER_US                   500
//#define TICK_STEPPER_US                   250

/*
  Global variables
  Access: ISR
*/
//Definir variables de interrupcion (aquellas que seran modificadas en los ISR)
static volatile uint8_t stepPinState=LOW;

/*
  Function prototypes
  Access  Public
*/
extern void STEPPER_A4988_Initialize(void);
extern void STEPPER_A4988_Rotate(const bool direction);
extern void STEPPER_A4988_Stop(void);

/*
  Function prototypes
  Access  Private
*/
static void STEPPER_A4988_Enable(void);
static void STEPPER_A4988_Disable(void);

/*
  Function prototypes
  Access  ISR  (Solo sera accesada por el ISR)
*/
void toogleSTEP_PAP(void);

#endif