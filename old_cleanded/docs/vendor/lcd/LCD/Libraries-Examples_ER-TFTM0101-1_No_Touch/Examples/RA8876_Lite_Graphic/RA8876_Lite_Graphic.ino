#include <SPI.h>
#include "Arduino.h"
#include "Print.h"
#include "Ra8876_Lite.h"
#include "pic16bpp_byte.h"
#include "pic16bpp_word.h"

#define DEMO_ASCII_8X12
#define DEMO_ASCII_16X24
#define DEMO_ASCII_32X48

#ifdef DEMO_ASCII_8X12
#include "ascii_table_8x12.h"
#endif

#ifdef DEMO_ASCII_16X24
#include "ascii_table_16x24.h"
#endif

#ifdef DEMO_ASCII_32X48
#include "ascii_table_32x48.h"
#endif

void lcdPutChar8x12(unsigned short x,unsigned short y,unsigned short fgcolor,unsigned short bgcolor,boolean bg_transparent,unsigned char code);
void lcdPutString8x12(unsigned short x,unsigned short y, unsigned short fgcolor, unsigned short bgcolor,boolean bg_transparent,char *ptr);
void lcdPutChar16x24(unsigned short x,unsigned short y,unsigned short fgcolor,unsigned short bgcolor,boolean bg_transparent,unsigned char code);
void lcdPutString16x24(unsigned short x,unsigned short y, unsigned short fgcolor, unsigned short bgcolor,boolean bg_transparent,char *ptr);
void lcdPutChar32x48(unsigned short x,unsigned short y,unsigned short fgcolor,unsigned short bgcolor,boolean bg_transparent, unsigned char code);
void lcdPutString32x48(unsigned short x,unsigned short y, unsigned short fgcolor, unsigned short bgcolor,boolean bg_transparent, char *ptr);


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
   unsigned long i;
  //clear page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE);
  
  //write a pixel dot to page1
  ra8876lite.setPixelCursor(20,20);
  ra8876lite.ramAccessPrepare();
  ra8876lite.lcdDataWrite(0x00);//RGB565 LSB data
  ra8876lite.lcdDataWrite(0xf8);//RGB565 MSB data
  
  //write a pixel dot to page1
  ra8876lite.setPixelCursor(30,30);
  ra8876lite.ramAccessPrepare();
  ra8876lite.lcdDataWrite16bbp(COLOR65K_WHITE);//RGB565 16bpp data
  
  //write a pixel dot to page1
  ra8876lite.putPixel_16bpp(40,40,COLOR65K_MAGENTA);
  delay(3000);
 //while(1);
  //clear page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE);
  

  ra8876lite.putPicture_16bpp(50,50,128,128);
  for(i=0;i<16384;i++)
  {
   ra8876lite.lcdDataWrite16bbp(COLOR65K_YELLOW);//RGB565 16bpp data
  }

  ra8876lite.putPicture_16bpp(50+128,50+128,128,128,pic16bpp_byte);

  ra8876lite.putPicture_16bpp(50+128+128,50+128+128,128,128,pic16bpp_word);
  
  delay(3000);
  //while(1);
  //clear page1
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLUE);

  #ifdef DEMO_ASCII_8X12
  lcdPutString8x12(0,0,0xFFFF,0x0000,true," !\"#$%&'()*+,-./012345678");
  lcdPutString8x12(0,12,0xFFFF,0x0000,true,"9:;<=>?@ABCDEFGHIJKLMNOPQ");
  lcdPutString8x12(0,24,0xFFFF,0x0000,true,"RSTUVWXYZ[\\]^_`abcdefghij");
  lcdPutString8x12(0,36,0xFFFF,0x0000,true,"klmnopqrstuvwxyz{|}~");
  #endif
  
  #ifdef DEMO_ASCII_16X24
  lcdPutString16x24(0,48,0xFFFF,0x0000,true," !\"#$%&'()*+,-./012345678");
  lcdPutString16x24(0,72,0xFFFF,0x0000,true,"9:;<=>?@ABCDEFGHIJKLMNOPQ");
  lcdPutString16x24(0,96,0xFFFF,0x0000,true,"RSTUVWXYZ[\\]^_`abcdefghij");
  lcdPutString16x24(0,120,0xFFFF,0x0000,true,"klmnopqrstuvwxyz{|}~");
  #endif
  
  #ifdef DEMO_ASCII_32X48
  lcdPutString32x48(0,144,0xFFFF,0x0000,false," !\"#$%&'()*+,-./012345678");
  lcdPutString32x48(0,192,0xFFFF,0x0000,false,"9:;<=>?@ABCDEFGHIJKLMNOPQ");
  lcdPutString32x48(0,240,0xFFFF,0x0000,false,"RSTUVWXYZ[\\]^_`abcdefghij");
  lcdPutString32x48(0,288,0xFFFF,0x0000,false,"klmnopqrstuvwxyz{|}~");
  #endif

  delay(3000);
}


