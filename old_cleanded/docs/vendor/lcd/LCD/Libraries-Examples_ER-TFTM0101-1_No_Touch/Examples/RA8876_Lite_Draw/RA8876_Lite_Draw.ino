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
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLACK);
  
  //draw on page1 
  ra8876lite.drawLine(40,40,159,159,COLOR65K_RED);
  ra8876lite.drawLine(40,159,159,40,COLOR65K_LIGHTRED);
  
  ra8876lite.drawSquare(200+30, 50, 399-30, 199-50, COLOR65K_GRAYSCALE23);
  ra8876lite.drawSquareFill(420, 20, 1023, 179, COLOR65K_GREEN);
  
  ra8876lite.drawCircleSquare(600+30,0+50, 1023-30, 199-50, 20, 20, COLOR65K_BLUE2);
  ra8876lite.drawCircleSquareFill(50,200, 149, 399, 10, 10, COLOR65K_BLUE);
  
  ra8876lite.drawTriangle(220,250,360,360,250,380,COLOR65K_MAGENTA);
  ra8876lite.drawTriangleFill(500,220,580,380,420,380,COLOR65K_LIGHTMAGENTA);

  ra8876lite.drawCircle(700,300,80,COLOR65K_YELLOW);
  ra8876lite.drawCircleFill(900,300,80,COLOR65K_YELLOW);
  ra8876lite.drawCircleFill(100,500,60,COLOR65K_LIGHTYELLOW);
  
  ra8876lite.drawEllipse(300,500,50,80,COLOR65K_CYAN);
  ra8876lite.drawEllipseFill(500,500,80,50,COLOR65K_LIGHTCYAN);
   ra8876lite.drawEllipseFill(800,500,150,50,COLOR65K_LIGHTCYAN); 

 delay(3000);
i=25;  
ra8876lite.drawSquareFill(0, 0, 1023, i-1, COLOR65K_WHITE);
ra8876lite.drawSquareFill(0, i, 1023, i*2-1, COLOR65K_BLACK);
ra8876lite.drawSquareFill(0, i*2, 1023, i*3-1, COLOR65K_RED);
ra8876lite.drawSquareFill(0, i*3, 1023, i*4-1, COLOR65K_LIGHTRED);
ra8876lite.drawSquareFill(0, i*4, 1023, i*5-1, COLOR65K_DARKRED);
ra8876lite.drawSquareFill(0, i*5, 1023, i*6-1, COLOR65K_GREEN);
ra8876lite.drawSquareFill(0, i*6, 1023, i*7-1, COLOR65K_LIGHTGREEN);
ra8876lite.drawSquareFill(0, i*7, 1023, i*8-1, COLOR65K_DARKGREEN);
ra8876lite.drawSquareFill(0, i*8, 1023, i*9-1, COLOR65K_BLUE);
ra8876lite.drawSquareFill(0, i*9, 1023, i*10-1, COLOR65K_BLUE2);
ra8876lite.drawSquareFill(0, i*10, 1023, i*11-1, COLOR65K_LIGHTBLUE);
ra8876lite.drawSquareFill(0, i*11, 1023, i*12-1, COLOR65K_DARKBLUE);
ra8876lite.drawSquareFill(0, i*12, 1023, i*13-1, COLOR65K_YELLOW);
ra8876lite.drawSquareFill(0, i*13, 1023, i*14-1, COLOR65K_LIGHTYELLOW);
ra8876lite.drawSquareFill(0, i*14, 1023, i*15-1, COLOR65K_DARKYELLOW);
ra8876lite.drawSquareFill(0, i*15, 1023, i*16-1, COLOR65K_CYAN);
ra8876lite.drawSquareFill(0, i*16, 1023, i*17-1, COLOR65K_LIGHTCYAN);
ra8876lite.drawSquareFill(0, i*17, 1023, i*18-1, COLOR65K_DARKCYAN);
ra8876lite.drawSquareFill(0, i*18, 1023, i*19-1, COLOR65K_MAGENTA);
ra8876lite.drawSquareFill(0, i*19, 1023, i*20-1, COLOR65K_LIGHTMAGENTA);
ra8876lite.drawSquareFill(0, i*20, 1023, i*21-1, COLOR65K_DARKMAGENTA);
ra8876lite.drawSquareFill(0, i*21, 1023, i*22-1, COLOR65K_BROWN);
 delay(3000);
//  while(1);
}


