#include <SPI.h>
#include "Arduino.h"
#include "Print.h"
#include "Ra8876_Lite.h"

const int RA8876_XNSCS = 10;
const int RA8876_XNRESET = 9;

 char stringEnd ='\0';
 char string1[] = {0xa6,0xb0,0xa4,0xe9,0xaa,0x46,0xa4,0xe8,stringEnd};  //BIG5
 char string2[] = {0xbb,0xb6,0xd3,0xad,0xca,0xb9,0xd3,0xc3,stringEnd};  //BG2312

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
   unsigned int i;
   double float_data;
   //char char_buffer[20];
  //clear page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE);

  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_16,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
  ra8876lite.putString(10,0,"Show internal font 8x16   www.buydisplay.com");
  
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.textColor(COLOR65K_BLUE,COLOR65K_MAGENTA);
  ra8876lite.putString(10,26,"Show internal font 12x24  www.buydisplay.com");
  
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_32,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.textColor(COLOR65K_RED,COLOR65K_YELLOW);
  ra8876lite.putString(10,60,"Show internal font 16x32 www.buydisplay.com");
  
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_16,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X2,RA8876_TEXT_HEIGHT_ENLARGEMENT_X2);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_RED);
  ra8876lite.putString(10,102,"font enlarge x2");
  
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_16,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X3,RA8876_TEXT_HEIGHT_ENLARGEMENT_X3);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_RED);
  ra8876lite.putString(10,144,"font enlarge x3");
  
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_16,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X4,RA8876_TEXT_HEIGHT_ENLARGEMENT_X4);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_LIGHTCYAN);
  ra8876lite.putString(10,202,"font enlarge x4");
  
  //used genitop rom 
  
  ra8876lite.setTextParameter1(RA8876_SELECT_EXTERNAL_CGROM,RA8876_CHAR_HEIGHT_16,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.genitopCharacterRomParameter(RA8876_SERIAL_FLASH_SELECT0,RA8876_SPI_DIV4,RA8876_GT30L24T3Y,RA8876_BIG5,RA8876_GT_FIXED_WIDTH);
  ra8876lite.textColor(COLOR65K_BLACK,COLOR65K_RED);
  ra8876lite.putString(10,276,"show external GT font 16x16");
  
  ra8876lite.setTextParameter1(RA8876_SELECT_EXTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_ENABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.genitopCharacterRomParameter(RA8876_SERIAL_FLASH_SELECT0,RA8876_SPI_DIV4,RA8876_GT30L24T3Y,RA8876_BIG5,RA8876_GT_VARIABLE_WIDTH_ARIAL);
  ra8876lite.putString(10,302,"show external GT font 24x24 with Arial font");
  
  ra8876lite.putString(10,336,string1);
  ra8876lite.setTextParameter1(RA8876_SELECT_EXTERNAL_CGROM,RA8876_CHAR_HEIGHT_24,RA8876_SELECT_8859_1);//cch
  ra8876lite.genitopCharacterRomParameter(RA8876_SERIAL_FLASH_SELECT0,RA8876_SPI_DIV4,RA8876_GT30L24T3Y,RA8876_GB2312,RA8876_GT_FIXED_WIDTH);
  ra8876lite.putString(10,370,string2);
  delay(3000);
//while(1);  

 //demo display decimals
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE);

  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_32,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
  
  ra8876lite.putDec(10,10,1,2,"n");
  ra8876lite.putDec(10,44,2147483647,11,"n");
  ra8876lite.putDec(10,78,-12345,10,"n");
  ra8876lite.putDec(10,112,-2147483648,11,"n");
  
  ra8876lite.putDec(10,146,1,2,"-");
  ra8876lite.putDec(10,180,2147483647,11,"-");
  ra8876lite.putDec(10,214,-12345,10,"-");
  ra8876lite.putDec(10,248,-2147483648,11,"-");
  
  ra8876lite.putDec(10,282,1,2,"+");
  ra8876lite.putDec(10,316,2147483647,11,"+");
  ra8876lite.putDec(10,350,-12345,10,"+");
  ra8876lite.putDec(10,384,-2147483648,11,"+");
  
  ra8876lite.putDec(10,418,1,2,"0");
  ra8876lite.putDec(10,452,2147483647,11,"0");
  ra8876lite.putDec(10,486,-12345,10,"0");
  ra8876lite.putDec(10,520,-2147483648,11,"0");
  
  delay(3000);
  
  //demo display float
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE);

  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_32,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
 
  ra8876lite.putFloat(10,10,1.1,7,1,"n");
  ra8876lite.putFloat(10,44,483647.12,11,2,"n");
  ra8876lite.putFloat(10,78,-12345.123,11,3,"n");
  ra8876lite.putFloat(10,112,-123456.1234,11,4,"n");
  
  ra8876lite.putFloat(10,146,1.1234,7,1,"-");
  ra8876lite.putFloat(10,180,483647.12,11,2,"-");
  ra8876lite.putFloat(10,214,-12345.123,11,3,"-");
  ra8876lite.putFloat(10,248,-123456.1234,11,4,"-");
  
  ra8876lite.putFloat(10,282,1.1,7,1,"+");
  ra8876lite.putFloat(10,316,483647.12,11,2,"+");
  ra8876lite.putFloat(10,350,-12345.123,11,3,"+");
  ra8876lite.putFloat(10,384,-123456.1234,11,4,"+");
  
  ra8876lite.putFloat(10,418,1.1,7,1,"0");
  ra8876lite.putFloat(10,452,483647.12,11,2,"0");
  ra8876lite.putFloat(10,486,-12345.123,11,3,"0");
  ra8876lite.putFloat(10,520,-123456.1234,11,4,"0");
  delay(3000);
  //while(1);
//demo display Hex
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE);

  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_32,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
  
  ra8876lite.putHex(10,10,1,4,"n");
  ra8876lite.putHex(10,44,255,6,"n");
  ra8876lite.putHex(10,78,0xa7c8,6,"n");
  ra8876lite.putHex(10,112,0xdd11ff55,10,"n");
  
  ra8876lite.putHex(10,146,1,4,"0");
  ra8876lite.putHex(10,180,255,6,"0");
  ra8876lite.putHex(10,214,0xa7c8,6,"0");
  ra8876lite.putHex(10,248,0xdd11ff55,10,"0");
  
  ra8876lite.putHex(10,282,1,4,"#");
  ra8876lite.putHex(10,316,255,6,"#");
  ra8876lite.putHex(10,350,0xa7c8,6,"#");
  ra8876lite.putHex(10,384,0xdd11ff55,10,"#");
  
  ra8876lite.putHex(10,418,1,4,"x");
  ra8876lite.putHex(10,452,255,6,"x");
  ra8876lite.putHex(10,486,0xa7c8,6,"x");
  ra8876lite.putHex(10,520,0xdd11ff55,10,"x");
  delay(3000);
  //while(1);
}


