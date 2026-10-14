













 
#ifndef _TFT_eSPIH_
#define _TFT_eSPIH_

#define TFT_ESPI_VERSION "2.5.43"

 
 
#define TFT_ESPI_FEATURES 1





 
#include <Arduino.h>
#include <Print.h>
#if !defined (TFT_PARALLEL_8_BIT) && !defined (RP2040_PIO_INTERFACE)
  #include <SPI.h>
#endif



 
 
#ifdef CONFIG_TFT_eSPI_ESPIDF
  #include "TFT_config.h"
#endif

 
 
#if defined (ESP8266)
  #ifndef ARDUINO_ARCH_ESP8266
    #define ARDUINO_ARCH_ESP8266
  #endif
#endif

 
 
#if !defined __has_include
  #if !defined(DISABLE_ALL_LIBRARY_WARNINGS)
    #warning Compiler does not support __has_include, so sketches cannot define the setup
  #endif
#else
  #if __has_include(<tft_setup.h>)
     
    #include <tft_setup.h>
    #ifndef USER_SETUP_LOADED
       
      #define USER_SETUP_LOADED
    #endif
  #endif
#endif

#include "User_Setup_Select.h"

 
#if defined(ARDUINO_ARCH_RP2040)
  #undef pgm_read_byte
  #define pgm_read_byte(addr)   (*(const unsigned char *)(addr))
  #undef pgm_read_word
  #define pgm_read_word(addr) ({ \
    typeof(addr) _addr = (addr); \
    *(const unsigned short *)(_addr); \
  })
  #undef pgm_read_dword
  #define pgm_read_dword(addr) ({ \
    typeof(addr) _addr = (addr); \
    *(const unsigned long *)(_addr); \
  })
#elif defined(__AVR__)
  #include <avr/pgmspace.h>
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ESP32)
  #include <pgmspace.h>
#else
  #ifndef PROGMEM
    #define PROGMEM
  #endif
#endif

 
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  #include "Processors/TFT_eSPI_ESP32_S3.h"
 
#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) 
  #include "Processors/TFT_eSPI_ESP32_C3.h"
#elif defined (ESP32)
  #include "Processors/TFT_eSPI_ESP32.h"
#elif defined (ARDUINO_ARCH_ESP8266)
  #include "Processors/TFT_eSPI_ESP8266.h"
#elif defined (STM32)
  #include "Processors/TFT_eSPI_STM32.h"
#elif defined(ARDUINO_ARCH_RP2040)
  #include "Processors/TFT_eSPI_RP2040.h"
#else
  #include "Processors/TFT_eSPI_Generic.h"
  #define GENERIC_PROCESSOR
#endif




#ifndef TAB_COLOUR
  #define TAB_COLOUR 0
#endif

 
#ifndef SPI_FREQUENCY
  #define SPI_FREQUENCY  20000000
#endif

 
#ifndef SPI_READ_FREQUENCY
  #define SPI_READ_FREQUENCY 10000000
#endif

 
#ifndef TFT_SPI_MODE
  #if defined(ST7789_DRIVER) || defined(ST7789_2_DRIVER)
    #define TFT_SPI_MODE SPI_MODE3
  #else
    #define TFT_SPI_MODE SPI_MODE0
  #endif
#endif

 
#ifndef SPI_TOUCH_FREQUENCY
  #define SPI_TOUCH_FREQUENCY  2500000
#endif

#ifndef SPI_BUSY_CHECK
  #define SPI_BUSY_CHECK
#endif

 
#ifdef TFT_SDA_READ
  #ifdef TFT_MISO
    #if TFT_MISO != -1
      #undef TFT_MISO
      #define TFT_MISO -1
      #warning TFT_MISO set to -1
    #endif
  #endif
#endif  




 
 
#ifdef SMOOTH_FONT
  #ifndef LOAD_GLCD
    #define LOAD_GLCD
  #endif
#endif

 
 
#ifdef LOAD_GLCD
  #include "Fonts/glcdfont.c.h"
#endif

#ifdef LOAD_FONT2
  #include "Fonts/Font16.h"
#endif

#ifdef LOAD_FONT4
  #include "Fonts/Font32rle.h"
  #define LOAD_RLE
