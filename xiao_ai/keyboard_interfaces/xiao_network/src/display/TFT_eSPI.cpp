














#include "TFT_eSPI.h"

#if defined (ESP32)
  #if defined(CONFIG_IDF_TARGET_ESP32S3)
    #include "Processors/TFT_eSPI_ESP32_S3.c.h"  
  #elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) 
    #include "Processors/TFT_eSPI_ESP32_C3.c.h"  
  #else
    #include "Processors/TFT_eSPI_ESP32.c.h"
  #endif
#elif defined (ARDUINO_ARCH_ESP8266)
  #include "Processors/TFT_eSPI_ESP8266.c.h"
#elif defined (STM32)  
  #include "Processors/TFT_eSPI_STM32.c.h"
#elif defined (ARDUINO_ARCH_RP2040)  || defined (ARDUINO_ARCH_MBED)  
  #include "Processors/TFT_eSPI_RP2040.c.h"
#else
  #include "Processors/TFT_eSPI_Generic.c.h"
#endif

#ifndef SPI_BUSY_CHECK
  #define SPI_BUSY_CHECK
#endif

 
#define PI_CLIP                                        \
  if (_vpOoB) return;                                  \
  x+= _xDatum;                                         \
  y+= _yDatum;                                         \
                                                       \
  if ((x >= _vpW) || (y >= _vpH)) return;              \
                                                       \
  int32_t dx = 0;                                      \
  int32_t dy = 0;                                      \
  int32_t dw = w;                                      \
  int32_t dh = h;                                      \
                                                       \
  if (x < _vpX) { dx = _vpX - x; dw -= dx; x = _vpX; } \
  if (y < _vpY) { dy = _vpY - y; dh -= dy; y = _vpY; } \
                                                       \
  if ((x + dw) > _vpW ) dw = _vpW - x;                 \
  if ((y + dh) > _vpH ) dh = _vpH - y;                 \
                                                       \
  if (dw < 1 || dh < 1) return;





  void TFT_eSPI::spi_begin()       {begin_tft_write();}
  void TFT_eSPI::spi_end()         {  end_tft_write();}
  void TFT_eSPI::spi_begin_read()  {begin_tft_read(); }
  void TFT_eSPI::spi_end_read()    {  end_tft_read(); }





inline void TFT_eSPI::begin_tft_write(void){
  if (locked) {
    locked = false;  
#if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS) && !defined(TFT_PARALLEL_8_BIT) && !defined(RP2040_PIO_INTERFACE)
    spi.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
#endif
    CS_L;
    SET_BUS_WRITE_MODE;   
  }
}

 
void TFT_eSPI::begin_nin_write(void){
  if (locked) {
    locked = false;  
#if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS) && !defined(TFT_PARALLEL_8_BIT) && !defined(RP2040_PIO_INTERFACE)
    spi.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
#endif
    CS_L;
    SET_BUS_WRITE_MODE;   
  }
}





inline void TFT_eSPI::end_tft_write(void){
  if(!inTransaction) {       
    if (!locked) {           
      locked = true;         
      SPI_BUSY_CHECK;        
      CS_H;
      SET_BUS_READ_MODE;     
#if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS) && !defined(TFT_PARALLEL_8_BIT) && !defined(RP2040_PIO_INTERFACE)
      spi.endTransaction();
#endif
    }
  }
}

 
inline void TFT_eSPI::end_nin_write(void){
  if(!inTransaction) {       
    if (!locked) {           
      locked = true;         
      SPI_BUSY_CHECK;        
      CS_H;
      SET_BUS_READ_MODE;     
#if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS) && !defined(TFT_PARALLEL_8_BIT) && !defined(RP2040_PIO_INTERFACE)
      spi.endTransaction();
#endif
    }
  }
}





 
inline void TFT_eSPI::begin_tft_read(void){
  DMA_BUSY_CHECK;  
#if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS) && !defined(TFT_PARALLEL_8_BIT) && !defined(RP2040_PIO_INTERFACE)
  if (locked) {
    locked = false;
    spi.beginTransaction(SPISettings(SPI_READ_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
    CS_L;
  }
#else
  #if !defined(TFT_PARALLEL_8_BIT) && !defined(RP2040_PIO_INTERFACE)
    spi.setFrequency(SPI_READ_FREQUENCY);
  #endif
   CS_L;
#endif
  SET_BUS_READ_MODE;
}





inline void TFT_eSPI::end_tft_read(void){
#if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS) && !defined(TFT_PARALLEL_8_BIT) && !defined(RP2040_PIO_INTERFACE)
  if(!inTransaction) {
    if (!locked) {
      locked = true;
      CS_H;
      spi.endTransaction();
    }
  }
#else
  #if !defined(TFT_PARALLEL_8_BIT) && !defined(RP2040_PIO_INTERFACE)
    spi.setFrequency(SPI_FREQUENCY);
  #endif
   if(!inTransaction) {CS_H;}
#endif
  SET_BUS_WRITE_MODE;
}





void TFT_eSPI::setViewport(int32_t x, int32_t y, int32_t w, int32_t h, bool vpDatum)
{
   
  _xDatum  = x;  
  _yDatum  = y;  
  _xWidth  = w;  
  _yHeight = h;  

   
  _vpDatum = false;  
  _vpOoB   = false;  
  _vpX = 0;          
  _vpY = 0;          
  _vpW = width();    
  _vpH = height();   

   
  if (x<0) { w += x; x = 0; }
  if (y<0) { h += y; y = 0; }
  if ((x + w) > width() ) { w = width()  - x; }
  if ((y + h) > height() ) { h = height() - y; }

   
   

   
  if (w < 1 || h < 1)
  {
     
    _xDatum = 0;
    _yDatum = 0;
    _xWidth  = width();
    _yHeight = height();
    _vpOoB = true;       
    return;
  }

  if (!vpDatum)
  {
    _xDatum = 0;  
    _yDatum = 0;
    _xWidth  = width();
    _yHeight = height();
  }

   
  _vpX = x;
  _vpY = y;
  _vpW = x + w;
  _vpH = y + h;
  _vpDatum = vpDatum;

   
   

   
   
}





 
bool TFT_eSPI::checkViewport(int32_t x, int32_t y, int32_t w, int32_t h)
{
  if (_vpOoB) return false;
  x+= _xDatum;
  y+= _yDatum;

  if ((x >= _vpW) || (y >= _vpH)) return false;

  int32_t dx = 0;
  int32_t dy = 0;
  int32_t dw = w;
  int32_t dh = h;

  if (x < _vpX) { dx = _vpX - x; dw -= dx; x = _vpX; }
  if (y < _vpY) { dy = _vpY - y; dh -= dy; y = _vpY; }

  if ((x + dw) > _vpW ) dw = _vpW - x;
  if ((y + dh) > _vpH ) dh = _vpH - y;

  if (dw < 1 || dh < 1) return false;

  return true;
}





void TFT_eSPI::resetViewport(void)
{
   
  _vpDatum = false;
  _vpOoB   = false;
  _xDatum = 0;
  _yDatum = 0;
  _vpX = 0;
  _vpY = 0;
  _vpW = width();
  _vpH = height();
  _xWidth  = width();
  _yHeight = height();
}





int32_t  TFT_eSPI::getViewportX(void)
{
  return _xDatum;
}





int32_t  TFT_eSPI::getViewportY(void)
{
  return _yDatum;
}





int32_t TFT_eSPI::getViewportWidth(void)
{
  return _xWidth;
}





int32_t TFT_eSPI::getViewportHeight(void)
{
  return _yHeight;
}





bool  TFT_eSPI::getViewportDatum(void)
{
  return _vpDatum;
}





void TFT_eSPI::frameViewport(uint16_t color, int32_t w)
{
   
  bool _dT = _vpDatum;

   
   
  if (w>0)
  {
     
    _vpDatum = true;
    fillRect(0, 0, _vpW - _vpX, w, color);                 
    fillRect(0, w, w, _vpH - _vpY - w - w, color);         
    fillRect(_xWidth - w, w, w, _yHeight - w - w, color);  
    fillRect(0, _yHeight - w, _xWidth, w, color);          
  }
  else
   
   
  {
    w = -w;

     
    int32_t _xT = _vpX; _vpX = 0;
    int32_t _yT = _vpY; _vpY = 0;
    int32_t _wT = _vpW;
    int32_t _hT = _vpH;

     
    _vpDatum = false;  
    _vpH = height();
    _vpW = width();

     
    fillRect(_xT - w - _xDatum, _yT - w - _yDatum, _wT - _xT + w + w, w, color);  
    fillRect(_xT - w - _xDatum, _yT - _yDatum, w, _hT - _yT, color);              
    fillRect(_wT - _xDatum, _yT - _yDatum, w, _hT - _yT, color);                  
    fillRect(_xT - w - _xDatum, _hT - _yDatum, _wT - _xT + w + w, w, color);      

     
    _vpX = _xT;
    _vpY = _yT;
    _vpW = _wT;
    _vpH = _hT;
  }

   
  _vpDatum = _dT;
}





bool TFT_eSPI::clipAddrWindow(int32_t *x, int32_t *y, int32_t *w, int32_t *h)
{
  if (_vpOoB) return false;  

  *x+= _xDatum;
  *y+= _yDatum;

  if ((*x >= _vpW) || (*y >= _vpH)) return false;   

   
  if (*x < _vpX) { *w -= _vpX - *x; *x = _vpX; }
  if (*y < _vpY) { *h -= _vpY - *y; *y = _vpY; }

  if ((*x + *w) > _vpW ) *w = _vpW - *x;
  if ((*y + *h) > _vpH ) *h = _vpH - *y;

  if (*w < 1 || *h < 1) return false;  

  return true;   
}





bool TFT_eSPI::clipWindow(int32_t *xs, int32_t *ys, int32_t *xe, int32_t *ye)
{
  if (_vpOoB) return false;  

  *xs+= _xDatum;
  *ys+= _yDatum;
  *xe+= _xDatum;
  *ye+= _yDatum;

  if ((*xs >= _vpW) || (*ys >= _vpH)) return false;   
  if ((*xe <  _vpX) || (*ye <  _vpY)) return false;   

   
  if (*xs < _vpX) *xs = _vpX;
  if (*ys < _vpY) *ys = _vpY;

  if (*xe > _vpW) *xe = _vpW - 1;
  if (*ye > _vpH) *ye = _vpH - 1;

  return true;   
}





TFT_eSPI::TFT_eSPI(int16_t w, int16_t h)
{
  _init_width  = _width  = w;  
  _init_height = _height = h;  

   
  resetViewport();

  rotation  = 0;
  cursor_y  = cursor_x  = last_cursor_x = bg_cursor_x = 0;
  textfont  = 1;
  textsize  = 1;
  textcolor   = bitmap_fg = 0xFFFF;  
  textbgcolor = bitmap_bg = 0x0000;  
  padX        = 0;                   

  _fillbg    = false;    

  isDigits   = false;    
  textwrapX  = true;     
  textwrapY  = false;    
  textdatum = TL_DATUM;  
  fontsloaded = 0;

  _swapBytes = false;    

  locked = true;            
  inTransaction = false;    
  lockTransaction = false;  

  _booted   = true;      
  _cp437    = false;     
  _utf8     = true;      

#if defined (FONT_FS_AVAILABLE) && defined (SMOOTH_FONT)
  fs_font  = true;      
#endif

#if defined (ESP32) && defined (CONFIG_SPIRAM_SUPPORT)
  if (psramFound()) _psram_enable = true;  
  else
#endif
  _psram_enable = false;

  addr_row = 0xFFFF;   
  addr_col = 0xFFFF;   

  _xPivot = 0;
  _yPivot = 0;

 
  cspinmask = 0;
  dcpinmask = 0;
  wrpinmask = 0;
  sclkpinmask = 0;

 
#ifdef LOAD_GLCD
  fontsloaded  = 0x0002;  
#endif

#ifdef LOAD_FONT2
  fontsloaded |= 0x0004;  
#endif

#ifdef LOAD_FONT4
  fontsloaded |= 0x0010;  
#endif

#ifdef LOAD_FONT6
  fontsloaded |= 0x0040;  
#endif

#ifdef LOAD_FONT7
  fontsloaded |= 0x0080;  
#endif

#ifdef LOAD_FONT8
  fontsloaded |= 0x0100;  
#endif

#ifdef LOAD_FONT8N
  fontsloaded |= 0x0200;  
#endif

#ifdef SMOOTH_FONT
  fontsloaded |= 0x8000;  
#endif
}





void TFT_eSPI::initBus(void) {

#ifdef TFT_CS
  if (TFT_CS >= 0) {
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);  
  }
#endif

 
#ifdef TOUCH_CS
  if (TOUCH_CS >= 0) {
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);  
  }
#endif

 
#if defined (TFT_WR) && !defined (ARDUINO_ARCH_RP2040) && !defined (ARDUINO_ARCH_MBED)
  if (TFT_WR >= 0) {
    pinMode(TFT_WR, OUTPUT);
    digitalWrite(TFT_WR, HIGH);  
  }
#endif

#ifdef TFT_DC
  if (TFT_DC >= 0) {
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);  
  }
#endif

#ifdef TFT_RST
  if (TFT_RST >= 0) {
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, HIGH);  
  }
#endif

#if defined (TFT_PARALLEL_8_BIT)

   
  if (TFT_RD >= 0) {
    pinMode(TFT_RD, OUTPUT);
    digitalWrite(TFT_RD, HIGH);
  }

  #if  !defined (ARDUINO_ARCH_RP2040)  && !defined (ARDUINO_ARCH_MBED) 
     
    pinMode(TFT_D0, OUTPUT); digitalWrite(TFT_D0, HIGH);
    pinMode(TFT_D1, OUTPUT); digitalWrite(TFT_D1, HIGH);
    pinMode(TFT_D2, OUTPUT); digitalWrite(TFT_D2, HIGH);
    pinMode(TFT_D3, OUTPUT); digitalWrite(TFT_D3, HIGH);
    pinMode(TFT_D4, OUTPUT); digitalWrite(TFT_D4, HIGH);
    pinMode(TFT_D5, OUTPUT); digitalWrite(TFT_D5, HIGH);
    pinMode(TFT_D6, OUTPUT); digitalWrite(TFT_D6, HIGH);
    pinMode(TFT_D7, OUTPUT); digitalWrite(TFT_D7, HIGH);
  #endif

  PARALLEL_INIT_TFT_DATA_BUS;

#endif
}





void TFT_eSPI::begin(uint8_t tc)
{
 init(tc);
}






void TFT_eSPI::init(uint8_t tc)
{
  if (_booted)
  {
    initBus();

#if !defined (ESP32) && !defined(TFT_PARALLEL_8_BIT) && !defined(ARDUINO_ARCH_RP2040) && !defined (ARDUINO_ARCH_MBED)
   
  #if defined (TFT_CS) && (TFT_CS >= 0)
    cspinmask = (uint32_t) digitalPinToBitMask(TFT_CS);
  #endif

  #if defined (TFT_DC) && (TFT_DC >= 0)
    dcpinmask = (uint32_t) digitalPinToBitMask(TFT_DC);
  #endif

  #if defined (TFT_WR) && (TFT_WR >= 0)
    wrpinmask = (uint32_t) digitalPinToBitMask(TFT_WR);
  #endif

  #if defined (TFT_SCLK) && (TFT_SCLK >= 0)
    sclkpinmask = (uint32_t) digitalPinToBitMask(TFT_SCLK);
  #endif

  #if defined (TFT_SPI_OVERLAP) && defined (ARDUINO_ARCH_ESP8266)
     
     
     
    spi.pins(6, 7, 8, 0);
  #endif

  spi.begin();  

#else
  #if !defined(TFT_PARALLEL_8_BIT) && !defined(RP2040_PIO_INTERFACE)
    #if defined (TFT_MOSI) && !defined (TFT_SPI_OVERLAP) && !defined(ARDUINO_ARCH_RP2040) && !defined (ARDUINO_ARCH_MBED)
      spi.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);  
    #else
      spi.begin();  
    #endif
  #endif
#endif
    lockTransaction = false;
    inTransaction = false;
    locked = true;

    INIT_TFT_DATA_BUS;


#if defined (TFT_CS) && !defined(RP2040_PIO_INTERFACE)
   
  if (TFT_CS >= 0) {
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);  
  }
#elif defined (ARDUINO_ARCH_ESP8266) && !defined (TFT_PARALLEL_8_BIT) && !defined (RP2040_PIO_SPI)
  spi.setHwCs(1);  
#endif


   
#if defined (TFT_DC) && !defined(RP2040_PIO_INTERFACE)
  if (TFT_DC >= 0) {
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);  
  }
#endif

    _booted = false;
    end_tft_write();
  }  

   
#ifdef TFT_RST
  #if !defined(RP2040_PIO_INTERFACE)
     
    if (TFT_RST >= 0) {
      pinMode(TFT_RST, OUTPUT);
    }
  #endif
  if (TFT_RST >= 0) {
    writecommand(0x00);  
    digitalWrite(TFT_RST, HIGH);
    delay(5);
    digitalWrite(TFT_RST, LOW);
    delay(20);
    digitalWrite(TFT_RST, HIGH);
  }
  else writecommand(TFT_SWRST);  
#else
  writecommand(TFT_SWRST);  
#endif

  delay(150);  

  begin_tft_write();

  tc = tc;  

   
#if   defined (ILI9341_DRIVER) || defined(ILI9341_2_DRIVER) || defined (ILI9342_DRIVER)
    #include "TFT_Drivers/ILI9341_Init.h"

#elif defined (ST7735_DRIVER)
    tabcolor = tc;
    #include "TFT_Drivers/ST7735_Init.h"

#elif defined (ILI9163_DRIVER)
    #include "TFT_Drivers/ILI9163_Init.h"

#elif defined (S6D02A1_DRIVER)
    #include "TFT_Drivers/S6D02A1_Init.h"

#elif defined (ST7796_DRIVER)
    #include "TFT_Drivers/ST7796_Init.h"

#elif defined (ILI9486_DRIVER)
    #include "TFT_Drivers/ILI9486_Init.h"

#elif defined (ILI9481_DRIVER)
    #include "TFT_Drivers/ILI9481_Init.h"

#elif defined (ILI9488_DRIVER)
    #include "TFT_Drivers/ILI9488_Init.h"

#elif defined (HX8357D_DRIVER)
    #include "TFT_Drivers/HX8357D_Init.h"

#elif defined (ST7789_DRIVER)
    #include "TFT_Drivers/ST7789_Init.h"

