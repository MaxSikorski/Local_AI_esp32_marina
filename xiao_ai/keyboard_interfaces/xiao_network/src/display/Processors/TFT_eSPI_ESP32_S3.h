         
         
         

 

#ifndef _TFT_eSPI_ESP32H_
#define _TFT_eSPI_ESP32H_

 
#define PROCESSOR_ID 0x32

 
#include "soc/spi_reg.h"
#include "driver/spi_master.h"
#include "hal/gpio_ll.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32)
  #define CONFIG_IDF_TARGET_ESP32
#endif

#ifndef VSPI
  #define VSPI FSPI
#endif

 
 
#if CONFIG_IDF_TARGET_ESP32S3
   
  #ifndef REG_SPI_BASE  
    #define REG_SPI_BASE(i) (((i)>1) ? (DR_REG_SPI3_BASE) : (DR_REG_SPI2_BASE))
  #endif

   
  #ifndef SPI_MOSI_DLEN_REG
    #define SPI_MOSI_DLEN_REG(x) SPI_MS_DLEN_REG(x)
  #endif

#endif

 
#if !defined (SUPPORT_TRANSACTIONS)
  #define SUPPORT_TRANSACTIONS
#endif























 
#ifdef USE_HSPI_PORT
  #ifdef CONFIG_IDF_TARGET_ESP32
    #define SPI_PORT HSPI   
  #else
    #define SPI_PORT 3      
  #endif
#elif defined(USE_FSPI_PORT)
    #define SPI_PORT 2  
#else
  #ifdef CONFIG_IDF_TARGET_ESP32
    #define SPI_PORT VSPI
  #elif CONFIG_IDF_TARGET_ESP32S2
    #define SPI_PORT 2  
  #elif CONFIG_IDF_TARGET_ESP32S3
    #define SPI_PORT 2 
  #endif
#endif

#ifdef RPI_DISPLAY_TYPE
  #define CMD_BITS (16-1)
#else
  #define CMD_BITS (8-1)
#endif

 
#define INIT_TFT_DATA_BUS  

 
#if defined (ESP32_PARALLEL)  
  #if !defined (TFT_PARALLEL_8_BIT)
    #define TFT_PARALLEL_8_BIT  
  #endif
#endif

 
#if defined (TFT_PARALLEL_8_BIT)
  #if !defined (ESP32_PARALLEL)
    #define ESP32_PARALLEL
  #endif
#endif

#if !defined(DISABLE_ALL_LIBRARY_WARNINGS) && defined (ESP32_PARALLEL)
 #warning >>>>------>> DMA is not supported in parallel mode
#endif

 
#if !defined (ESP32_PARALLEL)
  #define _spi_cmd       (volatile uint32_t*)(SPI_CMD_REG(SPI_PORT))
  #define _spi_user      (volatile uint32_t*)(SPI_USER_REG(SPI_PORT))
  #define _spi_mosi_dlen (volatile uint32_t*)(SPI_MOSI_DLEN_REG(SPI_PORT))
  #define _spi_w         (volatile uint32_t*)(SPI_W0_REG(SPI_PORT))

  #if (TFT_SPI_MODE == SPI_MODE1) || (TFT_SPI_MODE == SPI_MODE2)
    #define SET_BUS_WRITE_MODE *_spi_user = SPI_USR_MOSI | SPI_CK_OUT_EDGE
    #define SET_BUS_READ_MODE  *_spi_user = SPI_USR_MOSI | SPI_USR_MISO | SPI_DOUTDIN | SPI_CK_OUT_EDGE
  #else
    #define SET_BUS_WRITE_MODE *_spi_user = SPI_USR_MOSI
    #define SET_BUS_READ_MODE  *_spi_user = SPI_USR_MOSI | SPI_USR_MISO | SPI_DOUTDIN
  #endif
#else
     
    #define SET_BUS_WRITE_MODE
    #define SET_BUS_READ_MODE
#endif

 
#if !defined(TFT_PARALLEL_8_BIT) && !defined(SPI_18BIT_DRIVER)
  #define ESP32_DMA
   
  #define DMA_BUSY_CHECK  dmaWait()
#else
  #define DMA_BUSY_CHECK
#endif

#if defined(TFT_PARALLEL_8_BIT)
  #define SPI_BUSY_CHECK
#else
  #define SPI_BUSY_CHECK while (*_spi_cmd&SPI_USR)
