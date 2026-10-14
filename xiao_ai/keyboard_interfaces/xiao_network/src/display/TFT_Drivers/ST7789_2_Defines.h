 
 
#ifndef TFT_WIDTH
  #define TFT_WIDTH  240
#endif
#ifndef TFT_HEIGHT
  #define TFT_HEIGHT 320
#endif

#if (TFT_HEIGHT == 240) && (TFT_WIDTH == 240)
  #ifndef CGRAM_OFFSET
    #define CGRAM_OFFSET
  #endif
#endif

 
#if (TFT_HEIGHT == 240) && (TFT_WIDTH == 135)
  #ifndef CGRAM_OFFSET
    #define CGRAM_OFFSET
  #endif
#endif

 
#if (TFT_HEIGHT == 280) && (TFT_WIDTH == 240)
  #ifndef CGRAM_OFFSET
    #define CGRAM_OFFSET
  #endif
#endif

 
#if (TFT_HEIGHT == 320) && (TFT_WIDTH == 172)
  #ifndef CGRAM_OFFSET
    #define CGRAM_OFFSET
  #endif
#endif

#if (TFT_HEIGHT == 320) && (TFT_WIDTH == 170)
  #ifndef CGRAM_OFFSET
    #define CGRAM_OFFSET
  #endif
#endif

#if (TFT_HEIGHT == 300) && (TFT_WIDTH == 240)
  #ifndef CGRAM_OFFSET
    #define CGRAM_OFFSET
  #endif
#endif

 
#define TFT_INIT_DELAY 0x80  


 
#define TFT_NOP     0x00
#define TFT_SWRST   0x01

#define TFT_SLPIN   0x10
#define TFT_SLPOUT  0x11
#define TFT_NORON   0x13

#define TFT_INVOFF  0x20
#define TFT_INVON   0x21
#define TFT_DISPOFF 0x28
#define TFT_DISPON  0x29
#define TFT_CASET   0x2A
#define TFT_PASET   0x2B
#define TFT_RAMWR   0x2C
#define TFT_RAMRD   0x2E
#define TFT_MADCTL  0x36
#define TFT_COLMOD  0x3A

 
#define TFT_MAD_MY  0x80
#define TFT_MAD_MX  0x40
#define TFT_MAD_MV  0x20
#define TFT_MAD_ML  0x10
#define TFT_MAD_RGB 0x00
#define TFT_MAD_BGR 0x08
#define TFT_MAD_MH  0x04
#define TFT_MAD_SS  0x02
#define TFT_MAD_GS  0x01

#ifdef TFT_RGB_ORDER
  #if (TFT_RGB_ORDER == 1)
    #define TFT_MAD_COLOR_ORDER TFT_MAD_RGB
  #else
    #define TFT_MAD_COLOR_ORDER TFT_MAD_BGR
  #endif
#else
  #ifdef CGRAM_OFFSET
    #define TFT_MAD_COLOR_ORDER TFT_MAD_BGR
  #else
    #define TFT_MAD_COLOR_ORDER TFT_MAD_RGB
  #endif
#endif

#define TFT_IDXRD   0x00  

#define ST_CMD_DELAY   0x80     
#define ST7789_240x240_XSTART 0
#define ST7789_240x240_YSTART 0

 
#define ST7789_NOP			0x00
#define ST7789_SWRESET		0x01
#define ST7789_RDDID		0x04
#define ST7789_RDDST		0x09

#define ST7789_RDDPM		0x0A       
#define ST7789_RDD_MADCTL	0x0B       
#define ST7789_RDD_COLMOD	0x0C       
#define ST7789_RDDIM		0x0D       
#define ST7789_RDDSM		0x0E       
#define ST7789_RDDSR		0x0F       

#define ST7789_SLPIN		0x10
#define ST7789_SLPOUT		0x11
#define ST7789_PTLON		0x12
#define ST7789_NORON		0x13

#define ST7789_INVOFF		0x20
#define ST7789_INVON		0x21
#define ST7789_GAMSET		0x26       
#define ST7789_DISPOFF		0x28
#define ST7789_DISPON		0x29
#define ST7789_CASET		0x2A
#define ST7789_RASET		0x2B
#define ST7789_RAMWR		0x2C
#define ST7789_RGBSET		0x2D       
#define ST7789_RAMRD		0x2E

#define ST7789_PTLAR		0x30
#define ST7789_VSCRDEF		0x33       
#define ST7789_TEOFF		0x34       
#define ST7789_TEON			0x35       
#define ST7789_MADCTL		0x36       
#define ST7789_IDMOFF		0x38       
#define ST7789_IDMON		0x39       
#define ST7789_RAMWRC		0x3C       
#define ST7789_RAMRDC		0x3E       
#define ST7789_COLMOD		0x3A

#define ST7789_RAMCTRL		0xB0       
#define ST7789_RGBCTRL		0xB1       
#define ST7789_PORCTRL		0xB2       
#define ST7789_FRCTRL1		0xB3       
#define ST7789_PARCTRL		0xB5       
#define ST7789_GCTRL		0xB7       
#define ST7789_GTADJ		0xB8       
#define ST7789_DGMEN		0xBA       
#define ST7789_VCOMS		0xBB       
#define ST7789_LCMCTRL		0xC0       
#define ST7789_IDSET		0xC1       
#define ST7789_VDVVRHEN		0xC2       
#define ST7789_VRHS			0xC3       
#define ST7789_VDVSET		0xC4       
#define ST7789_VCMOFSET		0xC5       
#define ST7789_FRCTR2		0xC6       
#define ST7789_CABCCTRL		0xC7       
#define ST7789_REGSEL1		0xC8       
#define ST7789_REGSEL2		0xCA       
#define ST7789_PWMFRSEL		0xCC       
#define ST7789_PWCTRL1		0xD0       
#define ST7789_VAPVANEN		0xD2       
#define ST7789_CMD2EN		0xDF       
#define ST7789_PVGAMCTRL	0xE0       
#define ST7789_NVGAMCTRL	0xE1       
#define ST7789_DGMLUTR		0xE2       
#define ST7789_DGMLUTB		0xE3       
#define ST7789_GATECTRL		0xE4       
#define ST7789_SPI2EN		0xE7       
#define ST7789_PWCTRL2		0xE8       
#define ST7789_EQCTRL		0xE9       
#define ST7789_PROMCTRL		0xEC       
#define ST7789_PROMEN		0xFA       
#define ST7789_NVMSET		0xFC       
#define ST7789_PROMACT		0xFE       

