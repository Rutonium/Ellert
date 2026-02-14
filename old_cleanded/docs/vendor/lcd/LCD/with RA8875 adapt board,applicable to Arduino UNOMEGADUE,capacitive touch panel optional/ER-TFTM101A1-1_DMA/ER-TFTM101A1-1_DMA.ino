/***************************************************
//Web: http://www.buydisplay.com
EastRising Technology Co.,LTD
Examples for ER-TFTM101A1-1    DMA Function:
Direct Data Transfer from External Serial Flash to Frame Buffer
Display is Hardward SPI 4-Wire SPI Interface and 5V Power Supply
Tested and worked with:
Works with Arduino 1.6.0 IDE  
Test ok:  Arduino Due,Arduino UNO,Arduino MEGA2560
****************************************************/

#include <SPI.h>
#include <Wire.h>
#include "ER_TFTM101A1_1.h"

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
void loop() {
  ER5517.Select_Main_Window_16bpp();
  ER5517.Main_Image_Start_Address(layer1_start_addr);        
  ER5517.Main_Image_Width(LCD_XSIZE_TFT);
  ER5517.Main_Window_Start_XY(0,0);
  ER5517.Canvas_Image_Start_address(0);
  ER5517.Canvas_image_width(LCD_XSIZE_TFT);
  ER5517.Active_Window_XY(0,0);
  ER5517.Active_Window_WH(LCD_XSIZE_TFT,LCD_YSIZE_TFT); 
  
  ER5517.DrawSquare_Fill(0,0,LCD_XSIZE_TFT,LCD_YSIZE_TFT,Black);
 
  ER5517.Foreground_color_65k(White);
  ER5517.Background_color_65k(Red);
  ER5517.CGROM_Select_Internal_CGROM();  
  ER5517.Font_Select_12x24_24x24();
  ER5517.Goto_Text_XY(0,10); 
  ER5517.Show_String( "www.buydisplay.com");
  ER5517.Goto_Text_XY(0,40); 
  ER5517.Show_String( "DMA Function:");
    delay(500);
 
  ER5517.DMA_24bit_Block(1,1,0,0,LCD_XSIZE_TFT,LCD_YSIZE_TFT,LCD_XSIZE_TFT,0);
// Select SPI : SCS��0       SCS��1
// SPI Clock = System Clock /{(Clk+1)*2}
// Transfer to SDRAM address:X1
 // Transfer to SDRAM address:Y1
// DMA data width
// DMA data height
// Picture's width
// Flash address
  delay(2000);
  ER5517.DMA_24bit_Block(1,1,0,0,LCD_XSIZE_TFT,LCD_YSIZE_TFT,LCD_XSIZE_TFT,1228800);
  delay(2000);  
  ER5517.DMA_24bit_Block(1,1,0,0,LCD_XSIZE_TFT,LCD_YSIZE_TFT,LCD_XSIZE_TFT,1228800*2);
  delay(2000);  
  ER5517.DMA_24bit_Block(1,1,0,0,LCD_XSIZE_TFT,LCD_YSIZE_TFT,LCD_XSIZE_TFT,1228800*3);
  delay(2000); 
}