#elif defined (R61581_DRIVER)
    #include "TFT_Drivers/R61581_Init.h"

#elif defined (RM68140_DRIVER)
	#include "TFT_Drivers/RM68140_Init.h"

#elif defined (ST7789_2_DRIVER)
    #include "TFT_Drivers/ST7789_2_Init.h"

#elif defined (SSD1351_DRIVER)
    #include "TFT_Drivers/SSD1351_Init.h"

#elif defined (SSD1963_DRIVER)
    #include "TFT_Drivers/SSD1963_Init.h"

#elif defined (GC9A01_DRIVER)
     #include "TFT_Drivers/GC9A01_Init.h"

#elif defined (ILI9225_DRIVER)
     #include "TFT_Drivers/ILI9225_Init.h"

#elif defined (RM68120_DRIVER)
     #include "TFT_Drivers/RM68120_Init.h"

#elif defined (HX8357B_DRIVER)
    #include "TFT_Drivers/HX8357B_Init.h"

#elif defined (HX8357C_DRIVER)
    #include "TFT_Drivers/HX8357C_Init.h"

#endif

#ifdef TFT_INVERSION_ON
  writecommand(TFT_INVON);
#endif

#ifdef TFT_INVERSION_OFF
  writecommand(TFT_INVOFF);
#endif

  end_tft_write();

  setRotation(rotation);

#if defined (TFT_BL) && defined (TFT_BACKLIGHT_ON)
  if (TFT_BL >= 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  }
#else
  #if defined (TFT_BL) && defined (M5STACK)
     
    if (TFT_BL >= 0) {
      pinMode(TFT_BL, OUTPUT);
      digitalWrite(TFT_BL, HIGH);
    }
  #endif
#endif
}






void TFT_eSPI::setRotation(uint8_t m)
{

  begin_tft_write();

     
#if   defined (ILI9341_DRIVER) || defined(ILI9341_2_DRIVER) || defined (ILI9342_DRIVER)
    #include "TFT_Drivers/ILI9341_Rotation.h"

#elif defined (ST7735_DRIVER)
    #include "TFT_Drivers/ST7735_Rotation.h"

#elif defined (ILI9163_DRIVER)
    #include "TFT_Drivers/ILI9163_Rotation.h"

#elif defined (S6D02A1_DRIVER)
    #include "TFT_Drivers/S6D02A1_Rotation.h"

#elif defined (ST7796_DRIVER)
    #include "TFT_Drivers/ST7796_Rotation.h"

#elif defined (ILI9486_DRIVER)
    #include "TFT_Drivers/ILI9486_Rotation.h"

#elif defined (ILI9481_DRIVER)
    #include "TFT_Drivers/ILI9481_Rotation.h"

#elif defined (ILI9488_DRIVER)
    #include "TFT_Drivers/ILI9488_Rotation.h"

#elif defined (HX8357D_DRIVER)
    #include "TFT_Drivers/HX8357D_Rotation.h"

#elif defined (ST7789_DRIVER)
    #include "TFT_Drivers/ST7789_Rotation.h"

#elif defined (R61581_DRIVER)
    #include "TFT_Drivers/R61581_Rotation.h"

#elif defined (RM68140_DRIVER)
	#include "TFT_Drivers/RM68140_Rotation.h"

#elif defined (ST7789_2_DRIVER)
    #include "TFT_Drivers/ST7789_2_Rotation.h"

#elif defined (SSD1351_DRIVER)
    #include "TFT_Drivers/SSD1351_Rotation.h"

#elif defined (SSD1963_DRIVER)
    #include "TFT_Drivers/SSD1963_Rotation.h"

#elif defined (GC9A01_DRIVER)
     #include "TFT_Drivers/GC9A01_Rotation.h"

#elif defined (ILI9225_DRIVER)
     #include "TFT_Drivers/ILI9225_Rotation.h"

#elif defined (RM68120_DRIVER)
     #include "TFT_Drivers/RM68120_Rotation.h"

#elif defined (HX8357B_DRIVER)
    #include "TFT_Drivers/HX8357B_Rotation.h"

#elif defined (HX8357C_DRIVER)
    #include "TFT_Drivers/HX8357C_Rotation.h"

#endif

  delayMicroseconds(10);

  end_tft_write();

  addr_row = 0xFFFF;
  addr_col = 0xFFFF;

   
  resetViewport();
}






uint8_t TFT_eSPI::getRotation(void)
{
  return rotation;
}






 
void TFT_eSPI::setOrigin(int32_t x, int32_t y)
{
  _xDatum = x;
  _yDatum = y;
}






int32_t TFT_eSPI::getOriginX(void)
{
  return _xDatum;
}






int32_t TFT_eSPI::getOriginY(void)
{
  return _yDatum;
}






void TFT_eSPI::commandList (const uint8_t *addr)
{
  uint8_t  numCommands;
  uint8_t  numArgs;
  uint8_t  ms;

  numCommands = pgm_read_byte(addr++);    

  while (numCommands--)                   
  {
    writecommand(pgm_read_byte(addr++));  
    numArgs = pgm_read_byte(addr++);      
    ms = numArgs & TFT_INIT_DELAY;        
    numArgs &= ~TFT_INIT_DELAY;           

    while (numArgs--)                     
    {
      writedata(pgm_read_byte(addr++));   
    }

    if (ms)
    {
      ms = pgm_read_byte(addr++);         
      delay( (ms==255 ? 500 : ms) );
    }
  }

}






void TFT_eSPI::spiwrite(uint8_t c)
{
  begin_tft_write();
  tft_Write_8(c);
  end_tft_write();
}






#ifndef RM68120_DRIVER
void TFT_eSPI::writecommand(uint8_t c)
{
  begin_tft_write();

  DC_C;

  tft_Write_8(c);

  DC_D;

  end_tft_write();
}
#else
void TFT_eSPI::writecommand(uint16_t c)
{
  begin_tft_write();

  DC_C;

  tft_Write_16(c);

  DC_D;

  end_tft_write();

}
void TFT_eSPI::writeRegister8(uint16_t c, uint8_t d)
{
  begin_tft_write();

  DC_C;

  tft_Write_16(c);

  DC_D;

  tft_Write_8(d);

  end_tft_write();

}
void TFT_eSPI::writeRegister16(uint16_t c, uint16_t d)
{
  begin_tft_write();

  DC_C;

  tft_Write_16(c);

  DC_D;

  tft_Write_16(d);

  end_tft_write();

}

#endif





void TFT_eSPI::writedata(uint8_t d)
{
  begin_tft_write();

  DC_D;         

  tft_Write_8(d);

  CS_L;         

  end_tft_write();
}






uint8_t TFT_eSPI::readcommand8(uint8_t cmd_function, uint8_t index)
{
  uint8_t reg = 0;
#if defined(TFT_PARALLEL_8_BIT) || defined(RP2040_PIO_INTERFACE)

  writecommand(cmd_function);  

  busDir(GPIO_DIR_MASK, INPUT);

  CS_L;

   
  while(index--) reg = readByte();

  busDir(GPIO_DIR_MASK, OUTPUT);

  CS_H;

#else  
   
  begin_tft_read();
  index = 0x10 + (index & 0x0F);

  DC_C; tft_Write_8(0xD9);
  DC_D; tft_Write_8(index);

  CS_H;  
  CS_L;

  DC_C; tft_Write_8(cmd_function);
  DC_D;
  reg = tft_Read_8();

  end_tft_read();
#endif
  return reg;
}






uint16_t TFT_eSPI::readcommand16(uint8_t cmd_function, uint8_t index)
{
  uint32_t reg;

  reg  = (readcommand8(cmd_function, index + 0) <<  8);
  reg |= (readcommand8(cmd_function, index + 1) <<  0);

  return reg;
}






uint32_t TFT_eSPI::readcommand32(uint8_t cmd_function, uint8_t index)
{
  uint32_t reg;

  reg  = ((uint32_t)readcommand8(cmd_function, index + 0) << 24);
  reg |= ((uint32_t)readcommand8(cmd_function, index + 1) << 16);
  reg |= ((uint32_t)readcommand8(cmd_function, index + 2) <<  8);
  reg |= ((uint32_t)readcommand8(cmd_function, index + 3) <<  0);

  return reg;
}






uint16_t TFT_eSPI::readPixel(int32_t x0, int32_t y0)
{
  if (_vpOoB) return 0;

  x0+= _xDatum;
  y0+= _yDatum;

   
  if ((x0 < _vpX) || (y0 < _vpY) ||(x0 >= _vpW) || (y0 >= _vpH)) return 0;

#if defined(TFT_PARALLEL_8_BIT) || defined(RP2040_PIO_INTERFACE)

  if (!inTransaction) { CS_L; }  

  readAddrWindow(x0, y0, 1, 1);

   
  busDir(GPIO_DIR_MASK, INPUT);

  #if  !defined (SSD1963_DRIVER)
   
  readByte();
  #endif

   
   

  #if defined (ILI9341_DRIVER)  || defined(ILI9341_2_DRIVER) || defined (ILI9488_DRIVER) || defined (SSD1963_DRIVER) 

     
    uint16_t rgb = ((readByte() & 0xF8) << 8) | ((readByte() & 0xFC) << 3) | (readByte() >> 3);

    if (!inTransaction) { CS_H; }  

     
    busDir(GPIO_DIR_MASK, OUTPUT);

    return rgb;

  #else  

     
    uint16_t bgr = (readByte() << 8) | readByte();

    if (!inTransaction) { CS_H; }  

     
    busDir(GPIO_DIR_MASK, OUTPUT);

    #if defined (ILI9486_DRIVER) || defined (ST7796_DRIVER)
      return  bgr;
    #else
       
      return  (bgr>>11) | (bgr<<11) | (bgr & 0x7E0);
    #endif

  #endif

#else  

   
   
  bool wasInTransaction = inTransaction;
  if (inTransaction) { inTransaction= false; end_tft_write();}

  uint16_t color = 0;

  begin_tft_read();  

  readAddrWindow(x0, y0, 1, 1);

  #ifdef TFT_SDA_READ
    begin_SDA_Read();
  #endif

   
  tft_Read_8();

   

    #if defined (ST7796_DRIVER)
       
      color = ((tft_Read_8()) << 8) | (tft_Read_8());
    #elif defined (ST7735_DRIVER)
       
       
      uint8_t r = tft_Read_8()<<1;
      uint8_t g = tft_Read_8()<<1;
      uint8_t b = tft_Read_8()<<1;
      color = color565(r, g, b);
    #else
       
       
      uint8_t r = tft_Read_8();
      uint8_t g = tft_Read_8();
      uint8_t b = tft_Read_8();
      color = color565(r, g, b);
    #endif













  CS_H;

  #ifdef TFT_SDA_READ
    end_SDA_Read();
  #endif

  end_tft_read();

   
  if(wasInTransaction) { begin_tft_write(); inTransaction = true; }

  return color;

#endif
}

void TFT_eSPI::setCallback(getColorCallback getCol)
{
  getColor = getCol;
}






void TFT_eSPI::readRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data)
{
  PI_CLIP ;

#if defined(TFT_PARALLEL_8_BIT) || defined(RP2040_PIO_INTERFACE)

  CS_L;

  readAddrWindow(x, y, dw, dh);

  data += dx + dy * w;

   
  busDir(GPIO_DIR_MASK, INPUT);

  #if defined (ILI9341_DRIVER)  || defined(ILI9341_2_DRIVER) || defined (ILI9488_DRIVER)  
     
    readByte();

     
    while (dh--) {
      int32_t lw = dw;
      uint16_t* line = data;
      while (lw--) {
         
        uint16_t rgb = ((readByte() & 0xF8) << 8) | ((readByte() & 0xFC) << 3) | (readByte() >> 3);

         
        *line++ = (rgb<<8) | (rgb>>8);
      }
      data += w;
    }

  #elif  defined (SSD1963_DRIVER)
     
    while (dh--) {
      int32_t lw = dw;
      uint16_t* line = data;
      while (lw--) {
        uint16_t bgr = ((readByte() & 0xF8) >> 3);;  
        bgr |= ((readByte() & 0xFC) << 3);
        bgr |= (readByte() << 8);
         
        uint16_t rgb = (bgr>>11) | (bgr<<11) | (bgr & 0x7E0);
         
        *line++ = (rgb<<8) | (rgb>>8);
      }
      data += w;
    }

  #else  
     
    readByte();

     
    while (dh--) {
      int32_t lw = dw;
      uint16_t* line = data;
      while (lw--) {
      #if defined (ILI9486_DRIVER) || defined (ST7796_DRIVER)
         
        *line++ = readByte() | (readByte() << 8);
      #else
         
        uint16_t bgr = (readByte() << 8) | readByte();
         
        uint16_t rgb = (bgr>>11) | (bgr<<11) | (bgr & 0x7E0);
         
        *line++ = (rgb<<8) | (rgb>>8);
      #endif
      }
      data += w;
    }
  #endif

  CS_H;

   
  busDir(GPIO_DIR_MASK, OUTPUT);

#else  

   
   
  bool wasInTransaction = inTransaction;
  if (inTransaction) { inTransaction= false; end_tft_write();}

  uint16_t color = 0;

  begin_tft_read();

  readAddrWindow(x, y, dw, dh);

  data += dx + dy * w;

  #ifdef TFT_SDA_READ
    begin_SDA_Read();
  #endif

   
  tft_Read_8();

   
  while (dh--) {
    int32_t lw = dw;
    uint16_t* line = data;
    while (lw--) {

  #if !defined (ILI9488_DRIVER)

    #if defined (ST7796_DRIVER)
       
      color = ((tft_Read_8()) << 8) | (tft_Read_8());
    #elif defined (ST7735_DRIVER)
       
       
      uint8_t r = tft_Read_8()<<1;
      uint8_t g = tft_Read_8()<<1;
      uint8_t b = tft_Read_8()<<1;
      color = color565(r, g, b);
    #else
       
       
      uint8_t r = tft_Read_8();
      uint8_t g = tft_Read_8();
      uint8_t b = tft_Read_8();
      color = color565(r, g, b);
    #endif

  #else

       
       
      uint8_t r = (tft_Read_8()&0x7E)<<1;
      uint8_t g = (tft_Read_8()&0x7E)<<1;
      uint8_t b = (tft_Read_8()&0x7E)<<1;
      color = color565(r, g, b);
  #endif

       
      *line++ = color << 8 | color >> 8;
    }
    data += w;
  }

   

  #ifdef TFT_SDA_READ
    end_SDA_Read();
  #endif

  end_tft_read();

   
  if(wasInTransaction) { begin_tft_write(); inTransaction = true; }
#endif
}






void TFT_eSPI::pushRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data)
{
  bool swap = _swapBytes; _swapBytes = false;
  pushImage(x, y, w, h, data);
  _swapBytes = swap;
}






void TFT_eSPI::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data)
{
  PI_CLIP;

  begin_tft_write();
  inTransaction = true;

  setWindow(x, y, x + dw - 1, y + dh - 1);

  data += dx + dy * w;

   
  if (dw == w) pushPixels(data, dw * dh);
  else {
     
    while (dh--)
    {
      pushPixels(data, dw);
      data += w;
    }
  }

  inTransaction = lockTransaction;
  end_tft_write();
}





void TFT_eSPI::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint16_t transp)
{
  PI_CLIP;

  begin_tft_write();
  inTransaction = true;

  data += dx + dy * w;


  uint16_t  lineBuf[dw];  

   
  if (!_swapBytes) transp = transp >> 8 | transp << 8;

  while (dh--)
  {
    int32_t len = dw;
    uint16_t* ptr = data;
    int32_t px = x, sx = x;
    bool move = true;
    uint16_t np = 0;

    while (len--)
    {
      if (transp != *ptr)
      {
        if (move) { move = false; sx = px; }
        lineBuf[np] = *ptr;
        np++;
      }
      else
      {
        move = true;
        if (np)
        {
          setWindow(sx, y, sx + np - 1, y);
          pushPixels((uint16_t*)lineBuf, np);
          np = 0;
        }
      }
      px++;
      ptr++;
    }
    if (np) { setWindow(sx, y, sx + np - 1, y); pushPixels((uint16_t*)lineBuf, np); }

    y++;
    data += w;
  }

  inTransaction = lockTransaction;
  end_tft_write();
}






void TFT_eSPI::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data)
{
   
  PI_CLIP;

  begin_tft_write();
  inTransaction = true;

  data += dx + dy * w;

  uint16_t  buffer[dw];

  setWindow(x, y, x + dw - 1, y + dh - 1);

   
  for (int32_t i = 0; i < dh; i++) {
    for (int32_t j = 0; j < dw; j++) {
      buffer[j] = pgm_read_word(&data[i * w + j]);
    }
    pushPixels(buffer, dw);
  }

  inTransaction = lockTransaction;
  end_tft_write();
}





void TFT_eSPI::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data, uint16_t transp)
{
   
  PI_CLIP;

  begin_tft_write();
  inTransaction = true;

  data += dx + dy * w;


  uint16_t  lineBuf[dw];

   
  if (!_swapBytes) transp = transp >> 8 | transp << 8;

  while (dh--) {
    int32_t len = dw;
    uint16_t* ptr = (uint16_t*)data;
    int32_t px = x, sx = x;
    bool move = true;

    uint16_t np = 0;

    while (len--) {
      uint16_t color = pgm_read_word(ptr);
      if (transp != color) {
        if (move) { move = false; sx = px; }
        lineBuf[np] = color;
        np++;
      }
      else {
        move = true;
        if (np) {
          setWindow(sx, y, sx + np - 1, y);
          pushPixels(lineBuf, np);
          np = 0;
        }
      }
      px++;
      ptr++;
    }
    if (np) { setWindow(sx, y, sx + np - 1, y); pushPixels(lineBuf, np); }

    y++;
    data += w;
  }

  inTransaction = lockTransaction;
  end_tft_write();
}





