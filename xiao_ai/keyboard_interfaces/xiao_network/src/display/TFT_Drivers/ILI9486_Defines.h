 
 
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

 
#if defined (TFT_WR) && defined (RPI_ILI9486_DRIVER)
  #define RPI_WRITE_STROBE
#endif

 
 
#define ILI9486_BLACK       0x0000       
#define ILI9486_NAVY        0x000F       
#define ILI9486_DARKGREEN   0x03E0       
#define ILI9486_DARKCYAN    0x03EF       
#define ILI9486_MAROON      0x7800       
#define ILI9486_PURPLE      0x780F       
#define ILI9486_OLIVE       0x7BE0       
#define ILI9486_LIGHTGREY   0xC618       
#define ILI9486_DARKGREY    0x7BEF       
#define ILI9486_BLUE        0x001F       
#define ILI9486_GREEN       0x07E0       
#define ILI9486_CYAN        0x07FF       
#define ILI9486_RED         0xF800       
#define ILI9486_MAGENTA     0xF81F       
#define ILI9486_YELLOW      0xFFE0       
#define ILI9486_WHITE       0xFFFF       
#define ILI9486_ORANGE      0xFD20       
#define ILI9486_GREENYELLOW 0xAFE5       
#define ILI9486_PINK        0xF81F


 
#define TFT_INIT_DELAY 0x80  


 
#define TFT_NOP     0x00
#define TFT_SWRST   0x01

#define TFT_INVOFF  0x20
#define TFT_INVON   0x21

#define TFT_DISPOFF 0x28
#define TFT_DISPON  0x29

#define TFT_CASET   0x2A
#define TFT_PASET   0x2B
#define TFT_RAMWR   0x2C

#define TFT_RAMRD   0x2E
#define TFT_IDXRD   0x00  

#define TFT_MADCTL  0x36
#define TFT_MAD_MY  0x80
#define TFT_MAD_MX  0x40
#define TFT_MAD_MV  0x20
#define TFT_MAD_ML  0x10
#define TFT_MAD_BGR 0x08
#define TFT_MAD_MH  0x04
#define TFT_MAD_SS  0x02
#define TFT_MAD_GS  0x01
#define TFT_MAD_RGB 0x00
