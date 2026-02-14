#include <SD.h>
#include <SPI.h>
#include "Arduino.h"
#include "Print.h"
#include "Ra8876_Lite.h"

void sdCardShowPicture16bpp(unsigned short x,unsigned short y,unsigned short width, unsigned short height,char *filename);
void sdCardShowPicture16bppBteMpuWriteWithROP(unsigned long s1_addr, unsigned short s1_image_width, unsigned short s1_x, unsigned short s1_y,
                                             unsigned long des_addr, unsigned short des_image_width, unsigned short des_x, unsigned short des_y, 
                                              unsigned short width, unsigned short height,unsigned char rop_code,char *filename);
void sdCardShowPicture16bppBteMpuWriteWithChromaKey(unsigned long des_addr, unsigned short des_image_width, unsigned short des_x, unsigned short des_y, unsigned short width, unsigned short height, unsigned short chromakey_color,char *filename);
void sdCardShowPicture16bppBteMpuWriteColorExpansion(unsigned long des_addr, unsigned short des_image_width, unsigned short des_x, unsigned short des_y, unsigned short width, unsigned short height,
                                                       unsigned short foreground_color,unsigned short background_color,char *filename);
void sdCardShowPicture16bppBteMpuWriteColorExpansionWithChromaKey(unsigned long des_addr, unsigned short des_image_width, unsigned short des_x, unsigned short des_y, unsigned short width, unsigned short height,
                                                       unsigned short foreground_color,unsigned short background_color,char *filename);
                                                       
const int SD_CARD_SCS = 5;

const int RA8876_XNSCS = 10;
const int RA8876_XNRESET = 9;

Ra8876_Lite ra8876lite(RA8876_XNSCS, RA8876_XNRESET);  
   
void setup() {
   Serial.begin(9600);
   Serial.println("RA8876 Lite");

  pinMode(8, OUTPUT);  //backlight 
  digitalWrite(8, HIGH);//on

   if(SD.begin(SD_CARD_SCS))
   Serial.println("SD card initialized");
   else
   Serial.println("SD card failure");
   
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

  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLACK);
  
  sdCardShowPicture16bpp(0,0,800,600,"wp2.bin");
  //while(1);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_ENABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.putString(0,10,"Read picture from sd card and write to ra8876 with BTE ROP");
  
  sdCardShowPicture16bppBteMpuWriteWithROP(PAGE1_START_ADDR, SCREEN_WIDTH, 50, 50, PAGE1_START_ADDR, SCREEN_WIDTH, 50, 50, 
                                           128, 128,RA8876_BTE_ROP_CODE_3,"home.bin");
  sdCardShowPicture16bppBteMpuWriteWithROP(PAGE1_START_ADDR, SCREEN_WIDTH, 50+128, 50, PAGE1_START_ADDR, SCREEN_WIDTH, 50+128, 50, 
                                           128, 128,RA8876_BTE_ROP_CODE_6,"appli.bin");                                            
  sdCardShowPicture16bppBteMpuWriteWithROP(PAGE1_START_ADDR, SCREEN_WIDTH, 50+128+128, 50, PAGE1_START_ADDR, SCREEN_WIDTH, 50+128+128, 50, 
                                           128, 128,RA8876_BTE_ROP_CODE_8,"sound.bin"); 
 // while(1);                                         
 ra8876lite.putString(0,50+128+10,"Read picture from sd card and write to ra8876 with BTE Chroma Key");
   
  sdCardShowPicture16bppBteMpuWriteWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH,50, 50+128+50,128,128,0xf800,"home.bin");
  sdCardShowPicture16bppBteMpuWriteWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH,50+128, 50+128+50,128,128,0xf800,"appli.bin");
  sdCardShowPicture16bppBteMpuWriteWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH,50+128+128, 50+128+50,128,128,0xf800,"sound.bin");      
  delay(5000);
  //while(1);
  sdCardShowPicture16bpp(0,0,800,600,"wp23.bin");
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_ENABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.putString(0,10,"Read picture from sd card and write to ra8876 with BTE color expansion");
  
  sdCardShowPicture16bppBteMpuWriteColorExpansion(PAGE1_START_ADDR,SCREEN_WIDTH,50, 50, 128, 128,COLOR65K_CYAN,COLOR65K_MAGENTA,"sun.bin");
  sdCardShowPicture16bppBteMpuWriteColorExpansion(PAGE1_START_ADDR,SCREEN_WIDTH,50+128, 50, 128, 128,COLOR65K_BLACK,COLOR65K_WHITE,"cloud.bin");
  sdCardShowPicture16bppBteMpuWriteColorExpansion(PAGE1_START_ADDR,SCREEN_WIDTH,50+128+128, 50, 128, 128,COLOR65K_BLUE,COLOR65K_RED,"rain.bin");
  
  ra8876lite.putString(0,50+128+10,"Read picture from sd card and write to ra8876 with BTE color expansion with chroma key");
  sdCardShowPicture16bppBteMpuWriteColorExpansionWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH, 50, 50+128+50+10, 128, 128,COLOR65K_WHITE,COLOR65K_BLACK,"sun.bin");
  sdCardShowPicture16bppBteMpuWriteColorExpansionWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH, 50+128, 50+128+50+10, 128, 128,COLOR65K_WHITE,COLOR65K_BLACK,"cloud.bin");
  sdCardShowPicture16bppBteMpuWriteColorExpansionWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH, 50+128+128, 50+128+50+10, 128, 128,COLOR65K_WHITE,COLOR65K_BLACK,"rain.bin");
  delay(5000);
  //while(1);
}

