         
         
         

#ifndef _TFT_eSPI_STM32H_
#define _TFT_eSPI_STM32H_

 
#define PROCESSOR_ID 0x32F

 
 

 

 
#define SET_BUS_WRITE_MODE  
#define SET_BUS_READ_MODE   

 
#if !defined (SUPPORT_TRANSACTIONS)
  #define SUPPORT_TRANSACTIONS
#endif

 
 
 
#if defined(TFT_PARALLEL_8_BIT)

   
   
   
   
   
   
   
   
   
   
   
   
   
   

 
 
 
  #if defined (ILI9341_DRIVER) || defined(ILI9341_2_DRIVER) || defined (ST7796_DRIVER)   || defined (ILI9486_DRIVER)  

     
    #if defined (STM32F1xx)
      #define WR_TWRL_0        
    #elif defined (STM32F2xx) || defined (STM32F4xx)
      #define WR_TWRL_0        
       
    #elif defined (STM32F7xx)
      #define WR_TWRL_1        
    #else
      #define WR_TWRL_5
    #endif

     
    #if defined (STM32F1xx)
      #define WR_TWRH_0
    #elif defined (STM32F2xx) || defined (STM32F4xx)
      #define WR_TWRH_0        
       
    #elif defined (STM32F7xx)
      #define WR_TWRH_1        
    #else
      #define WR_TWRH_5
    #endif

  #elif defined (ILI9481_DRIVER)  

     
    #if defined (STM32F1xx)
      #define WR_TWRL_0
    #elif defined (STM32F2xx) || defined (STM32F4xx)
       
       
      #define WR_TWRL_2       
       
       
       
    #elif defined (STM32F7xx)
       
       
       
      #define WR_TWRL_3
    #else
       
       
       
      #define WR_TWRH_3       
    #endif

     
    #if defined (STM32F1xx)
      #define WR_TWRH_0
    #elif defined (STM32F2xx) || defined (STM32F4xx)
       
       
      #define WR_TWRH_2
       
    #elif defined (STM32F7xx)
       
       
       
      #define WR_TWRH_3
       
       
    #else
       
       
       
      #define WR_TWRH_3        
    #endif

  #else  
    #if defined (STM32F1xx)
       
      #define WR_TWRL_0
      #define WR_TWRH_0
    #else
       
       
       
       
      #define WR_TWRL_3
       
       

       
       
       
       
       
       
      #define WR_TWRH_5
    #endif
  #endif

 
 
 
#else

   
  #ifndef TFT_SPI_PORT
    #define TFT_SPI_PORT 1
  #endif

  #if (TFT_SPI_PORT == 1)
    #define SPIX SPI1
  #elif (TFT_SPI_PORT == 2)
    #define SPIX SPI2
  #elif (TFT_SPI_PORT == 3)
    #define SPIX SPI3
  #elif (TFT_SPI_PORT == 4)
    #define SPIX SPI4
  #endif
   
  #if defined (STM32F2xx) || defined (STM32F4xx) || defined (STM32F7xx)

    #define STM32_DMA  

    #if (TFT_SPI_PORT == 1)
       
      #define INIT_TFT_DATA_BUS spiHal.Instance = SPI1; \
                                dmaHal.Instance = DMA2_Stream3
       
       
       
    #elif (TFT_SPI_PORT == 2)
       
      #define INIT_TFT_DATA_BUS spiHal.Instance = SPI2; \
                                dmaHal.Instance = DMA1_Stream4
       
       
       
    #elif (TFT_SPI_PORT == 3)
       
      #define INIT_TFT_DATA_BUS spiHal.Instance = SPI3; \
                                dmaHal.Instance = DMA1_Stream5
       
       
       
    #endif

  #elif defined (STM32F1xx)
     
    #define STM32_DMA  
    #if (TFT_SPI_PORT == 1)
      #define INIT_TFT_DATA_BUS spiHal.Instance = SPI1; \
                                dmaHal.Instance = DMA1_Channel3
    #elif (TFT_SPI_PORT == 2)
      #define INIT_TFT_DATA_BUS spiHal.Instance = SPI2; \
                                dmaHal.Instance = DMA1_Channel5
    #endif
  #else
     
    #if (TFT_SPI_PORT == 1)
      #define INIT_TFT_DATA_BUS spiHal.Instance = SPI1
    #elif (TFT_SPI_PORT == 2)
      #define INIT_TFT_DATA_BUS spiHal.Instance = SPI2
    #endif
  #endif

#endif

#ifdef STM32_DMA
   
  #define DMA_BUSY_CHECK { if (DMA_Enabled) while(dmaBusy()); }
#else
  #define DMA_BUSY_CHECK
#endif

 
#ifdef SMOOTH_FONT
   
   
   
#endif  

 
 
 
#if !defined (TFT_DC) || (TFT_DC < 0)
  #define DC_C  
  #define DC_D  
  #undef  TFT_DC
#else
   
  #define DC_PORT     digitalPinToPort(TFT_DC)
  #define DC_PIN_MASK digitalPinToBitMask(TFT_DC)
   
  #define DC_C DC_DELAY; DC_PORT->BSRR = DC_PIN_MASK<<16
  #define DC_D DC_DELAY; DC_PORT->BSRR = DC_PIN_MASK
#endif

 
 
 
#if !defined (TFT_CS) || (TFT_CS < 0)
  #define CS_L  
  #define CS_H  
  #undef  TFT_CS
#else
   
  #define CS_PORT      digitalPinToPort(TFT_CS)
  #define CS_PIN_MASK  digitalPinToBitMask(TFT_CS)
   
  #define CS_L CS_PORT->BSRR = CS_PIN_MASK<<16
  #define CS_H CS_PORT->BSRR = CS_PIN_MASK
#endif

 
 
 
#ifdef TFT_RD
  #if (TFT_RD >= 0)
     
    #define RD_PORT      digitalPinToPort(TFT_RD)
    #define RD_PIN_MASK  digitalPinToBitMask(TFT_RD)
     
    #define RD_L RD_PORT->BSRR = RD_PIN_MASK<<16
    #define RD_H RD_PORT->BSRR = RD_PIN_MASK
  #else
    #define RD_L
    #define RD_H
  #endif
#else
  #define TFT_RD -1
  #define RD_L
  #define RD_H
#endif

 
 
 
#ifdef TFT_WR
   
  #define WR_PORT      digitalPinToPort(TFT_WR)
  #define WR_PIN_MASK  digitalPinToBitMask(TFT_WR)
   
  #define WR_L WR_PORT->BSRR = WR_PIN_MASK<<16
  #define WR_H WR_PORT->BSRR = WR_PIN_MASK
#endif

 
 
 
#if !defined (TOUCH_CS) || (TOUCH_CS < 0)
  #define T_CS_L  
  #define T_CS_H  
#else
   
  #define T_CS_L digitalWrite(TOUCH_CS, LOW)
  #define T_CS_H digitalWrite(TOUCH_CS, HIGH)
