 
 
 
 
 

 
 
 
 
 
 
 

 
 

#ifndef USER_SETUP_LOADED  
                           

 
 
 

 

 

 
 
 
 
 
 
 
 
 
 

 
 
 
 
 
 
 
 

 

 
 

 
 
 
 
 
 

 
 

 
 
 
 

 

 
 

 

 
 
 
 
 

 

 

 
 

 
 
 

#include "RoundSetup.h"      

 
 
 
 

 
 

 
 
 
 
 

 
 
 
 

 

 
 

 

 

 

 

 

 

 

 
 
 
 
 
 
 

 
 

 
 

 


 
 
 
 
 
 
 
 
 

#endif  



 
 
 
 
 
 


 
#define TFT_BGR 0    
#define TFT_RGB 1    

 
#if defined (RPI_DRIVER)
  #if !defined (RPI_DISPLAY_TYPE)
    #define RPI_DISPLAY_TYPE
  #endif
#endif

 
 
#if defined (RPI_ILI9486_DRIVER)
  #if !defined (ILI9486_DRIVER)
    #define ILI9486_DRIVER
  #endif
  #if !defined (RPI_DISPLAY_TYPE)
    #define RPI_DISPLAY_TYPE
  #endif
#endif

 
#if !defined (RPI_DISPLAY_TYPE) && !defined (TFT_PARALLEL_8_BIT) && !defined (TFT_PARALLEL_16_BIT) && !defined (ESP32_PARALLEL)
  #if defined (ILI9481_DRIVER) || defined (ILI9486_DRIVER) || defined (ILI9488_DRIVER)
    #define SPI_18BIT_DRIVER
  #endif
#endif

 
#if   defined (ILI9341_DRIVER) || defined(ILI9341_2_DRIVER) || defined (ILI9342_DRIVER)
     #include <TFT_Drivers/ILI9341_Defines.h>
     #define  TFT_DRIVER 0x9341
#elif defined (ST7735_DRIVER)
     #include <TFT_Drivers/ST7735_Defines.h>
     #define  TFT_DRIVER 0x7735
#elif defined (ILI9163_DRIVER)
     #include <TFT_Drivers/ILI9163_Defines.h>
     #define  TFT_DRIVER 0x9163
#elif defined (S6D02A1_DRIVER)
     #include <TFT_Drivers/S6D02A1_Defines.h>
     #define  TFT_DRIVER 0x6D02
#elif defined (ST7796_DRIVER)
      #include "TFT_Drivers/ST7796_Defines.h"
      #define  TFT_DRIVER 0x7796
#elif defined (ILI9486_DRIVER)
     #include <TFT_Drivers/ILI9486_Defines.h>
     #define  TFT_DRIVER 0x9486
#elif defined (ILI9481_DRIVER)
     #include <TFT_Drivers/ILI9481_Defines.h>
     #define  TFT_DRIVER 0x9481
#elif defined (ILI9488_DRIVER)
     #include <TFT_Drivers/ILI9488_Defines.h>
     #define  TFT_DRIVER 0x9488
#elif defined (HX8357D_DRIVER)
     #include "TFT_Drivers/HX8357D_Defines.h"
     #define  TFT_DRIVER 0x8357
#elif defined (EPD_DRIVER)
     #include "TFT_Drivers/EPD_Defines.h"
     #define  TFT_DRIVER 0xE9D
#elif defined (ST7789_DRIVER)
     #include "TFT_Drivers/ST7789_Defines.h"
     #define  TFT_DRIVER 0x7789
#elif defined (R61581_DRIVER)
     #include "TFT_Drivers/R61581_Defines.h"
     #define  TFT_DRIVER 0x6158
#elif defined (ST7789_2_DRIVER)
     #include "TFT_Drivers/ST7789_2_Defines.h"
     #define  TFT_DRIVER 0x778B
#elif defined (RM68140_DRIVER)
     #include "TFT_Drivers/RM68140_Defines.h"
     #define  TFT_DRIVER 0x6814
#elif defined (SSD1351_DRIVER)
     #include "TFT_Drivers/SSD1351_Defines.h"
     #define  TFT_DRIVER 0x1351
#elif defined (SSD1963_480_DRIVER)
     #include "TFT_Drivers/SSD1963_Defines.h"
     #define  TFT_DRIVER 0x1963
#elif defined (SSD1963_800_DRIVER)
     #include "TFT_Drivers/SSD1963_Defines.h"
     #define  TFT_DRIVER 0x1963
#elif defined (SSD1963_800ALT_DRIVER)
     #include "TFT_Drivers/SSD1963_Defines.h"
     #define  TFT_DRIVER 0x1963
#elif defined (SSD1963_800BD_DRIVER)
     #include "TFT_Drivers/SSD1963_Defines.h"
     #define  TFT_DRIVER 0x1963
#elif defined (GC9A01_DRIVER)
     #include "TFT_Drivers/GC9A01_Defines.h"
     #define  TFT_DRIVER 0x9A01
#elif defined (ILI9225_DRIVER)
     #include "TFT_Drivers/ILI9225_Defines.h"
     #define  TFT_DRIVER 0x9225
#elif defined (RM68120_DRIVER)
     #include "TFT_Drivers/RM68120_Defines.h"
     #define  TFT_DRIVER 0x6812
#elif defined (HX8357B_DRIVER)
     #include "TFT_Drivers/HX8357B_Defines.h"
     #define  TFT_DRIVER 0x835B
#elif defined (HX8357C_DRIVER)
     #include "TFT_Drivers/HX8357C_Defines.h"
     #define  TFT_DRIVER 0x835C

                               
                               
#elif defined (XYZZY_DRIVER)
     #include "TFT_Drivers/XYZZY_Defines.h"
     #define  TFT_DRIVER 0x0000
#else
     #define  TFT_DRIVER 0x0000
#endif

 
 
#define PIN_D0  16   
#define PIN_D1   5   
#define PIN_D2   4   
#define PIN_D3   0   
#define PIN_D4   2   
#define PIN_D5  14   
#define PIN_D6  12   
#define PIN_D7  13   
#define PIN_D8  15   
#define PIN_D9   3   
#define PIN_D10  1   

#define PIN_MOSI 8   
#define PIN_MISO 7   
#define PIN_SCLK 6   
#define PIN_HWCS 0   

#define PIN_D11  9   
#define PIN_D12 10   