#endif

 
#ifdef SMOOTH_FONT
   
  #define FS_NO_GLOBALS
  #include <FS.h>
  #include "SPIFFS.h"  
  #define FONT_FS_AVAILABLE
#endif

 
 
 
#ifndef TFT_DC
  #define DC_C  
  #define DC_D  
#else
  #if defined (TFT_PARALLEL_8_BIT)
     
    #if (TFT_DC >= 0) &&  (TFT_DC < 32)
      #define DC_C GPIO.out_w1tc = (1 << TFT_DC)
      #define DC_D GPIO.out_w1ts = (1 << TFT_DC)
    #elif (TFT_DC >= 32)
      #define DC_C GPIO.out1_w1tc.val = (1 << (TFT_DC- 32))
      #define DC_D GPIO.out1_w1ts.val = (1 << (TFT_DC- 32))
    #else
      #define DC_C
      #define DC_D
    #endif
  #else
    #if (TFT_DC >= 32)
      #ifdef RPI_DISPLAY_TYPE   
        #define DC_C GPIO.out1_w1ts.val = (1 << (TFT_DC - 32)); \
                     GPIO.out1_w1tc.val = (1 << (TFT_DC - 32))
        #define DC_D GPIO.out1_w1tc.val = (1 << (TFT_DC - 32)); \
                     GPIO.out1_w1ts.val = (1 << (TFT_DC - 32))
      #else
        #define DC_C GPIO.out1_w1tc.val = (1 << (TFT_DC - 32)) 
        #define DC_D GPIO.out1_w1ts.val = (1 << (TFT_DC - 32)) 
      #endif
    #elif (TFT_DC >= 0)
      #if defined (RPI_DISPLAY_TYPE)
        #if defined (ILI9486_DRIVER)
           
          #define DC_C GPIO.out_w1tc = (1 << TFT_DC); \
                       GPIO.out_w1tc = (1 << TFT_DC)
          #define DC_D GPIO.out_w1tc = (1 << TFT_DC); \
                       GPIO.out_w1ts = (1 << TFT_DC)
        #else
           
          #define DC_C GPIO.out_w1tc = (1 << TFT_DC)
          #define DC_D GPIO.out_w1tc = (1 << TFT_DC); \
                       GPIO.out_w1ts = (1 << TFT_DC)
        #endif
      #else
        #define DC_C GPIO.out_w1tc = (1 << TFT_DC) 
        #define DC_D GPIO.out_w1ts = (1 << TFT_DC) 
      #endif
    #else
      #define DC_C
      #define DC_D
    #endif
  #endif
#endif

 
 
 
#ifndef TFT_CS
  #define TFT_CS -1   
  #define CS_L        
  #define CS_H        
#else
  #if defined (TFT_PARALLEL_8_BIT)
    #if TFT_CS >= 32
        #define CS_L GPIO.out1_w1tc.val = (1 << (TFT_CS - 32))
        #define CS_H GPIO.out1_w1ts.val = (1 << (TFT_CS - 32))
    #elif TFT_CS >= 0
        #define CS_L GPIO.out_w1tc = (1 << TFT_CS)
        #define CS_H GPIO.out_w1ts = (1 << TFT_CS)
    #else
      #define CS_L
      #define CS_H
    #endif
  #else
    #if (TFT_CS >= 32)
      #ifdef RPI_DISPLAY_TYPE   
        #define CS_L GPIO.out1_w1ts.val = (1 << (TFT_CS - 32)); \
                     GPIO.out1_w1tc.val = (1 << (TFT_CS - 32))
        #define CS_H GPIO.out1_w1tc.val = (1 << (TFT_CS - 32)); \
                     GPIO.out1_w1ts.val = (1 << (TFT_CS - 32))
      #else
        #define CS_L GPIO.out1_w1tc.val = (1 << (TFT_CS - 32)); GPIO.out1_w1tc.val = (1 << (TFT_CS - 32))
        #define CS_H GPIO.out1_w1ts.val = (1 << (TFT_CS - 32)) 
      #endif
    #elif (TFT_CS >= 0)
      #ifdef RPI_DISPLAY_TYPE   
        #define CS_L GPIO.out_w1ts = (1 << TFT_CS); GPIO.out_w1tc = (1 << TFT_CS)
        #define CS_H GPIO.out_w1tc = (1 << TFT_CS); GPIO.out_w1ts = (1 << TFT_CS)
      #else
        #define CS_L GPIO.out_w1tc = (1 << TFT_CS); GPIO.out_w1tc = (1 << TFT_CS)
        #define CS_H GPIO.out_w1ts = (1 << TFT_CS) 
      #endif
    #else
      #define CS_L
      #define CS_H
    #endif
  #endif