#endif

 
 
 
#if !defined (TFT_PARALLEL_8_BIT)
  #ifndef TFT_MISO
    #define TFT_MISO -1
  #endif
#endif

 
 
 
#if defined (TFT_PARALLEL_8_BIT)

   
  #define GPIO_DIR_MASK 0

  #define PARALLEL_INIT_TFT_DATA_BUS  

  #define INIT_TFT_DATA_BUS  

 
 
 
   
  #if   defined WR_TWRL_0
    #define     WR_TWRL
  #elif defined WR_TWRL_1  
    #define     WR_TWRL WR_L
  #elif defined WR_TWRL_2  
    #define     WR_TWRL WR_L; WR_L
  #elif defined WR_TWRL_3  
    #define     WR_TWRL WR_L; WR_L; WR_L
  #elif defined WR_TWRL_4  
    #define     WR_TWRL WR_L; WR_L; WR_L; WR_L
  #elif defined WR_TWRL_5  
    #define     WR_TWRL WR_L; WR_L; WR_L; WR_L; WR_L
  #endif

   
  #if   defined WR_TWRH_0
    #define     WR_TWRH WR_H
  #elif defined WR_TWRH_1  
    #define     WR_TWRH  WR_H; WR_H
  #elif defined WR_TWRH_2  
    #define     WR_TWRH  WR_H; WR_H; WR_H
  #elif defined WR_TWRH_3  
    #define     WR_TWRH  WR_H; WR_H; WR_H; WR_H
  #elif defined WR_TWRH_4  
    #define     WR_TWRH  WR_H; WR_H; WR_H; WR_H; WR_H
  #elif defined WR_TWRH_5  
    #define     WR_TWRH  WR_H; WR_H; WR_H; WR_H; WR_H; WR_H
  #endif

  #define WR_STB WR_TWRL; WR_TWRH  

 
 
 
  #ifdef NUCLEO_64_TFT

     
    #define D0_PIN_NAME  digitalPinToPinName(TFT_D0)
    #define D1_PIN_NAME  digitalPinToPinName(TFT_D1)
    #define D2_PIN_NAME  digitalPinToPinName(TFT_D2)
    #define D3_PIN_NAME  digitalPinToPinName(TFT_D3)
    #define D4_PIN_NAME  digitalPinToPinName(TFT_D4)
    #define D5_PIN_NAME  digitalPinToPinName(TFT_D5)
    #define D6_PIN_NAME  digitalPinToPinName(TFT_D6)
    #define D7_PIN_NAME  digitalPinToPinName(TFT_D7)

     
    #define D0_PIN_BIT  (D0_PIN_NAME & 0xF)
    #define D1_PIN_BIT  (D1_PIN_NAME & 0xF)
    #define D2_PIN_BIT  (D2_PIN_NAME & 0xF)
    #define D3_PIN_BIT  (D3_PIN_NAME & 0xF)
    #define D4_PIN_BIT  (D4_PIN_NAME & 0xF)
    #define D5_PIN_BIT  (D5_PIN_NAME & 0xF)
    #define D6_PIN_BIT  (D6_PIN_NAME & 0xF)
    #define D7_PIN_BIT  (D7_PIN_NAME & 0xF)

     
    #define D0_PIN_PORT GPIOA
    #define D1_PIN_PORT GPIOC
    #define D2_PIN_PORT GPIOA
    #define D3_PIN_PORT GPIOB
    #define D4_PIN_PORT GPIOB
    #define D5_PIN_PORT GPIOB
    #define D6_PIN_PORT GPIOB
    #define D7_PIN_PORT GPIOA

     
    #define D0_PIN_MASK (1UL<< 9)  
    #define D1_PIN_MASK (1UL<< 7)  
    #define D2_PIN_MASK (1UL<<10)  
    #define D3_PIN_MASK (1UL<< 3)  
    #define D4_PIN_MASK (1UL<< 5)  
    #define D5_PIN_MASK (1UL<< 4)  
    #define D6_PIN_MASK (1UL<<10)  
    #define D7_PIN_MASK (1UL<< 8)  

     
    #define  D0_BSR_MASK(B) ((D0_PIN_MASK<<16)>>(((B)<< 4)&0x10))
    #define  D1_BSR_MASK(B) ((D1_PIN_MASK<<16)>>(((B)<< 3)&0x10))
    #define  D2_BSR_MASK(B) ((D2_PIN_MASK<<16)>>(((B)<< 2)&0x10))
    #define  D3_BSR_MASK(B) ((D3_PIN_MASK<<16)>>(((B)<< 1)&0x10))
    #define  D4_BSR_MASK(B) ((D4_PIN_MASK<<16)>>(((B)<< 0)&0x10))
    #define  D5_BSR_MASK(B) ((D5_PIN_MASK<<16)>>(((B)>> 1)&0x10))
    #define  D6_BSR_MASK(B) ((D6_PIN_MASK<<16)>>(((B)>> 2)&0x10))
    #define  D7_BSR_MASK(B) ((D7_PIN_MASK<<16)>>(((B)>> 3)&0x10))
     
    #define  D8_BSR_MASK(B) ((D0_PIN_MASK<<16)>>(((B)>> 4)&0x10))
    #define  D9_BSR_MASK(B) ((D1_PIN_MASK<<16)>>(((B)>> 5)&0x10))
    #define D10_BSR_MASK(B) ((D2_PIN_MASK<<16)>>(((B)>> 6)&0x10))
    #define D11_BSR_MASK(B) ((D3_PIN_MASK<<16)>>(((B)>> 7)&0x10))
    #define D12_BSR_MASK(B) ((D4_PIN_MASK<<16)>>(((B)>> 8)&0x10))
    #define D13_BSR_MASK(B) ((D5_PIN_MASK<<16)>>(((B)>> 9)&0x10))
    #define D14_BSR_MASK(B) ((D6_PIN_MASK<<16)>>(((B)>>10)&0x10))
    #define D15_BSR_MASK(B) ((D7_PIN_MASK<<16)>>(((B)>>11)&0x10))

     
    #define tft_Write_8(C)   GPIOA->BSRR = D0_BSR_MASK(C) | D2_BSR_MASK(C) | D7_BSR_MASK(C); \
                             WR_L; \
                             GPIOC->BSRR = D1_BSR_MASK(C); \
                             GPIOB->BSRR = D3_BSR_MASK(C) | D4_BSR_MASK(C) | D5_BSR_MASK(C) | D6_BSR_MASK(C); \
                             WR_STB  

    #if defined (SSD1963_DRIVER)

       
      #define tft_Write_16(C)  r6 = (((C) & 0xF800)>> 8); g6 = (((C) & 0x07E0)>> 3); b6 = (((C) & 0x001F)<< 3); \
                               GPIOA->BSRR = D0_BSR_MASK(r6) | D2_BSR_MASK(r6) | D7_BSR_MASK(r6); \
                               WR_L; \
                               GPIOC->BSRR = D1_BSR_MASK(r6); \
                               GPIOB->BSRR = D3_BSR_MASK(r6) | D4_BSR_MASK(r6) | D5_BSR_MASK(r6) | D6_BSR_MASK(r6); \
                               WR_STB; \
                               GPIOA->BSRR = D0_BSR_MASK(g6) | D2_BSR_MASK(g6) | D7_BSR_MASK(g6); \
                               WR_L; \
                               GPIOC->BSRR = D1_BSR_MASK(g6); \
                               GPIOB->BSRR = D3_BSR_MASK(g6) | D4_BSR_MASK(g6) | D5_BSR_MASK(g6) | D6_BSR_MASK(g6); \
                               WR_STB; \
                               GPIOA->BSRR = D0_BSR_MASK(b6) | D2_BSR_MASK(b6) | D7_BSR_MASK(b6); \
                               WR_L; \
                               GPIOC->BSRR = D1_BSR_MASK(b6); \
                               GPIOB->BSRR = D3_BSR_MASK(b6) | D4_BSR_MASK(b6) | D5_BSR_MASK(b6) | D6_BSR_MASK(b6); \
                               WR_STB  

       
      #define tft_Write_16S(C) Cswap = ((C) >>8 | (C) << 8); tft_Write_16(Cswap)

    #else
       
      #define tft_Write_16(C)  GPIOA->BSRR = D8_BSR_MASK(C) | D10_BSR_MASK(C) | D15_BSR_MASK(C); \
                               WR_L; \
                               GPIOC->BSRR = D9_BSR_MASK(C); \
                               GPIOB->BSRR = D11_BSR_MASK(C) | D12_BSR_MASK(C) | D13_BSR_MASK(C) | D14_BSR_MASK(C); \
                               WR_STB; \
                               GPIOA->BSRR = D0_BSR_MASK(C) | D2_BSR_MASK(C) | D7_BSR_MASK(C); \
                               WR_L; \
                               GPIOC->BSRR = D1_BSR_MASK(C); \
                               GPIOB->BSRR = D3_BSR_MASK(C) | D4_BSR_MASK(C) | D5_BSR_MASK(C) | D6_BSR_MASK(C); \
                               WR_STB  

       
      #define tft_Write_16S(C) GPIOA->BSRR = D0_BSR_MASK(C) | D2_BSR_MASK(C) | D7_BSR_MASK(C); \
                               WR_L; \
                               GPIOC->BSRR = D1_BSR_MASK(C); \
                               GPIOB->BSRR = D3_BSR_MASK(C) | D4_BSR_MASK(C) | D5_BSR_MASK(C) | D6_BSR_MASK(C); \
                               WR_STB; \
                               GPIOA->BSRR = D8_BSR_MASK(C) | D10_BSR_MASK(C) | D15_BSR_MASK(C); \
                               WR_L; \
                               GPIOC->BSRR = D9_BSR_MASK(C); \
                               GPIOB->BSRR = D11_BSR_MASK(C) | D12_BSR_MASK(C) | D13_BSR_MASK(C) | D14_BSR_MASK(C); \
                               WR_STB
    #endif

    #define tft_Write_32(C)    tft_Write_16((uint16_t)((C)>>16)); tft_Write_16((uint16_t)(C))

    #define tft_Write_32C(C,D) tft_Write_16((uint16_t)(C)); tft_Write_16((uint16_t)(D))

    #define tft_Write_32D(C)   tft_Write_16((uint16_t)(C)); tft_Write_16((uint16_t)(C))

     
    #define RD_TFT_D0 (((GPIOA->IDR)&(D0_PIN_MASK))>>( 9-0))  
    #define RD_TFT_D1 (((GPIOC->IDR)&(D1_PIN_MASK))>>( 7-1))  
    #define RD_TFT_D2 (((GPIOA->IDR)&(D2_PIN_MASK))>>(10-2))  
    #define RD_TFT_D3 (((GPIOB->IDR)&(D3_PIN_MASK))>>( 3-3))  
    #define RD_TFT_D4 (((GPIOB->IDR)&(D4_PIN_MASK))>>( 5-4))  
    #define RD_TFT_D5 (((GPIOB->IDR)&(D5_PIN_MASK))<<(-4+5))  
    #define RD_TFT_D6 (((GPIOB->IDR)&(D6_PIN_MASK))>>(10-6))  
    #define RD_TFT_D7 (((GPIOA->IDR)&(D7_PIN_MASK))>>( 8-7))  

 
 
 
  #elif defined (NUCLEO_144_TFT)

     
     
    #define D0_PIN_NAME  digitalPinToPinName(TFT_D0)
    #define D1_PIN_NAME  digitalPinToPinName(TFT_D1)
    #define D2_PIN_NAME  digitalPinToPinName(TFT_D2)
    #define D3_PIN_NAME  digitalPinToPinName(TFT_D3)
    #define D4_PIN_NAME  digitalPinToPinName(TFT_D4)
    #define D5_PIN_NAME  digitalPinToPinName(TFT_D5)
    #define D6_PIN_NAME  digitalPinToPinName(TFT_D6)
    #define D7_PIN_NAME  digitalPinToPinName(TFT_D7)

     
    #define D0_PIN_BIT  (D0_PIN_NAME & 0xF)
    #define D1_PIN_BIT  (D1_PIN_NAME & 0xF)
    #define D2_PIN_BIT  (D2_PIN_NAME & 0xF)
    #define D3_PIN_BIT  (D3_PIN_NAME & 0xF)
    #define D4_PIN_BIT  (D4_PIN_NAME & 0xF)
    #define D5_PIN_BIT  (D5_PIN_NAME & 0xF)
    #define D6_PIN_BIT  (D6_PIN_NAME & 0xF)
    #define D7_PIN_BIT  (D7_PIN_NAME & 0xF)


    #if !defined (STM32H7xx)

       
      #define D0_PIN_PORT GPIOF
      #define D1_PIN_PORT GPIOD
      #define D2_PIN_PORT GPIOF
      #define D3_PIN_PORT GPIOE
      #define D4_PIN_PORT GPIOF
      #define D5_PIN_PORT GPIOE
      #define D6_PIN_PORT GPIOE
      #define D7_PIN_PORT GPIOF

       
      #define D0_PIN_MASK (1UL<<12)  
      #define D1_PIN_MASK (1UL<<15)  
      #define D2_PIN_MASK (1UL<<15)  
      #define D3_PIN_MASK (1UL<<13)  
      #define D4_PIN_MASK (1UL<<14)  
      #define D5_PIN_MASK (1UL<<11)  
      #define D6_PIN_MASK (1UL<< 9)  
      #define D7_PIN_MASK (1UL<<13)  

       
      #define  D0_BSR_MASK(B) ((D0_PIN_MASK<<16)>>(((B)<< 4)&0x10))
      #define  D1_BSR_MASK(B) ((D1_PIN_MASK<<16)>>(((B)<< 3)&0x10))
      #define  D2_BSR_MASK(B) ((D2_PIN_MASK<<16)>>(((B)<< 2)&0x10))
      #define  D3_BSR_MASK(B) ((D3_PIN_MASK<<16)>>(((B)<< 1)&0x10))
      #define  D4_BSR_MASK(B) ((D4_PIN_MASK<<16)>>(((B)<< 0)&0x10))
      #define  D5_BSR_MASK(B) ((D5_PIN_MASK<<16)>>(((B)>> 1)&0x10))
      #define  D6_BSR_MASK(B) ((D6_PIN_MASK<<16)>>(((B)>> 2)&0x10))
      #define  D7_BSR_MASK(B) ((D7_PIN_MASK<<16)>>(((B)>> 3)&0x10))
       
      #define  D8_BSR_MASK(B) ((D0_PIN_MASK<<16)>>(((B)>> 4)&0x10))
      #define  D9_BSR_MASK(B) ((D1_PIN_MASK<<16)>>(((B)>> 5)&0x10))
      #define D10_BSR_MASK(B) ((D2_PIN_MASK<<16)>>(((B)>> 6)&0x10))
      #define D11_BSR_MASK(B) ((D3_PIN_MASK<<16)>>(((B)>> 7)&0x10))
      #define D12_BSR_MASK(B) ((D4_PIN_MASK<<16)>>(((B)>> 8)&0x10))
      #define D13_BSR_MASK(B) ((D5_PIN_MASK<<16)>>(((B)>> 9)&0x10))
      #define D14_BSR_MASK(B) ((D6_PIN_MASK<<16)>>(((B)>>10)&0x10))
      #define D15_BSR_MASK(B) ((D7_PIN_MASK<<16)>>(((B)>>11)&0x10))


       
      #define tft_Write_8(C)   GPIOF->BSRR = D0_BSR_MASK(C) | D2_BSR_MASK(C) | D4_BSR_MASK(C) | D7_BSR_MASK(C); \
                               WR_L; \
                               GPIOD->BSRR = D1_BSR_MASK(C); \
                               GPIOE->BSRR = D3_BSR_MASK(C) | D5_BSR_MASK(C) | D6_BSR_MASK(C); \
                               WR_STB

      #if defined (SSD1963_DRIVER)

         
        #define tft_Write_16(C)  r6 = (((C) & 0xF800)>> 8); g6 = (((C) & 0x07E0)>> 3); b6 = (((C) & 0x001F)<< 3); \
                                 GPIOF->BSRR = D0_BSR_MASK(r6) | D2_BSR_MASK(r6) | D4_BSR_MASK(r6) | D7_BSR_MASK(r6); \
                                 WR_L; \
                                 GPIOD->BSRR = D1_BSR_MASK(r6); \
                                 GPIOE->BSRR = D3_BSR_MASK(r6) | D5_BSR_MASK(r6) | D6_BSR_MASK(r6); \
                                 WR_STB; \
                                 GPIOF->BSRR = D0_BSR_MASK(g6) | D2_BSR_MASK(g6) | D4_BSR_MASK(g6) | D7_BSR_MASK(g6); \
                                 WR_L; \
                                 GPIOD->BSRR = D1_BSR_MASK(g6); \
                                 GPIOE->BSRR = D3_BSR_MASK(g6) | D5_BSR_MASK(g6) | D6_BSR_MASK(g6); \
                                 WR_STB; \
                                 GPIOF->BSRR = D0_BSR_MASK(b6) | D2_BSR_MASK(b6) | D4_BSR_MASK(b6) | D7_BSR_MASK(b6); \
                                 WR_L; \
                                 GPIOD->BSRR = D1_BSR_MASK(b6); \
                                 GPIOE->BSRR = D3_BSR_MASK(b6) | D5_BSR_MASK(b6) | D6_BSR_MASK(b6); \
                                 WR_STB  

         
        #define tft_Write_16S(C) Cswap = ((C) >>8 | (C) << 8); tft_Write_16(Cswap)

      #else

         
        #define tft_Write_16(C)  GPIOF->BSRR = D8_BSR_MASK(C) | D10_BSR_MASK(C) | D12_BSR_MASK(C) | D15_BSR_MASK(C); \
                                 WR_L; \
                                 GPIOD->BSRR = D9_BSR_MASK(C); \
                                 GPIOE->BSRR = D11_BSR_MASK(C) | D13_BSR_MASK(C) | D14_BSR_MASK(C); \
                                 WR_STB;\
                                 GPIOF->BSRR = D0_BSR_MASK(C) | D2_BSR_MASK(C) | D4_BSR_MASK(C) | D7_BSR_MASK(C); \
                                 WR_L; \
                                 GPIOD->BSRR = D1_BSR_MASK(C); \
                                 GPIOE->BSRR = D3_BSR_MASK(C) | D5_BSR_MASK(C) | D6_BSR_MASK(C); \
                                 WR_STB

         
        #define tft_Write_16S(C) GPIOF->BSRR = D0_BSR_MASK(C) | D2_BSR_MASK(C) | D4_BSR_MASK(C) | D7_BSR_MASK(C); \
                                 WR_L; \
                                 GPIOD->BSRR = D1_BSR_MASK(C); \
                                 GPIOE->BSRR = D3_BSR_MASK(C) | D5_BSR_MASK(C) | D6_BSR_MASK(C); \
                                 WR_STB; \
                                 GPIOF->BSRR = D8_BSR_MASK(C) | D10_BSR_MASK(C) | D12_BSR_MASK(C) | D15_BSR_MASK(C); \
                                 WR_L; \
                                 GPIOD->BSRR = D9_BSR_MASK(C); \
                                 GPIOE->BSRR = D11_BSR_MASK(C) | D13_BSR_MASK(C) | D14_BSR_MASK(C); \
                                 WR_STB

      #endif

      #define tft_Write_32(C)    tft_Write_16((uint16_t)((C)>>16)); tft_Write_16((uint16_t)(C))

      #define tft_Write_32C(C,D) tft_Write_16((uint16_t)(C)); tft_Write_16((uint16_t)(D))

      #define tft_Write_32D(C)   tft_Write_16((uint16_t)(C)); tft_Write_16((uint16_t)(C))

       
      #define RD_TFT_D0 (((GPIOF->IDR)&(D0_PIN_MASK))>>(12-0))  
      #define RD_TFT_D1 (((GPIOD->IDR)&(D1_PIN_MASK))>>(15-1))  
      #define RD_TFT_D2 (((GPIOF->IDR)&(D2_PIN_MASK))>>(15-2))  
      #define RD_TFT_D3 (((GPIOE->IDR)&(D3_PIN_MASK))>>(13-3))  
      #define RD_TFT_D4 (((GPIOF->IDR)&(D4_PIN_MASK))>>(14-4))  
      #define RD_TFT_D5 (((GPIOE->IDR)&(D5_PIN_MASK))>>(11-5))  
      #define RD_TFT_D6 (((GPIOE->IDR)&(D6_PIN_MASK))>>( 9-6))  
      #define RD_TFT_D7 (((GPIOF->IDR)&(D7_PIN_MASK))>>(13-7))  

    #else

       

       
      #define D0_PIN_PORT GPIOF
      #define D1_PIN_PORT GPIOD
      #define D2_PIN_PORT GPIOG
      #define D3_PIN_PORT GPIOE
      #define D4_PIN_PORT GPIOE
      #define D5_PIN_PORT GPIOE
      #define D6_PIN_PORT GPIOE
      #define D7_PIN_PORT GPIOG

       
      #define D0_PIN_MASK (1UL<< 3)  
      #define D1_PIN_MASK (1UL<<15)  
      #define D2_PIN_MASK (1UL<<14)  
      #define D3_PIN_MASK (1UL<<13)  
      #define D4_PIN_MASK (1UL<<14)  
      #define D5_PIN_MASK (1UL<<11)  
      #define D6_PIN_MASK (1UL<< 9)  
      #define D7_PIN_MASK (1UL<<12)  

       
      #define  D0_BSR_MASK(B) ((D0_PIN_MASK<<16)>>(((B)<< 4)&0x10))
      #define  D1_BSR_MASK(B) ((D1_PIN_MASK<<16)>>(((B)<< 3)&0x10))
      #define  D2_BSR_MASK(B) ((D2_PIN_MASK<<16)>>(((B)<< 2)&0x10))
      #define  D3_BSR_MASK(B) ((D3_PIN_MASK<<16)>>(((B)<< 1)&0x10))
      #define  D4_BSR_MASK(B) ((D4_PIN_MASK<<16)>>(((B)<< 0)&0x10))
      #define  D5_BSR_MASK(B) ((D5_PIN_MASK<<16)>>(((B)>> 1)&0x10))
      #define  D6_BSR_MASK(B) ((D6_PIN_MASK<<16)>>(((B)>> 2)&0x10))
      #define  D7_BSR_MASK(B) ((D7_PIN_MASK<<16)>>(((B)>> 3)&0x10))
       
      #define  D8_BSR_MASK(B) ((D0_PIN_MASK<<16)>>(((B)>> 4)&0x10))
      #define  D9_BSR_MASK(B) ((D1_PIN_MASK<<16)>>(((B)>> 5)&0x10))
      #define D10_BSR_MASK(B) ((D2_PIN_MASK<<16)>>(((B)>> 6)&0x10))
      #define D11_BSR_MASK(B) ((D3_PIN_MASK<<16)>>(((B)>> 7)&0x10))
      #define D12_BSR_MASK(B) ((D4_PIN_MASK<<16)>>(((B)>> 8)&0x10))
      #define D13_BSR_MASK(B) ((D5_PIN_MASK<<16)>>(((B)>> 9)&0x10))
      #define D14_BSR_MASK(B) ((D6_PIN_MASK<<16)>>(((B)>>10)&0x10))
      #define D15_BSR_MASK(B) ((D7_PIN_MASK<<16)>>(((B)>>11)&0x10))


       
      #define tft_Write_8(C)   GPIOF->BSRR = D0_BSR_MASK(C); \
                               GPIOG->BSRR = D2_BSR_MASK(C) | D7_BSR_MASK(C); \
                               WR_L; \
                               GPIOD->BSRR = D1_BSR_MASK(C); \
                               GPIOE->BSRR = D3_BSR_MASK(C) | D4_BSR_MASK(C) | D5_BSR_MASK(C) | D6_BSR_MASK(C); \
                               WR_STB

       
      #define tft_Write_16(C)  GPIOF->BSRR = D8_BSR_MASK(C); \
                               GPIOG->BSRR = D10_BSR_MASK(C) | D15_BSR_MASK(C); \
                               WR_L; \
                               GPIOD->BSRR = D9_BSR_MASK(C); \
                               GPIOE->BSRR = D11_BSR_MASK(C) | D12_BSR_MASK(C) | D13_BSR_MASK(C) | D14_BSR_MASK(C); \
                               WR_STB;\
                               GPIOF->BSRR = D0_BSR_MASK(C); \
                               GPIOG->BSRR = D2_BSR_MASK(C) | D7_BSR_MASK(C); \
                               WR_L; \
                               GPIOD->BSRR = D1_BSR_MASK(C); \
                               GPIOE->BSRR = D3_BSR_MASK(C) | D4_BSR_MASK(C) | D5_BSR_MASK(C) | D6_BSR_MASK(C); \
                               WR_STB

       
      #define tft_Write_16S(C) GPIOF->BSRR = D0_BSR_MASK(C); \
                               GPIOG->BSRR = D2_BSR_MASK(C) | D7_BSR_MASK(C); \
                               WR_L; \
                               GPIOD->BSRR = D1_BSR_MASK(C); \
                               GPIOE->BSRR = D3_BSR_MASK(C) | D4_BSR_MASK(C) | D5_BSR_MASK(C) | D6_BSR_MASK(C); \
                               WR_STB; \
                               GPIOF->BSRR = D8_BSR_MASK(C); \
                               GPIOG->BSRR = D10_BSR_MASK(C) | D15_BSR_MASK(C); \
                               WR_L; \
                               GPIOD->BSRR = D9_BSR_MASK(C); \
                               GPIOE->BSRR = D11_BSR_MASK(C) | D12_BSR_MASK(C) | D13_BSR_MASK(C) | D14_BSR_MASK(C); \
                               WR_STB

      #define tft_Write_32(C)    tft_Write_16((uint16_t)((C)>>16)); tft_Write_16((uint16_t)(C))

      #define tft_Write_32C(C,D) tft_Write_16((uint16_t)(C)); tft_Write_16((uint16_t)(D))

      #define tft_Write_32D(C)   tft_Write_16((uint16_t)(C)); tft_Write_16((uint16_t)(C))

       
      #define RD_TFT_D0 (((GPIOF->IDR)&(D0_PIN_MASK))>>( 3-0))  
      #define RD_TFT_D1 (((GPIOD->IDR)&(D1_PIN_MASK))>>(15-1))  
      #define RD_TFT_D2 (((GPIOG->IDR)&(D2_PIN_MASK))>>(14-2))  
      #define RD_TFT_D3 (((GPIOE->IDR)&(D3_PIN_MASK))>>(13-3))  
      #define RD_TFT_D4 (((GPIOE->IDR)&(D4_PIN_MASK))>>(14-4))  
      #define RD_TFT_D5 (((GPIOE->IDR)&(D5_PIN_MASK))>>(11-5))  
      #define RD_TFT_D6 (((GPIOE->IDR)&(D6_PIN_MASK))>>( 9-6))  
      #define RD_TFT_D7 (((GPIOG->IDR)&(D7_PIN_MASK))>>(12-7))  

    #endif
 
 
 
  #else
    #if defined (STM_PORTA_DATA_BUS) || defined (STM_PORTB_DATA_BUS) || defined (STM_PORTC_DATA_BUS) || defined (STM_PORTD_DATA_BUS)
      #if defined (STM_PORTA_DATA_BUS)
        #define GPIOX GPIOA
      #elif defined (STM_PORTB_DATA_BUS)
        #define GPIOX GPIOB
      #elif defined (STM_PORTC_DATA_BUS)
        #define GPIOX GPIOC
      #elif defined (STM_PORTD_DATA_BUS)
        #define GPIOX GPIOD
      #endif

       
      #define tft_Write_8(C)   GPIOX->BSRR = (0x00FF0000 | (uint8_t)(C)); WR_L; WR_STB

      #if defined (SSD1963_DRIVER)

         

        #define tft_Write_16(C)  r6 = (((C) & 0xF800)>> 8); g6 = (((C) & 0x07E0)>> 3); b6 = (((C) & 0x001F)<< 3); \
                             GPIOX->BSRR = (0x00FF0000 | (uint8_t)(r6)); WR_L; WR_STB; \
                             GPIOX->BSRR = (0x00FF0000 | (uint8_t)(g6)); WR_L; WR_STB; \
                             GPIOX->BSRR = (0x00FF0000 | (uint8_t)(b6)); WR_L; WR_STB

         
        #define tft_Write_16S(C) Cswap = ((C) >>8 | (C) << 8); tft_Write_16(Cswap)

      #else

           
          #define tft_Write_16(C)  GPIOX->BSRR = (0x00FF0000 | (uint8_t)(C>>8)); WR_L; WR_STB; \
                                   GPIOX->BSRR = (0x00FF0000 | (uint8_t)(C>>0)); WR_L; WR_STB

           
          #define tft_Write_16S(C) GPIOX->BSRR = (0x00FF0000 | (uint8_t)(C>>0)); WR_L; WR_STB; \
                                   GPIOX->BSRR = (0x00FF0000 | (uint8_t)(C>>8)); WR_L; WR_STB
      #endif

      #define tft_Write_32(C)    tft_Write_16((uint16_t)((C)>>16)); tft_Write_16((uint16_t)(C))

      #define tft_Write_32C(C,D) tft_Write_16((uint16_t)(C)); tft_Write_16((uint16_t)(D))

      #define tft_Write_32D(C)   tft_Write_16((uint16_t)(C)); tft_Write_16((uint16_t)(C))

       
      #define RD_TFT_D0 ((GPIOX->IDR) & 0x01)  
      #define RD_TFT_D1 ((GPIOX->IDR) & 0x02)  
      #define RD_TFT_D2 ((GPIOX->IDR) & 0x04)  
      #define RD_TFT_D3 ((GPIOX->IDR) & 0x08)  
      #define RD_TFT_D4 ((GPIOX->IDR) & 0x10)  
      #define RD_TFT_D5 ((GPIOX->IDR) & 0x20)  
      #define RD_TFT_D6 ((GPIOX->IDR) & 0x40)  
      #define RD_TFT_D7 ((GPIOX->IDR) & 0x80)  

    #else
       

       
      #define D0_PIN_NAME  digitalPinToPinName(TFT_D0)
      #define D1_PIN_NAME  digitalPinToPinName(TFT_D1)
      #define D2_PIN_NAME  digitalPinToPinName(TFT_D2)
      #define D3_PIN_NAME  digitalPinToPinName(TFT_D3)
      #define D4_PIN_NAME  digitalPinToPinName(TFT_D4)
      #define D5_PIN_NAME  digitalPinToPinName(TFT_D5)
      #define D6_PIN_NAME  digitalPinToPinName(TFT_D6)
      #define D7_PIN_NAME  digitalPinToPinName(TFT_D7)

       
      #define D0_PIN_BIT  (D0_PIN_NAME & 0xF)
      #define D1_PIN_BIT  (D1_PIN_NAME & 0xF)
      #define D2_PIN_BIT  (D2_PIN_NAME & 0xF)
      #define D3_PIN_BIT  (D3_PIN_NAME & 0xF)
      #define D4_PIN_BIT  (D4_PIN_NAME & 0xF)
      #define D5_PIN_BIT  (D5_PIN_NAME & 0xF)
      #define D6_PIN_BIT  (D6_PIN_NAME & 0xF)
      #define D7_PIN_BIT  (D7_PIN_NAME & 0xF)

       
      #define D0_PIN_PORT  digitalPinToPort(TFT_D0)
      #define D1_PIN_PORT  digitalPinToPort(TFT_D1)
      #define D2_PIN_PORT  digitalPinToPort(TFT_D2)
      #define D3_PIN_PORT  digitalPinToPort(TFT_D3)
      #define D4_PIN_PORT  digitalPinToPort(TFT_D4)
      #define D5_PIN_PORT  digitalPinToPort(TFT_D5)
      #define D6_PIN_PORT  digitalPinToPort(TFT_D6)
      #define D7_PIN_PORT  digitalPinToPort(TFT_D7)

       
      #define D0_PIN_MASK  digitalPinToBitMask(TFT_D0)
      #define D1_PIN_MASK  digitalPinToBitMask(TFT_D1)
      #define D2_PIN_MASK  digitalPinToBitMask(TFT_D2)
      #define D3_PIN_MASK  digitalPinToBitMask(TFT_D3)
      #define D4_PIN_MASK  digitalPinToBitMask(TFT_D4)
      #define D5_PIN_MASK  digitalPinToBitMask(TFT_D5)
      #define D6_PIN_MASK  digitalPinToBitMask(TFT_D6)
      #define D7_PIN_MASK  digitalPinToBitMask(TFT_D7)

       
      #define  D0_BSR_MASK(B) ((D0_PIN_MASK<<16)>>(((B)<< 4)&0x10))
      #define  D1_BSR_MASK(B) ((D1_PIN_MASK<<16)>>(((B)<< 3)&0x10))
      #define  D2_BSR_MASK(B) ((D2_PIN_MASK<<16)>>(((B)<< 2)&0x10))
      #define  D3_BSR_MASK(B) ((D3_PIN_MASK<<16)>>(((B)<< 1)&0x10))
      #define  D4_BSR_MASK(B) ((D4_PIN_MASK<<16)>>(((B)<< 0)&0x10))
      #define  D5_BSR_MASK(B) ((D5_PIN_MASK<<16)>>(((B)>> 1)&0x10))
      #define  D6_BSR_MASK(B) ((D6_PIN_MASK<<16)>>(((B)>> 2)&0x10))
      #define  D7_BSR_MASK(B) ((D7_PIN_MASK<<16)>>(((B)>> 3)&0x10))
       
      #define  D8_BSR_MASK(B) ((D0_PIN_MASK<<16)>>(((B)>> 4)&0x10))
      #define  D9_BSR_MASK(B) ((D1_PIN_MASK<<16)>>(((B)>> 5)&0x10))
      #define D10_BSR_MASK(B) ((D2_PIN_MASK<<16)>>(((B)>> 6)&0x10))
      #define D11_BSR_MASK(B) ((D3_PIN_MASK<<16)>>(((B)>> 7)&0x10))
      #define D12_BSR_MASK(B) ((D4_PIN_MASK<<16)>>(((B)>> 8)&0x10))
      #define D13_BSR_MASK(B) ((D5_PIN_MASK<<16)>>(((B)>> 9)&0x10))
      #define D14_BSR_MASK(B) ((D6_PIN_MASK<<16)>>(((B)>>10)&0x10))
      #define D15_BSR_MASK(B) ((D7_PIN_MASK<<16)>>(((B)>>11)&0x10))


       
      #define tft_Write_8(C)   D0_PIN_PORT->BSRR = D0_BSR_MASK(C); \
                               D1_PIN_PORT->BSRR = D1_BSR_MASK(C); \
                               D2_PIN_PORT->BSRR = D2_BSR_MASK(C); \
                               D3_PIN_PORT->BSRR = D3_BSR_MASK(C); \
                               WR_L; \
                               D4_PIN_PORT->BSRR = D4_BSR_MASK(C); \
                               D5_PIN_PORT->BSRR = D5_BSR_MASK(C); \
                               D6_PIN_PORT->BSRR = D6_BSR_MASK(C); \
                               D7_PIN_PORT->BSRR = D7_BSR_MASK(C); \
                               WR_STB

      #if defined (SSD1963_DRIVER)

         
        #define tft_Write_16(C)  r6 = (((C) & 0xF800)>> 8); g6 = (((C) & 0x07E0)>> 3); b6 = (((C) & 0x001F)<< 3); \
                                 D0_PIN_PORT->BSRR = D8_BSR_MASK(r6);  \
                                 D1_PIN_PORT->BSRR = D9_BSR_MASK(r6);  \
                                 D2_PIN_PORT->BSRR = D10_BSR_MASK(r6); \
                                 D3_PIN_PORT->BSRR = D11_BSR_MASK(r6); \
                                 WR_L; \
                                 D4_PIN_PORT->BSRR = D12_BSR_MASK(r6); \
                                 D5_PIN_PORT->BSRR = D13_BSR_MASK(r6); \
                                 D6_PIN_PORT->BSRR = D14_BSR_MASK(r6); \
                                 D7_PIN_PORT->BSRR = D15_BSR_MASK(r6); \
                                 WR_STB;\
                                 D0_PIN_PORT->BSRR = D8_BSR_MASK(g6);  \
                                 D1_PIN_PORT->BSRR = D9_BSR_MASK(g6);  \
                                 D2_PIN_PORT->BSRR = D10_BSR_MASK(g6); \
                                 D3_PIN_PORT->BSRR = D11_BSR_MASK(g6); \
                                 WR_L; \
                                 D4_PIN_PORT->BSRR = D12_BSR_MASK(g6); \
                                 D5_PIN_PORT->BSRR = D13_BSR_MASK(g6); \
                                 D6_PIN_PORT->BSRR = D14_BSR_MASK(g6); \
                                 D7_PIN_PORT->BSRR = D15_BSR_MASK(g6); \
                                 WR_STB;\
                                 D0_PIN_PORT->BSRR = D0_BSR_MASK(b6); \
                                 D1_PIN_PORT->BSRR = D1_BSR_MASK(b6); \
                                 D2_PIN_PORT->BSRR = D2_BSR_MASK(b6); \
                                 D3_PIN_PORT->BSRR = D3_BSR_MASK(b6); \
                                 WR_L; \
                                 D4_PIN_PORT->BSRR = D4_BSR_MASK(b6); \
                                 D5_PIN_PORT->BSRR = D5_BSR_MASK(b6); \
                                 D6_PIN_PORT->BSRR = D6_BSR_MASK(b6); \
                                 D7_PIN_PORT->BSRR = D7_BSR_MASK(b6); \
                                 WR_STB

         
        #define tft_Write_16S(C) Cswap = ((C) >>8 | (C) << 8); tft_Write_16(Cswap)

      #else

         
        #define tft_Write_16(C)  D0_PIN_PORT->BSRR = D8_BSR_MASK(C);  \
                                 D1_PIN_PORT->BSRR = D9_BSR_MASK(C);  \
                                 D2_PIN_PORT->BSRR = D10_BSR_MASK(C); \
                                 D3_PIN_PORT->BSRR = D11_BSR_MASK(C); \
                                 WR_L; \
                                 D4_PIN_PORT->BSRR = D12_BSR_MASK(C); \
                                 D5_PIN_PORT->BSRR = D13_BSR_MASK(C); \
                                 D6_PIN_PORT->BSRR = D14_BSR_MASK(C); \
                                 D7_PIN_PORT->BSRR = D15_BSR_MASK(C); \
                                 WR_STB;\
                                 D0_PIN_PORT->BSRR = D0_BSR_MASK(C); \
                                 D1_PIN_PORT->BSRR = D1_BSR_MASK(C); \
                                 D2_PIN_PORT->BSRR = D2_BSR_MASK(C); \
                                 D3_PIN_PORT->BSRR = D3_BSR_MASK(C); \
                                 WR_L; \
                                 D4_PIN_PORT->BSRR = D4_BSR_MASK(C); \
                                 D5_PIN_PORT->BSRR = D5_BSR_MASK(C); \
                                 D6_PIN_PORT->BSRR = D6_BSR_MASK(C); \
                                 D7_PIN_PORT->BSRR = D7_BSR_MASK(C); \
                                 WR_STB

         
        #define tft_Write_16S(C) D0_PIN_PORT->BSRR = D0_BSR_MASK(C); \
                                 D1_PIN_PORT->BSRR = D1_BSR_MASK(C); \
                                 D2_PIN_PORT->BSRR = D2_BSR_MASK(C); \
                                 D3_PIN_PORT->BSRR = D3_BSR_MASK(C); \
                                 WR_L; \
                                 D4_PIN_PORT->BSRR = D4_BSR_MASK(C); \
                                 D5_PIN_PORT->BSRR = D5_BSR_MASK(C); \
                                 D6_PIN_PORT->BSRR = D6_BSR_MASK(C); \
                                 D7_PIN_PORT->BSRR = D7_BSR_MASK(C); \
                                 WR_STB; \
                                 D0_PIN_PORT->BSRR = D8_BSR_MASK(C);  \
                                 D1_PIN_PORT->BSRR = D9_BSR_MASK(C);  \
                                 D2_PIN_PORT->BSRR = D10_BSR_MASK(C); \
                                 D3_PIN_PORT->BSRR = D11_BSR_MASK(C); \
                                 WR_L; \
                                 D4_PIN_PORT->BSRR = D12_BSR_MASK(C); \
                                 D5_PIN_PORT->BSRR = D13_BSR_MASK(C); \
                                 D6_PIN_PORT->BSRR = D14_BSR_MASK(C); \
                                 D7_PIN_PORT->BSRR = D15_BSR_MASK(C); \
                                 WR_STB
      #endif

      #define tft_Write_32(C)    tft_Write_16((uint16_t)((C)>>16)); tft_Write_16((uint16_t)(C))

      #define tft_Write_32C(C,D) tft_Write_16((uint16_t)(C)); tft_Write_16((uint16_t)(D))

      #define tft_Write_32D(C)   tft_Write_16((uint16_t)(C)); tft_Write_16((uint16_t)(C))

       
      #define RD_TFT_D0 ((((D0_PIN_PORT->IDR) >> (D0_PIN_BIT))&1)<<0)  
      #define RD_TFT_D1 ((((D1_PIN_PORT->IDR) >> (D1_PIN_BIT))&1)<<1)  
      #define RD_TFT_D2 ((((D2_PIN_PORT->IDR) >> (D2_PIN_BIT))&1)<<2)  
      #define RD_TFT_D3 ((((D3_PIN_PORT->IDR) >> (D3_PIN_BIT))&1)<<3)  
      #define RD_TFT_D4 ((((D4_PIN_PORT->IDR) >> (D4_PIN_BIT))&1)<<4)  
      #define RD_TFT_D5 ((((D5_PIN_PORT->IDR) >> (D5_PIN_BIT))&1)<<5)  
      #define RD_TFT_D6 ((((D6_PIN_PORT->IDR) >> (D6_PIN_BIT))&1)<<6)  
      #define RD_TFT_D7 ((((D7_PIN_PORT->IDR) >> (D7_PIN_BIT))&1)<<7)  
    #endif
  #endif
 
 
 