void TFT_eSPI::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8,  uint16_t *cmap)
{
  PI_CLIP;

  begin_tft_write();
  inTransaction = true;
  bool swap = _swapBytes;

  setWindow(x, y, x + dw - 1, y + dh - 1);  

   
  uint16_t  lineBuf[dw];

  if (bpp8)
  {
    _swapBytes = false;

    uint8_t  blue[] = {0, 11, 21, 31};  

    _lastColor = -1;  

     
    uint8_t msbColor = 0;
    uint8_t lsbColor = 0;

    data += dx + dy * w;
    while (dh--) {
      uint32_t len = dw;
      uint8_t* ptr = (uint8_t*)data;
      uint8_t* linePtr = (uint8_t*)lineBuf;

      while(len--) {
        uint32_t color = pgm_read_byte(ptr++);

         
        if (color != _lastColor) {
           
          msbColor = (color & 0x1C)>>2 | (color & 0xC0)>>3 | (color & 0xE0);
           
          lsbColor = (color & 0x1C)<<3 | blue[color & 0x03];
          _lastColor = color;
        }

       *linePtr++ = msbColor;
       *linePtr++ = lsbColor;
      }

      pushPixels(lineBuf, dw);

      data += w;
    }
    _swapBytes = swap;  
  }
  else if (cmap != nullptr)  
  {
    _swapBytes = true;

    w = (w+1) & 0xFFFE;    
    bool splitFirst = (dx & 0x01) != 0;  

    if (splitFirst) {
      data += ((dx - 1 + dy * w) >> 1);
    }
    else {
      data += ((dx + dy * w) >> 1);
    }

    while (dh--) {
      uint32_t len = dw;
      uint8_t * ptr = (uint8_t*)data;
      uint16_t *linePtr = lineBuf;
      uint8_t colors;  
      uint16_t index;

      if (splitFirst) {
        colors = pgm_read_byte(ptr);
        index = (colors & 0x0F);
        *linePtr++ = cmap[index];
        len--;
        ptr++;
      }

      while (len--)
      {
        colors = pgm_read_byte(ptr);
        index = ((colors & 0xF0) >> 4) & 0x0F;
        *linePtr++ = cmap[index];

        if (len--)
        {
          index = colors & 0x0F;
          *linePtr++ = cmap[index];
        } else {
          break;   
        }

        ptr++;
      }

      pushPixels(lineBuf, dw);
      data += (w >> 1);
    }
    _swapBytes = swap;  
  }
  else  
  {
    _swapBytes = false;
    uint8_t * ptr = (uint8_t*)data;
    uint32_t ww =  (w+7)>>3;  
    for (int32_t yp = dy;  yp < dy + dh; yp++)
    {
      uint8_t* linePtr = (uint8_t*)lineBuf;
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        uint16_t col = (pgm_read_byte(ptr + (xp>>3)) & (0x80 >> (xp & 0x7)) );
        if (col) {*linePtr++ = bitmap_fg>>8; *linePtr++ = (uint8_t) bitmap_fg;}
        else     {*linePtr++ = bitmap_bg>>8; *linePtr++ = (uint8_t) bitmap_bg;}
      }
      ptr += ww;
      pushPixels(lineBuf, dw);
    }
  }

  _swapBytes = swap;  
  inTransaction = lockTransaction;
  end_tft_write();
}






void TFT_eSPI::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8,  uint16_t *cmap)
{
  PI_CLIP;

  begin_tft_write();
  inTransaction = true;
  bool swap = _swapBytes;

  setWindow(x, y, x + dw - 1, y + dh - 1);  

   
  uint16_t  lineBuf[dw];

  if (bpp8)
  {
    _swapBytes = false;

    uint8_t  blue[] = {0, 11, 21, 31};  

    _lastColor = -1;  

     
    uint8_t msbColor = 0;
    uint8_t lsbColor = 0;

    data += dx + dy * w;
    while (dh--) {
      uint32_t len = dw;
      uint8_t* ptr = data;
      uint8_t* linePtr = (uint8_t*)lineBuf;

      while(len--) {
        uint32_t color = *ptr++;

         
        if (color != _lastColor) {
           
          msbColor = (color & 0x1C)>>2 | (color & 0xC0)>>3 | (color & 0xE0);
           
          lsbColor = (color & 0x1C)<<3 | blue[color & 0x03];
          _lastColor = color;
        }

       *linePtr++ = msbColor;
       *linePtr++ = lsbColor;
      }

      pushPixels(lineBuf, dw);

      data += w;
    }
    _swapBytes = swap;  
  }
  else if (cmap != nullptr)  
  {
    _swapBytes = true;

    w = (w+1) & 0xFFFE;    
    bool splitFirst = (dx & 0x01) != 0;  

    if (splitFirst) {
      data += ((dx - 1 + dy * w) >> 1);
    }
    else {
      data += ((dx + dy * w) >> 1);
    }

    while (dh--) {
      uint32_t len = dw;
      uint8_t * ptr = data;
      uint16_t *linePtr = lineBuf;
      uint8_t colors;  
      uint16_t index;

      if (splitFirst) {
        colors = *ptr;
        index = (colors & 0x0F);
        *linePtr++ = cmap[index];
        len--;
        ptr++;
      }

      while (len--)
      {
        colors = *ptr;
        index = ((colors & 0xF0) >> 4) & 0x0F;
        *linePtr++ = cmap[index];

        if (len--)
        {
          index = colors & 0x0F;
          *linePtr++ = cmap[index];
        } else {
          break;   
        }

        ptr++;
      }

      pushPixels(lineBuf, dw);
      data += (w >> 1);
    }
    _swapBytes = swap;  
  }
  else  
  {
    _swapBytes = false;

    uint32_t ww =  (w+7)>>3;  
    for (int32_t yp = dy;  yp < dy + dh; yp++)
    {
      uint8_t* linePtr = (uint8_t*)lineBuf;
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        uint16_t col = (data[(xp>>3)] & (0x80 >> (xp & 0x7)) );
        if (col) {*linePtr++ = bitmap_fg>>8; *linePtr++ = (uint8_t) bitmap_fg;}
        else     {*linePtr++ = bitmap_bg>>8; *linePtr++ = (uint8_t) bitmap_bg;}
      }
      data += ww;
      pushPixels(lineBuf, dw);
    }
  }

  _swapBytes = swap;  
  inTransaction = lockTransaction;
  end_tft_write();
}






void TFT_eSPI::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, uint8_t transp, bool bpp8, uint16_t *cmap)
{
  PI_CLIP;

  begin_tft_write();
  inTransaction = true;
  bool swap = _swapBytes;


   
  uint16_t  lineBuf[dw];

  if (bpp8) {  
    _swapBytes = false;

    data += dx + dy * w;

    uint8_t  blue[] = {0, 11, 21, 31};  

    _lastColor = -1;  

     
    uint8_t msbColor = 0;
    uint8_t lsbColor = 0;

    while (dh--) {
      int32_t len = dw;
      uint8_t* ptr = data;
      uint8_t* linePtr = (uint8_t*)lineBuf;

      int32_t px = x, sx = x;
      bool move = true;
      uint16_t np = 0;

      while (len--) {
        if (transp != *ptr) {
          if (move) { move = false; sx = px; }
          uint8_t color = *ptr;

           
          if (color != _lastColor) {
             
            msbColor = (color & 0x1C)>>2 | (color & 0xC0)>>3 | (color & 0xE0);
             
            lsbColor = (color & 0x1C)<<3 | blue[color & 0x03];
            _lastColor = color;
          }
          *linePtr++ = msbColor;
          *linePtr++ = lsbColor;
          np++;
        }
        else {
          move = true;
          if (np) {
            setWindow(sx, y, sx + np - 1, y);
            pushPixels(lineBuf, np);
            linePtr = (uint8_t*)lineBuf;
            np = 0;
          }
        }
        px++;
        ptr++;
      }

      if (np) { setWindow(sx, y, sx + np - 1, y); pushPixels(lineBuf, np); }
      y++;
      data += w;
    }
  }
  else if (cmap != nullptr)  
  {
    _swapBytes = true;

    w = (w+1) & 0xFFFE;  
    bool splitFirst = ((dx & 0x01) != 0);
    if (splitFirst) {
      data += ((dx - 1 + dy * w) >> 1);
    }
    else {
      data += ((dx + dy * w) >> 1);
    }

    while (dh--) {
      uint32_t len = dw;
      uint8_t * ptr = data;

      int32_t px = x, sx = x;
      bool move = true;
      uint16_t np = 0;

      uint8_t index;   

      if (splitFirst) {
        index = (*ptr & 0x0F);   
        if (index != transp) {
          move = false; sx = px;
          lineBuf[np] = cmap[index];
          np++;
        }
        px++; ptr++;
        len--;
      }

      while (len--)
      {
        uint8_t color = *ptr;

         
         
        uint16_t index = ((color & 0xF0) >> 4) & 0x0F;   
        if (index != transp) {
          if (move) {
            move = false; sx = px;
          }
          lineBuf[np] = cmap[index];
          np++;  
        }
        else {
          move = true;
          if (np) {
            setWindow(sx, y, sx + np - 1, y);
            pushPixels(lineBuf, np);
            np = 0;
          }
        }
        px++;

        if (len--)
        {
          index = color & 0x0F;  
          if (index != transp) {
            if (move) {
              move = false; sx = px;
             }
            lineBuf[np] = cmap[index];
            np++;
          }
          else {
            move = true;
            if (np) {
              setWindow(sx, y, sx + np - 1, y);
              pushPixels(lineBuf, np);
              np = 0;
            }
          }
          px++;
        }
        else {
          break;   
        }
        ptr++;   
      }

      if (np) {
        setWindow(sx, y, sx + np - 1, y);
        pushPixels(lineBuf, np);
        np = 0;
      }
      data += (w>>1);
      y++;
    }
  }
  else {  
    _swapBytes = false;

    uint32_t ww =  (w+7)>>3;  
    uint16_t np = 0;

    for (int32_t yp = dy;  yp < dy + dh; yp++)
    {
      int32_t px = x, sx = x;
      bool move = true;
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        if (data[(xp>>3)] & (0x80 >> (xp & 0x7))) {
          if (move) {
            move = false;
            sx = px;
          }
          np++;
        }
        else {
          move = true;
          if (np) {
            setWindow(sx, y, sx + np - 1, y);
            pushBlock(bitmap_fg, np);
            np = 0;
          }
        }
        px++;
      }
      if (np) { setWindow(sx, y, sx + np - 1, y); pushBlock(bitmap_fg, np); np = 0; }
      y++;
      data += ww;
    }
  }
  _swapBytes = swap;  
  inTransaction = lockTransaction;
  end_tft_write();
}





 
void TFT_eSPI::pushMaskedImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *img, uint8_t *mask)
{
  if (_vpOoB || w < 1 || h < 1) return;

   
   

  begin_tft_write();
  inTransaction = true;

  uint8_t  *mptr = mask;
  uint8_t  *eptr = mask + ((w + 7) >> 3);
  uint16_t *iptr = img;
  uint32_t setCount = 0;

   
  while (h--) {
    uint32_t xp = 0;
    uint32_t clearCount = 0;
    uint8_t  mbyte= *mptr++;
    uint32_t bits  = 8;
     
    do {
      setCount = 0;

       
      while ((mbyte & 0x80) == 0x00) {
         
        if (mbyte == 0) {
          clearCount += bits;       
          if (mptr >= eptr) break;  
          mbyte = *mptr++;
          bits  = 8;
          continue;
        }
        mbyte = mbyte << 1;  
        clearCount ++;
        if (--bits) continue;;
        if (mptr >= eptr) break;
        mbyte = *mptr++;
        bits  = 8;
      }

       
      while ((mbyte & 0x80) == 0x80) {
         
        if (mbyte == 0xFF) {
          setCount += bits;
          if (mptr >= eptr) break;
          mbyte = *mptr++;
           
          continue;
        }
        mbyte = mbyte << 1;  
        setCount ++;
        if (--bits) continue;
        if (mptr >= eptr) break;
        mbyte = *mptr++;
        bits  = 8;
      }

       
      if (setCount) {
        xp += clearCount;
        clearCount = 0;
        pushImage(x + xp, y, setCount, 1, iptr + xp);       
        if (mptr >= eptr) break;
        xp += setCount;
      }
    } while (setCount || mptr < eptr);

    y++;
    iptr += w;
    eptr += ((w + 7) >> 3);
  }

  inTransaction = lockTransaction;
  end_tft_write();
}






void TFT_eSPI::setSwapBytes(bool swap)
{
  _swapBytes = swap;
}






bool TFT_eSPI::getSwapBytes(void)
{
  return _swapBytes;
}






 
void  TFT_eSPI::readRectRGB(int32_t x0, int32_t y0, int32_t w, int32_t h, uint8_t *data)
{
#if defined(TFT_PARALLEL_8_BIT) || defined(RP2040_PIO_INTERFACE)

  uint32_t len = w * h;
  uint8_t* buf565 = data + len;

  readRect(x0, y0, w, h, (uint16_t*)buf565);

  while (len--) {
    uint16_t pixel565 = (*buf565++)<<8;
    pixel565 |= *buf565++;
    uint8_t red   = (pixel565 & 0xF800) >> 8; red   |= red   >> 5;
    uint8_t green = (pixel565 & 0x07E0) >> 3; green |= green >> 6;
    uint8_t blue  = (pixel565 & 0x001F) << 3; blue  |= blue  >> 5;
    *data++ = red;
    *data++ = green;
    *data++ = blue;
  }

#else   

  begin_tft_read();

  readAddrWindow(x0, y0, w, h);  

  #ifdef TFT_SDA_READ
    begin_SDA_Read();
  #endif

   
  tft_Read_8();

   
  uint32_t len = w * h;
  while (len--) {

  #if !defined (ILI9488_DRIVER)

     
     
    *data++ = tft_Read_8();
    *data++ = tft_Read_8();
    *data++ = tft_Read_8();

  #else

     
     
    *data++ = (tft_Read_8()&0x7E)<<1;
    *data++ = (tft_Read_8()&0x7E)<<1;
    *data++ = (tft_Read_8()&0x7E)<<1;

  #endif

  }

  CS_H;

  #ifdef TFT_SDA_READ
    end_SDA_Read();
  #endif

  end_tft_read();

#endif
}






 
void TFT_eSPI::drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color)
{
  if ( r <= 0 ) return;

   
  inTransaction = true;

    int32_t f     = 1 - r;
    int32_t ddF_y = -2 * r;
    int32_t ddF_x = 1;
    int32_t xs    = -1;
    int32_t xe    = 0;
    int32_t len   = 0;

    bool first = true;
    do {
      while (f < 0) {
        ++xe;
        f += (ddF_x += 2);
      }
      f += (ddF_y += 2);

      if (xe-xs>1) {
        if (first) {
          len = 2*(xe - xs)-1;
          drawFastHLine(x0 - xe, y0 + r, len, color);
          drawFastHLine(x0 - xe, y0 - r, len, color);
          drawFastVLine(x0 + r, y0 - xe, len, color);
          drawFastVLine(x0 - r, y0 - xe, len, color);
          first = false;
        }
        else {
          len = xe - xs++;
          drawFastHLine(x0 - xe, y0 + r, len, color);
          drawFastHLine(x0 - xe, y0 - r, len, color);
          drawFastHLine(x0 + xs, y0 - r, len, color);
          drawFastHLine(x0 + xs, y0 + r, len, color);

          drawFastVLine(x0 + r, y0 + xs, len, color);
          drawFastVLine(x0 + r, y0 - xe, len, color);
          drawFastVLine(x0 - r, y0 - xe, len, color);
          drawFastVLine(x0 - r, y0 + xs, len, color);
        }
      }
      else {
        ++xs;
        drawPixel(x0 - xe, y0 + r, color);
        drawPixel(x0 - xe, y0 - r, color);
        drawPixel(x0 + xs, y0 - r, color);
        drawPixel(x0 + xs, y0 + r, color);

        drawPixel(x0 + r, y0 + xs, color);
        drawPixel(x0 + r, y0 - xe, color);
        drawPixel(x0 - r, y0 - xe, color);
        drawPixel(x0 - r, y0 + xs, color);
      }
      xs = xe;
    } while (xe < --r);

  inTransaction = lockTransaction;
  end_tft_write();               
}






void TFT_eSPI::drawCircleHelper( int32_t x0, int32_t y0, int32_t rr, uint8_t cornername, uint32_t color)
{
  if (rr <= 0) return;
  int32_t f     = 1 - rr;
  int32_t ddF_x = 1;
  int32_t ddF_y = -2 * rr;
  int32_t xe    = 0;
  int32_t xs    = 0;
  int32_t len   = 0;

   
  inTransaction = true;

  do
  {
    while (f < 0) {
      ++xe;
      f += (ddF_x += 2);
    }
    f += (ddF_y += 2);

    if (xe-xs==1) {
      if (cornername & 0x1) {  
        drawPixel(x0 - xe, y0 - rr, color);
        drawPixel(x0 - rr, y0 - xe, color);
      }
      if (cornername & 0x2) {  
        drawPixel(x0 + rr    , y0 - xe, color);
        drawPixel(x0 + xs + 1, y0 - rr, color);
      }
      if (cornername & 0x4) {  
        drawPixel(x0 + xs + 1, y0 + rr    , color);
        drawPixel(x0 + rr, y0 + xs + 1, color);
      }
      if (cornername & 0x8) {  
        drawPixel(x0 - rr, y0 + xs + 1, color);
        drawPixel(x0 - xe, y0 + rr    , color);
      }
    }
    else {
      len = xe - xs++;
      if (cornername & 0x1) {  
        drawFastHLine(x0 - xe, y0 - rr, len, color);
        drawFastVLine(x0 - rr, y0 - xe, len, color);
      }
      if (cornername & 0x2) {  
        drawFastVLine(x0 + rr, y0 - xe, len, color);
        drawFastHLine(x0 + xs, y0 - rr, len, color);
      }
      if (cornername & 0x4) {  
        drawFastHLine(x0 + xs, y0 + rr, len, color);
        drawFastVLine(x0 + rr, y0 + xs, len, color);
      }
      if (cornername & 0x8) {  
        drawFastVLine(x0 - rr, y0 + xs, len, color);
        drawFastHLine(x0 - xe, y0 + rr, len, color);
      }
    }
    xs = xe;
  } while (xe < rr--);

  inTransaction = lockTransaction;
  end_tft_write();               
}





 
 
