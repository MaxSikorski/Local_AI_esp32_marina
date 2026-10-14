         
         
         

 
 

#ifndef _TFT_eSPI_GENERICH_
#define _TFT_eSPI_GENERICH_

 
#define PROCESSOR_ID 0x0000

 
 

 
#define SET_BUS_WRITE_MODE  
#define SET_BUS_READ_MODE   

 
#define DMA_BUSY_CHECK  

 
#if !defined (SUPPORT_TRANSACTIONS)
  #define SUPPORT_TRANSACTIONS
#endif

 
#define INIT_TFT_DATA_BUS

 
#ifdef SMOOTH_FONT
   
   
   
#endif

 
 
 
#ifndef TFT_DC
  #define DC_C  
  #define DC_D  
#else
  #define DC_C digitalWrite(TFT_DC, LOW)
  #define DC_D digitalWrite(TFT_DC, HIGH)
#endif

 
 
 
#ifndef TFT_CS
  #define CS_L  
  #define CS_H  
#else
  #define CS_L digitalWrite(TFT_CS, LOW)
  #define CS_H digitalWrite(TFT_CS, HIGH)
#endif

 
 
 
#ifndef TFT_RD
  #define TFT_RD -1
#endif

 
 
 
#ifdef TFT_WR
  #define WR_L digitalWrite(TFT_WR, LOW)
  #define WR_H digitalWrite(TFT_WR, HIGH)
#endif

 
 
 
#if !defined TOUCH_CS || (TOUCH_CS < 0)
  #define T_CS_L  
  #define T_CS_H  
#else
  #define T_CS_L digitalWrite(TOUCH_CS, LOW)
  #define T_CS_H digitalWrite(TOUCH_CS, HIGH)
#endif

 
 
 
#ifndef TFT_MISO
  #define TFT_MISO -1
#endif

 
 
 
#if  defined (SPI_18BIT_DRIVER)  

   
  #define tft_Write_8(C)   spi.transfer(C)

   
  #define tft_Write_16(C)  spi.transfer(((C) & 0xF800)>>8); \
                           spi.transfer(((C) & 0x07E0)>>3); \
                           spi.transfer(((C) & 0x001F)<<3)

   
  #define tft_Write_16S(C) spi.transfer((C) & 0xF8); \
                           spi.transfer(((C) & 0xE000)>>11 | ((C) & 0x07)<<5); \
                           spi.transfer(((C) & 0x1F00)>>5)
   
  #define tft_Write_32(C)  spi.transfer16((C)>>16); spi.transfer16((uint16_t)(C))

   
  #define tft_Write_32C(C,D) spi.transfer16(C); spi.transfer16(D)

   
  #define tft_Write_32D(C) spi.transfer16(C); spi.transfer16(C)

 
 
 
#else
  #if  defined (RPI_DISPLAY_TYPE)  
    #define tft_Write_8(C)   spi.transfer(C); spi.transfer(C)
    #define tft_Write_16(C)  spi.transfer((uint8_t)((C)>>8));spi.transfer((uint8_t)((C)>>0))
    #define tft_Write_16S(C) spi.transfer((uint8_t)((C)>>0));spi.transfer((uint8_t)((C)>>8))

    #define tft_Write_32(C) \
      tft_Write_16((uint16_t) ((C)>>16)); \
      tft_Write_16((uint16_t) ((C)>>0))

    #define tft_Write_32C(C,D) \
      spi.transfer(0); spi.transfer((C)>>8); \
      spi.transfer(0); spi.transfer((C)>>0); \
      spi.transfer(0); spi.transfer((D)>>8); \
      spi.transfer(0); spi.transfer((D)>>0)

    #define tft_Write_32D(C) \
      spi.transfer(0); spi.transfer((C)>>8); \
      spi.transfer(0); spi.transfer((C)>>0); \
      spi.transfer(0); spi.transfer((C)>>8); \
      spi.transfer(0); spi.transfer((C)>>0)

  #else
    #ifdef __AVR__  
      #define tft_Write_8(C)   {SPDR=(C); while (!(SPSR&_BV(SPIF)));}
      #define tft_Write_16(C)  tft_Write_8((uint8_t)((C)>>8));tft_Write_8((uint8_t)((C)>>0))
      #define tft_Write_16S(C) tft_Write_8((uint8_t)((C)>>0));tft_Write_8((uint8_t)((C)>>8))
    #else
      #define tft_Write_8(C)   spi.transfer(C)
      #define tft_Write_16(C)  spi.transfer16(C)
      #define tft_Write_16S(C) spi.transfer16(((C)>>8) | ((C)<<8))
    #endif  

    #define tft_Write_32(C) \
    tft_Write_16((uint16_t) ((C)>>16)); \
    tft_Write_16((uint16_t) ((C)>>0))

    #define tft_Write_32C(C,D) \
    tft_Write_16((uint16_t) (C)); \
    tft_Write_16((uint16_t) (D))

    #define tft_Write_32D(C) \
    tft_Write_16((uint16_t) (C)); \
    tft_Write_16((uint16_t) (C))
  #endif  
#endif

#ifndef tft_Write_16N
  #define tft_Write_16N tft_Write_16
#endif

 
 
 
#if defined (TFT_SDA_READ)
   
  #define TFT_eSPI_ENABLE_8_BIT_READ  
  #define SCLK_L digitalWrite(TFT_SCLK, LOW)
  #define SCLK_H digitalWrite(TFT_SCLK, LOW)
#else
   
  #define tft_Read_8() spi.transfer(0)
#endif


#endif  