#elif  defined (SPI_18BIT_DRIVER)  

  #define SPI_TXE_CHECK  while(!__HAL_SPI_GET_FLAG(&spiHal, SPI_FLAG_TXE)){}
                          
  #define SPI_BUSY_CHECK SPI_TXE_CHECK; while( __HAL_SPI_GET_FLAG(&spiHal, SPI_FLAG_BSY)){}
  #define TX_FIFO        SPI_TXE_CHECK; *((__IO uint8_t *)&SPIX->DR)

   
  #define tft_Write_8(C)   TX_FIFO = (C); SPI_BUSY_CHECK
  #define tft_Write_16(C)  TX_FIFO = ((C) & 0xF800)>>8; TX_FIFO = ((C) & 0x07E0)>>3; TX_FIFO = ((C) & 0x001F)<<3; SPI_BUSY_CHECK
  #define tft_Write_16S(C) TX_FIFO = (C) & 0xF8; TX_FIFO = ((C) & 0xE000)>>11 | ((C) & 0x07)<<5; TX_FIFO = ((C) & 0x1F00)>>5; SPI_BUSY_CHECK
  #define tft_Write_16N(C) TX_FIFO = ((C) & 0xF800)>>8; TX_FIFO = ((C) & 0x07E0)>>3; TX_FIFO = ((C) & 0x001F)<<3

  #define tft_Write_32(C) \
    TX_FIFO = (C)>>24; TX_FIFO = (C)>>16; \
    TX_FIFO = (C)>>8; TX_FIFO = (C); SPI_BUSY_CHECK

  #define tft_Write_32C(C,D) \
    TX_FIFO = (C)>>8; TX_FIFO = (C); \
    TX_FIFO = (D)>>8; TX_FIFO = (D); SPI_BUSY_CHECK

  #define tft_Write_32D(C) \
    TX_FIFO = (C)>>8; TX_FIFO = (C); \
    TX_FIFO = (C)>>8; TX_FIFO = (C); SPI_BUSY_CHECK

 
 
 