void TFT_eSPI::fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color)
{
  int32_t  x  = 0;
  int32_t  dx = 1;
  int32_t  dy = r+r;
  int32_t  p  = -(r>>1);

   
  inTransaction = true;

  drawFastHLine(x0 - r, y0, dy+1, color);

  while(x<r){

    if(p>=0) {
      drawFastHLine(x0 - x, y0 + r, dx, color);
      drawFastHLine(x0 - x, y0 - r, dx, color);
      dy-=2;
      p-=dy;
      r--;
    }

    dx+=2;
    p+=dx;
    x++;

    drawFastHLine(x0 - r, y0 + x, dy+1, color);
    drawFastHLine(x0 - r, y0 - x, dy+1, color);

  }

  inTransaction = lockTransaction;
  end_tft_write();               
}





 
void TFT_eSPI::fillCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, int32_t delta, uint32_t color)
{
  int32_t f     = 1 - r;
  int32_t ddF_x = 1;
  int32_t ddF_y = -r - r;
  int32_t y     = 0;

  delta++;

  while (y < r) {
    if (f >= 0) {
      if (cornername & 0x1) drawFastHLine(x0 - y, y0 + r, y + y + delta, color);
      if (cornername & 0x2) drawFastHLine(x0 - y, y0 - r, y + y + delta, color);
      r--;
      ddF_y += 2;
      f     += ddF_y;
    }

    y++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) drawFastHLine(x0 - r, y0 + y, r + r + delta, color);
    if (cornername & 0x2) drawFastHLine(x0 - r, y0 - y, r + r + delta, color);
  }
}






void TFT_eSPI::drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color)
{
  if (rx<2) return;
  if (ry<2) return;
  int32_t x, y;
  int32_t rx2 = rx * rx;
  int32_t ry2 = ry * ry;
  int32_t fx2 = 4 * rx2;
  int32_t fy2 = 4 * ry2;
  int32_t s;

   
  inTransaction = true;

  for (x = 0, y = ry, s = 2*ry2+rx2*(1-2*ry); ry2*x <= rx2*y; x++) {
     
     
    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + x, y0 - y, color);
    if (s >= 0) {
      s += fx2 * (1 - y);
      y--;
    }
    s += ry2 * ((4 * x) + 6);
  }

  for (x = rx, y = 0, s = 2*rx2+ry2*(1-2*rx); rx2*y <= ry2*x; y++) {
     
     
    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + x, y0 - y, color);
    if (s >= 0)
    {
      s += fy2 * (1 - x);
      x--;
    }
    s += rx2 * ((4 * y) + 6);
  }

  inTransaction = lockTransaction;
  end_tft_write();               
}






void TFT_eSPI::fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color)
{
  if (rx<2) return;
  if (ry<2) return;
  int32_t x, y;
  int32_t rx2 = rx * rx;
  int32_t ry2 = ry * ry;
  int32_t fx2 = 4 * rx2;
  int32_t fy2 = 4 * ry2;
  int32_t s;

   
  inTransaction = true;

  for (x = 0, y = ry, s = 2*ry2+rx2*(1-2*ry); ry2*x <= rx2*y; x++) {
    drawFastHLine(x0 - x, y0 - y, x + x + 1, color);
    drawFastHLine(x0 - x, y0 + y, x + x + 1, color);

    if (s >= 0) {
      s += fx2 * (1 - y);
      y--;
    }
    s += ry2 * ((4 * x) + 6);
  }

  for (x = rx, y = 0, s = 2*rx2+ry2*(1-2*rx); rx2*y <= ry2*x; y++) {
    drawFastHLine(x0 - x, y0 - y, x + x + 1, color);
    drawFastHLine(x0 - x, y0 + y, x + x + 1, color);

    if (s >= 0) {
      s += fy2 * (1 - x);
      x--;
    }
    s += rx2 * ((4 * y) + 6);
  }

  inTransaction = lockTransaction;
  end_tft_write();               
}






void TFT_eSPI::fillScreen(uint32_t color)
{
  fillRect(0, 0, _width, _height, color);
}






 
void TFT_eSPI::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
   
  inTransaction = true;

  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
   
  drawFastVLine(x, y+1, h-2, color);
  drawFastVLine(x + w - 1, y+1, h-2, color);

  inTransaction = lockTransaction;
  end_tft_write();               
}






 
void TFT_eSPI::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color)
{
   
  inTransaction = true;

   
  drawFastHLine(x + r  , y    , w - r - r, color);  
  drawFastHLine(x + r  , y + h - 1, w - r - r, color);  
  drawFastVLine(x    , y + r  , h - r - r, color);  
  drawFastVLine(x + w - 1, y + r  , h - r - r, color);  
   
  drawCircleHelper(x + r    , y + r    , r, 1, color);
  drawCircleHelper(x + w - r - 1, y + r    , r, 2, color);
  drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
  drawCircleHelper(x + r    , y + h - r - 1, r, 8, color);

  inTransaction = lockTransaction;
  end_tft_write();               
}






 
void TFT_eSPI::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color)
{
   
  inTransaction = true;

   
  fillRect(x, y + r, w, h - r - r, color);

   
  fillCircleHelper(x + r, y + h - r - 1, r, 1, w - r - r - 1, color);
  fillCircleHelper(x + r    , y + r, r, 2, w - r - r - 1, color);

  inTransaction = lockTransaction;
  end_tft_write();               
}






 
void TFT_eSPI::drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color)
{
   
  inTransaction = true;

  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);

  inTransaction = lockTransaction;
  end_tft_write();               
}






 
void TFT_eSPI::fillTriangle ( int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color)
{
  int32_t a, b, y, last;

   
  if (y0 > y1) {
    transpose(y0, y1); transpose(x0, x1);
  }
  if (y1 > y2) {
    transpose(y2, y1); transpose(x2, x1);
  }
  if (y0 > y1) {
    transpose(y0, y1); transpose(x0, x1);
  }

  if (y0 == y2) {  
    a = b = x0;
    if (x1 < a)      a = x1;
    else if (x1 > b) b = x1;
    if (x2 < a)      a = x2;
    else if (x2 > b) b = x2;
    drawFastHLine(a, y0, b - a + 1, color);
    return;
  }

   
  inTransaction = true;

  int32_t
  dx01 = x1 - x0,
  dy01 = y1 - y0,
  dx02 = x2 - x0,
  dy02 = y2 - y0,
  dx12 = x2 - x1,
  dy12 = y2 - y1,
  sa   = 0,
  sb   = 0;

   
   
   
   
   
   
  if (y1 == y2) last = y1;   
  else         last = y1 - 1;  

  for (y = y0; y <= last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;

    if (a > b) transpose(a, b);
    drawFastHLine(a, y, b - a + 1, color);
  }

   
   
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for (; y <= y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;

    if (a > b) transpose(a, b);
    drawFastHLine(a, y, b - a + 1, color);
  }

  inTransaction = lockTransaction;
  end_tft_write();               
}






void TFT_eSPI::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color)
{
   
  inTransaction = true;

  int32_t i, j, byteWidth = (w + 7) / 8;

  for (j = 0; j < h; j++) {
    for (i = 0; i < w; i++ ) {
      if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        drawPixel(x + i, y + j, color);
      }
    }
  }

  inTransaction = lockTransaction;
  end_tft_write();               
}






void TFT_eSPI::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor, uint16_t bgcolor)
{
   
  inTransaction = true;

  int32_t i, j, byteWidth = (w + 7) / 8;

  for (j = 0; j < h; j++) {
    for (i = 0; i < w; i++ ) {
      if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7)))
           drawPixel(x + i, y + j, fgcolor);
      else drawPixel(x + i, y + j, bgcolor);
    }
  }

  inTransaction = lockTransaction;
  end_tft_write();               
}





void TFT_eSPI::drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color)
{
   
  inTransaction = true;

  int32_t i, j, byteWidth = (w + 7) / 8;

  for (j = 0; j < h; j++) {
    for (i = 0; i < w; i++ ) {
      if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (1 << (i & 7))) {
        drawPixel(x + i, y + j, color);
      }
    }
  }

  inTransaction = lockTransaction;
  end_tft_write();               
}






void TFT_eSPI::drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bgcolor)
{
   
  inTransaction = true;

  int32_t i, j, byteWidth = (w + 7) / 8;

  for (j = 0; j < h; j++) {
    for (i = 0; i < w; i++ ) {
      if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (1 << (i & 7)))
           drawPixel(x + i, y + j,   color);
      else drawPixel(x + i, y + j, bgcolor);
    }
  }

  inTransaction = lockTransaction;
  end_tft_write();               
}






void TFT_eSPI::setCursor(int16_t x, int16_t y)
{
  cursor_x = x;
  cursor_y = y;
}






void TFT_eSPI::setCursor(int16_t x, int16_t y, uint8_t font)
{
  setTextFont(font);
  cursor_x = x;
  cursor_y = y;
}






int16_t TFT_eSPI::getCursorX(void)
{
  return cursor_x;
}





int16_t TFT_eSPI::getCursorY(void)
{
  return cursor_y;
}






void TFT_eSPI::setTextSize(uint8_t s)
{
  if (s>7) s = 7;  
  textsize = (s > 0) ? s : 1;  
}






void TFT_eSPI::setTextColor(uint16_t c)
{
   
   
  textcolor = textbgcolor = c;
}






 
 
 
void TFT_eSPI::setTextColor(uint16_t c, uint16_t b, bool bgfill)
{
  textcolor   = c;
  textbgcolor = b;
  _fillbg     = bgfill;
}






void TFT_eSPI::setPivot(int16_t x, int16_t y)
{
  _xPivot = x;
  _yPivot = y;
}






int16_t TFT_eSPI::getPivotX(void)
{
  return _xPivot;
}






int16_t TFT_eSPI::getPivotY(void)
{
  return _yPivot;
}






void TFT_eSPI::setBitmapColor(uint16_t c, uint16_t b)
{
  if (c == b) b = ~c;
  bitmap_fg = c;
  bitmap_bg = b;
}






void TFT_eSPI::setTextWrap(bool wrapX, bool wrapY)
{
  textwrapX = wrapX;
  textwrapY = wrapY;
}






void TFT_eSPI::setTextDatum(uint8_t d)
{
  textdatum = d;
}






void TFT_eSPI::setTextPadding(uint16_t x_width)
{
  padX = x_width;
}





uint16_t TFT_eSPI::getTextPadding(void)
{
  return padX;
}





uint8_t TFT_eSPI::getTextDatum(void)
{
  return textdatum;
}






 
int16_t TFT_eSPI::width(void)
{
  if (_vpDatum) return _xWidth;
  return _width;
}






int16_t TFT_eSPI::height(void)
{
  if (_vpDatum) return _yHeight;
  return _height;
}






int16_t TFT_eSPI::textWidth(const String& string)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return textWidth(buffer, textfont);
}

int16_t TFT_eSPI::textWidth(const String& string, uint8_t font)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return textWidth(buffer, font);
}

int16_t TFT_eSPI::textWidth(const char *string)
{
  return textWidth(string, textfont);
}

int16_t TFT_eSPI::textWidth(const char *string, uint8_t font)
{
  int32_t str_width = 0;
  uint16_t uniCode  = 0;

#ifdef SMOOTH_FONT
  if(fontLoaded) {
    while (*string) {
      uniCode = decodeUTF8(*string++);
      if (uniCode) {
        if (uniCode == 0x20) str_width += gFont.spaceWidth;
        else {
          uint16_t gNum = 0;
          bool found = getUnicodeIndex(uniCode, &gNum);
          if (found) {
            if(str_width == 0 && gdX[gNum] < 0) str_width -= gdX[gNum];
            if (*string || isDigits) str_width += gxAdvance[gNum];
            else str_width += (gdX[gNum] + gWidth[gNum]);
          }
          else str_width += gFont.spaceWidth + 1;
        }
      }
    }
    isDigits = false;
    return str_width;
  }
#endif

  if (font>1 && font<9) {
    char *widthtable = (char *)pgm_read_dword( &(fontdata[font].widthtbl ) ) - 32;  

    while (*string) {
      uniCode = *(string++);
      if (uniCode > 31 && uniCode < 128)
      str_width += pgm_read_byte( widthtable + uniCode);  
      else str_width += pgm_read_byte( widthtable + 32);  
    }

  }
  else {

#ifdef LOAD_GFXFF
    if(gfxFont) {  
      while (*string) {
        uniCode = decodeUTF8(*string++);
        if ((uniCode >= pgm_read_word(&gfxFont->first)) && (uniCode <= pgm_read_word(&gfxFont->last ))) {
          uniCode -= pgm_read_word(&gfxFont->first);
          GFXglyph *glyph  = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[uniCode]);
           
          if (*string  || isDigits) str_width += pgm_read_byte(&glyph->xAdvance);
           
          else str_width += ((int8_t)pgm_read_byte(&glyph->xOffset) + pgm_read_byte(&glyph->width));
        }
      }
    }
    else
#endif
    {
#ifdef LOAD_GLCD
      while (*string++) str_width += 6;
#endif
    }
  }
  isDigits = false;
  return str_width * textsize;
}






 
uint16_t TFT_eSPI::fontsLoaded(void)
{
  return fontsloaded;
}






int16_t TFT_eSPI::fontHeight(uint8_t font)
{
  if (font > 8) return 0;

#ifdef SMOOTH_FONT
  if(fontLoaded) return gFont.yAdvance;
#endif

#ifdef LOAD_GFXFF
  if (font==1) {
    if(gfxFont) {  
      return pgm_read_byte(&gfxFont->yAdvance) * textsize;
    }
  }
#endif
  return pgm_read_byte( &fontdata[font].height ) * textsize;
}

int16_t TFT_eSPI::fontHeight(void)
{
  return fontHeight(textfont);
}





void TFT_eSPI::drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size)
{
  if (_vpOoB) return;

#ifdef LOAD_GLCD
 
  #ifdef LOAD_GFXFF
  if(!gfxFont) {  
  #endif
 

  int32_t xd = x + _xDatum;
  int32_t yd = y + _yDatum;

  if ((xd >= _vpW)                 ||  
     ( yd >= _vpH)                 ||  
     ((xd + 6 * size - 1) < _vpX)  ||  
     ((yd + 8 * size - 1) < _vpY))     
    return;

  if (c > 255) return;
  if (!_cp437 && c > 175) c++;

  bool fillbg = (bg != color);
  bool clip = xd < _vpX || xd + 6  * textsize >= _vpW || yd < _vpY || yd + 8 * textsize >= _vpH;

  if ((size==1) && fillbg && !clip) {
    uint8_t column[6];
    uint8_t mask = 0x1;
    begin_tft_write();

    setWindow(xd, yd, xd+5, yd+7);

    for (int8_t i = 0; i < 5; i++ ) column[i] = pgm_read_byte(&font[0] + (c * 5) + i);
    column[5] = 0;

    for (int8_t j = 0; j < 8; j++) {
      for (int8_t k = 0; k < 5; k++ ) {
        if (column[k] & mask) {tft_Write_16(color);}
        else {tft_Write_16(bg);}
      }
      mask <<= 1;
      tft_Write_16(bg);
    }

    end_tft_write();
  }
  else {
     
    inTransaction = true;

    for (int8_t i = 0; i < 6; i++ ) {
      uint8_t line;
      if (i == 5)
        line = 0x0;
      else
        line = pgm_read_byte(&font[0] + (c * 5) + i);

      if (size == 1 && !fillbg) {  
        for (int8_t j = 0; j < 8; j++) {
          if (line & 0x1) drawPixel(x + i, y + j, color);
          line >>= 1;
        }
      }
      else {   
        for (int8_t j = 0; j < 8; j++) {
          if (line & 0x1) fillRect(x + (i * size), y + (j * size), size, size, color);
          else if (fillbg) fillRect(x + i * size, y + j * size, size, size, bg);
          line >>= 1;
        }
      }
    }
    inTransaction = lockTransaction;
    end_tft_write();               
  }

 
  #ifdef LOAD_GFXFF
  } else {  
  #endif
 
#endif  

#ifdef LOAD_GFXFF
     
    if ((c >= pgm_read_word(&gfxFont->first)) && (c <= pgm_read_word(&gfxFont->last ))) {
       
      inTransaction = true;
 

      c -= pgm_read_word(&gfxFont->first);
      GFXglyph *glyph  = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c]);
      uint8_t  *bitmap = (uint8_t *)pgm_read_dword(&gfxFont->bitmap);

      uint32_t bo = pgm_read_word(&glyph->bitmapOffset);
      uint8_t  w  = pgm_read_byte(&glyph->width),
               h  = pgm_read_byte(&glyph->height);
                
      int8_t   xo = pgm_read_byte(&glyph->xOffset),
               yo = pgm_read_byte(&glyph->yOffset);
      uint8_t  xx, yy, bits=0, bit=0;
      int16_t  xo16 = 0, yo16 = 0;

      if(size > 1) {
        xo16 = xo;
        yo16 = yo;
      }

       
      uint16_t hpc = 0;  
      for(yy=0; yy<h; yy++) {
        for(xx=0; xx<w; xx++) {
          if(bit == 0) {
            bits = pgm_read_byte(&bitmap[bo++]);
            bit  = 0x80;
          }
          if(bits & bit) hpc++;
          else {
           if (hpc) {
              if(size == 1) drawFastHLine(x+xo+xx-hpc, y+yo+yy, hpc, color);
              else fillRect(x+(xo16+xx-hpc)*size, y+(yo16+yy)*size, size*hpc, size, color);
              hpc=0;
            }
          }
          bit >>= 1;
        }
         
        if (hpc) {
          if(size == 1) drawFastHLine(x+xo+xx-hpc, y+yo+yy, hpc, color);
          else fillRect(x+(xo16+xx-hpc)*size, y+(yo16+yy)*size, size*hpc, size, color);
          hpc=0;
        }
      }

      inTransaction = lockTransaction;
      end_tft_write();               
    }
#endif

#ifdef LOAD_GLCD
  #ifdef LOAD_GFXFF
  }  
  #endif
#else
  #ifndef LOAD_GFXFF
     
    x = x;
    y = y;
    color = color;
    bg = bg;
    size = size;
  #endif
#endif

}






 
void TFT_eSPI::setAddrWindow(int32_t x0, int32_t y0, int32_t w, int32_t h)
{
  begin_tft_write();

  setWindow(x0, y0, x0 + w - 1, y0 + h - 1);

  end_tft_write();
}






 
void TFT_eSPI::setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
   
  addr_row = 0xFFFF;
  addr_col = 0xFFFF;

#if defined (ILI9225_DRIVER)
  if (rotation & 0x01) { transpose(x0, y0); transpose(x1, y1); }
  SPI_BUSY_CHECK;
  DC_C; tft_Write_8(TFT_CASET1);
  DC_D; tft_Write_16(x0);
  DC_C; tft_Write_8(TFT_CASET2);
  DC_D; tft_Write_16(x1);

  DC_C; tft_Write_8(TFT_PASET1);
  DC_D; tft_Write_16(y0);
  DC_C; tft_Write_8(TFT_PASET2);
  DC_D; tft_Write_16(y1);

  DC_C; tft_Write_8(TFT_RAM_ADDR1);
  DC_D; tft_Write_16(x0);
  DC_C; tft_Write_8(TFT_RAM_ADDR2);
  DC_D; tft_Write_16(y0);

   
  DC_C; tft_Write_8(TFT_RAMWR);
  DC_D;
   
  #if (defined(ARDUINO_ARCH_RP2040)  || defined (ARDUINO_ARCH_MBED)) && !defined(RP2040_PIO_INTERFACE)
     
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
    hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
  #endif
