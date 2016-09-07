#include <Stepper.h>
#include <LiquidCrystal.h>
#include "M2tk.h"
#include "utility/m2ghnlc.h"
#include <EEPROMex.h>    //https://github.com/thijse/Arduino-Libraries/tree/master/EEPROMEx


//constantes mecanicas
#define STEPS_REVOLUTION   200
#define PUMP_DIAMETER      45.3
#define TUBE_DIAMETER      0.8
const float angleStep = (float)360.0/STEPS_REVOLUTION;
const double mLStep   = 360 * 1000 / ( 2 * pow(PI,2) * PUMP_DIAMETER * pow(TUBE_DIAMETER,2) * angleStep );
/*
  L = 2 · Pi · r · a/360      
 V = Pi · r² · L
 A = 360 · L / ( 2 · Pi · r )
 N = a/Ap
 N = 360 * V / ( 2 * Pi * Ro * Ri*Ri * Ap )
 V = N * 2 * Pi*Pi * Ro * Ri*Ri * Ap / 360  
 */




//pin out
#define LCD_DATA_7      2
#define LCD_DATA_6      3
#define LCD_DATA_5      4
#define LCD_DATA_4      5
#define LCD_DATA_E      6
#define LCD_DATA_RS     7   
#define MOTOR_PIN1      12
#define MOTOR_PIN2      13
#define MOTOR_PIN3      10
#define MOTOR_PIN4      11
#define BUTTON_SELECT   8
#define BUTTON_NEXT     9


//Define los estados del menu
#define STATE_SETUP_FLUX    1
#define STATE_WAIT_FLUX     2
#define STATE_SETUP_VOL     3
#define STATE_WAIT_VOL      4
#define STATE_SETUP_INFO    5
#define STATE_WAIT_INFO     6


//Define EEPROM addresses
#define ADDR_FLUJO      1
#define ADDR_VOL_FINAL  10


//Definicion de variables
long  rpm;    
long  currentSteps;         //numero actual de pasos
long  totalSteps;           //numero total de pasos a dar
uint32_t  volumenFinal;         //mL                              //FIXME!!: se puede eliminar y calcular directamente a partir de totalSteps
uint8_t   flujo;                //##.# mL/min                     //FIXME!!: se puede eliminar y calcular directamente a partir de velocidad de rotacion 
uint8_t   state; 	        //contiene el estado de ejecucion
extern    M2tk m2;              //gestor de menus


//inicializaciones
Stepper         stepper    (STEPS_REVOLUTION, MOTOR_PIN1,MOTOR_PIN2,MOTOR_PIN3,MOTOR_PIN4);
LiquidCrystal   lcd        (LCD_DATA_RS, LCD_DATA_E, LCD_DATA_4, LCD_DATA_5, LCD_DATA_6, LCD_DATA_7);


/* ===== M E N U ===== */

void fn_flujo_input_ok(m2_el_fnarg_p fnarg) {
  EEPROM.writeByte(ADDR_FLUJO, flujo); 

  rpm   = 360.0 * (flujo) / ( 2 * pow(PI,2) * PUMP_DIAMETER * pow(TUBE_DIAMETER,2) * angleStep );    //N = 360 * V / ( 2 * Pi * Ro * Ri*Ri * Ap )  
  stepper.setSpeed(rpm);
  
  Serial.print("flujo: ");
  Serial.println(flujo);
  Serial.println(rpm);

  state = STATE_SETUP_VOL;  
  m2.clear();  //elimina el menu y para de procesar eventos hasta que se vuelva a llamar a setRoot()
}


void fn_volumen_input_ok(m2_el_fnarg_p fnarg) {
  EEPROM.writeLong(ADDR_VOL_FINAL, volumenFinal);  

  totalSteps = 360.0 * volumenFinal * 1000 / ( 2 * pow(PI,2) * PUMP_DIAMETER * pow(TUBE_DIAMETER,2) * angleStep );    
  Serial.print("total steps: ");
  Serial.println(totalSteps);
  
  state = STATE_SETUP_INFO;
  m2.clear();
}


void fn_status_input_editar(m2_el_fnarg_p fnarg) {
  state = STATE_SETUP_FLUX;
  Serial.println("editar");
  m2.clear();
}


void fn_status_input_stop(m2_el_fnarg_p fnarg) {
  Serial.println("stop");
  
  stepper.setSpeed(0);

  rpm          = 0;
  currentSteps = 0;         
  totalSteps   = 0;
  state        = STATE_SETUP_FLUX;  
  m2.clear();
}


const char *tiempoRestante_cb(m2_rom_void_p element) {
  Serial.print("tiempo restante: ");
  static const char buff[] = "     ";
  float kk = (float)totalSteps / ( STEPS_REVOLUTION * rpm );
  //sprintf(buff, "%f", kk);
  
  Serial.println(buff);
  Serial.println(kk);
  return buff;  
}

const char *volumenActual_cb(m2_rom_void_p element) { 
  Serial.print("volumen actual: ");
  char buff[7]; 
  sprintf(buff, "%f", (float) currentSteps * 2 * pow(PI,2) * PUMP_DIAMETER * pow(TUBE_DIAMETER,2) * angleStep / 360.0 );          //V = N * 2 * Pi*Pi * Ro * Ri*Ri * Ap / 360

  Serial.println(buff);
  return buff;
}


