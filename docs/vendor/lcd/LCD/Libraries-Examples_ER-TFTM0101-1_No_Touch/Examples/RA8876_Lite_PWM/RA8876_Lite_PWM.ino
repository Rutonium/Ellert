#include <SPI.h>
#include "Arduino.h"
#include "Print.h"
#include "Ra8876_Lite.h"

const int RA8876_XNSCS = 10;
const int RA8876_XNRESET = 9;

Ra8876_Lite ra8876lite(RA8876_XNSCS, RA8876_XNRESET);  
   
void setup() {
   Serial.begin(9600);
   Serial.println("RA8876 Lite");
   
  pinMode(8, OUTPUT);  //backlight 
  digitalWrite(8, HIGH);//on

   
   delay(100);
   if (!ra8876lite.begin()) 
   {
   Serial.println("RA8876 or RA8877 Fail");
   while (1);
   }
   Serial.println("RA8876 or RA8877 Pass!");
   
   ra8876lite.displayOn(true);
 
}

void loop() {
   unsigned short i;
  //clear page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE);
  
  /*pwm demo please measure by scope*/
  ra8876lite.pwm_Prescaler(RA8876_PRESCALER); //if core_freq = 120MHz, pwm base clock = 120/(3+1) = 30MHz
  ra8876lite.pwm_ClockMuxReg(RA8876_PWM_TIMER_DIV4, RA8876_PWM_TIMER_DIV4, RA8876_XPWM1_OUTPUT_PWM_TIMER1,RA8876_XPWM0_OUTPUT_PWM_TIMER0);
                               //pwm timer clock = 30/4 = 7.5MHz
                 
  ra8876lite.pwm0_ClocksPerPeriod(1024); // pwm0 = 7.5MHz/1024 = 7.3KHz
  ra8876lite.pwm0_Duty(10);//pwm0 set 10/1024 duty
               
  ra8876lite.pwm1_ClocksPerPeriod(256);  // pwm1 = 7.5MHz/256 = 29.2KHz
  ra8876lite.pwm1_Duty(5); //pwm1 set 5/256 duty
 
  ra8876lite.pwm_Configuration(RA8876_PWM_TIMER1_INVERTER_ON,RA8876_PWM_TIMER1_AUTO_RELOAD,RA8876_PWM_TIMER1_START,RA8876_PWM_TIMER0_DEAD_ZONE_DISABLE 
                      , RA8876_PWM_TIMER0_INVERTER_ON, RA8876_PWM_TIMER0_AUTO_RELOAD,RA8876_PWM_TIMER0_START);                     
  
  
 while(1);
  
}