#elif defined (SSD1351_DRIVER)
  if (rotation & 1) {
    transpose(x0, y0);
    transpose(x1, y1);
  }
  SPI_BUSY_CHECK;
  DC_C; tft_Write_8(TFT_CASET);
  DC_D; tft_Write_16(x1 | (x0 << 8));
  DC_C; tft_Write_8(TFT_PASET);
  DC_D; tft_Write_16(y1 | (y0 << 8));
  DC_C; tft_Write_8(TFT_RAMWR);
  DC_D;
#else
  #if defined (SSD1963_DRIVER)
    if ((rotation & 0x1) == 0) { transpose(x0, y0); transpose(x1, y1); }
  #endif

  #ifdef CGRAM_OFFSET
    x0+=colstart;
    x1+=colstart;
    y0+=rowstart;
    y1+=rowstart;
  #endif

   
  #if (defined(ARDUINO_ARCH_RP2040)  || defined (ARDUINO_ARCH_MBED))
    #if !defined(RP2040_PIO_INTERFACE)
       
       
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      DC_C;
      #if !defined (SPI_18BIT_DRIVER)
        #if  defined (RPI_DISPLAY_TYPE)  
          hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
        #else
          hw_write_masked(&spi_get_hw(SPI_X)->cr0, (8 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
        #endif
      #endif
      spi_get_hw(SPI_X)->dr = (uint32_t)TFT_CASET;

      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      DC_D;
      spi_get_hw(SPI_X)->dr = (uint32_t)x0>>8;
      spi_get_hw(SPI_X)->dr = (uint32_t)x0;
      spi_get_hw(SPI_X)->dr = (uint32_t)x1>>8;
      spi_get_hw(SPI_X)->dr = (uint32_t)x1;

      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      DC_C;
      spi_get_hw(SPI_X)->dr = (uint32_t)TFT_PASET;

      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      DC_D;
      spi_get_hw(SPI_X)->dr = (uint32_t)y0>>8;
      spi_get_hw(SPI_X)->dr = (uint32_t)y0;
      spi_get_hw(SPI_X)->dr = (uint32_t)y1>>8;
      spi_get_hw(SPI_X)->dr = (uint32_t)y1;

      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      DC_C;
      spi_get_hw(SPI_X)->dr = (uint32_t)TFT_RAMWR;

      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      #if !defined (SPI_18BIT_DRIVER)
        hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
      #endif
      DC_D;
    #elif defined (RM68120_DRIVER)
      DC_C; tft_Write_16(TFT_CASET+0); DC_D; tft_Write_16(x0 >> 8);
      DC_C; tft_Write_16(TFT_CASET+1); DC_D; tft_Write_16(x0 & 0xFF);
      DC_C; tft_Write_16(TFT_CASET+2); DC_D; tft_Write_16(x1 >> 8);
      DC_C; tft_Write_16(TFT_CASET+3); DC_D; tft_Write_16(x1 & 0xFF);
      DC_C; tft_Write_16(TFT_PASET+0); DC_D; tft_Write_16(y0 >> 8);
      DC_C; tft_Write_16(TFT_PASET+1); DC_D; tft_Write_16(y0 & 0xFF);
      DC_C; tft_Write_16(TFT_PASET+2); DC_D; tft_Write_16(y1 >> 8);
      DC_C; tft_Write_16(TFT_PASET+3); DC_D; tft_Write_16(y1 & 0xFF);

      DC_C; tft_Write_16(TFT_RAMWR);
      DC_D;
    #else
       
      WAIT_FOR_STALL;
      tft_pio->sm[pio_sm].instr = pio_instr_addr;

      TX_FIFO = TFT_CASET;
      TX_FIFO = (x0<<16) | x1;
      TX_FIFO = TFT_PASET;
      TX_FIFO = (y0<<16) | y1;
      TX_FIFO = TFT_RAMWR;
    #endif
  #else
    SPI_BUSY_CHECK;
    DC_C; tft_Write_8(TFT_CASET);
    DC_D; tft_Write_32C(x0, x1);
    DC_C; tft_Write_8(TFT_PASET);
    DC_D; tft_Write_32C(y0, y1);
    DC_C; tft_Write_8(TFT_RAMWR);
    DC_D;
  #endif  
#endif
   
}





void TFT_eSPI::readAddrWindow(int32_t xs, int32_t ys, int32_t w, int32_t h)
{
   

  int32_t xe = xs + w - 1;
  int32_t ye = ys + h - 1;

  addr_col = 0xFFFF;
  addr_row = 0xFFFF;

#if defined (SSD1963_DRIVER)
  if ((rotation & 0x1) == 0) { transpose(xs, ys); transpose(xe, ye); }
#endif

#ifdef CGRAM_OFFSET
  xs += colstart;
  xe += colstart;
  ys += rowstart;
  ye += rowstart;
#endif

   
#if (defined(ARDUINO_ARCH_RP2040)  || defined (ARDUINO_ARCH_MBED)) && !defined(RP2040_PIO_INTERFACE)
   
   
  while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
  DC_C;
  hw_write_masked(&spi_get_hw(SPI_X)->cr0, (8 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
  spi_get_hw(SPI_X)->dr = (uint32_t)TFT_CASET;

  while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
  DC_D;
  spi_get_hw(SPI_X)->dr = (uint32_t)xs>>8;
  spi_get_hw(SPI_X)->dr = (uint32_t)xs;
  spi_get_hw(SPI_X)->dr = (uint32_t)xe>>8;
  spi_get_hw(SPI_X)->dr = (uint32_t)xe;

  while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
  DC_C;
  spi_get_hw(SPI_X)->dr = (uint32_t)TFT_PASET;

  while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
  DC_D;
  spi_get_hw(SPI_X)->dr = (uint32_t)ys>>8;
  spi_get_hw(SPI_X)->dr = (uint32_t)ys;
  spi_get_hw(SPI_X)->dr = (uint32_t)ye>>8;
  spi_get_hw(SPI_X)->dr = (uint32_t)ye;

  while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
  DC_C;
  spi_get_hw(SPI_X)->dr = (uint32_t)TFT_RAMRD;

  while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
  DC_D;

   
  while (spi_is_readable(SPI_X)) (void)spi_get_hw(SPI_X)->dr;
  spi_get_hw(SPI_X)->icr = SPI_SSPICR_RORIC_BITS;

#else
   
  DC_C; tft_Write_8(TFT_CASET);
  DC_D; tft_Write_32C(xs, xe);

   
  DC_C; tft_Write_8(TFT_PASET);
  DC_D; tft_Write_32C(ys, ye);

   
  DC_C; tft_Write_8(TFT_RAMRD);

  DC_D;
#endif  

   
}






void TFT_eSPI::drawPixel(int32_t x, int32_t y, uint32_t color)
{
  if (_vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((x < _vpX) || (y < _vpY) ||(x >= _vpW) || (y >= _vpH)) return;

#ifdef CGRAM_OFFSET
  x+=colstart;
  y+=rowstart;
#endif

#if (defined (MULTI_TFT_SUPPORT) || defined (GC9A01_DRIVER)) && !defined (ILI9225_DRIVER)
  addr_row = 0xFFFF;
  addr_col = 0xFFFF;
#endif

  begin_tft_write();

#if defined (ILI9225_DRIVER)
  if (rotation & 0x01) { transpose(x, y); }
  SPI_BUSY_CHECK;

   
  if (addr_row != 0x9225) {
    addr_row = 0x9225;  
    DC_C; tft_Write_8(TFT_CASET1);
    DC_D; tft_Write_16(0);
    DC_C; tft_Write_8(TFT_CASET2);
    DC_D; tft_Write_16(175);

    DC_C; tft_Write_8(TFT_PASET1);
    DC_D; tft_Write_16(0);
    DC_C; tft_Write_8(TFT_PASET2);
    DC_D; tft_Write_16(219);
  }

   
  DC_C; tft_Write_8(TFT_RAM_ADDR1);
  DC_D; tft_Write_16(x);
  DC_C; tft_Write_8(TFT_RAM_ADDR2);
  DC_D; tft_Write_16(y);

   
  DC_C; tft_Write_8(TFT_RAMWR);
  #if defined(TFT_PARALLEL_8_BIT) || defined(TFT_PARALLEL_16_BIT) || !defined(ESP32)
    DC_D; tft_Write_16(color);
  #else
    DC_D; tft_Write_16N(color);
  #endif

 
#elif (defined (ARDUINO_ARCH_RP2040) || defined (ARDUINO_ARCH_MBED)) && !defined (SSD1351_DRIVER)

  #if defined (SSD1963_DRIVER)
    if ((rotation & 0x1) == 0) { transpose(x, y); }
  #endif

  #if !defined(RP2040_PIO_INTERFACE)
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};

    #if  defined (RPI_DISPLAY_TYPE)  
      hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
    #else
      hw_write_masked(&spi_get_hw(SPI_X)->cr0, (8 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
    #endif

    if (addr_col != x) {
      DC_C;
      spi_get_hw(SPI_X)->dr = (uint32_t)TFT_CASET;
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS){};
      DC_D;
      spi_get_hw(SPI_X)->dr = (uint32_t)x>>8;
      spi_get_hw(SPI_X)->dr = (uint32_t)x;
      spi_get_hw(SPI_X)->dr = (uint32_t)x>>8;
      spi_get_hw(SPI_X)->dr = (uint32_t)x;
      addr_col = x;
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
    }

    if (addr_row != y) {
      DC_C;
      spi_get_hw(SPI_X)->dr = (uint32_t)TFT_PASET;
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      DC_D;
      spi_get_hw(SPI_X)->dr = (uint32_t)y>>8;
      spi_get_hw(SPI_X)->dr = (uint32_t)y;
      spi_get_hw(SPI_X)->dr = (uint32_t)y>>8;
      spi_get_hw(SPI_X)->dr = (uint32_t)y;
      addr_row = y;
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
    }

    DC_C;
    spi_get_hw(SPI_X)->dr = (uint32_t)TFT_RAMWR;

    #if defined (SPI_18BIT_DRIVER)  
      uint8_t r = (color & 0xF800)>>8;
      uint8_t g = (color & 0x07E0)>>3;
      uint8_t b = (color & 0x001F)<<3;
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      DC_D;
      tft_Write_8N(r); tft_Write_8N(g); tft_Write_8N(b);
    #else
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      DC_D;
      #if  defined (RPI_DISPLAY_TYPE)  
        spi_get_hw(SPI_X)->dr = (uint32_t)color;
      #else
        spi_get_hw(SPI_X)->dr = (uint32_t)color>>8;
        spi_get_hw(SPI_X)->dr = (uint32_t)color;
      #endif
    #endif
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
  #elif defined (RM68120_DRIVER)
    if (addr_col != x) {
      DC_C; tft_Write_16(TFT_CASET+0); DC_D; tft_Write_16(x >> 8);
      DC_C; tft_Write_16(TFT_CASET+1); DC_D; tft_Write_16(x & 0xFF);
      DC_C; tft_Write_16(TFT_CASET+2); DC_D; tft_Write_16(x >> 8);
      DC_C; tft_Write_16(TFT_CASET+3); DC_D; tft_Write_16(x & 0xFF);
      addr_col = x;
    }
    if (addr_row != y) {
      DC_C; tft_Write_16(TFT_PASET+0); DC_D; tft_Write_16(y >> 8);
      DC_C; tft_Write_16(TFT_PASET+1); DC_D; tft_Write_16(y & 0xFF);
      DC_C; tft_Write_16(TFT_PASET+2); DC_D; tft_Write_16(y >> 8);
      DC_C; tft_Write_16(TFT_PASET+3); DC_D; tft_Write_16(y & 0xFF);
      addr_row = y;
    }
    DC_C; tft_Write_16(TFT_RAMWR); DC_D;

    TX_FIFO = color;
  #else
     
    WAIT_FOR_STALL;
    tft_pio->sm[pio_sm].instr = pio_instr_addr;
    TX_FIFO = TFT_CASET;
    TX_FIFO = (x<<16) | x;
    TX_FIFO = TFT_PASET;
    TX_FIFO = (y<<16) | y;
    TX_FIFO = TFT_RAMWR;
     
    #if  defined (SPI_18BIT_DRIVER) || (defined (SSD1963_DRIVER) && defined (TFT_PARALLEL_8_BIT))
      TX_FIFO = ((color & 0xF800)<<8) | ((color & 0x07E0)<<5) | ((color & 0x001F)<<3);
    #else
      TX_FIFO = color;
    #endif

  #endif

#else

  #if defined (SSD1963_DRIVER)
    if ((rotation & 0x1) == 0) { transpose(x, y); }
  #endif

    SPI_BUSY_CHECK;

  #if defined (SSD1351_DRIVER)
    if (rotation & 0x1) { transpose(x, y); }
     
    if (addr_col != x) {
      DC_C; tft_Write_8(TFT_CASET);
      DC_D; tft_Write_16(x | (x << 8));
      addr_col = x;
    }

     
    if (addr_row != y) {
      DC_C; tft_Write_8(TFT_PASET);
      DC_D; tft_Write_16(y | (y << 8));
      addr_row = y;
    }
  #else
     
    if (addr_col != x) {
      DC_C; tft_Write_8(TFT_CASET);
      DC_D; tft_Write_32D(x);
      addr_col = x;
    }

     
    if (addr_row != y) {
      DC_C; tft_Write_8(TFT_PASET);
      DC_D; tft_Write_32D(y);
      addr_row = y;
    }
  #endif

  DC_C; tft_Write_8(TFT_RAMWR);

  #if defined(TFT_PARALLEL_8_BIT) || defined(TFT_PARALLEL_16_BIT) || !defined(ESP32)
    DC_D; tft_Write_16(color);
  #else
    DC_D; tft_Write_16N(color);
  #endif
#endif

  end_tft_write();
}





void TFT_eSPI::pushColor(uint16_t color)
{
  begin_tft_write();

  SPI_BUSY_CHECK;
  tft_Write_16N(color);

  end_tft_write();
}






void TFT_eSPI::pushColor(uint16_t color, uint32_t len)
{
  begin_tft_write();

  pushBlock(color, len);

  end_tft_write();
}





void TFT_eSPI::startWrite(void)
{
  begin_tft_write();
  lockTransaction = true;  
  inTransaction = true;
}





void TFT_eSPI::endWrite(void)
{
  lockTransaction = false;  
  inTransaction = false;
  DMA_BUSY_CHECK;           
  end_tft_write();          
}





void TFT_eSPI::writeColor(uint16_t color, uint32_t len)
{
  pushBlock(color, len);
}





 
 
void TFT_eSPI::pushColors(uint8_t *data, uint32_t len)
{
  begin_tft_write();

  pushPixels(data, len>>1);

  end_tft_write();
}






void TFT_eSPI::pushColors(uint16_t *data, uint32_t len, bool swap)
{
  begin_tft_write();
  if (swap) {swap = _swapBytes; _swapBytes = true; }

  pushPixels(data, len);

  _swapBytes = swap;  
  end_tft_write();
}






 
 
void TFT_eSPI::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
  if (_vpOoB) return;

   
  inTransaction = true;

   
   

  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    transpose(x0, y0);
    transpose(x1, y1);
  }

  if (x0 > x1) {
    transpose(x0, x1);
    transpose(y0, y1);
  }

  int32_t dx = x1 - x0, dy = abs(y1 - y0);;

  int32_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

  if (y0 < y1) ystep = 1;

   
  if (steep) {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        if (dlen == 1) drawPixel(y0, xs, color);
        else drawFastVLine(y0, xs, dlen, color);
        dlen = 0;
        y0 += ystep; xs = x0 + 1;
        err += dx;
      }
    }
    if (dlen) drawFastVLine(y0, xs, dlen, color);
  }
  else
  {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        if (dlen == 1) drawPixel(xs, y0, color);
        else drawFastHLine(xs, y0, dlen, color);
        dlen = 0;
        y0 += ystep; xs = x0 + 1;
        err += dx;
      }
    }
    if (dlen) drawFastHLine(xs, y0, dlen, color);
  }

  inTransaction = lockTransaction;
  end_tft_write();
}





constexpr float PixelAlphaGain   = 255.0;
constexpr float LoAlphaTheshold  = 1.0/32.0;
constexpr float HiAlphaTheshold  = 1.0 - LoAlphaTheshold;
constexpr float deg2rad      = 3.14159265359/180.0;





uint16_t TFT_eSPI::drawPixel(int32_t x, int32_t y, uint32_t color, uint8_t alpha, uint32_t bg_color)
{
  if (bg_color == 0x00FFFFFF) bg_color = readPixel(x, y);
  color = fastBlend(alpha, color, bg_color);
  drawPixel(x, y, color);
  return color;
}






void TFT_eSPI::drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle, uint32_t fg_color, uint32_t bg_color, bool roundEnds)
 
 
 
 
 
 
{
  inTransaction = true;

  if (endAngle != startAngle && (startAngle != 0 || endAngle != 360))
  {
    float sx = -sinf(startAngle * deg2rad);
    float sy = +cosf(startAngle * deg2rad);
    float ex = -sinf(  endAngle * deg2rad);
    float ey = +cosf(  endAngle * deg2rad);

    if (roundEnds)
    {  
      sx = sx * (r + ir)/2.0 + x;
      sy = sy * (r + ir)/2.0 + y;
      drawSpot(sx, sy, (r - ir)/2.0, fg_color, bg_color);

      ex = ex * (r + ir)/2.0 + x;
      ey = ey * (r + ir)/2.0 + y;
      drawSpot(ex, ey, (r - ir)/2.0, fg_color, bg_color);
    }
    else
    {  
      float asx = sx * ir + x;
      float asy = sy * ir + y;
      float aex = sx *  r + x;
      float aey = sy *  r + y;
      drawWedgeLine(asx, asy, aex, aey, 0.3, 0.3, fg_color, bg_color);

      asx = ex * ir + x;
      asy = ey * ir + y;
      aex = ex *  r + x;
      aey = ey *  r + y;
      drawWedgeLine(asx, asy, aex, aey, 0.3, 0.3, fg_color, bg_color);
    }

     
    drawArc(x, y, r, ir, startAngle, endAngle, fg_color, bg_color);

  }
  else  
  {
    drawArc(x, y, r, ir, 0, 360, fg_color, bg_color);
  }

  inTransaction = lockTransaction;
  end_tft_write();
}





 
 
 
inline uint8_t TFT_eSPI::sqrt_fraction(uint32_t num) {
  if (num > (0x40000000)) return 0;
  uint32_t bsh = 0x00004000;
  uint32_t fpr = 0;
  uint32_t osh = 0;

   
  while (num>bsh) {bsh <<= 2; osh++;}

  do {
    uint32_t bod = bsh + fpr;
    if(num >= bod)
    {
      num -= bod;
      fpr = bsh + bod;
    }
    num <<= 1;
  } while(bsh >>= 1);

  return fpr>>osh;
}





 
 
 
 
 
 