#elif  defined (RPI_DISPLAY_TYPE)

  #define SPI_TXE_CHECK  while(!__HAL_SPI_GET_FLAG(&spiHal, SPI_FLAG_TXE)){}
                          
  #define SPI_BUSY_CHECK SPI_TXE_CHECK; while( __HAL_SPI_GET_FLAG(&spiHal, SPI_FLAG_BSY)){}
  #define TX_FIFO        SPI_TXE_CHECK; *((__IO uint8_t *)&SPIX->DR)

   
  #define tft_Write_8(C)   TX_FIFO = (0); TX_FIFO = (C); SPI_BUSY_CHECK
  #define tft_Write_16(C)  TX_FIFO = (C)>>8; TX_FIFO = (C); SPI_BUSY_CHECK
  #define tft_Write_16S(C) TX_FIFO = (C); TX_FIFO = (C)>>8; SPI_BUSY_CHECK
  #define tft_Write_16N(C) TX_FIFO = (C)>>8; TX_FIFO = (C)

  #define tft_Write_32(C) \
    TX_FIFO = (C)>>24; TX_FIFO = (C)>>16; \
    TX_FIFO = (C)>>8; TX_FIFO = (C); SPI_BUSY_CHECK

  #define tft_Write_32C(C,D) \
    TX_FIFO = (0); TX_FIFO = (C)>>8; TX_FIFO = (0); TX_FIFO = (C); \
    TX_FIFO = (0); TX_FIFO = (D)>>8; TX_FIFO = (0); TX_FIFO = (D); SPI_BUSY_CHECK

  #define tft_Write_32D(C) \
    TX_FIFO = (0); TX_FIFO = (C)>>8; TX_FIFO = (0); TX_FIFO = (C); \
    TX_FIFO = (0); TX_FIFO = (C)>>8; TX_FIFO = (0); TX_FIFO = (C); SPI_BUSY_CHECK

 
 
 

