/***************************************************
//Web: http://www.buydisplay.com
EastRising Technology Co.,LTD
Examples for ER-TFTM101-1 with Capacitive Touch Panel 
Display is Hardward SPI 4-Wire SPI Interface and 5V Power Supply
Capacitive Touch Panel is I2C Interface
Tested and worked with:
Works with Arduino 1.6.0 IDE  
Works with Arduino Due Board
Note: If you use our company's adapter board, you must remove the R6 resistor on the adapter board, otherwise the capacitive touch screen will not work.
****************************************************/
 
#include <stdint.h>
#include <Wire.h>
#include <SPI.h>
#include "Arduino.h"
#include "Print.h"
#include "Ra8876_Lite.h"


uint8_t addr  = 0x5d;  //CTP IIC ADDRESS


#define GT9271_INT   7  

const int RA8876_XNSCS = 10;
const int RA8876_XNRESET = 9;


 char stringEnd ='\0';
 char string1[] = {0xa6,0xb0,0xa4,0xe9,0xaa,0x46,0xa4,0xe8,stringEnd};  //BIG5
 char string2[] = {0xbb,0xb6,0xd3,0xad,0xca,0xb9,0xd3,0xc3,stringEnd};  //BG2312


unsigned char  GTP_CFG_DATA[] =
{

0x63,0x00,0x04,0x58,0x02,0x0A,0x3D,0x00,
0x01,0x08,0x28,0x0F,0x50,0x32,0x03,0x05,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x17,
0x19,0x1D,0x14,0x90,0x2F,0x89,0x23,0x25,
0xD3,0x07,0x00,0x00,0x00,0x02,0x03,0x1D,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x19,0x32,0x94,0xD5,0x02,
0x07,0x00,0x00,0x04,0xA2,0x1A,0x00,0x90,
0x1E,0x00,0x80,0x23,0x00,0x73,0x28,0x00,
0x68,0x2E,0x00,0x68,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x16,0x15,0x14,0x11,0x10,0x0F,0x0E,0x0D,
0x0C,0x09,0x08,0x07,0x06,0x05,0x04,0x01,
0x00,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x29,0x28,
0x27,0x26,0x25,0x24,0x23,0x22,0x21,0x20,
0x1F,0x1E,0x1C,0x1B,0x19,0x14,0x13,0x12,
0x11,0x10,0x0F,0x0E,0x0D,0x0C,0x0A,0x08,
0x07,0x06,0x04,0x02,0x00,0xFF,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x71,0x01


};

struct TouchLocation
{
  uint16_t x;
  uint16_t y;
};
TouchLocation touchLocations[10];


void inttostr(uint16_t value,uint8_t *str);
uint8_t gt9271_Send_Cfg(uint8_t * buf,uint16_t cfg_len);
void writeGT9271TouchRegister( uint16_t regAddr,uint8_t *val, uint16_t cnt);
uint8_t readGT9271TouchAddr( uint16_t regAddr, uint8_t * pBuf, uint8_t len );
uint8_t readGT9271TouchLocation( TouchLocation * pLoc, uint8_t num );
uint32_t dist(const TouchLocation & loc);
uint32_t dist(const TouchLocation & loc1, const TouchLocation & loc2);
bool sameLoc( const TouchLocation & loc, const TouchLocation & loc2 );

uint8_t buf[30];


uint8_t gt9271_Send_Cfg(uint8_t * buf,uint16_t cfg_len)
{
	//uint8_t ret=0;
	uint8_t retry=0;
	for(retry=0;retry<5;retry++)
	{
		writeGT9271TouchRegister(0x8047,buf,cfg_len);
		//if(ret==0)break;
		delay(10);	 
	}
	//return ret;
}


void writeGT9271TouchRegister( uint16_t regAddr,uint8_t *val, uint16_t cnt)
{	uint16_t i=0;
  Wire.beginTransmission(addr);
   Wire.write( regAddr>>8 );  // register 0
  Wire.write( regAddr);  // register 0 
	for(i=0;i<cnt;i++,val++)//data
	{		
          Wire.write( *val );  // value
	}
  uint8_t retVal = Wire.endTransmission(); 
}