void TFT_eSPI::drawArc(int32_t x, int32_t y, int32_t r, int32_t ir,
                       uint32_t startAngle, uint32_t endAngle,
                       uint32_t fg_color, uint32_t bg_color,
                       bool smooth)
{
  if (endAngle   > 360)   endAngle = 360;
  if (startAngle > 360) startAngle = 360;
  if (_vpOoB || startAngle == endAngle) return;
  if (r < ir) transpose(r, ir);   
  if (r <= 0 || ir < 0) return;   

  if (endAngle < startAngle) {
     
    if (startAngle < 360) drawArc(x, y, r, ir, startAngle, 360, fg_color, bg_color, smooth);
    if (endAngle == 0) return;
    startAngle = 0;
  }
  inTransaction = true;

  int32_t xs = 0;         
  uint8_t alpha = 0;      

  uint32_t r2 = r * r;    
  if (smooth) r++;        
  uint32_t r1 = r * r;    
  int16_t w  = r - ir;    
  uint32_t r3 = ir * ir;  
  if (smooth) ir--;       
  uint32_t r4 = ir * ir;  

   
   
   
   
  uint32_t startSlope[4] = {0, 0, 0xFFFFFFFF, 0};
  uint32_t   endSlope[4] = {0, 0xFFFFFFFF, 0, 0};

   
  constexpr float minDivisor = 1.0f/0x8000;

   
  float fabscos = fabsf(cosf(startAngle * deg2rad));
  float fabssin = fabsf(sinf(startAngle * deg2rad));

   
  uint32_t slope = (fabscos/(fabssin + minDivisor)) * (float)(1UL<<16);

   
  if (startAngle <= 90) {
    startSlope[0] =  slope;
  }
  else if (startAngle <= 180) {
    startSlope[1] =  slope;
  }
  else if (startAngle <= 270) {
    startSlope[1] = 0xFFFFFFFF;
    startSlope[2] = slope;
  }
  else {
    startSlope[1] = 0xFFFFFFFF;
    startSlope[2] =  0;
    startSlope[3] = slope;
  }

   
  fabscos  = fabsf(cosf(endAngle * deg2rad));
  fabssin  = fabsf(sinf(endAngle * deg2rad));

   
  slope   = (uint32_t)((fabscos/(fabssin + minDivisor)) * (float)(1UL<<16));

   
  if (endAngle <= 90) {
    endSlope[0] = slope;
    endSlope[1] =  0;
    startSlope[2] =  0;
  }
  else if (endAngle <= 180) {
    endSlope[1] = slope;
    startSlope[2] =  0;
  }
  else if (endAngle <= 270) {
    endSlope[2] =  slope;
  }
  else {
    endSlope[3] =  slope;
  }

   
  for (int32_t cy = r - 1; cy > 0; cy--)
  {
    uint32_t len[4] = { 0,  0,  0,  0};  
    int32_t  xst[4] = {-1, -1, -1, -1};  
    uint32_t dy2 = (r - cy) * (r - cy);

     
    while ((r - xs) * (r - xs) + dy2 >= r1) xs++;

    for (int32_t cx = xs; cx < r; cx++)
    {
       
      uint32_t hyp = (r - cx) * (r - cx) + dy2;

       
      if (hyp > r2) {
        alpha = ~sqrt_fraction(hyp);  
      }
       
      else if (hyp >= r3) {
         
        slope = ((r - cy) << 16)/(r - cx);
        if (slope <= startSlope[0] && slope >= endSlope[0]) {  
          xst[0] = cx;  
          len[0]++;
        }
        if (slope >= startSlope[1] && slope <= endSlope[1]) {  
          xst[1] = cx;  
          len[1]++;
        }
        if (slope <= startSlope[2] && slope >= endSlope[2]) {  
          xst[2] = cx;  
          len[2]++;
        }
        if (slope <= endSlope[3] && slope >= startSlope[3]) {  
          xst[3] = cx;  
          len[3]++;
        }
        continue;  
      }
      else {
        if (hyp <= r4) break;   
        alpha = sqrt_fraction(hyp);  
      }

      if (alpha < 16) continue;   

       
      uint16_t pcol = fastBlend(alpha, fg_color, bg_color);
       
      slope = ((r - cy)<<16)/(r - cx);
      if (slope <= startSlope[0] && slope >= endSlope[0])  
        drawPixel(x + cx - r, y - cy + r, pcol);
      if (slope >= startSlope[1] && slope <= endSlope[1])  
        drawPixel(x + cx - r, y + cy - r, pcol);
      if (slope <= startSlope[2] && slope >= endSlope[2])  
        drawPixel(x - cx + r, y + cy - r, pcol);
      if (slope <= endSlope[3] && slope >= startSlope[3])  
        drawPixel(x - cx + r, y - cy + r, pcol);
    }
     
    if (len[0]) drawFastHLine(x + xst[0] - len[0] + 1 - r, y - cy + r, len[0], fg_color);  
    if (len[1]) drawFastHLine(x + xst[1] - len[1] + 1 - r, y + cy - r, len[1], fg_color);  
    if (len[2]) drawFastHLine(x - xst[2] + r, y + cy - r, len[2], fg_color);  
    if (len[3]) drawFastHLine(x - xst[3] + r, y - cy + r, len[3], fg_color);  
  }

   
  if (startAngle ==   0 || endAngle == 360) drawFastVLine(x, y + r - w, w, fg_color);  
  if (startAngle <=  90 && endAngle >=  90) drawFastHLine(x - r + 1, y, w, fg_color);  
  if (startAngle <= 180 && endAngle >= 180) drawFastVLine(x, y - r + 1, w, fg_color);  
  if (startAngle <= 270 && endAngle >= 270) drawFastHLine(x + r - w, y, w, fg_color);  

  inTransaction = lockTransaction;
  end_tft_write();
}





 
void TFT_eSPI::drawSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t fg_color, uint32_t bg_color)
{
  drawSmoothRoundRect(x-r, y-r, r, r-1, 0, 0, fg_color, bg_color);
}





void TFT_eSPI::fillSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t color, uint32_t bg_color)
{
  if (r <= 0) return;

  inTransaction = true;

  drawFastHLine(x - r, y, 2 * r + 1, color);
  int32_t xs = 1;
  int32_t cx = 0;

  int32_t r1 = r * r;
  r++;
  int32_t r2 = r * r;
  
  for (int32_t cy = r - 1; cy > 0; cy--)
  {
    int32_t dy2 = (r - cy) * (r - cy);
    for (cx = xs; cx < r; cx++)
    {
      int32_t hyp2 = (r - cx) * (r - cx) + dy2;
      if (hyp2 <= r1) break;
      if (hyp2 >= r2) continue;

      uint8_t alpha = ~sqrt_fraction(hyp2);
      if (alpha > 246) break;
      xs = cx;
      if (alpha < 9) continue;

      if (bg_color == 0x00FFFFFF) {
        drawPixel(x + cx - r, y + cy - r, color, alpha, bg_color);
        drawPixel(x - cx + r, y + cy - r, color, alpha, bg_color);
        drawPixel(x - cx + r, y - cy + r, color, alpha, bg_color);
        drawPixel(x + cx - r, y - cy + r, color, alpha, bg_color);
      }
      else {
        uint16_t pcol = drawPixel(x + cx - r, y + cy - r, color, alpha, bg_color);
        drawPixel(x - cx + r, y + cy - r, pcol);
        drawPixel(x - cx + r, y - cy + r, pcol);
        drawPixel(x + cx - r, y - cy + r, pcol);
      }
    }
    drawFastHLine(x + cx - r, y + cy - r, 2 * (r - cx) + 1, color);
    drawFastHLine(x + cx - r, y - cy + r, 2 * (r - cx) + 1, color);
  }
  inTransaction = lockTransaction;
  end_tft_write();
}






 
 
 
 
 
 
 
 
 
 
void TFT_eSPI::drawSmoothRoundRect(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t w, int32_t h, uint32_t fg_color, uint32_t bg_color, uint8_t quadrants)
{
  if (_vpOoB) return;
  if (r < ir) transpose(r, ir);  
  if (r <= 0 || ir < 0) return;  

  w -= 2*r;
  h -= 2*r;

  if (w < 0) w = 0;
  if (h < 0) h = 0;

  inTransaction = true;

  x += r;
  y += r;

  uint16_t t = r - ir + 1;
  int32_t xs = 0;
  int32_t cx = 0;

  int32_t r2 = r * r;    
  r++;
  int32_t r1 = r * r;    

  int32_t r3 = ir * ir;  
  ir--;
  int32_t r4 = ir * ir;  

  uint8_t alpha = 0;

   
  for (int32_t cy = r - 1; cy > 0; cy--)
  {
    int32_t len = 0;   
    int32_t lxst = 0;  
    int32_t rxst = 0;  
    int32_t dy2 = (r - cy) * (r - cy);

     
    while ((r - xs) * (r - xs) + dy2 >= r1) xs++;

    for (cx = xs; cx < r; cx++)
    {
       
      int32_t hyp = (r - cx) * (r - cx) + dy2;

       
      if (hyp > r2) {
        alpha = ~sqrt_fraction(hyp);  
      }
       
      else if (hyp >= r3) {
        rxst = cx;  
        len++;      
        continue;   
      }
      else {
        if (hyp <= r4) break;   
        alpha = sqrt_fraction(hyp);  
      }

      if (alpha < 16) continue;   

       
      uint16_t pcol = fastBlend(alpha, fg_color, bg_color);
      if (quadrants & 0x8) drawPixel(x + cx - r, y - cy + r + h, pcol);      
      if (quadrants & 0x1) drawPixel(x + cx - r, y + cy - r, pcol);          
      if (quadrants & 0x2) drawPixel(x - cx + r + w, y + cy - r, pcol);      
      if (quadrants & 0x4) drawPixel(x - cx + r + w, y - cy + r + h, pcol);  
    }
     
    lxst = rxst - len + 1;  
    if (quadrants & 0x8) drawFastHLine(x + lxst - r, y - cy + r + h, len, fg_color);      
    if (quadrants & 0x1) drawFastHLine(x + lxst - r, y + cy - r, len, fg_color);          
    if (quadrants & 0x2) drawFastHLine(x - rxst + r + w, y + cy - r, len, fg_color);      
    if (quadrants & 0x4) drawFastHLine(x - rxst + r + w, y - cy + r + h, len, fg_color);  
  }

   
  if ((quadrants & 0xC) == 0xC) fillRect(x, y + r - t + h, w + 1, t, fg_color);  
  if ((quadrants & 0x9) == 0x9) fillRect(x - r + 1, y, t, h + 1, fg_color);      
  if ((quadrants & 0x3) == 0x3) fillRect(x, y - r + 1, w + 1, t, fg_color);      
  if ((quadrants & 0x6) == 0x6) fillRect(x + r - t + w, y, t, h + 1, fg_color);  

  inTransaction = lockTransaction;
  end_tft_write();
}





void TFT_eSPI::fillSmoothRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color, uint32_t bg_color)
{
  inTransaction = true;

  int32_t xs = 0;
  int32_t cx = 0;

   
  if (r < 0)   r = 0;
  if (r > w/2) r = w/2;
  if (r > h/2) r = h/2;

  y += r;
  h -= 2*r;
  fillRect(x, y, w, h, color);

  h--;
  x += r;
  w -= 2*r+1;

  int32_t r1 = r * r;
  r++;
  int32_t r2 = r * r;

  for (int32_t cy = r - 1; cy > 0; cy--)
  {
    int32_t dy2 = (r - cy) * (r - cy);
    for (cx = xs; cx < r; cx++)
    {
      int32_t hyp2 = (r - cx) * (r - cx) + dy2;
      if (hyp2 <= r1) break;
      if (hyp2 >= r2) continue;

      uint8_t alpha = ~sqrt_fraction(hyp2);
      if (alpha > 246) break;
      xs = cx;
      if (alpha < 9) continue;

      drawPixel(x + cx - r, y + cy - r, color, alpha, bg_color);
      drawPixel(x - cx + r + w, y + cy - r, color, alpha, bg_color);
      drawPixel(x - cx + r + w, y - cy + r + h, color, alpha, bg_color);
      drawPixel(x + cx - r, y - cy + r + h, color, alpha, bg_color);
    }
    drawFastHLine(x + cx - r, y + cy - r, 2 * (r - cx) + 1 + w, color);
    drawFastHLine(x + cx - r, y - cy + r + h, 2 * (r - cx) + 1 + w, color);
  }
  inTransaction = lockTransaction;
  end_tft_write();
}





 
void TFT_eSPI::drawSpot(float ax, float ay, float r, uint32_t fg_color, uint32_t bg_color)
{
   
  drawWedgeLine( ax, ay, ax, ay, r, r, fg_color, bg_color);
}





void TFT_eSPI::drawWideLine(float ax, float ay, float bx, float by, float wd, uint32_t fg_color, uint32_t bg_color)
{
  drawWedgeLine( ax, ay, bx, by, wd/2.0, wd/2.0, fg_color, bg_color);
}





void TFT_eSPI::drawWedgeLine(float ax, float ay, float bx, float by, float ar, float br, uint32_t fg_color, uint32_t bg_color)
{
  if ( (ar < 0.0) || (br < 0.0) )return;
  if ( (fabsf(ax - bx) < 0.01f) && (fabsf(ay - by) < 0.01f) ) bx += 0.01f;   

   
  int32_t x0 = (int32_t)floorf(fminf(ax-ar, bx-br));
  int32_t x1 = (int32_t) ceilf(fmaxf(ax+ar, bx+br));
  int32_t y0 = (int32_t)floorf(fminf(ay-ar, by-br));
  int32_t y1 = (int32_t) ceilf(fmaxf(ay+ar, by+br));

  if (!clipWindow(&x0, &y0, &x1, &y1)) return;

   
  int32_t ys = ay;
  if ((ax-ar)>(bx-br)) ys = by;

  float rdt = ar - br;  
  float alpha = 1.0f;
  ar += 0.5;

  uint16_t bg = bg_color;
  float xpax, ypay, bax = bx - ax, bay = by - ay;

  begin_nin_write();
  inTransaction = true;

  int32_t xs = x0;
   
  for (int32_t yp = ys; yp <= y1; yp++) {
    bool swin = true;   
    bool endX = false;  
    ypay = yp - ay;
    for (int32_t xp = xs; xp <= x1; xp++) {
      if (endX) if (alpha <= LoAlphaTheshold) break;   
      xpax = xp - ax;
      alpha = ar - wedgeLineDistance(xpax, ypay, bax, bay, rdt);
      if (alpha <= LoAlphaTheshold ) continue;
       
      if (!endX) { endX = true; xs = xp; }
      if (alpha > HiAlphaTheshold) {
        #ifdef GC9A01_DRIVER
          drawPixel(xp, yp, fg_color);
        #else
          if (swin) { setWindow(xp, yp, x1, yp); swin = false; }
          pushColor(fg_color);
        #endif
        continue;
      }
       
      if (bg_color == 0x00FFFFFF) {
        bg = readPixel(xp, yp); swin = true;
      }
      #ifdef GC9A01_DRIVER
        uint16_t pcol = fastBlend((uint8_t)(alpha * PixelAlphaGain), fg_color, bg);
        drawPixel(xp, yp, pcol);
        swin = swin;
      #else
        if (swin) { setWindow(xp, yp, x1, yp); swin = false; }
        pushColor(fastBlend((uint8_t)(alpha * PixelAlphaGain), fg_color, bg));
      #endif
    }
  }

   
  xs = x0;
   
  for (int32_t yp = ys-1; yp >= y0; yp--) {
    bool swin = true;   
    bool endX = false;  
    ypay = yp - ay;
    for (int32_t xp = xs; xp <= x1; xp++) {
      if (endX) if (alpha <= LoAlphaTheshold) break;   
      xpax = xp - ax;
      alpha = ar - wedgeLineDistance(xpax, ypay, bax, bay, rdt);
      if (alpha <= LoAlphaTheshold ) continue;
       
      if (!endX) { endX = true; xs = xp; }
      if (alpha > HiAlphaTheshold) {
        #ifdef GC9A01_DRIVER
          drawPixel(xp, yp, fg_color);
        #else
          if (swin) { setWindow(xp, yp, x1, yp); swin = false; }
          pushColor(fg_color);
        #endif
        continue;
      }
       
      if (bg_color == 0x00FFFFFF) {
        bg = readPixel(xp, yp); swin = true;
      }
      #ifdef GC9A01_DRIVER
        uint16_t pcol = fastBlend((uint8_t)(alpha * PixelAlphaGain), fg_color, bg);
        drawPixel(xp, yp, pcol);
        swin = swin;
      #else
        if (swin) { setWindow(xp, yp, x1, yp); swin = false; }
        pushColor(fastBlend((uint8_t)(alpha * PixelAlphaGain), fg_color, bg));
      #endif
    }
  }

  inTransaction = lockTransaction;
  end_nin_write();
}