#endif

#ifdef LOAD_FONT6
  #include "Fonts/Font64rle.h"
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#ifdef LOAD_FONT7
  #include "Fonts/Font7srle.h"
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#ifdef LOAD_FONT8
  #include "Fonts/Font72rle.h"
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#elif defined LOAD_FONT8N  
  #define LOAD_FONT8
  #include "Fonts/Font72x53rle.h"
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#ifdef LOAD_GFXFF
   
   
  #include "Fonts/GFXFF/gfxfont.h"
   
  #include <User_Setups/User_Custom_Fonts.h>
#endif  

 
const  uint8_t widtbl_null[1] = {0};
PROGMEM const uint8_t chr_null[1] = {0};
PROGMEM const uint8_t* const chrtbl_null[1] = {chr_null};

 
 
typedef struct {
    const uint8_t *chartbl;
    const uint8_t *widthtbl;
    uint8_t height;
    uint8_t baseline;
    } fontinfo;

 
const PROGMEM fontinfo fontdata [] = {
  #ifdef LOAD_GLCD
   { (const uint8_t *)font, widtbl_null, 0, 0 },
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif
    
   { (const uint8_t *)chrtbl_null, widtbl_null, 8, 7 },

  #ifdef LOAD_FONT2
   { (const uint8_t *)chrtbl_f16, widtbl_f16, chr_hgt_f16, baseline_f16},
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

    
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },

  #ifdef LOAD_FONT4
   { (const uint8_t *)chrtbl_f32, widtbl_f32, chr_hgt_f32, baseline_f32},
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

    
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },

  #ifdef LOAD_FONT6
   { (const uint8_t *)chrtbl_f64, widtbl_f64, chr_hgt_f64, baseline_f64},
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

  #ifdef LOAD_FONT7
   { (const uint8_t *)chrtbl_f7s, widtbl_f7s, chr_hgt_f7s, baseline_f7s},
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

  #ifdef LOAD_FONT8
   { (const uint8_t *)chrtbl_f72, widtbl_f72, chr_hgt_f72, baseline_f72}
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 }
  #endif
};




 
#define TL_DATUM 0  
#define TC_DATUM 1  
#define TR_DATUM 2  
#define ML_DATUM 3  
#define CL_DATUM 3  
#define MC_DATUM 4  
#define CC_DATUM 4  
#define MR_DATUM 5  
#define CR_DATUM 5  
#define BL_DATUM 6  
#define BC_DATUM 7  
#define BR_DATUM 8  
#define L_BASELINE  9  
#define C_BASELINE 10  
#define R_BASELINE 11  




 
#define TFT_BLACK       0x0000       
#define TFT_NAVY        0x000F       
#define TFT_DARKGREEN   0x03E0       
#define TFT_DARKCYAN    0x03EF       
#define TFT_MAROON      0x7800       
#define TFT_PURPLE      0x780F       
#define TFT_OLIVE       0x7BE0       
#define TFT_LIGHTGREY   0xD69A       
#define TFT_DARKGREY    0x7BEF       
#define TFT_BLUE        0x001F       
#define TFT_GREEN       0x07E0       
#define TFT_CYAN        0x07FF       
#define TFT_RED         0xF800       
#define TFT_MAGENTA     0xF81F       
#define TFT_YELLOW      0xFFE0       
#define TFT_WHITE       0xFFFF       
#define TFT_ORANGE      0xFDA0       
#define TFT_GREENYELLOW 0xB7E0       
#define TFT_PINK        0xFE19         
#define TFT_BROWN       0x9A60       
#define TFT_GOLD        0xFEA0       
#define TFT_SILVER      0xC618       
#define TFT_SKYBLUE     0x867D       
#define TFT_VIOLET      0x915C       

 
 
 
#define TFT_TRANSPARENT 0x0120  

 
static const uint16_t default_4bit_palette[] PROGMEM = {
  TFT_BLACK,     
  TFT_BROWN,     
  TFT_RED,       
  TFT_ORANGE,    
  TFT_YELLOW,    
  TFT_GREEN,     
  TFT_BLUE,      
  TFT_PURPLE,    
  TFT_DARKGREY,  
  TFT_WHITE,     
  TFT_CYAN,      
  TFT_MAGENTA,   
  TFT_MAROON,    
  TFT_DARKGREEN, 
  TFT_NAVY,      
  TFT_PINK       
};




 
 

 
 
