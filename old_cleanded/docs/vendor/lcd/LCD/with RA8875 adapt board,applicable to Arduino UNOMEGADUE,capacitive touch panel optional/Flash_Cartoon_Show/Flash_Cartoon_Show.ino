/***************************************************
//Web: http://www.buydisplay.com
EastRising Technology Co.,LTD
Examples for ER-TFTM101A1-1  Flash_Cartoon_Show
Display is Hardward SPI 4-Wire SPI Interface and 5V Power Supply
Tested and worked with:
Works with Arduino 1.6.0 IDE  
Test ok:  Arduino Due,Arduino UNO,Arduino MEGA2560
****************************************************/

#include <SPI.h>
#include <Wire.h>
#include "LCD.h"

void setup() {
  
  pinMode(5,   OUTPUT);
  digitalWrite(5, HIGH);//Disable  SD 
 
  
  ER5517.Parallel_Init();
  ER5517.HW_Reset();
  ER5517.System_Check_Temp();
  delay(100);
  while(ER5517.LCD_StatusRead()&0x02);
  ER5517.initial();
  ER5517.Display_ON();

}


void LCD_BTE_Memory_Copy
(
 unsigned long S0_Addr     
,unsigned short S0_W       
,unsigned short XS0        
,unsigned short YS0      
,unsigned long S1_Addr    
,unsigned short S1_W       
,unsigned short XS1        
,unsigned short YS1        
,unsigned long Des_Addr    
,unsigned short Des_W     
,unsigned short XDes      
,unsigned short YDes    
,unsigned int ROP_Code    
/*ROP_Code :
   0000b		0(Blackness)
   0001b		~S0!E~S1 or ~(S0+S1)
   0010b		~S0!ES1
   0011b		~S0
   0100b		S0!E~S1
   0101b		~S1
   0110b		S0^S1
   0111b		~S0 + ~S1 or ~(S0 + S1)
   1000b		S0!ES1
   1001b		~(S0^S1)
   1010b		S1
   1011b		~S0+S1
   1100b		S0
   1101b		S0+~S1
   1110b		S0+S1
   1111b		1(whiteness)*/
,unsigned short X_W      
,unsigned short Y_H       
)
{
  ER5517.BTE_S0_Color_16bpp();
  ER5517.BTE_S0_Memory_Start_Address(S0_Addr);
  ER5517.BTE_S0_Image_Width(S0_W);
  ER5517.BTE_S0_Window_Start_XY(XS0,YS0);

  ER5517.BTE_S1_Color_16bpp();
  ER5517.BTE_S1_Memory_Start_Address(S1_Addr);
  ER5517.BTE_S1_Image_Width(S1_W); 
  ER5517.BTE_S1_Window_Start_XY(XS1,YS1);

  ER5517.BTE_Destination_Color_16bpp();
  ER5517.BTE_Destination_Memory_Start_Address(Des_Addr);
  ER5517.BTE_Destination_Image_Width(Des_W);
  ER5517.BTE_Destination_Window_Start_XY(XDes,YDes);	
   
  ER5517.BTE_ROP_Code(ROP_Code);	
  ER5517.BTE_Operation_Code(0x02); //BTE Operation: Memory copy (move) with ROP.
  ER5517.BTE_Window_Size(X_W,Y_H); 
  ER5517.BTE_Enable();
  ER5517.Check_BTE_Busy();
}





void loop() {

	 



#define Cartoon_Addr      6144000

#define Picture_1_Addr	  0
#define Picture_2_Addr	  1228800//(LCD_XSIZE_TFT*LCD_YSIZE_TFT*2)
#define Picture_3_Addr	  2457600//(LCD_XSIZE_TFT*LCD_YSIZE_TFT*4)
#define Picture_4_Addr	  3686400//(LCD_XSIZE_TFT*LCD_YSIZE_TFT*6)
#define Picture_5_Addr	  4915200//(LCD_XSIZE_TFT*LCD_YSIZE_TFT*8)

#define layer1_start_addr 0
#define layer2_start_addr 1228800//LCD_XSIZE_TFT*LCD_YSIZE_TFT*2
#define layer3_start_addr 2457600//LCD_XSIZE_TFT*LCD_YSIZE_TFT*2*2
#define layer4_start_addr 3686400//LCD_XSIZE_TFT*LCD_YSIZE_TFT*2*3
#define layer5_start_addr 4915200//LCD_XSIZE_TFT*LCD_YSIZE_TFT*2*4
#define layer6_start_addr 6144000//LCD_XSIZE_TFT*LCD_YSIZE_TFT*2*5



#define FLASH_ADDR_16	  8221560
#define FLASH_ADDR_24	  8488824
#define FLASH_ADDR_32	  9090168

#define SIZE_16_NUM		267264
#define SIZE_24_NUM     601344
#define SIZE_32_NUM     1069056

#define MEMORY_ADDR_32	6144000//1024*600*2*5
#define MEMORY_ADDR_24	(MEMORY_ADDR_32+SIZE_32_NUM)
#define MEMORY_ADDR_16	(MEMORY_ADDR_24+SIZE_24_NUM)

 
   ER5517.Select_Main_Window_16bpp();
  ER5517.Main_Image_Start_Address(layer2_start_addr);        
  ER5517.Main_Image_Width(LCD_XSIZE_TFT);
  ER5517.Main_Window_Start_XY(0,0);
  ER5517.Canvas_Image_Start_address(layer2_start_addr);
  ER5517.Canvas_image_width(LCD_XSIZE_TFT);
  ER5517.Active_Window_XY(0,0);
  ER5517.Active_Window_WH(LCD_XSIZE_TFT,LCD_YSIZE_TFT); 
  ER5517.DrawSquare_Fill(0,0,LCD_XSIZE_TFT,LCD_YSIZE_TFT,Blue);
  ER5517.DMA_24bit_Block(1,1,0,0,LCD_XSIZE_TFT,LCD_YSIZE_TFT,LCD_XSIZE_TFT,Picture_3_Addr);
	
  ER5517.Canvas_image_width(200);
  ER5517.Canvas_Image_Start_address(layer3_start_addr);
  ER5517.DMA_24bit_Block(1,1,0,0, 200,150*24,200,Cartoon_Addr); 
  

	while(1)
	{			
		for(unsigned char i = 0 ; i < 24 ; i++)
		{
			LCD_BTE_Memory_Copy(layer3_start_addr,200,0,150*i,layer3_start_addr,200,0,150*i,layer2_start_addr,LCD_XSIZE_TFT,100,35,0X0C,200,150);
                       
                       
                  		for(unsigned char j = 0 ; j < 24 ; j++)     
			        {LCD_BTE_Memory_Copy(layer3_start_addr,200,0,150*j,layer3_start_addr,200,0,150*j,layer2_start_addr,LCD_XSIZE_TFT,100,320,0X0C,200,150);}
			    					
		}
		

	}					   
  
    
}
