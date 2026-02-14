#include <SPI.h>
#include "Arduino.h"
#include "Print.h"
#include "Ra8876_Lite.h"

#define DMA_DEMO_24BIT_ADDR
//#define DMA_DEMO_32BIT_ADDR

const int WP1=0;
const int WP2=960000;
const int WP3=1920000;
const int WP4=2880000;
const int WP5=3840000;
const int WP6=4800000;
const int WP7=5760000;
const int WP8=6720000;
const int WP9=7680000;
const int WP10=8640000;
const int WP11=9600000;
const int WP12=10560000;
const int WP13=11520000;
const int WP14=12480000;
const int WP15=13440000;
const int WP16=14400000;
const int WP17=15360000;
const int WP18=16320000;
const int WP19=17280000;
const int WP20=18240000;
const int WP21=19200000;
const int WP22=20160000;
const int WP23=21120000;
const int WP24=22080000;
const int WP25=23040000;
const int WP26=24000000;
const int WP27=24960000;
const int WP28=25920000;
const int WP29=26880000;
const int WP30=27840000;

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

#ifdef DMA_DEMO_24BIT_ADDR   
  /***Demo function demo***/
  /*DMA demo 24bit address*/ 
  while(1)
  {
  //clear page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLACK); 

//  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
//  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
//  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_ENABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
//  ra8876lite.putString(0,0,"DMA demo 24bit address");
//  delay(2000);
 //demo 24bit address serial flash DMA function
  ra8876lite.dma_24bitAddressBlockMode(RA8876_SERIAL_FLASH_SELECT1,RA8876_SPI_DIV2,0,0,800,600,800,WP2);
  delay(2000);

//demo  24bit address serial flash DMA partial 
// x0 = 50
// y0 = 60
// width = 400
// height = 300
// picture_width = 800
// addr = WP2+(800*2*300)+(400*2)=960000+480000+800 = 1440800

  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLACK); 
  ra8876lite.dma_24bitAddressBlockMode(RA8876_SERIAL_FLASH_SELECT1,RA8876_SPI_DIV2,50,60,400,300,800,1440800);
    delay(2000);
  //while(1);
  }
#endif   

#ifdef DMA_DEMO_32BIT_ADDR  
  /*DMA demo 32bit address*/ 
  //when using the 32bit address serial flash, must be setting serial flash to 4Bytes mode 
  //only needs set one times after power on
  ra8876lite.setSerialFlash4BytesMode(1);  
  while(1)
  {
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLACK); 
  delay(1000);
//  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
//  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
//  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_ENABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
//  ra8876lite.putString(0,0,"DMA demo 32bit address");
  //delay(2000);
  //demo 32bit address serial flash DMA function
  ra8876lite.dma_32bitAddressBlockMode(RA8876_SERIAL_FLASH_SELECT1,RA8876_SPI_DIV2,0,0,800,600,800,WP10);
  //delay(2000);
  delay(1000);
  ra8876lite.dma_32bitAddressBlockMode(RA8876_SERIAL_FLASH_SELECT1,RA8876_SPI_DIV2,0,0,800,600,800,WP21);
  //delay(2000);
  delay(1000);
  ra8876lite.dma_32bitAddressBlockMode(RA8876_SERIAL_FLASH_SELECT1,RA8876_SPI_DIV2,0,0,800,600,800,WP27);
  //delay(2000);
  delay(1000);
//while(1);
  }
  //while(1);
 #endif    
}