typedef struct
{
String  version = TFT_ESPI_VERSION;
String  setup_info;   
uint32_t setup_id;    
int32_t esp;          
uint8_t trans;        
uint8_t serial;       
#ifndef GENERIC_PROCESSOR
uint8_t  port;        
#endif
uint8_t overlap;      
uint8_t interface;    

uint16_t tft_driver;  
uint16_t tft_width;   
uint16_t tft_height;

uint8_t r0_x_offset;  
uint8_t r0_y_offset;
uint8_t r1_x_offset;
uint8_t r1_y_offset;
uint8_t r2_x_offset;
uint8_t r2_y_offset;
uint8_t r3_x_offset;
uint8_t r3_y_offset;

int8_t pin_tft_mosi;  
int8_t pin_tft_miso;
int8_t pin_tft_clk;
int8_t pin_tft_cs;

int8_t pin_tft_dc;    
int8_t pin_tft_rd;
int8_t pin_tft_wr;
int8_t pin_tft_rst;

int8_t pin_tft_d0;    
int8_t pin_tft_d1;
int8_t pin_tft_d2;
int8_t pin_tft_d3;
int8_t pin_tft_d4;
int8_t pin_tft_d5;
int8_t pin_tft_d6;
int8_t pin_tft_d7;

int8_t pin_tft_led;
int8_t pin_tft_led_on;

int8_t pin_tch_cs;    

int16_t tft_spi_freq; 
int16_t tft_rd_freq;  
int16_t tch_spi_freq; 
} setup_t;





 
typedef uint16_t (*getColorCallback)(uint16_t x, uint16_t y);

 
class TFT_eSPI : public Print { friend class TFT_eSprite;  

  
 public:

  TFT_eSPI(int16_t _W = TFT_WIDTH, int16_t _H = TFT_HEIGHT);

   
   