#endif

 
 
 
#if defined (TFT_WR)
  #if (TFT_WR >= 32)
     
    #define WR_L GPIO.out1_w1tc.val = (1 << (TFT_WR - 32))
    #define WR_H GPIO.out1_w1ts.val = (1 << (TFT_WR - 32))
  #elif (TFT_WR >= 0)
     
    #define WR_L GPIO.out_w1tc = (1 << TFT_WR)
    #define WR_H GPIO.out_w1ts = (1 << TFT_WR)
  #else
    #define WR_L
    #define WR_H
  #endif
#else
  #define WR_L
  #define WR_H
#endif

 
 
 
#ifndef TOUCH_CS
  #define T_CS_L  
  #define T_CS_H  
#else  
  #define T_CS_L digitalWrite(TOUCH_CS, LOW)
  #define T_CS_H digitalWrite(TOUCH_CS, HIGH)
#endif

 
 
 
#if !defined (TFT_PARALLEL_8_BIT)

  #ifdef USE_HSPI_PORT

    #ifndef TFT_MISO
      #define TFT_MISO -1
    #endif

    #ifndef TFT_MOSI
      #define TFT_MOSI 13
    #endif
    #if (TFT_MOSI == -1)
      #undef TFT_MOSI
      #define TFT_MOSI 13
    #endif

    #ifndef TFT_SCLK
      #define TFT_SCLK 14
    #endif
    #if (TFT_SCLK == -1)
      #undef TFT_SCLK
      #define TFT_SCLK 14
    #endif

  #else  

    #ifndef TFT_MISO
      #define TFT_MISO -1
    #endif

    #ifndef TFT_MOSI
      #define TFT_MOSI 23
    #endif
    #if (TFT_MOSI == -1)
      #undef TFT_MOSI
      #define TFT_MOSI 23
    #endif

    #ifndef TFT_SCLK
      #define TFT_SCLK 18
    #endif
    #if (TFT_SCLK == -1)
      #undef TFT_SCLK
      #define TFT_SCLK 18
    #endif

    #if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S2)
      #if (TFT_MISO == -1)
        #undef TFT_MISO
        #define TFT_MISO TFT_MOSI
      #endif
    #endif

  #endif

