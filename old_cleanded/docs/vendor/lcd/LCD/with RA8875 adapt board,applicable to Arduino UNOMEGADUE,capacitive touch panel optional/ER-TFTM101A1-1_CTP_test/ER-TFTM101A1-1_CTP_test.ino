/***************************************************
//Web: http://www.buydisplay.com
EastRising Technology Co.,LTD
Examples for ER-TFTM101A1-1 Capacitive touch screen test
Display is Hardward SPI 4-Wire SPI Interface and 5V Power Supply,CTP is I2C interface.
Tested and worked with:
Works with Arduino 1.6.0 IDE  
Test ok:  Arduino Due,Arduino UNO,Arduino MEGA2560
****************************************************/
#include <SPI.h>
#include <Wire.h>
#include "ER_TFTM101A1_1.h"
#include <stdint.h>
#include "Arduino.h"
#include "Print.h"

uint8_t addr  = 0x5d;  //CTP IIC ADDRESS

#define GT9271_RST 6
#define GT9271_INT   7  

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

void inttostr(uint16_t value,uint8_t *str)
{
	str[0]=value/1000+0x30;
	str[1]=value%1000/100+0x30;
	str[2]=value%1000%100/10+0x30;
	str[3]=value%1000%100%10+0x30;

}



void setup() {
  Serial.begin(9600);
  Wire.begin();        // join i2c bus (address optional for master)   

  pinMode(5,   OUTPUT);
  digitalWrite(5, HIGH);//Disable  SD 

  
 
    pinMode(GT9271_RST, OUTPUT); 
    pinMode     (GT9271_INT, OUTPUT);
    digitalWrite(GT9271_RST, LOW);
    digitalWrite(GT9271_INT, LOW);
    delay(20);
     digitalWrite(GT9271_RST, HIGH);
     delay(50);  
    pinMode     (GT9271_INT, INPUT);
     delay(100);    
    uint8_t re=gt9271_Send_Cfg((uint8_t*)GTP_CFG_DATA,sizeof(GTP_CFG_DATA));
    
     pinMode(GT9271_RST, OUTPUT); 
    pinMode     (GT9271_INT, OUTPUT);
    digitalWrite(GT9271_RST, LOW);
    digitalWrite(GT9271_INT, LOW);
    delay(20);
     digitalWrite(GT9271_RST, HIGH);
     delay(50);  
    pinMode     (GT9271_INT, INPUT);
     delay(100);   
     re=gt9271_Send_Cfg((uint8_t*)GTP_CFG_DATA,sizeof(GTP_CFG_DATA));   
  
  
  ER5517.Parallel_Init();
  ER5517.HW_Reset();
  ER5517.System_Check_Temp();
  delay(100);
  while(ER5517.LCD_StatusRead()&0x02);
  ER5517.initial();
  ER5517.Display_ON();

}
void loop() {
   static uint16_t w = LCD_XSIZE_TFT;
  static uint16_t h = LCD_YSIZE_TFT; 
   unsigned int i;
   double float_data;  
  
  ER5517.Select_Main_Window_16bpp();
  ER5517.Main_Image_Start_Address(layer1_start_addr);        
  ER5517.Main_Image_Width(LCD_XSIZE_TFT);
  ER5517.Main_Window_Start_XY(0,0);
  ER5517.Canvas_Image_Start_address(0);
  ER5517.Canvas_image_width(LCD_XSIZE_TFT);
  ER5517.Active_Window_XY(0,0);
  ER5517.Active_Window_WH(LCD_XSIZE_TFT,LCD_YSIZE_TFT); 
  
  ER5517.Foreground_color_65k(Black);
  ER5517.Line_Start_XY(0,0);
  ER5517.Line_End_XY(LCD_XSIZE_TFT-1,LCD_YSIZE_TFT-1);
  ER5517.Start_Square_Fill(); 
  
   ER5517.Foreground_color_65k(White);
  ER5517.Background_color_65k(Black);
  ER5517.CGROM_Select_Internal_CGROM();  
  ER5517.Font_Select_8x16_16x16();
  ER5517.Goto_Text_XY(0,0); 
  ER5517.Show_String( "www.buydisplay.com");   
  ER5517.Goto_Text_XY(0,16); 
  ER5517.Show_String( "Capacitive touch screen tese"); 
  ER5517.Goto_Text_XY(5,LCD_YSIZE_TFT-25); 
  ER5517.Show_String( "Clear");       
  
  while(1) 
  {    pinMode     (GT9271_INT, INPUT);
       uint8_t st=digitalRead(GT9271_INT);       
      if(!st)    //Hardware touch interrupt
    {  
      Serial.println("Touch: ");
      
      uint8_t count = readGT9271TouchLocation( touchLocations, 10 );
                
        for (i = 0; i < count; i++)
        {
            
            if (((LCD_XSIZE_TFT-touchLocations[i].x)<50) &&((LCD_YSIZE_TFT-touchLocations[i].y)>LCD_YSIZE_TFT-30))
              {    ER5517.Foreground_color_65k(Black);
                  ER5517.Line_Start_XY(0,0);
                  ER5517.Line_End_XY(LCD_XSIZE_TFT-1,LCD_YSIZE_TFT-1);
                  ER5517.Start_Square_Fill(); 
                  
                   ER5517.Foreground_color_65k(White);
                  ER5517.Background_color_65k(Black);
                  ER5517.CGROM_Select_Internal_CGROM();  
                  ER5517.Font_Select_8x16_16x16();
                  ER5517.Goto_Text_XY(0,0); 
                  ER5517.Show_String( "www.buydisplay.com");   
                  ER5517.Goto_Text_XY(0,16); 
                  ER5517.Show_String( "Capacitive touch screen test"); 
                  ER5517.Goto_Text_XY(5,LCD_YSIZE_TFT-25); 
                  ER5517.Show_String( "Clear");                  
              }  
              
          else{                                       
              snprintf((char*)buf,sizeof(buf),"(%3d,%3d)",LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y); 
            const  char *str=(const char *)buf;
             ER5517.Foreground_color_65k(Red);  
            ER5517.Text_Mode();
            ER5517.Goto_Text_XY(50,80+16*i);
            ER5517.LCD_CmdWrite(0x04);
            while(*str != '\0')
            {
            ER5517.LCD_DataWrite(*str);
            ER5517.Check_Mem_WR_FIFO_not_Full();      
            ++str; 
            } 
            ER5517.Check_2D_Busy();
            ER5517.Graphic_Mode(); //back to graphic mode;
                   
           
         if(i==0)  ER5517.DrawCircle_Fill(LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y, 3, Red);  
        else if(i==1)  ER5517.DrawCircle_Fill(LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y, 3, Green); 
        else if(i==2)  ER5517.DrawCircle_Fill(LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y, 3, Blue);        
        else if(i==3)  ER5517.DrawCircle_Fill(LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y, 3, Cyan); 
        else if(i==4)  ER5517.DrawCircle_Fill(LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y, 3, Yellow); 
        else if(i==5)  ER5517.DrawCircle_Fill(LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y, 3, Purple);         
        else if(i==6)  ER5517.DrawCircle_Fill(LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y, 3, White);        
        else if(i==7)  ER5517.DrawCircle_Fill(LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y, 3, LIGHTRED);         
        else if(i==8)  ER5517.DrawCircle_Fill(LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y, 3, LIGHTGREEN);         
        else if(i==9)  ER5517.DrawCircle_Fill(LCD_XSIZE_TFT-touchLocations[i].x,LCD_YSIZE_TFT-touchLocations[i].y, 3, LIGHTBLUE);   
          }
        }
     } 
    
  }
 
}