  void     init(uint8_t tc = TAB_COLOUR), begin(uint8_t tc = TAB_COLOUR);

   
  virtual void     drawPixel(int32_t x, int32_t y, uint32_t color),
                   drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size),
                   drawLine(int32_t xs, int32_t ys, int32_t xe, int32_t ye, uint32_t color),
                   drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color),
                   drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color),
                   fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

  virtual int16_t  drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font),
                   drawChar(uint16_t uniCode, int32_t x, int32_t y),
                   height(void),
                   width(void);

                    
  virtual uint16_t readPixel(int32_t x, int32_t y);

  virtual void     setWindow(int32_t xs, int32_t ys, int32_t xe, int32_t ye);    

                    
  virtual void     pushColor(uint16_t color);

                    
  virtual void     begin_nin_write();
  virtual void     end_nin_write();

  void     setRotation(uint8_t r);  
  uint8_t  getRotation(void);       

   
   
  void     setOrigin(int32_t x, int32_t y);
  int32_t  getOriginX(void);
  int32_t  getOriginY(void);

  void     invertDisplay(bool i);   


   
  void     setAddrWindow(int32_t xs, int32_t ys, int32_t w, int32_t h);  

   
  void     setViewport(int32_t x, int32_t y, int32_t w, int32_t h, bool vpDatum = true);
  bool     checkViewport(int32_t x, int32_t y, int32_t w, int32_t h);
  int32_t  getViewportX(void);
  int32_t  getViewportY(void);
  int32_t  getViewportWidth(void);
  int32_t  getViewportHeight(void);
  bool     getViewportDatum(void);
  void     frameViewport(uint16_t color, int32_t w);
  void     resetViewport(void);

            
  bool     clipAddrWindow(int32_t* x, int32_t* y, int32_t* w, int32_t* h);
            
  bool     clipWindow(int32_t* xs, int32_t* ys, int32_t* xe, int32_t* ye);

            
  void     pushColor(uint16_t color, uint32_t len),   
           pushColors(uint16_t  *data, uint32_t len, bool swap = true),  
           pushColors(uint8_t  *data, uint32_t len);  

            
  void     pushBlock(uint16_t color, uint32_t len);

            
  void     pushPixels(const void * data_in, uint32_t len);

            
           #ifdef TFT_SDA_READ
             #if defined (TFT_eSPI_ENABLE_8_BIT_READ)
  uint8_t  tft_Read_8(void);      
             #endif
  void     begin_SDA_Read(void);  
  void     end_SDA_Read(void);    
           #endif


   
  void     fillScreen(uint32_t color),
           drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color),
           drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color),
           fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color);

  void     fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
  void     fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);

  void     drawCircle(int32_t x, int32_t y, int32_t r, uint32_t color),
           drawCircleHelper(int32_t x, int32_t y, int32_t r, uint8_t cornername, uint32_t color),
           fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color),
           fillCircleHelper(int32_t x, int32_t y, int32_t r, uint8_t cornername, int32_t delta, uint32_t color),

           drawEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color),
           fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color),

            
           drawTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color),
           fillTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color);


   
            
            
  uint16_t drawPixel(int32_t x, int32_t y, uint32_t color, uint8_t alpha, uint32_t bg_color = 0x00FFFFFF);

            
            
            
            
  void     drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle, uint32_t fg_color, uint32_t bg_color, bool roundEnds = false);

            
            
            
  void     drawArc(int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle, uint32_t fg_color, uint32_t bg_color, bool smoothArc = true);

            
            
            
  void     drawSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t fg_color, uint32_t bg_color);
  
            
            
  void     fillSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t color, uint32_t bg_color = 0x00FFFFFF);

            
            
            
  void     drawSmoothRoundRect(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t w, int32_t h, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF, uint8_t quadrants = 0xF);

            
  void     fillSmoothRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color, uint32_t bg_color = 0x00FFFFFF);

            
            
  void     drawSpot(float ax, float ay, float r, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF);

            
            
  void     drawWideLine(float ax, float ay, float bx, float by, float wd, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF);

            
            
  void     drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF);


   
            
  void     setSwapBytes(bool swap);
  bool     getSwapBytes(void);

            
  void     drawBitmap( int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor),
           drawBitmap( int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor, uint16_t bgcolor),
           drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor),
           drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor, uint16_t bgcolor),
           setBitmapColor(uint16_t fgcolor, uint16_t bgcolor);  

            
  void     setPivot(int16_t x, int16_t y);
  int16_t  getPivotX(void),  
           getPivotY(void);  

            
            
  void     readRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);
            
  void     pushRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);

            
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint16_t transparent);

            
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data, uint16_t transparent);
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data);

            
            
            
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t  *data, bool bpp8 = true, uint16_t *cmap = nullptr);
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t  *data, uint8_t  transparent, bool bpp8 = true, uint16_t *cmap = nullptr);
            
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8,  uint16_t *cmap = nullptr);

            
  void     pushMaskedImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *img, uint8_t *mask);

            
            
            
  void     readRectRGB(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data);


   
  int16_t  drawNumber(long intNumber, int32_t x, int32_t y, uint8_t font),  
           drawNumber(long intNumber, int32_t x, int32_t y),                

            
            
           drawFloat(float floatNumber, uint8_t decimal, int32_t x, int32_t y, uint8_t font),  
           drawFloat(float floatNumber, uint8_t decimal, int32_t x, int32_t y),                

            
            
           drawString(const char *string, int32_t x, int32_t y, uint8_t font),   
           drawString(const char *string, int32_t x, int32_t y),                 
           drawString(const String& string, int32_t x, int32_t y, uint8_t font), 
           drawString(const String& string, int32_t x, int32_t y),               

           drawCentreString(const char *string, int32_t x, int32_t y, uint8_t font),   
           drawRightString(const char *string, int32_t x, int32_t y, uint8_t font),    
           drawCentreString(const String& string, int32_t x, int32_t y, uint8_t font), 
           drawRightString(const String& string, int32_t x, int32_t y, uint8_t font);  


   
  void     setCursor(int16_t x, int16_t y),                  
           setCursor(int16_t x, int16_t y, uint8_t font);    

  int16_t  getCursorX(void),                                 
           getCursorY(void);                                 

  void     setTextColor(uint16_t color),                     
           setTextColor(uint16_t fgcolor, uint16_t bgcolor, bool bgfill = false),   
           setTextSize(uint8_t size);                        

  void     setTextWrap(bool wrapX, bool wrapY = false);      

  void     setTextDatum(uint8_t datum);                      
  uint8_t  getTextDatum(void);

  void     setTextPadding(uint16_t x_width);                 
  uint16_t getTextPadding(void);                             