//**************************************************************//
//**************************************************************//
void sdCardShowPicture16bpp(unsigned short x,unsigned short y,unsigned short width, unsigned short height,char *filename)
{
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(filename);
  // if the file is available, read it and write to ra8876:
  if (dataFile) {  
       ra8876lite.putPicture_16bpp(x,y,width,height);    
    while (dataFile.available()) 
    {
        //Serial.write(dataFile.read());
        //checkWriteFifoNotFull();//if high speed mcu and without Xnwait check
        ra8876lite.lcdDataWrite(dataFile.read());
        ra8876lite.lcdDataWrite(dataFile.read());
    }
    dataFile.close();
  }   
}
//**************************************************************//
//**************************************************************//
void sdCardShowPicture16bppBteMpuWriteWithROP(unsigned long s1_addr, unsigned short s1_image_width, unsigned short s1_x, unsigned short s1_y,
                                             unsigned long des_addr, unsigned short des_image_width, unsigned short des_x, unsigned short des_y, 
                                              unsigned short width, unsigned short height,unsigned char rop_code,char *filename)
{
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(filename);
  // if the file is available, read it and write to ra8876:
  if (dataFile) {  
      ra8876lite.bteMpuWriteWithROP(s1_addr, s1_image_width, s1_x, s1_y, des_addr, des_image_width,
                                   des_x, des_y, width, height, rop_code); 
                         
    while (dataFile.available()) 
    {
        //Serial.write(dataFile.read());
        //checkWriteFifoNotFull();//if high speed mcu and without Xnwait check
        ra8876lite.lcdDataWrite(dataFile.read());
        ra8876lite.lcdDataWrite(dataFile.read());
    }
    dataFile.close();
  }   
}
//**************************************************************//
//**************************************************************//
void sdCardShowPicture16bppBteMpuWriteWithChromaKey(unsigned long des_addr, unsigned short des_image_width, unsigned short des_x, unsigned short des_y, unsigned short width, unsigned short height,unsigned short chromakey_color,char *filename)
{
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(filename);
  // if the file is available, read it and write to ra8876:
  if (dataFile) {  
       ra8876lite.bteMpuWriteWithChromaKey(des_addr,des_image_width,des_x,des_y, width,height,chromakey_color); 
    while (dataFile.available()) 
    {
        //Serial.write(dataFile.read());
        //checkWriteFifoNotFull();//if high speed mcu and without Xnwait check
        ra8876lite.lcdDataWrite(dataFile.read());
        ra8876lite.lcdDataWrite(dataFile.read());
    }
    dataFile.close();
  }   
}
//**************************************************************//
//**************************************************************//
void sdCardShowPicture16bppBteMpuWriteColorExpansion(unsigned long des_addr, unsigned short des_image_width, unsigned short des_x, unsigned short des_y, unsigned short width, unsigned short height,
                                                       unsigned short foreground_color,unsigned short background_color,char *filename)
{
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(filename);
  // if the file is available, read it and write to ra8876:
  if (dataFile) {  
       ra8876lite.bteMpuWriteColorExpansion(des_addr, des_image_width, des_x, des_y, width, height, foreground_color, background_color); 
    while (dataFile.available()) 
    {
        //Serial.write(dataFile.read());
        //checkWriteFifoNotFull();//if high speed mcu and without Xnwait check
        ra8876lite.lcdDataWrite(dataFile.read());
        ra8876lite.lcdDataWrite(dataFile.read());
    }
    dataFile.close();
  }   
}
//**************************************************************//
//**************************************************************//
void sdCardShowPicture16bppBteMpuWriteColorExpansionWithChromaKey(unsigned long des_addr, unsigned short des_image_width, unsigned short des_x, unsigned short des_y, unsigned short width, unsigned short height,
                                                       unsigned short foreground_color,unsigned short background_color,char *filename)
{
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(filename);
  // if the file is available, read it and write to ra8876:
  if (dataFile) {  
       ra8876lite.bteMpuWriteColorExpansionWithChromaKey(des_addr, des_image_width, des_x, des_y, width, height, foreground_color,background_color); 
    while (dataFile.available()) 
    {
        //Serial.write(dataFile.read());
        //checkWriteFifoNotFull();//if high speed mcu and without Xnwait check
        ra8876lite.lcdDataWrite(dataFile.read());
        ra8876lite.lcdDataWrite(dataFile.read());
    }
    dataFile.close();
  }   
}