M2_U8NUM  (el_flujo_input_u8, "c2", 0, 25, &flujo);      //TODO: cambiar para que se llame a una funcion en lugar de guardar 2 variables: rpm y flujo! (https://code.google.com/p/m2tklib/wiki/elref#U32NUM)
M2_BUTTON (el_flujo_input_ok, "", "ok", fn_flujo_input_ok);
M2_LIST   (el_flujo_list_a) = {&el_flujo_input_u8,&el_flujo_input_ok};
M2_HLIST  (el_flujo_hlist_a, NULL, el_flujo_list_a);
M2_LABEL  (el_flujo_label, NULL, "Flujo (mL/min)");
M2_LIST   (el_flujo_list_b) = {&el_flujo_label, &el_flujo_hlist_a};
M2_VLIST  (top_el_flujo, NULL, el_flujo_list_b);

M2_LABEL  (el_volumen_label, NULL, "Volumen (mL)");
M2_U32NUM (el_volumen_input_u32, "c4", &volumenFinal);      //TODO: cambiar para que se llame a una funcion en lugar de guardar 2 variables: steps y volumen!
M2_BUTTON (el_volumen_input_ok, "", "start", fn_volumen_input_ok);
M2_LIST   (el_volumen_list_a) = {&el_volumen_input_u32, &el_volumen_input_ok };
M2_HLIST  (el_volumen_hlist, NULL, el_volumen_list_a);
M2_LIST   (el_volumen_list_b) = {&el_volumen_label, &el_volumen_hlist};
M2_VLIST  (top_el_volumen, NULL, el_volumen_list_b);

M2_LABELFN (el_status_tiempoRestanteFn, NULL, tiempoRestante_cb);
M2_LABEL   (el_status_tiempoRestante_label, NULL, "min");
M2_LABELFN (el_status_volumenActualFn, NULL, volumenActual_cb);
M2_LABEL   (el_status_volumenActual_label, NULL, "mL");
M2_LIST    (el_status_list_a) = {&el_status_tiempoRestanteFn, &el_status_tiempoRestante_label, &el_status_volumenActualFn, &el_status_volumenActual_label};
M2_HLIST   (el_status_hlist_a, NULL, el_status_list_a);
M2_BUTTON  (el_status_input_edit, "", "editar", fn_status_input_editar);
M2_BUTTON  (el_status_input_stop, "", "STOP", fn_status_input_stop);
M2_LIST    (el_status_list_b) = {&el_status_input_edit, &el_status_input_stop};
M2_HLIST   (el_status_hlist_b, NULL, el_status_list_b);
M2_LIST    (el_status_list_c) = {&el_status_hlist_a, &el_status_hlist_b};
M2_VLIST   (top_el_status, NULL, el_status_list_c);

M2tk  m2   (&top_el_flujo, m2_es_arduino, m2_eh_2bs, m2_gh_nlc);

/* ===== E N D   M E N U ===== */




void setup() {  
  Serial.begin(9600);
  Serial.println("init...");  
  
  m2_SetNewLiquidCrystal(&lcd, 16, 2);
  m2.setPin(M2_KEY_SELECT, BUTTON_SELECT);
  m2.setPin(M2_KEY_NEXT, BUTTON_NEXT);

  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(MOTOR_PIN3, OUTPUT);
  pinMode(MOTOR_PIN4, OUTPUT);

  flujo        = EEPROM.readByte(ADDR_FLUJO);
  volumenFinal = EEPROM.readLong(ADDR_VOL_FINAL);
  rpm          = 0;
  currentSteps = 0;         
  totalSteps   = 0;
  state        = STATE_SETUP_FLUX;  
  
  Serial.println("Done!");
}



void loop() {
  m2.checkKey();
  m2.checkKey();
  if ( m2.handleKey() ) {
    m2.draw();                
  }

  m2.checkKey();

  set_next_menu_state();  

  m2.checkKey();

  if (currentSteps < totalSteps){        
    stepper.step(1);                
    currentSteps++;    
    
    float eta = (float)(totalSteps-currentSteps) / ( STEPS_REVOLUTION * rpm );
    float vol = (float) currentSteps * 2 * pow(PI,2) * PUMP_DIAMETER * pow(TUBE_DIAMETER,2) * angleStep / 360.0 / 1000;
    
    lcd.home();
    lcd.print("Tiempo:  "); 
    lcd.print(eta);
    lcd.setCursor ( 0, 1 );
    lcd.print("Volumen: ");
    lcd.print (vol);
  }


  /*  Serial.println("clockwise");
   // read the sensor value:
   int sensorReading = analogRead(A0);
   // map it to a range from 0 to 100:
   int motorSpeed = map(sensorReading, 0, 1023, 0, 100);
   // set the motor speed:
   if (motorSpeed > 0) {
   myStepper.setSpeed(motorSpeed);
   // step 1/100 of a revolution:
   myStepper.step(stepsPerRevolution/100);
   } */
}






void set_next_menu_state(void) {
  switch(state){
  case STATE_SETUP_FLUX:
    m2.setRoot(&top_el_flujo); 
    state = STATE_WAIT_FLUX;
    Serial.println("setup flux");
    break;  

  case STATE_WAIT_FLUX:          //state is changed in callback procedure
    break;

  case STATE_SETUP_VOL:
    m2.setRoot(&top_el_volumen); 
    state = STATE_WAIT_VOL;
    Serial.println("setup vol");
    break;  

  case STATE_WAIT_VOL:          //state is changed in callback procedure
    break;

  case STATE_SETUP_INFO:
    //m2.setRoot(&top_el_status); 
    m2.setRoot(&m2_null_element);
    state = STATE_WAIT_INFO;
    Serial.println("setup info");
    break;  

  case STATE_WAIT_INFO:        //state is changed in callback procedure
    break;   

  default:
    state = STATE_SETUP_FLUX;
    break;     
  }
}

