inline float TFT_eSPI::wedgeLineDistance(float xpax, float ypay, float bax, float bay, float dr)
{
  float h = fmaxf(fminf((xpax * bax + ypay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
  float dx = xpax - bax * h, dy = ypay - bay * h;
  return sqrtf(dx * dx + dy * dy) + h * dr;
}






void TFT_eSPI::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color)
{
  if (_vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((x < _vpX) || (x >= _vpW) || (y >= _vpH)) return;

  if (y < _vpY) { h += y - _vpY; y = _vpY; }

  if ((y + h) > _vpH) h = _vpH - y;

  if (h < 1) return;

  begin_tft_write();

  setWindow(x, y, x, y + h - 1);

  pushBlock(color, h);

  end_tft_write();
}






void TFT_eSPI::drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color)
{
  if (_vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((y < _vpY) || (x >= _vpW) || (y >= _vpH)) return;

  if (x < _vpX) { w += x - _vpX; x = _vpX; }

  if ((x + w) > _vpW) w = _vpW - x;

  if (w < 1) return;

  begin_tft_write();

  setWindow(x, y, x + w - 1, y);

  pushBlock(color, w);

  end_tft_write();
}






void TFT_eSPI::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
  if (_vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((x >= _vpW) || (y >= _vpH)) return;

  if (x < _vpX) { w += x - _vpX; x = _vpX; }
  if (y < _vpY) { h += y - _vpY; y = _vpY; }

  if ((x + w) > _vpW) w = _vpW - x;
  if ((y + h) > _vpH) h = _vpH - y;

  if ((w < 1) || (h < 1)) return;

   
   

   
   

   
   

  begin_tft_write();

  setWindow(x, y, x + w - 1, y + h - 1);

  pushBlock(color, w * h);

  end_tft_write();
}






void TFT_eSPI::fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2)
{
  if (_vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((x >= _vpW) || (y >= _vpH)) return;

  if (x < _vpX) { w += x - _vpX; x = _vpX; }
  if (y < _vpY) { h += y - _vpY; y = _vpY; }

  if ((x + w) > _vpW) w = _vpW - x;
  if ((y + h) > _vpH) h = _vpH - y;

  if ((w < 1) || (h < 1)) return;

  begin_nin_write();

  float delta = -255.0/h;
  float alpha = 255.0;
  uint32_t color = color1;

  while (h--) {
    drawFastHLine(x, y++, w, color);
    alpha += delta;
    color = fastBlend((uint8_t)alpha, color1, color2);
  }

  end_nin_write();
}






void TFT_eSPI::fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2)
{
  if (_vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((x >= _vpW) || (y >= _vpH)) return;

  if (x < _vpX) { w += x - _vpX; x = _vpX; }
  if (y < _vpY) { h += y - _vpY; y = _vpY; }

  if ((x + w) > _vpW) w = _vpW - x;
  if ((y + h) > _vpH) h = _vpH - y;

  if ((w < 1) || (h < 1)) return;

  begin_nin_write();

  float delta = -255.0/w;
  float alpha = 255.0;
  uint32_t color = color1;

  while (w--) {
    drawFastVLine(x++, y, h, color);
    alpha += delta;
    color = fastBlend((uint8_t)alpha, color1, color2);
  }

  end_nin_write();
}






uint16_t TFT_eSPI::color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}






uint8_t TFT_eSPI::color16to8(uint16_t c)
{
  return ((c & 0xE000)>>8) | ((c & 0x0700)>>6) | ((c & 0x0018)>>3);
}






uint16_t TFT_eSPI::color8to16(uint8_t color)
{
  uint8_t  blue[] = {0, 11, 21, 31};  
  uint16_t color16 = 0;

   
  color16  = (color & 0x1C)<<6 | (color & 0xC0)<<5 | (color & 0xE0)<<8;
   
  color16 |= (color & 0x1C)<<3 | blue[color & 0x03];

  return color16;
}





uint32_t TFT_eSPI::color16to24(uint16_t color565)
{
  uint8_t r = (color565 >> 8) & 0xF8; r |= (r >> 5);
  uint8_t g = (color565 >> 3) & 0xFC; g |= (g >> 6);
  uint8_t b = (color565 << 3) & 0xF8; b |= (b >> 5);

  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b << 0);
}





uint32_t TFT_eSPI::color24to16(uint32_t color888)
{
  uint16_t r = (color888 >> 8) & 0xF800;
  uint16_t g = (color888 >> 5) & 0x07E0;
  uint16_t b = (color888 >> 3) & 0x001F;

  return (r | g | b);
}





void TFT_eSPI::invertDisplay(bool i)
{
  begin_tft_write();
   
  writecommand(i ? TFT_INVON : TFT_INVOFF);
  writecommand(i ? TFT_INVON : TFT_INVOFF);
  end_tft_write();
}






void TFT_eSPI::setAttribute(uint8_t attr_id, uint8_t param) {
    switch (attr_id) {
            break;
        case CP437_SWITCH:
            _cp437 = param;
            break;
        case UTF8_SWITCH:
            _utf8  = param;
            decoderState = 0;
            break;
        case PSRAM_ENABLE:
#if defined (ESP32) && defined (CONFIG_SPIRAM_SUPPORT)
            if (psramFound()) _psram_enable = param;  
            else
#endif
            _psram_enable = false;
            break;
         
         
         
    }
}






uint8_t TFT_eSPI::getAttribute(uint8_t attr_id) {
    switch (attr_id) {
        case CP437_SWITCH:  
            return _cp437;
        case UTF8_SWITCH:  
            return _utf8;
        case PSRAM_ENABLE:
            return _psram_enable;
         
         
         
    }

    return false;
}





uint16_t TFT_eSPI::decodeUTF8(uint8_t c)
{
  if (!_utf8) return c;

   
  if ((c & 0x80) == 0x00) {
    decoderState = 0;
    return c;
  }

  if (decoderState == 0) {
     
    if ((c & 0xE0) == 0xC0) {
      decoderBuffer = ((c & 0x1F)<<6);
      decoderState = 1;
      return 0;
    }
     
    if ((c & 0xF0) == 0xE0) {
      decoderBuffer = ((c & 0x0F)<<12);
      decoderState = 2;
      return 0;
    }
     
     
  }
  else {
    if (decoderState == 2) {
      decoderBuffer |= ((c & 0x3F)<<6);
      decoderState--;
      return 0;
    }
    else {
      decoderBuffer |= (c & 0x3F);
      decoderState = 0;
      return decoderBuffer;
    }
  }

  decoderState = 0;

  return c;  
}






uint16_t TFT_eSPI::decodeUTF8(uint8_t *buf, uint16_t *index, uint16_t remaining)
{
  uint16_t c = buf[(*index)++];
   

  if (!_utf8) return c;

   
  if ((c & 0x80) == 0x00) return c;

   
  if (((c & 0xE0) == 0xC0) && (remaining > 1))
    return ((c & 0x1F)<<6) | (buf[(*index)++]&0x3F);

   
  if (((c & 0xF0) == 0xE0) && (remaining > 2)) {
    c = ((c & 0x0F)<<12) | ((buf[(*index)++]&0x3F)<<6);
    return  c | ((buf[(*index)++]&0x3F));
  }

   
   
   
   
   

  return c;  
}






uint16_t TFT_eSPI::alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc)
{
   
  uint32_t rxb = bgc & 0xF81F;
  rxb += ((fgc & 0xF81F) - rxb) * (alpha >> 2) >> 6;
   
  uint32_t xgx = bgc & 0x07E0;
  xgx += ((fgc & 0x07E0) - xgx) * alpha >> 8;
   
  return (rxb & 0xF81F) | (xgx & 0x07E0);
}





uint16_t TFT_eSPI::alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc, uint8_t dither)
{
  if (dither) {
    int16_t alphaDither = (int16_t)alpha - dither + random(2*dither+1);  
    alpha = (uint8_t)alphaDither;
    if (alphaDither <  0) alpha = 0;
    if (alphaDither >255) alpha = 255;
  }

  return alphaBlend(alpha, fgc, bgc);
}





uint32_t TFT_eSPI::alphaBlend24(uint8_t alpha, uint32_t fgc, uint32_t bgc, uint8_t dither)
{

  if (dither) {
    int16_t alphaDither = (int16_t)alpha - dither + random(2*dither+1);  
    alpha = (uint8_t)alphaDither;
    if (alphaDither <  0) alpha = 0;
    if (alphaDither >255) alpha = 255;
  }

  uint32_t rxx = bgc & 0xFF0000;
  rxx += ((fgc & 0xFF0000) - rxx) * alpha >> 8;
  uint32_t xgx = bgc & 0x00FF00;
  xgx += ((fgc & 0x00FF00) - xgx) * alpha >> 8;
  uint32_t xxb = bgc & 0x0000FF;
  xxb += ((fgc & 0x0000FF) - xxb) * alpha >> 8;
  return (rxx & 0xFF0000) | (xgx & 0x00FF00) | (xxb & 0x0000FF);
}
























size_t TFT_eSPI::write(uint8_t utf8)
{
  if (_vpOoB) return 1;

  uint16_t uniCode = decodeUTF8(utf8);

  if (!uniCode) return 1;

  if (utf8 == '\r') return 1;

#ifdef SMOOTH_FONT
  if(fontLoaded) {
    if (uniCode < 32 && utf8 != '\n') return 1;

    drawGlyph(uniCode);

    return 1;
  }
#endif

  if (uniCode == '\n') uniCode+=22;  

  uint16_t cwidth = 0;
  uint16_t cheight = 0;

 
   
   
   
 

 
#ifdef LOAD_GFXFF
  if(!gfxFont) {
#endif
 

#ifdef LOAD_FONT2
  if (textfont == 2) {
    if (uniCode < 32 || uniCode > 127) return 1;

    cwidth = pgm_read_byte(widtbl_f16 + uniCode-32);
    cheight = chr_hgt_f16;
     
    cwidth = (cwidth + 6) / 8;   
    cwidth = cwidth * 8;         
  }
  #ifdef LOAD_RLE
  else
  #endif
#endif

#ifdef LOAD_RLE
  {
    if ((textfont>2) && (textfont<9)) {
      if (uniCode < 32 || uniCode > 127) return 1;
       
      cwidth = pgm_read_byte( (uint8_t *)pgm_read_dword( &(fontdata[textfont].widthtbl ) ) + uniCode-32 );
      cheight= pgm_read_byte( &fontdata[textfont].height );
    }
  }
#endif

#ifdef LOAD_GLCD
  if (textfont==1) {
      cwidth =  6;
      cheight = 8;
  }
#else
  if (textfont==1) return 1;
#endif

  cheight = cheight * textsize;

  if (utf8 == '\n') {
    cursor_y += cheight;
    cursor_x  = 0;
  }
  else {
    if (textwrapX && (cursor_x + cwidth * textsize > width())) {
      cursor_y += cheight;
      cursor_x = 0;
    }
    if (textwrapY && (cursor_y >= (int32_t) height())) cursor_y = 0;
    cursor_x += drawChar(uniCode, cursor_x, cursor_y, textfont);
  }

 
#ifdef LOAD_GFXFF
  }  
  else {
    if(utf8 == '\n') {
      cursor_x  = 0;
      cursor_y += (int16_t)textsize * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
    } else {
      if (uniCode > pgm_read_word(&gfxFont->last )) return 1;
      if (uniCode < pgm_read_word(&gfxFont->first)) return 1;

      uint16_t   c2    = uniCode - pgm_read_word(&gfxFont->first);
      GFXglyph *glyph = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c2]);
      uint8_t   w     = pgm_read_byte(&glyph->width),
                h     = pgm_read_byte(&glyph->height);
      if((w > 0) && (h > 0)) {  
        int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset);
        if(textwrapX && ((cursor_x + textsize * (xo + w)) > width())) {
           
          cursor_x  = 0;
          cursor_y += (int16_t)textsize * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
        }
        if (textwrapY && (cursor_y >= (int32_t) height())) cursor_y = 0;
        drawChar(cursor_x, cursor_y, uniCode, textcolor, textbgcolor, textsize);
      }
      cursor_x += pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize;
    }
  }
#endif  
 

  return 1;
}






   
   
int16_t TFT_eSPI::drawChar(uint16_t uniCode, int32_t x, int32_t y)
{
  return drawChar(uniCode, x, y, textfont);
}

   
int16_t TFT_eSPI::drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font)
{
  if (_vpOoB || !uniCode) return 0;

  if (font==1) {
#ifdef LOAD_GLCD
  #ifndef LOAD_GFXFF
    drawChar(x, y, uniCode, textcolor, textbgcolor, textsize);
    return 6 * textsize;
  #endif
#else
  #ifndef LOAD_GFXFF
    return 0;
  #endif
#endif

#ifdef LOAD_GFXFF
    drawChar(x, y, uniCode, textcolor, textbgcolor, textsize);
    if(!gfxFont) {  
    #ifdef LOAD_GLCD
      return 6 * textsize;
    #else
      return 0;
    #endif
    }
    else {
      if((uniCode >= pgm_read_word(&gfxFont->first)) && (uniCode <= pgm_read_word(&gfxFont->last) )) {
        uint16_t   c2    = uniCode - pgm_read_word(&gfxFont->first);
        GFXglyph *glyph = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c2]);
        return pgm_read_byte(&glyph->xAdvance) * textsize;
      }
      else {
        return 0;
      }
    }
#endif
  }

  if ((font>1) && (font<9) && ((uniCode < 32) || (uniCode > 127))) return 0;

  int32_t width  = 0;
  int32_t height = 0;
  uint32_t flash_address = 0;
  uniCode -= 32;

#ifdef LOAD_FONT2
  if (font == 2) {
    flash_address = pgm_read_dword(&chrtbl_f16[uniCode]);
    width = pgm_read_byte(widtbl_f16 + uniCode);
    height = chr_hgt_f16;
  }
  #ifdef LOAD_RLE
  else
  #endif
#endif

#ifdef LOAD_RLE
  {
    if ((font>2) && (font<9)) {
      flash_address = pgm_read_dword( (const void*)(pgm_read_dword( &(fontdata[font].chartbl ) ) + uniCode*sizeof(void *)) );
      width = pgm_read_byte( (uint8_t *)pgm_read_dword( &(fontdata[font].widthtbl ) ) + uniCode );
      height= pgm_read_byte( &fontdata[font].height );
    }
  }
#endif

  int32_t xd = x + _xDatum;
  int32_t yd = y + _yDatum;

  if ((xd + width * textsize < _vpX || xd >= _vpW) && (yd + height * textsize < _vpY || yd >= _vpH)) return width * textsize ;

  int32_t w = width;
  int32_t pX      = 0;
  int32_t pY      = y;
  uint8_t line = 0;
  bool clip = xd < _vpX || xd + width  * textsize >= _vpW || yd < _vpY || yd + height * textsize >= _vpH;

#ifdef LOAD_FONT2  
  if (font == 2) {
    w = w + 6;  
    w = w / 8;

    if (textcolor == textbgcolor || textsize != 1 || clip) {
       
      inTransaction = true;

      for (int32_t i = 0; i < height; i++) {
        if (textcolor != textbgcolor) fillRect(x, pY, width * textsize, textsize, textbgcolor);

        for (int32_t k = 0; k < w; k++) {
          line = pgm_read_byte((uint8_t *)flash_address + w * i + k);
          if (line) {
            if (textsize == 1) {
              pX = x + k * 8;
              if (line & 0x80) drawPixel(pX, pY, textcolor);
              if (line & 0x40) drawPixel(pX + 1, pY, textcolor);
              if (line & 0x20) drawPixel(pX + 2, pY, textcolor);
              if (line & 0x10) drawPixel(pX + 3, pY, textcolor);
              if (line & 0x08) drawPixel(pX + 4, pY, textcolor);
              if (line & 0x04) drawPixel(pX + 5, pY, textcolor);
              if (line & 0x02) drawPixel(pX + 6, pY, textcolor);
              if (line & 0x01) drawPixel(pX + 7, pY, textcolor);
            }
            else {
              pX = x + k * 8 * textsize;
              if (line & 0x80) fillRect(pX, pY, textsize, textsize, textcolor);
              if (line & 0x40) fillRect(pX + textsize, pY, textsize, textsize, textcolor);
              if (line & 0x20) fillRect(pX + 2 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x10) fillRect(pX + 3 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x08) fillRect(pX + 4 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x04) fillRect(pX + 5 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x02) fillRect(pX + 6 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x01) fillRect(pX + 7 * textsize, pY, textsize, textsize, textcolor);
            }
          }
        }
        pY += textsize;
      }

      inTransaction = lockTransaction;
      end_tft_write();
    }
    else {  

      begin_tft_write();

      setWindow(xd, yd, xd + width - 1, yd + height - 1);

      uint8_t mask;
      for (int32_t i = 0; i < height; i++) {
        pX = width;
        for (int32_t k = 0; k < w; k++) {
          line = pgm_read_byte((uint8_t *) (flash_address + w * i + k) );
          mask = 0x80;
          while (mask && pX) {
            if (line & mask) {tft_Write_16(textcolor);}
            else {tft_Write_16(textbgcolor);}
            pX--;
            mask = mask >> 1;
          }
        }
        if (pX) {tft_Write_16(textbgcolor);}
      }

      end_tft_write();
    }
  }

  #ifdef LOAD_RLE
  else
  #endif
#endif   

#ifdef LOAD_RLE   
   
  {
    begin_tft_write();
    inTransaction = true;

    w *= height;  
    if (textcolor == textbgcolor && !clip) {

      int32_t px = 0, py = pY;  
      int32_t pc = 0;  
      uint8_t np = textsize * textsize;  

      uint8_t tnp = 0;  
      uint8_t ts = textsize - 1;  
       
       
      while (pc < w) {
        line = pgm_read_byte((uint8_t *)flash_address);
        flash_address++;
        if (line & 0x80) {
          line &= 0x7F;
          line++;
          if (ts) {
            px = xd + textsize * (pc % width);  
            py = yd + textsize * (pc / width);
          }
          else {
            px = xd + pc % width;  
            py = yd + pc / width;
          }
          while (line--) {  
            pc++;  
            setWindow(px, py, px + ts, py + ts);

            if (ts) {
              tnp = np;
              while (tnp--) {tft_Write_16(textcolor);}
            }
            else {tft_Write_16(textcolor);}
            px += textsize;

            if (px >= (xd + width * textsize)) {
              px = xd;
              py += textsize;
            }
          }
        }
        else {
          line++;
          pc += line;
        }
      }
    }
    else {
       
       
      if (textcolor != textbgcolor && textsize == 1 && !clip)
      {
        setWindow(xd, yd, xd + width - 1, yd + height - 1);

         
        while (w > 0) {
          line = pgm_read_byte((uint8_t *)flash_address++);  
          if (line & 0x80) {
            line &= 0x7F;
            line++; w -= line;
            pushBlock(textcolor,line);
          }
          else {
            line++; w -= line;
            pushBlock(textbgcolor,line);
          }
        }
      }
      else
      {
        int32_t px = 0, py = 0;   
        int32_t tx = 0, ty = 0;   
        int32_t pc = 0;           
        int32_t pl = 0;           
        uint16_t pcol = 0;        
        bool     pf = true;       
        while (pc < w) {
          line = pgm_read_byte((uint8_t *)flash_address);
          flash_address++;
          if (line & 0x80) { pcol = textcolor; line &= 0x7F; pf = true;}
          else { pcol = textbgcolor; if (textcolor == textbgcolor) pf = false;}
          line++;
          px = pc % width;
          tx = x + textsize * px;
          py = pc / width;
          ty = y + textsize * py;

          pl = 0;
          pc += line;
          while (line--) {
            pl++;
            if ((px+pl) >= width) {
              if (pf) fillRect(tx, ty, pl * textsize, textsize, pcol);
              pl = 0;
              px = 0;
              tx = x;
              py ++;
              ty += textsize;
            }
          }
          if (pl && pf) fillRect(tx, ty, pl * textsize, textsize, pcol);
        }
      }
    }
    inTransaction = lockTransaction;
    end_tft_write();
  }
   