#endif

 
 
 
#if defined (TFT_PARALLEL_8_BIT)

  #if (TFT_D0 >= 32)  
    #define MASK_OFFSET 32
    #define GPIO_CLR_REG GPIO.out1_w1tc.val
    #define GPIO_SET_REG GPIO.out1_w1ts.val
  #else
    #define MASK_OFFSET 0
    #define GPIO_CLR_REG GPIO.out_w1tc
    #define GPIO_SET_REG GPIO.out_w1ts
  #endif

   
   
  #define PARALLEL_INIT_TFT_DATA_BUS               \
  for (int32_t c = 0; c<256; c++)                  \
  {                                                \
    xset_mask[c] = 0;                              \
    if ( c & 0x01 ) xset_mask[c] |= (1 << (TFT_D0-MASK_OFFSET)); \
    if ( c & 0x02 ) xset_mask[c] |= (1 << (TFT_D1-MASK_OFFSET)); \
    if ( c & 0x04 ) xset_mask[c] |= (1 << (TFT_D2-MASK_OFFSET)); \
    if ( c & 0x08 ) xset_mask[c] |= (1 << (TFT_D3-MASK_OFFSET)); \
    if ( c & 0x10 ) xset_mask[c] |= (1 << (TFT_D4-MASK_OFFSET)); \
    if ( c & 0x20 ) xset_mask[c] |= (1 << (TFT_D5-MASK_OFFSET)); \
    if ( c & 0x40 ) xset_mask[c] |= (1 << (TFT_D6-MASK_OFFSET)); \
    if ( c & 0x80 ) xset_mask[c] |= (1 << (TFT_D7-MASK_OFFSET)); \
  }                                                              \

   
  #define GPIO_DIR_MASK ((1 << (TFT_D0-MASK_OFFSET)) | (1 << (TFT_D1-MASK_OFFSET)) | (1 << (TFT_D2-MASK_OFFSET)) | (1 << (TFT_D3-MASK_OFFSET)) | (1 << (TFT_D4-MASK_OFFSET)) | (1 << (TFT_D5-MASK_OFFSET)) | (1 << (TFT_D6-MASK_OFFSET)) | (1 << (TFT_D7-MASK_OFFSET)))

  #if (TFT_WR >= 32)
    #if (TFT_D0 >= 32)
       
      #define GPIO_OUT_CLR_MASK (GPIO_DIR_MASK | (1 << (TFT_WR-32)))
    #elif (TFT_D0 >= 0)
       
      #define GPIO_OUT_CLR_MASK (GPIO_DIR_MASK); WR_L
    #endif
  #elif (TFT_WR >= 0)
    #if (TFT_D0 >= 32)
       
      #define GPIO_OUT_CLR_MASK (GPIO_DIR_MASK); WR_L
    #elif (TFT_D0 >= 0)
       
      #define GPIO_OUT_CLR_MASK (GPIO_DIR_MASK | (1 << TFT_WR))
    #endif
  #else
    #define GPIO_OUT_CLR_MASK
  #endif

   
  #define set_mask(C) xset_mask[C]  

   
  



   
  #define tft_Write_8(C)  GPIO_CLR_REG =  GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t)(C)); WR_H

  #if defined (SSD1963_DRIVER)

     
    #define tft_Write_16(C) GPIO.out_w1tc = GPIO_OUT_CLR_MASK; GPIO.out_w1ts = set_mask((uint8_t) (((C) & 0xF800)>> 8)); WR_H; \
                            GPIO.out_w1tc = GPIO_OUT_CLR_MASK; GPIO.out_w1ts = set_mask((uint8_t) (((C) & 0x07E0)>> 3)); WR_H; \
                            GPIO.out_w1tc = GPIO_OUT_CLR_MASK; GPIO.out_w1ts = set_mask((uint8_t) (((C) & 0x001F)<< 3)); WR_H

     
    #define tft_Write_16S(C) Cswap = ((C) >>8 | (C) << 8); tft_Write_16(Cswap)

  #else

    #ifdef PSEUDO_16_BIT
       
      #define tft_Write_16(C)  GPIO.out_w1tc = GPIO_OUT_CLR_MASK; GPIO.out_w1ts = set_mask((uint8_t) ((C) >> 0)); WR_H
      #define tft_Write_16S(C) GPIO.out_w1tc = GPIO_OUT_CLR_MASK; GPIO.out_w1ts = set_mask((uint8_t) ((C) >> 8)); WR_H
    #else
       
      #define tft_Write_16(C) GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 8)); WR_H; \
                              GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 0)); WR_H

       
      #define tft_Write_16S(C) GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 0)); WR_H; \
                               GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 8)); WR_H
    #endif

  #endif

   
  #define tft_Write_32(C) GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 24)); WR_H; \
                          GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 16)); WR_H; \
                          GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >>  8)); WR_H; \
                          GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >>  0)); WR_H

   
  #define tft_Write_32C(C,D) GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 8)); WR_H; \
                             GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 0)); WR_H; \
                             GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((D) >> 8)); WR_H; \
                             GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((D) >> 0)); WR_H

   
  #define tft_Write_32D(C) GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 8)); WR_H; \
                           GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 0)); WR_H; \
                           GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 8)); WR_H; \
                           GPIO_CLR_REG = GPIO_OUT_CLR_MASK; GPIO_SET_REG = set_mask((uint8_t) ((C) >> 0)); WR_H

    
  #ifdef TFT_RD
    #if (TFT_RD >= 32)
      #define RD_L GPIO.out1_w1tc.val = (1 << (TFT_RD - 32))
      #define RD_H GPIO.out1_w1ts.val = (1 << (TFT_RD - 32))
    #elif (TFT_RD >= 0)
      #define RD_L GPIO.out_w1tc = (1 << TFT_RD)
       
      #define RD_H GPIO.out_w1ts = (1 << TFT_RD)
       
    #else
      #define RD_L
      #define RD_H
    #endif
  #else
    #define TFT_RD -1
    #define RD_L
    #define RD_H
  #endif

 
 
 