void lcdPutChar8x12(unsigned short x,unsigned short y,unsigned short fgcolor,unsigned short bgcolor,boolean bg_transparent,unsigned char code)
{ unsigned short i=0;
  unsigned short j=0;
  unsigned char tmp_char=0;

  for (i=0;i<12;i++)
  {
    tmp_char = ascii_table_8x12[((code-0x20)*12)+i];//minus 32 offset, because this table from ascii table "space" 
   for (j=0;j<8;j++)
   {
    if ( (tmp_char >>7-j) & 0x01 == 0x01)
        ra8876lite.putPixel_16bpp(x+j,y+i,fgcolor); //
    else
    {   
        if(!bg_transparent)
        ra8876lite.putPixel_16bpp(x+j,y+i,bgcolor); //
    } 
   }
  }
}

void lcdPutString8x12(unsigned short x,unsigned short y, unsigned short fgcolor, unsigned short bgcolor,boolean bg_transparent,char *ptr)
{unsigned short i = 0;
  //screen width = 800,  800/8 = 100 
  //if string more then 100 fonts, no show
  while ((*ptr != 0) & (i < 100))
  {
    lcdPutChar8x12(x, y, fgcolor, bgcolor,bg_transparent, *ptr);
    x += 8;
    ptr++;
    i++;  
  }
}

void lcdPutChar16x24(unsigned short x,unsigned short y,unsigned short fgcolor,unsigned short bgcolor,boolean bg_transparent,unsigned char code)
{ unsigned short i=0;
  unsigned short j=0;
  unsigned long array_addr =0;
  unsigned int tmp_char=0;

  for (i=0;i<24;i++)
  {
    //minus 32 offset, because this table from ascii table "space"  
    array_addr = ((code-0x20)*2*24)+(i*2); 
    tmp_char = ascii_table_16x24[array_addr]<<8|ascii_table_16x24[array_addr+1];
   for (j=0;j<16;j++)
   {
    if ( (tmp_char >>15-j) & 0x01 == 0x01)
        ra8876lite.putPixel_16bpp(x+j,y+i,fgcolor); //
    else
       {
        if(!bg_transparent)
        ra8876lite.putPixel_16bpp(x+j,y+i,bgcolor); // 
       }
   }
  }
}

void lcdPutString16x24(unsigned short x,unsigned short y, unsigned short fgcolor, unsigned short bgcolor,boolean bg_transparent,char *ptr)
{unsigned short i = 0;
  //screen width = 800,  800/16 = 50 
  //if string more then 50 fonts, no show
  while ((*ptr != 0) & (i < 50))
  {
    lcdPutChar16x24(x, y, fgcolor, bgcolor,bg_transparent, *ptr);
    x += 16;
    ptr++;
    i++;  
  }
}

void lcdPutChar32x48(unsigned short x,unsigned short y,unsigned short fgcolor,unsigned short bgcolor,boolean bg_transparent,unsigned char code)
{ unsigned short i=0;
  unsigned short j=0;
  unsigned long array_addr =0;
  unsigned long tmp_char=0;

  for (i=0;i<48;i++)
  {
    //minus 32 offset, because this table from ascii table "space"  
    array_addr = ((code-0x20)*4*48)+(i*4); 
    tmp_char = ascii_table_32x48[array_addr]<<24|ascii_table_32x48[array_addr+1]<<16|ascii_table_32x48[array_addr+2]<<8|ascii_table_32x48[array_addr+3];
    
     for (j=0;j<32;j++)
     {
     if ( (tmp_char >> (31-j)) & 0x01 == 0x01)
        ra8876lite.putPixel_16bpp(x+j,y+i,fgcolor); //
     else
         {
         if(!bg_transparent)
         ra8876lite.putPixel_16bpp(x+j,y+i,bgcolor); // 
         }
      } 
  }
}

void lcdPutString32x48(unsigned short x,unsigned short y, unsigned short fgcolor, unsigned short bgcolor,boolean bg_transparent,char *ptr)
{unsigned short i = 0;
  //screen width = 800,  800/32 = 25 
  //if string more then 25 fonts, no show
  while ((*ptr != 0) & (i < 25))
  {
    lcdPutChar32x48(x, y, fgcolor, bgcolor,bg_transparent, *ptr);
    x += 32;
    ptr++;
    i++;  
  }
}