#ifdef LOAD_GFXFF
  void     setFreeFont(const GFXfont *f = NULL),             
           setTextFont(uint8_t font);                        
#else
  void     setFreeFont(uint8_t font),                        
           setTextFont(uint8_t font);                        
#endif

  int16_t  textWidth(const char *string, uint8_t font),      
           textWidth(const char *string),                    
           textWidth(const String& string, uint8_t font),    
           textWidth(const String& string),
           fontHeight(uint8_t font),                         
           fontHeight(void);                                 

            
  uint16_t decodeUTF8(uint8_t *buf, uint16_t *index, uint16_t remaining),
           decodeUTF8(uint8_t c);

            
  size_t   write(uint8_t);
            

            
  void     setCallback(getColorCallback getCol);

  uint16_t fontsLoaded(void);  


   
  void     spiwrite(uint8_t);         
#ifdef RM68120_DRIVER
  void     writecommand(uint16_t c);                  
  void     writeRegister8(uint16_t c, uint8_t d);     
  void     writeRegister16(uint16_t c, uint16_t d);   
#else
  void     writecommand(uint8_t c);   
#endif
  void     writedata(uint8_t d);      

  void     commandList(const uint8_t *addr);  

  uint8_t  readcommand8( uint8_t cmd_function, uint8_t index = 0);  
  uint16_t readcommand16(uint8_t cmd_function, uint8_t index = 0);  
  uint32_t readcommand32(uint8_t cmd_function, uint8_t index = 0);  


   
            
  uint16_t color565(uint8_t red, uint8_t green, uint8_t blue);

            
  uint16_t color8to16(uint8_t color332);
            
  uint8_t  color16to8(uint16_t color565);

            
  uint32_t color16to24(uint16_t color565);
  uint32_t color24to16(uint32_t color888);

            
            
            
  uint16_t alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc);

            
  uint16_t alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc, uint8_t dither);
            
  uint32_t alphaBlend24(uint8_t alpha, uint32_t fgc, uint32_t bgc, uint8_t dither = 0);

   
   
   
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

  bool     initDMA(bool ctrl_cs = false);   
                                            
                                            
  void     deInitDMA(void);    

            
            
            
            
            
            
            
            
            
            
            
  void     pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t* data, uint16_t* buffer = nullptr);

#if defined (ESP32)  
            
  void     pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t const* data);
#endif
            
  void     pushPixelsDMA(uint16_t* image, uint32_t len);

            
  bool     dmaBusy(void);  
  void     dmaWait(void);  

  bool     DMA_Enabled = false;    
  uint8_t  spiBusyCheck = 0;       

   
  void     startWrite(void);                          
  void     writeColor(uint16_t color, uint32_t len);  
  void     endWrite(void);                            

   
   
   
   
   
   
           #define CP437_SWITCH 1
           #define UTF8_SWITCH  2
           #define PSRAM_ENABLE 3
  void     setAttribute(uint8_t id = 0, uint8_t a = 0);  
  uint8_t  getAttribute(uint8_t id = 0);                 

            
  void     getSetup(setup_t& tft_settings);  
  bool     verifySetupID(uint32_t id);

   
#if !defined (TFT_PARALLEL_8_BIT) && !defined (RP2040_PIO_INTERFACE)
  static   SPIClass& getSPIinstance(void);  