uint8_t readGT9271TouchAddr( uint16_t regAddr, uint8_t * pBuf, uint8_t len )
{
  Wire.beginTransmission(addr);
  Wire.write( regAddr>>8 );  // register 0
  Wire.write( regAddr);  // register 0  
  uint8_t retVal = Wire.endTransmission();
  
  uint8_t returned = Wire.requestFrom(addr, len);    // request 1 bytes from slave device #2
  
  uint8_t i;
  for (i = 0; (i < len) && Wire.available(); i++)
  
  {
    pBuf[i] = Wire.read();
  }
  
  return i;
}

uint8_t readGT9271TouchLocation( TouchLocation * pLoc, uint8_t num )
{
  uint8_t retVal;
  uint8_t i;
  uint8_t k;
  uint8_t  ss[1];
  do
  {  
    
    if (!pLoc) break; // must have a buffer
    if (!num)  break; // must be able to take at least one
     ss[0]=0;
      readGT9271TouchAddr( 0x814e, ss, 1);
      uint8_t status=ss[0];

    if ((status & 0x0f) == 0) break; // no points detected
    uint8_t hitPoints = status & 0x0f;
    
    Serial.print("number of hit points = ");
    Serial.println( hitPoints );
    
   uint8_t tbuf[32]; uint8_t tbuf1[32];uint8_t tbuf2[16];  
    readGT9271TouchAddr( 0x8150, tbuf, 32);
    readGT9271TouchAddr( 0x8150+32, tbuf1, 32);
    readGT9271TouchAddr( 0x8150+64, tbuf2,16);
    
      if(hitPoints<=4)
            {   
              for (k=0,i = 0; (i <  4*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf[i+1] << 8 | tbuf[i+0];
                pLoc[k].y = tbuf[i+3] << 8 | tbuf[i+2];
              }   
            }
        if(hitPoints>4)   
            {  
               for (k=0,i = 0; (i <  4*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf[i+1] << 8 | tbuf[i+0];
                pLoc[k].y = tbuf[i+3] << 8 | tbuf[i+2];
              }               
              
              for (k=4,i = 0; (i <  4*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf1[i+1] << 8 | tbuf1[i+0];
                pLoc[k].y = tbuf1[i+3] << 8 | tbuf1[i+2];
              }   
            } 
            
          if(hitPoints>8)   
            {  
                for (k=0,i = 0; (i <  4*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf[i+1] << 8 | tbuf[i+0];
                pLoc[k].y = tbuf[i+3] << 8 | tbuf[i+2];
              }               
              
              for (k=4,i = 0; (i <  4*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf1[i+1] << 8 | tbuf1[i+0];
                pLoc[k].y = tbuf1[i+3] << 8 | tbuf1[i+2];
              }                          
             
              for (k=8,i = 0; (i <  2*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf2[i+1] << 8 | tbuf2[i+0];
                pLoc[k].y = tbuf2[i+3] << 8 | tbuf2[i+2];
              }   
            }       

            
    
    retVal = hitPoints;
    
  } while (0);
  
    ss[0]=0;
    writeGT9271TouchRegister( 0x814e,ss,1); 
  
  return retVal;
}
/*
void inttostr(uint16_t value,uint8_t *str)
{
	str[0]=value/1000+0x30;
	str[1]=value%1000/100+0x30;
	str[2]=value%1000%100/10+0x30;
	str[3]=value%1000%100%10+0x30;

}
*/

Ra8876_Lite ra8876lite(RA8876_XNSCS, RA8876_XNRESET);  
   
  void setup() {
   Serial.begin(9600);
   Serial.println("RA8876 Lite");
   Wire.begin();        // join i2c bus (address optional for master) 
  
   pinMode(RA8876_XNRESET, OUTPUT); 
    pinMode     (GT9271_INT, OUTPUT);
    digitalWrite(RA8876_XNRESET, LOW);
    digitalWrite(GT9271_INT, LOW);
    delay(20);
     digitalWrite(RA8876_XNRESET, HIGH);
     delay(50);  
    pinMode     (GT9271_INT, INPUT);
     delay(100);  
   
    uint8_t re=gt9271_Send_Cfg((uint8_t*)GTP_CFG_DATA,sizeof(GTP_CFG_DATA));
    
    
    
    pinMode(RA8876_XNRESET, OUTPUT); 
    pinMode     (GT9271_INT, OUTPUT);
    digitalWrite(RA8876_XNRESET, LOW);
    digitalWrite(GT9271_INT, LOW);
    delay(20);
     digitalWrite(RA8876_XNRESET, HIGH);
     delay(50);  
    pinMode     (GT9271_INT, INPUT);
     delay(100);  
   
     re=gt9271_Send_Cfg((uint8_t*)GTP_CFG_DATA,sizeof(GTP_CFG_DATA));  
    
   
    pinMode(8, OUTPUT);  //backlight 
   digitalWrite(8, HIGH);//on

   if (!ra8876lite.begin()) 
   {
   Serial.println("RA8876 or RA8877 Fail");
   while (1);
   }
   Serial.println("RA8876 or RA8877 Pass!");
   
   ra8876lite.displayOn(true);

   

/*
  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLACK);
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_16,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_RED);

                  uint8_t bb[32];
	 	readGT9271TouchAddr(0x8047,bb,32);

		  
		uint8_t in=0; 
                uint8_t ss[4];
		 ra8876lite.textMode(true);
		  ra8876lite.setTextCursor(0,100);
	 while(in<32)				
		{inttostr(bb[in],ss);
                ra8876lite.lcdRegWrite(0x04);
		 ra8876lite.lcdDataWrite(ss[0]);	  ra8876lite.checkWriteFifoNotFull();  	
		 ra8876lite.lcdDataWrite(ss[1]);	  ra8876lite.checkWriteFifoNotFull();  
		 ra8876lite.lcdDataWrite(ss[2]);	   ra8876lite.checkWriteFifoNotFull();  
		 ra8876lite.lcdDataWrite(ss[3]);	  ra8876lite.checkWriteFifoNotFull();  
		 ra8876lite.lcdDataWrite(',');	          ra8876lite.checkWriteFifoNotFull();  
		in+=1;
		}

    delay(3000);  */
  
}

void loop() {

  static uint16_t w = SCREEN_WIDTH;
  static uint16_t h = SCREEN_HEIGHT;  


  uint8_t flag=1;
    uint8_t im;
   double float_data;   

    ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
  ra8876lite.canvasImageWidth(SCREEN_WIDTH);
  ra8876lite.activeWindowXY(0,0);
  ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
  ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLACK);
  ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_16,RA8876_SELECT_8859_1);//cch
  ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
  ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_RED); 

  ra8876lite.putString(10,20," www.buydisplay.com  Capacitive touch screen test.Please touch the screen!"); 
 ra8876lite.textColor(COLOR65K_BLACK,COLOR65K_WHITE);  
  ra8876lite.putString(960,580," Clear");   
   ra8876lite.putString(0,580,"  Exit");
 
 while(flag) 
  {       
   /* uint8_t  a[1];a[0]=0;          
     readGT9271TouchAddr(0x814e,a,1); 
      if(a[0]&0x80)*/
      pinMode     (GT9271_INT, INPUT);
       uint8_t st=digitalRead(GT9271_INT);
       if(!st) 
    { 
      Serial.println("Touch: ");
      
      uint8_t count = readGT9271TouchLocation( touchLocations, 10 );
           

      
        for (im = 0; im < count; im++)
        {
            if ((touchLocations[im].x>960) &&(touchLocations[im].y<20)) flag=0;
  
           if ((touchLocations[im].x<64) &&(touchLocations[im].y<20))
              {  ra8876lite.canvasImageStartAddress(PAGE1_START_ADDR);
                ra8876lite.canvasImageWidth(SCREEN_WIDTH);
                ra8876lite.activeWindowXY(0,0);
                ra8876lite.activeWindowWH(SCREEN_WIDTH,SCREEN_HEIGHT); 
                ra8876lite.drawSquareFill(0, 0, 1023, 599, COLOR65K_BLACK);
                ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_16,RA8876_SELECT_8859_1);//cch
                ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);
                ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
                ra8876lite.putString(10,0," www.buydisplay.com  Capacitive touch screen test.Please touch the screen!"); 
               ra8876lite.textColor(COLOR65K_BLACK,COLOR65K_WHITE);  
               ra8876lite.putString(960,580," clear"); 
                ra8876lite.putString(0,580,"  Exit");
              }                         
          
           ra8876lite.setTextParameter1(RA8876_SELECT_INTERNAL_CGROM,RA8876_CHAR_HEIGHT_16,RA8876_SELECT_8859_1);//cch
            ra8876lite.setTextParameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE,RA8876_TEXT_WIDTH_ENLARGEMENT_X1,RA8876_TEXT_HEIGHT_ENLARGEMENT_X1); 
        ra8876lite.textColor(COLOR65K_WHITE,COLOR65K_BLACK);
          snprintf((char*)buf,sizeof(buf),"(%3d,%3d)",touchLocations[im].x,touchLocations[im].y);

        const  char *str=(const char *)buf;
         ra8876lite.textMode(true);
        ra8876lite.setTextCursor(380,380+16*im);
        ra8876lite.ramAccessPrepare();
        while(*str != '\0')
        {
        ra8876lite.checkWriteFifoNotFull();  
        ra8876lite.lcdDataWrite(*str);
        ++str; 
        } 
        ra8876lite.check2dBusy();
        ra8876lite.textMode(false);
        
       
        ra8876lite. graphicMode(true);         
        if(im==0)  ra8876lite.drawCircleFill(SCREEN_WIDTH-touchLocations[im].x,SCREEN_HEIGHT-touchLocations[im].y, 3, COLOR65K_RED);  
        else if(im==1)  ra8876lite.drawCircleFill(SCREEN_WIDTH-touchLocations[im].x,SCREEN_HEIGHT-touchLocations[im].y, 3, COLOR65K_GREEN); 
        else if(im==2)  ra8876lite.drawCircleFill(SCREEN_WIDTH-touchLocations[im].x,SCREEN_HEIGHT-touchLocations[im].y, 3, COLOR65K_BLUE);        
        else if(im==3)  ra8876lite.drawCircleFill(SCREEN_WIDTH-touchLocations[im].x,SCREEN_HEIGHT-touchLocations[im].y, 3, COLOR65K_CYAN); 
        else if(im==4)  ra8876lite.drawCircleFill(SCREEN_WIDTH-touchLocations[im].x,SCREEN_HEIGHT-touchLocations[im].y, 3, COLOR65K_MAGENTA); 
        else if(im==5)  ra8876lite.drawCircleFill(SCREEN_WIDTH-touchLocations[im].x,SCREEN_HEIGHT-touchLocations[im].y, 3, COLOR65K_YELLOW);         
        else if(im==6)  ra8876lite.drawCircleFill(SCREEN_WIDTH-touchLocations[im].x,SCREEN_HEIGHT-touchLocations[im].y, 3, COLOR65K_LIGHTGREEN);        
        else if(im==7)  ra8876lite.drawCircleFill(SCREEN_WIDTH-touchLocations[im].x,SCREEN_HEIGHT-touchLocations[im].y, 3, COLOR65K_LIGHTBLUE);         
        else if(im==8)  ra8876lite.drawCircleFill(SCREEN_WIDTH-touchLocations[im].x,SCREEN_HEIGHT-touchLocations[im].y, 3, COLOR65K_LIGHTRED);         
        else if(im==9)  ra8876lite.drawCircleFill(SCREEN_WIDTH-touchLocations[im].x,SCREEN_HEIGHT-touchLocations[im].y, 3, COLOR65K_LIGHTCYAN); 

        
        ra8876lite. graphicMode(false);       
        }
     } 
    
  }

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

  
}


