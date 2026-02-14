#include <SPI.h>
#include "Arduino.h"
#include "Print.h"
#include "Ra8876_Lite.h"
#include "pic16bpp_byte.h"
#include "pic16bpp_word.h"
#include "bw.h"

#include "pattern6.h"
#include "pattern11.h"
#include "bug1.h"

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
  /***bte memory copy demo***/
  //clear page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, COLOR65K_BLUE);
  
  //clear page2
  ra8876lite.canvasImageStartAddress(PAGE2_START_ADDR);
  ra8876lite.drawSquareFill(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, COLOR65K_RED);
  
  //write picture to page2
  ra8876lite.putPicture_16bpp(50,50,128,128,pic16bpp_word);
  
  //write string to page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_ENABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.putString(0,0,"BTE memory copy,copy page2 picture to page1 display");
  //copy page2 picture to page1 
  ra8876lite.bteMemoryCopy(PAGE2_START_ADDR,SCREEN_WIDTH,50,50,PAGE1_START_ADDR,SCREEN_WIDTH, 50,50,128,128); 
  ra8876lite.bteMemoryCopy(PAGE2_START_ADDR,SCREEN_WIDTH,50,50,PAGE1_START_ADDR,SCREEN_WIDTH, (50+128),50,128,128);
  ra8876lite.bteMemoryCopy(PAGE2_START_ADDR,SCREEN_WIDTH,50,50,PAGE1_START_ADDR,SCREEN_WIDTH, (50+128+128),50,128,128);
  //ra8876lite.mainImageStartAddress(PAGE2_START_ADDR);
  delay(3000);
  //while(1);
  /***bte memory copy with rop demo***/ 
  //write string to page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.putString(0,178,"BTE memory copy with ROP, copy page2 picture to page1 display"); 
  ra8876lite.bteMemoryCopyWithROP(PAGE2_START_ADDR,SCREEN_WIDTH,50,50,PAGE1_START_ADDR,SCREEN_WIDTH,50,228,
                            PAGE1_START_ADDR,SCREEN_WIDTH, 50,228,128,128,RA8876_BTE_ROP_CODE_1); 
  ra8876lite.bteMemoryCopyWithROP(PAGE2_START_ADDR,SCREEN_WIDTH,50,50,PAGE1_START_ADDR,SCREEN_WIDTH,(50+128),228,
                            PAGE1_START_ADDR,SCREEN_WIDTH, (50+128),228,128,128,RA8876_BTE_ROP_CODE_2);
  ra8876lite.bteMemoryCopyWithROP(PAGE2_START_ADDR,SCREEN_WIDTH,50,50,PAGE1_START_ADDR,SCREEN_WIDTH,(50+128+128),228,
                            PAGE1_START_ADDR,SCREEN_WIDTH, (50+128+128),228,128,128,RA8876_BTE_ROP_CODE_3);  
  /***bte memory copy with chromakey demo***/ 
  ra8876lite.putString(0,356,"BTE memory copy with ChromaKey, copy page2 picture to page1 display"); 
  ra8876lite.bteMemoryCopyWithChromaKey(PAGE2_START_ADDR,SCREEN_WIDTH,50,50, PAGE1_START_ADDR,SCREEN_WIDTH,50,406,128,128,0xf800);
  ra8876lite.bteMemoryCopyWithChromaKey(PAGE2_START_ADDR,SCREEN_WIDTH,50,50, PAGE1_START_ADDR,SCREEN_WIDTH,50+128,406,128,128,0xf800);
  ra8876lite.bteMemoryCopyWithChromaKey(PAGE2_START_ADDR,SCREEN_WIDTH,50,50, PAGE1_START_ADDR,SCREEN_WIDTH,50+128+128,406,128,128,0xf800);
  delay(3000); 
  //while(1);
  //clear page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE); 
  
  /***bte MPU write with ROP demo***/
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_ENABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.putString(0,0,"BTE MPU write with ROP, write picture to page1, format byte");
  ra8876lite.bteMpuWriteWithROP(PAGE1_START_ADDR,SCREEN_WIDTH,50,50,PAGE1_START_ADDR,SCREEN_WIDTH,
                                50,50,128,128,RA8876_BTE_ROP_CODE_4,pic16bpp_byte);
  ra8876lite.bteMpuWriteWithROP(PAGE1_START_ADDR,SCREEN_WIDTH,50+128,50,PAGE1_START_ADDR,SCREEN_WIDTH,
                                50+128,50,128,128,RA8876_BTE_ROP_CODE_5,pic16bpp_byte);                         
  ra8876lite.bteMpuWriteWithROP(PAGE1_START_ADDR,SCREEN_WIDTH,50+128+128,50,PAGE1_START_ADDR,SCREEN_WIDTH,
                                50+128+128,50,128,128,RA8876_BTE_ROP_CODE_6,pic16bpp_byte); 
  ra8876lite.putString(0,178,"BTE MPU write with ROP, write picture to page1, format word");                              
  ra8876lite.bteMpuWriteWithROP(PAGE1_START_ADDR,SCREEN_WIDTH,50,228,PAGE1_START_ADDR,SCREEN_WIDTH,
                                50,228,128,128,RA8876_BTE_ROP_CODE_7,pic16bpp_word);
  ra8876lite.bteMpuWriteWithROP(PAGE1_START_ADDR,SCREEN_WIDTH,50+128,228,PAGE1_START_ADDR,SCREEN_WIDTH,
                                50+128,228,128,128,RA8876_BTE_ROP_CODE_8,pic16bpp_word);                         
  ra8876lite.bteMpuWriteWithROP(PAGE1_START_ADDR,SCREEN_WIDTH,50+128+128,228,PAGE1_START_ADDR,SCREEN_WIDTH,
                                50+128+128,228,128,128,RA8876_BTE_ROP_CODE_9,pic16bpp_word); 
  ra8876lite.putString(0,356,"BTE MPU write with Chroma Key, write picture to page1, format byte,word"); 
  ra8876lite.bteMpuWriteWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH, 50,406,128,128,0xf800,pic16bpp_byte);
  ra8876lite.bteMpuWriteWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH, 50+128,406,128,128,0xf800,pic16bpp_word);
  delay(3000);
  //while(1);
  //clear page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE); 
  /***bte MPU write color expansion demo***/
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_ENABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.putString(0,0,"BTE MPU write with color expansion, write black and white picture data to page1");
  ra8876lite.bteMpuWriteColorExpansion(PAGE1_START_ADDR,SCREEN_WIDTH,50,50,128,128,COLOR65K_BLACK,COLOR65K_WHITE,bw);
  ra8876lite.bteMpuWriteColorExpansion(PAGE1_START_ADDR,SCREEN_WIDTH,50+128,50,128,128,COLOR65K_WHITE,COLOR65K_BLACK,bw);
  ra8876lite.bteMpuWriteColorExpansion(PAGE1_START_ADDR,SCREEN_WIDTH,50+128+128,50,128,128,COLOR65K_YELLOW,COLOR65K_CYAN,bw);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
  ra8876lite.putString(0,178,"BTE MPU write with color expansion with chroma key, write black and white picture data to page1");
  
  ra8876lite.bteMpuWriteColorExpansionWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH,50,228,128,128,COLOR65K_BLACK,COLOR65K_WHITE,bw);
  ra8876lite.bteMpuWriteColorExpansionWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH,50+128,228,128,128,COLOR65K_WHITE,COLOR65K_BLACK,bw);
  ra8876lite.bteMpuWriteColorExpansionWithChromaKey(PAGE1_START_ADDR,SCREEN_WIDTH,50+128+128,228,128,128,COLOR65K_YELLOW,COLOR65K_BLACK,bw);
  delay(3000);
  //while(1);
  //clear page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE); 

  //write picture to pattern1 ram 
  ra8876lite.canvasImageStartAddress(PATTERN1_RAM_START_ADDR);
  ra8876lite.canvasImageWidth(16);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(16,16); 
  ra8876lite.putPicture_16bpp(0,0,16,16,pattern6);
  
  //write picture to pattern2 ram 
  ra8876lite.canvasImageStartAddress(PATTERN2_RAM_START_ADDR);
  ra8876lite.putPicture_16bpp(0,0,16,16,pattern11);
  
  //write picture to pattern3 ram 
  ra8876lite.canvasImageStartAddress(PATTERN3_RAM_START_ADDR);
  ra8876lite.putPicture_16bpp(0,0,16,16,bug1);
  
  //set canvas and active window back
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  
    /***bte pattern fill demo***/
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_ENABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.putString(0,0,"BTE pattern fill, fill 16x16 pattern to page1");
  
  ra8876lite.btePatternFill(1,PATTERN1_RAM_START_ADDR,16,0,0,PAGE1_START_ADDR,SCREEN_WIDTH, 50,50,700,128);
  
  ra8876lite.btePatternFill(1,PATTERN2_RAM_START_ADDR,16,0,0,PAGE1_START_ADDR,SCREEN_WIDTH, 50,228,700,128);
  
  ra8876lite.putString(0,356,"BTE pattern fill with chroma key, fill with chroma key 16x16 pattern to page1");
  ra8876lite.btePatternFillWithChromaKey(1,PATTERN3_RAM_START_ADDR,16,0,0,PAGE1_START_ADDR,SCREEN_WIDTH, 50,406,700,128,0xe8e4);
  
  delay(3000);
  //while(1);
}