#endif
  uint32_t textcolor, textbgcolor;          

  uint32_t bitmap_fg, bitmap_bg;            

  uint8_t  textfont,   
           textsize,   
           textdatum,  
           rotation;   

  uint8_t  decoderState = 0;    
  uint16_t decoderBuffer;       

  
 private:
            
  void     spi_begin();
  void     spi_end();

  void     spi_begin_read();
  void     spi_end_read();

            
            
            
  inline void begin_tft_write() __attribute__((always_inline));
  inline void end_tft_write()   __attribute__((always_inline));

            
            
  inline void begin_tft_read()  __attribute__((always_inline));
  inline void end_tft_read()    __attribute__((always_inline));

            
  void     initBus(void);

            
  void     pushSwapBytePixels(const void* data_in, uint32_t len);

            
  void     readAddrWindow(int32_t xs, int32_t ys, int32_t w, int32_t h);

            
  uint8_t  readByte(void);

            
  void     busDir(uint32_t mask, uint8_t mode);

            
  void     gpioMode(uint8_t gpio, uint8_t mode);

            
  uint8_t  sqrt_fraction(uint32_t num);

            
  float    wedgeLineDistance(float pax, float pay, float bax, float bay, float dr);

            
  uint8_t  tabcolor,                    
           colstart = 0, rowstart = 0;  

            
  volatile uint32_t *dcport, *csport;
  uint32_t cspinmask, dcpinmask, wrpinmask, sclkpinmask;

           #if defined(ESP32_PARALLEL)
            
  uint32_t xclr_mask, xdir_mask;  

            
  uint32_t xset_mask[256];  
                            
           #endif

   

  getColorCallback getColor = nullptr;  

  bool     locked, inTransaction, lockTransaction;  

  
 protected:

   

  int32_t  _init_width, _init_height;  
  int32_t  _width, _height;            
  int32_t  addr_row, addr_col;         

  int16_t  _xPivot;    
  int16_t  _yPivot;    

   
  int32_t  _vpX, _vpY, _vpW, _vpH;     
  int32_t  _xDatum;
  int32_t  _yDatum;
  int32_t  _xWidth;
  int32_t  _yHeight;
  bool     _vpDatum;
  bool     _vpOoB;

  int32_t  cursor_x, cursor_y, padX;        
  int32_t  bg_cursor_x;                     
  int32_t  last_cursor_x;                   

  uint32_t fontsloaded;                

  uint8_t  glyph_ab,    
           glyph_bb;    

  bool     isDigits;    
  bool     textwrapX, textwrapY;   
  bool     _swapBytes;  

  bool     _booted;     

                        
  bool     _cp437;         
  bool     _utf8;          
  bool     _psram_enable;  

  uint32_t _lastColor;  

  bool     _fillbg;     

#if defined (SSD1963_DRIVER)
  uint16_t Cswap;       
  uint8_t r6, g6, b6;   
#endif

#ifdef LOAD_GFXFF
  GFXfont  *gfxFont;
#endif




 
#ifdef TOUCH_CS
  #if defined (TFT_PARALLEL_8_BIT) || defined (RP2040_PIO_INTERFACE)
    #if !defined(DISABLE_ALL_LIBRARY_WARNINGS)
      #error >>>>------>> Touch functions not supported in 8/16-bit parallel mode or with RP2040 PIO.
    #endif
  #else
    #include "Extensions/Touch.h"         
  #endif
#else
    #if !defined(DISABLE_ALL_LIBRARY_WARNINGS)
      #warning >>>>------>> TOUCH_CS pin not defined, TFT_eSPI touch functions will not be available!
    #endif
#endif

 
#ifdef SMOOTH_FONT
  #include "Extensions/Smooth_font.h"   
#endif

};  

 
template <typename T> static inline void
transpose(T& a, T& b) { T t = a; a = b; b = t; }

 
template <typename A, typename F, typename B> static inline uint16_t
fastBlend(A alpha, F fgc, B bgc)
{
   
  uint32_t rxb = bgc & 0xF81F;
  rxb += ((fgc & 0xF81F) - rxb) * (alpha >> 2) >> 6;
   
  uint32_t xgx = bgc & 0x07E0;
  xgx += ((fgc & 0x07E0) - xgx) * alpha >> 8;
   
  return (rxb & 0xF81F) | (xgx & 0x07E0);
}




 
#include "Extensions/Button.h"

 
#include "Extensions/Sprite.h"

#endif  