#endif

#if !defined (LOAD_FONT2) && !defined (LOAD_RLE)
   
  flash_address = flash_address;
  w = w;
  pX = pX;
  pY = pY;
  line = line;
  clip = clip;
#endif

  return width * textsize;     
}






 
int16_t TFT_eSPI::drawString(const String& string, int32_t poX, int32_t poY)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return drawString(buffer, poX, poY, textfont);
}
 
int16_t TFT_eSPI::drawString(const String& string, int32_t poX, int32_t poY, uint8_t font)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return drawString(buffer, poX, poY, font);
}

 
int16_t TFT_eSPI::drawString(const char *string, int32_t poX, int32_t poY)
{
  return drawString(string, poX, poY, textfont);
}

 
int16_t TFT_eSPI::drawString(const char *string, int32_t poX, int32_t poY, uint8_t font)
{
  if (font > 8) return 0;

  int16_t sumX = 0;
  uint8_t padding = 1, baseline = 0;
  uint16_t cwidth = textWidth(string, font);  
  uint16_t cheight = 8 * textsize;

#ifdef LOAD_GFXFF
  #ifdef SMOOTH_FONT
    bool freeFont = (font == 1 && gfxFont && !fontLoaded);
  #else
    bool freeFont = (font == 1 && gfxFont);
  #endif

  if (freeFont) {
    cheight = glyph_ab * textsize;
    poY += cheight;  
    baseline = cheight;
    padding =101;  

     
    if ((textdatum == BL_DATUM) || (textdatum == BC_DATUM) || (textdatum == BR_DATUM)) {
      cheight += glyph_bb * textsize;
    }
  }
#endif


   
#ifdef SMOOTH_FONT
  if(fontLoaded) {
    baseline = gFont.maxAscent;
    cheight  = fontHeight();
  }
  else
#endif
  if (font!=1) {
    baseline = pgm_read_byte( &fontdata[font].baseline ) * textsize;
    cheight = fontHeight(font);
  }

  if (textdatum || padX) {

    switch(textdatum) {
      case TC_DATUM:
        poX -= cwidth/2;
        padding += 1;
        break;
      case TR_DATUM:
        poX -= cwidth;
        padding += 2;
        break;
      case ML_DATUM:
        poY -= cheight/2;
         
        break;
      case MC_DATUM:
        poX -= cwidth/2;
        poY -= cheight/2;
        padding += 1;
        break;
      case MR_DATUM:
        poX -= cwidth;
        poY -= cheight/2;
        padding += 2;
        break;
      case BL_DATUM:
        poY -= cheight;
         
        break;
      case BC_DATUM:
        poX -= cwidth/2;
        poY -= cheight;
        padding += 1;
        break;
      case BR_DATUM:
        poX -= cwidth;
        poY -= cheight;
        padding += 2;
        break;
      case L_BASELINE:
        poY -= baseline;
         
        break;
      case C_BASELINE:
        poX -= cwidth/2;
        poY -= baseline;
        padding += 1;
        break;
      case R_BASELINE:
        poX -= cwidth;
        poY -= baseline;
        padding += 2;
        break;
    }
  }


  int8_t xo = 0;
#ifdef LOAD_GFXFF
  if (freeFont && (textcolor!=textbgcolor)) {
      cheight = (glyph_ab + glyph_bb) * textsize;
       
      uint16_t c2 = 0;
      uint16_t len = strlen(string);
      uint16_t n = 0;

      while (n < len && c2 == 0) c2 = decodeUTF8((uint8_t*)string, &n, len - n);

      if((c2 >= pgm_read_word(&gfxFont->first)) && (c2 <= pgm_read_word(&gfxFont->last) )) {
        c2 -= pgm_read_word(&gfxFont->first);
        GFXglyph *glyph = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c2]);
        xo = pgm_read_byte(&glyph->xOffset) * textsize;
         
        if (xo > 0) xo = 0;
        else cwidth -= xo;
         
         
         
        fillRect(poX+xo, poY - glyph_ab * textsize, cwidth, cheight, textbgcolor);
      }
      padding -=100;
    }
#endif

  uint16_t len = strlen(string);
  uint16_t n = 0;

#ifdef SMOOTH_FONT
  if(fontLoaded) {
    setCursor(poX, poY);

    bool fillbg = _fillbg;
     
    if (padX && !_fillbg) _fillbg = true;

    while (n < len) {
      uint16_t uniCode = decodeUTF8((uint8_t*)string, &n, len - n);
      drawGlyph(uniCode);
    }
    _fillbg = fillbg;  
    sumX += cwidth;
     
  }
  else
#endif
  {
    while (n < len) {
      uint16_t uniCode = decodeUTF8((uint8_t*)string, &n, len - n);
      sumX += drawChar(uniCode, poX+sumX, poY, font);
    }
  }

 
 
 

#ifndef PADDING_DEBUG
 

  if((padX>cwidth) && (textcolor!=textbgcolor)) {
    int16_t padXc = poX+cwidth+xo;
#ifdef LOAD_GFXFF
    if (freeFont) {
      poX +=xo;  
      poY -= glyph_ab * textsize;
      sumX += poX;
    }
#endif
    switch(padding) {
      case 1:
        fillRect(padXc,poY,padX-cwidth,cheight, textbgcolor);
        break;
      case 2:
        fillRect(padXc,poY,(padX-cwidth)>>1,cheight, textbgcolor);
        padXc = poX - ((padX-cwidth)>>1);
        fillRect(padXc,poY,(padX-cwidth)>>1,cheight, textbgcolor);
        break;
      case 3:
        if (padXc>padX) padXc = padX;
        fillRect(poX + cwidth - padXc,poY,padXc-cwidth,cheight, textbgcolor);
        break;
    }
  }


#else

 
 
 

  if((padX>sumX) && (textcolor!=textbgcolor)) {
    int16_t padXc = poX+sumX;  
#ifdef LOAD_GFXFF
    if ((font == 1) && (gfxFont)) poY -= glyph_ab;
#endif
    drawRect(poX,poY,sumX,cheight, TFT_GREEN);
    switch(padding) {
      case 1:
        drawRect(padXc,poY,padX-sumX,cheight, TFT_WHITE);
        break;
      case 2:
        drawRect(padXc,poY,(padX-sumX)>>1, cheight, TFT_WHITE);
        padXc = (padX-sumX)>>1;
        drawRect(poX - padXc,poY,(padX-sumX)>>1,cheight, TFT_WHITE);
        break;
      case 3:
        if (padXc>padX) padXc = padX;
        drawRect(poX + sumX - padXc,poY,padXc-sumX,cheight, TFT_WHITE);
        break;
    }
  }
#endif
 

return sumX;
}






int16_t TFT_eSPI::drawCentreString(const String& string, int32_t dX, int32_t poY, uint8_t font)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return drawCentreString(buffer, dX, poY, font);
}

int16_t TFT_eSPI::drawCentreString(const char *string, int32_t dX, int32_t poY, uint8_t font)
{
  uint8_t tempdatum = textdatum;
  int32_t sumX = 0;
  textdatum = TC_DATUM;
  sumX = drawString(string, dX, poY, font);
  textdatum = tempdatum;
  return sumX;
}






int16_t TFT_eSPI::drawRightString(const String& string, int32_t dX, int32_t poY, uint8_t font)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return drawRightString(buffer, dX, poY, font);
}

int16_t TFT_eSPI::drawRightString(const char *string, int32_t dX, int32_t poY, uint8_t font)
{
  uint8_t tempdatum = textdatum;
  int16_t sumX = 0;
  textdatum = TR_DATUM;
  sumX = drawString(string, dX, poY, font);
  textdatum = tempdatum;
  return sumX;
}






int16_t TFT_eSPI::drawNumber(long long_num, int32_t poX, int32_t poY)
{
  isDigits = true;  
  char str[12];
  ltoa(long_num, str, 10);
  return drawString(str, poX, poY, textfont);
}

int16_t TFT_eSPI::drawNumber(long long_num, int32_t poX, int32_t poY, uint8_t font)
{
  isDigits = true;  
  char str[12];
  ltoa(long_num, str, 10);
  return drawString(str, poX, poY, font);
}






 
 
int16_t TFT_eSPI::drawFloat(float floatNumber, uint8_t dp, int32_t poX, int32_t poY)
{
  return drawFloat(floatNumber, dp, poX, poY, textfont);
}

int16_t TFT_eSPI::drawFloat(float floatNumber, uint8_t dp, int32_t poX, int32_t poY, uint8_t font)
{
  isDigits = true;
  char str[14];                
  uint8_t ptr = 0;             
  int8_t  digits = 1;          
  float rounding = 0.5;        
  bool negative = false;

  if (dp > 7) dp = 7;  

   
  for (uint8_t i = 0; i < dp; ++i) rounding /= 10.0;

  if (floatNumber < -rounding) {    
    str[ptr++] = '-';  
    str[ptr] = 0;  
    digits = 0;    
    floatNumber = -floatNumber;  
    negative = true;
  }

  floatNumber += rounding;  

  if (dp == 0) {
    if (negative) floatNumber = -floatNumber;
    return drawNumber((long)floatNumber, poX, poY, font);
  }

   
  if (floatNumber >= 2147483647) {
    strcpy(str, "...");
    return drawString(str, poX, poY, font);
  }
   

   
  uint32_t temp = (uint32_t)floatNumber;

   
  ltoa(temp, str + ptr, 10);

   
  while ((uint8_t)str[ptr] != 0) ptr++;  
  digits += ptr;                   

  str[ptr++] = '.';  
  str[ptr] = '0';    
  str[ptr + 1] = 0;  

   
  floatNumber = floatNumber - temp;

   
   
  uint8_t i = 0;
  while ((i < dp) && (digits < 9)) {  
    i++;
    floatNumber *= 10;        
    temp = floatNumber;       
    ltoa(temp, str + ptr, 10);
    ptr++; digits++;          
    floatNumber -= temp;      
  }

   
  return drawString(str, poX, poY, font);
}







#ifdef LOAD_GFXFF

void TFT_eSPI::setFreeFont(const GFXfont *f)
{
  if (f == nullptr) {  
    setTextFont(1);  
    return;
  }

  textfont = 1;
  gfxFont = (GFXfont *)f;

  glyph_ab = 0;
  glyph_bb = 0;
  uint16_t numChars = pgm_read_word(&gfxFont->last) - pgm_read_word(&gfxFont->first);

   
  for (uint16_t c = 0; c < numChars; c++) {
    GFXglyph *glyph1  = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c]);
    int8_t ab = -pgm_read_byte(&glyph1->yOffset);
    if (ab > glyph_ab) glyph_ab = ab;
    int8_t bb = pgm_read_byte(&glyph1->height) - ab;
    if (bb > glyph_bb) glyph_bb = bb;
  }
}






void TFT_eSPI::setTextFont(uint8_t f)
{
  textfont = (f > 0) ? f : 1;  
  textfont = (f > 8) ? 1 : f;  
  gfxFont = NULL;
}

#else







 
void TFT_eSPI::setFreeFont(uint8_t font)
{
  setTextFont(font);
}






void TFT_eSPI::setTextFont(uint8_t f)
{
  textfont = (f > 0) ? f : 1;  
  textfont = (f > 8) ? 1 : f;  
}
#endif






#if !defined (TFT_PARALLEL_8_BIT) && !defined (RP2040_PIO_INTERFACE)
SPIClass& TFT_eSPI::getSPIinstance(void)
{
  return spi;
}
#endif






bool TFT_eSPI::verifySetupID(uint32_t id)
{
#if defined (USER_SETUP_ID)
  if (USER_SETUP_ID == id) return true;
#else
  id = id;  
#endif
  return false;
}





void TFT_eSPI::getSetup(setup_t &tft_settings)
{
 

#if defined (USER_SETUP_INFO)
  tft_settings.setup_info = USER_SETUP_INFO;
#else
  tft_settings.setup_info = "NA";
#endif

#if defined (USER_SETUP_ID)
  tft_settings.setup_id = USER_SETUP_ID;
#else
  tft_settings.setup_id = 0;
#endif

#if defined (PROCESSOR_ID)
  tft_settings.esp = PROCESSOR_ID;
#else
  tft_settings.esp = -1;
#endif

#if defined (SUPPORT_TRANSACTIONS)
  tft_settings.trans = true;
#else
  tft_settings.trans = false;
#endif

#if defined (TFT_PARALLEL_8_BIT) || defined(TFT_PARALLEL_16_BIT)
  tft_settings.serial = false;
  tft_settings.tft_spi_freq = 0;
#else
  tft_settings.serial = true;
  tft_settings.tft_spi_freq = SPI_FREQUENCY/100000;
  #ifdef SPI_READ_FREQUENCY
    tft_settings.tft_rd_freq = SPI_READ_FREQUENCY/100000;
  #endif
  #ifndef GENERIC_PROCESSOR
    #ifdef TFT_SPI_PORT
      tft_settings.port = TFT_SPI_PORT;
    #else
      tft_settings.port = 255;
    #endif
  #endif
  #ifdef RP2040_PIO_SPI
    tft_settings.interface = 0x10;
  #else
    tft_settings.interface = 0x0;
  #endif
#endif

#if defined(TFT_SPI_OVERLAP)
  tft_settings.overlap = true;
#else
  tft_settings.overlap = false;
#endif

  tft_settings.tft_driver = TFT_DRIVER;
  tft_settings.tft_width  = _init_width;
  tft_settings.tft_height = _init_height;

#ifdef CGRAM_OFFSET
  tft_settings.r0_x_offset = colstart;
  tft_settings.r0_y_offset = rowstart;
  tft_settings.r1_x_offset = 0;
  tft_settings.r1_y_offset = 0;
  tft_settings.r2_x_offset = 0;
  tft_settings.r2_y_offset = 0;
  tft_settings.r3_x_offset = 0;
  tft_settings.r3_y_offset = 0;
#else
  tft_settings.r0_x_offset = 0;
  tft_settings.r0_y_offset = 0;
  tft_settings.r1_x_offset = 0;
  tft_settings.r1_y_offset = 0;
  tft_settings.r2_x_offset = 0;
  tft_settings.r2_y_offset = 0;
  tft_settings.r3_x_offset = 0;
  tft_settings.r3_y_offset = 0;
#endif

#if defined (TFT_MOSI)
  tft_settings.pin_tft_mosi = TFT_MOSI;
#else
  tft_settings.pin_tft_mosi = -1;
#endif

#if defined (TFT_MISO)
  tft_settings.pin_tft_miso = TFT_MISO;
#else
  tft_settings.pin_tft_miso = -1;
#endif

#if defined (TFT_SCLK)
  tft_settings.pin_tft_clk  = TFT_SCLK;
#else
  tft_settings.pin_tft_clk  = -1;
#endif

#if defined (TFT_CS)
  tft_settings.pin_tft_cs   = TFT_CS;
#else
  tft_settings.pin_tft_cs   = -1;
#endif

#if defined (TFT_DC)
  tft_settings.pin_tft_dc  = TFT_DC;
#else
  tft_settings.pin_tft_dc  = -1;
#endif

#if defined (TFT_RD)
  tft_settings.pin_tft_rd  = TFT_RD;
#else
  tft_settings.pin_tft_rd  = -1;
#endif

#if defined (TFT_WR)
  tft_settings.pin_tft_wr  = TFT_WR;
#else
  tft_settings.pin_tft_wr  = -1;
#endif

#if defined (TFT_RST)
  tft_settings.pin_tft_rst = TFT_RST;
#else
  tft_settings.pin_tft_rst = -1;
#endif

#if defined (TFT_PARALLEL_8_BIT) || defined(TFT_PARALLEL_16_BIT)
  tft_settings.pin_tft_d0 = TFT_D0;
  tft_settings.pin_tft_d1 = TFT_D1;
  tft_settings.pin_tft_d2 = TFT_D2;
  tft_settings.pin_tft_d3 = TFT_D3;
  tft_settings.pin_tft_d4 = TFT_D4;
  tft_settings.pin_tft_d5 = TFT_D5;
  tft_settings.pin_tft_d6 = TFT_D6;
  tft_settings.pin_tft_d7 = TFT_D7;
#else
  tft_settings.pin_tft_d0 = -1;
  tft_settings.pin_tft_d1 = -1;
  tft_settings.pin_tft_d2 = -1;
  tft_settings.pin_tft_d3 = -1;
  tft_settings.pin_tft_d4 = -1;
  tft_settings.pin_tft_d5 = -1;
  tft_settings.pin_tft_d6 = -1;
  tft_settings.pin_tft_d7 = -1;
#endif

#if defined (TFT_BL)
  tft_settings.pin_tft_led = TFT_BL;
#endif

#if defined (TFT_BACKLIGHT_ON)
  tft_settings.pin_tft_led_on = TFT_BACKLIGHT_ON;
#endif

#if defined (TOUCH_CS)
  tft_settings.pin_tch_cs   = TOUCH_CS;
  tft_settings.tch_spi_freq = SPI_TOUCH_FREQUENCY/100000;
#else
  tft_settings.pin_tch_cs   = -1;
  tft_settings.tch_spi_freq = 0;
#endif
}


 
#ifdef TOUCH_CS
  #include "Extensions/Touch.cpp.h"
#endif

#include "Extensions/Button.cpp.h"

#include "Extensions/Sprite.cpp.h"

#ifdef SMOOTH_FONT
  #include "Extensions/Smooth_font.cpp.h"
#endif

#ifdef AA_GRAPHICS
  #include "Extensions/AA_graphics.cpp.h"   
#endif
 