#else

   

  #define SPI_TXE_CHECK  while(!__HAL_SPI_GET_FLAG(&spiHal, SPI_FLAG_TXE)){}
                          
  #define SPI_BUSY_CHECK SPI_TXE_CHECK; while( __HAL_SPI_GET_FLAG(&spiHal, SPI_FLAG_BSY)){}
  #define TX_FIFO        SPI_TXE_CHECK; *((__IO uint8_t *)&SPIX->DR)

  #define tft_Write_8(C)   TX_FIFO = (C); SPI_BUSY_CHECK
  #define tft_Write_16(C)  TX_FIFO = (C)>>8; TX_FIFO = (C); SPI_BUSY_CHECK
  #define tft_Write_16S(C) TX_FIFO = (C); TX_FIFO = (C)>>8; SPI_BUSY_CHECK
  #define tft_Write_16N(C) TX_FIFO = (C)>>8; TX_FIFO = (C)

  #define tft_Write_32(C) \
    TX_FIFO = (C)>>24; TX_FIFO = (C)>>16; \
    tft_Write_16((uint16_t) ((C)>>0))

  #define tft_Write_32C(C,D) \
    TX_FIFO = (C)>>8; TX_FIFO = (C); \
    tft_Write_16((uint16_t) (D))

  #define tft_Write_32D(C) \
    TX_FIFO = (C)>>8; TX_FIFO = (C); \
    tft_Write_16((uint16_t) (C))

#endif

#ifndef DC_DELAY
   
  #define DC_DELAY
#endif

#ifndef tft_Write_16N
  #define tft_Write_16N tft_Write_16
#endif

 
 
 
#if defined (TFT_SDA_READ)
   
  #define TFT_eSPI_ENABLE_8_BIT_READ  
  #define SCLK_L digitalWrite(TFT_SCLK, LOW)
  #define SCLK_H digitalWrite(TFT_SCLK, HIGH)
#elif !defined (TFT_PARALLEL_8_BIT)
   
  #define tft_Read_8() spi.transfer(0)
#endif

#endif  