#elif  defined (SPI_18BIT_DRIVER)  

   
  #define tft_Write_8(C)   spi.transfer(C)

   
  #define tft_Write_16(C)  spi.transfer(((C) & 0xF800)>>8); \
                           spi.transfer(((C) & 0x07E0)>>3); \
                           spi.transfer(((C) & 0x001F)<<3)

   
  #define tft_Write_16N(C) tft_Write_16(C)

   
  #define tft_Write_16S(C) spi.transfer((C) & 0xF8); \
                           spi.transfer(((C) & 0xE000)>>11 | ((C) & 0x07)<<5); \
                           spi.transfer(((C) & 0x1F00)>>5)

   
  #define tft_Write_32(C)  spi.write32(C)

   
  #define tft_Write_32C(C,D) spi.write32((C)<<16 | (D))

   
  #define tft_Write_32D(C)  spi.write32((C)<<16 | (C))

 
 
 
#elif  defined (RPI_DISPLAY_TYPE)
   
   
  #define TFT_WRITE_BITS(D, B) *_spi_mosi_dlen = B-1;    \
                               *_spi_w = D;              \
                               *_spi_cmd = SPI_UPDATE;   \
                        while (*_spi_cmd & SPI_UPDATE);  \
                               *_spi_cmd = SPI_USR;      \
                        while (*_spi_cmd & SPI_USR);

   
  #define tft_Write_8(C) TFT_WRITE_BITS((C)<<8, 16)

   
  #define tft_Write_16(C) TFT_WRITE_BITS((C)<<8 | (C)>>8, 16)

   
  #define tft_Write_16N(C) tft_Write_16(C)

   
  #define tft_Write_16S(C) TFT_WRITE_BITS(C, 16)

   
  #define tft_Write_32(C) TFT_WRITE_BITS(C, 32)

   
  #define tft_Write_32C(C,D)  TFT_WRITE_BITS((C)<<24 | (C), 32); \
                              TFT_WRITE_BITS((D)<<24 | (D), 32)

   
  #define tft_Write_32D(C) tft_Write_32C(C,C)

 
 
 
#else
  #if !defined(CONFIG_IDF_TARGET_ESP32S3)
    #define TFT_WRITE_BITS(D, B) *_spi_mosi_dlen = B-1;  \
                               *_spi_w = D;              \
                               *_spi_cmd = SPI_USR;      \
                        while (*_spi_cmd & SPI_USR);
  #else
    #define TFT_WRITE_BITS(D, B) *_spi_mosi_dlen = B-1;  \
                               *_spi_w = D;              \
                               *_spi_cmd = SPI_UPDATE;   \
                        while (*_spi_cmd & SPI_UPDATE);  \
                               *_spi_cmd = SPI_USR;      \
                        while (*_spi_cmd & SPI_USR);
  #endif
   
  #define tft_Write_8(C) TFT_WRITE_BITS(C, 8)

   
  #define tft_Write_16(C) TFT_WRITE_BITS((C)<<8 | (C)>>8, 16)

   
  #if !defined(CONFIG_IDF_TARGET_ESP32S3)
    #define tft_Write_16N(C) *_spi_mosi_dlen = 16-1;    \
                           *_spi_w = ((C)<<8 | (C)>>8); \
                           *_spi_cmd = SPI_USR;
  #else
    #define tft_Write_16N(C) *_spi_mosi_dlen = 16-1;    \
                           *_spi_w = ((C)<<8 | (C)>>8); \
                           *_spi_cmd = SPI_UPDATE;      \
                    while (*_spi_cmd & SPI_UPDATE);     \
                           *_spi_cmd = SPI_USR;
  #endif

   
  #define tft_Write_16S(C) TFT_WRITE_BITS(C, 16)

   
  #define tft_Write_32(C) TFT_WRITE_BITS(C, 32)

   
  #define tft_Write_32C(C,D)  TFT_WRITE_BITS((uint16_t)((D)<<8 | (D)>>8)<<16 | (uint16_t)((C)<<8 | (C)>>8), 32)

   
  #define tft_Write_32D(C) TFT_WRITE_BITS((uint16_t)((C)<<8 | (C)>>8)<<16 | (uint16_t)((C)<<8 | (C)>>8), 32)

#endif

#ifndef tft_Write_16N
  #define tft_Write_16N tft_Write_16
#endif

 
 
 
#if !defined (TFT_PARALLEL_8_BIT)
   
   
  #define tft_Read_8() spi.transfer(0)
#endif

 
#define DAT8TO32(P) ( (uint32_t)P[0]<<8 | P[1] | P[2]<<24 | P[3]<<16 )

#endif  
