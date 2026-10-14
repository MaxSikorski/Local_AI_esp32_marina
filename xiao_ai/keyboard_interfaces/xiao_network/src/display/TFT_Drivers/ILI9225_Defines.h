 
 
#define TFT_WIDTH  176
#define TFT_HEIGHT 220

 
#define TFT_NOP     0x00
#define TFT_SWRST   0x28

#define TFT_CASET 0
#define TFT_PASET 0

#define TFT_CASET1     ILI9225_HORIZONTAL_WINDOW_ADDR2
#define TFT_CASET2     ILI9225_HORIZONTAL_WINDOW_ADDR1

#define TFT_PASET1     ILI9225_VERTICAL_WINDOW_ADDR2
#define TFT_PASET2     ILI9225_VERTICAL_WINDOW_ADDR1

#define TFT_RAM_ADDR1  ILI9225_RAM_ADDR_SET1
#define TFT_RAM_ADDR2  ILI9225_RAM_ADDR_SET2

#define TFT_RAMWR      ILI9225_GRAM_DATA_REG

#define TFT_MAD_BGR 0x10
#define TFT_MAD_RGB 0x00

#ifdef TFT_RGB_ORDER
  #if (TFT_RGB_ORDER == 1)
    #define TFT_MAD_COLOR_ORDER TFT_MAD_RGB
  #else
    #define TFT_MAD_COLOR_ORDER TFT_MAD_BGR
  #endif
#else
  #define TFT_MAD_COLOR_ORDER TFT_MAD_BGR
#endif

 
#define TFT_INVOFF  0x00
#define TFT_INVON   0x00
#define TFT_RAMRD   0x00
#define TFT_IDXRD   0x00

 
#define ILI9225_DRIVER_OUTPUT_CTRL      0x01   
#define ILI9225_LCD_AC_DRIVING_CTRL     0x02   
#define ILI9225_ENTRY_MODE              0x03   
#define ILI9225_DISP_CTRL1              0x07   
#define ILI9225_BLANK_PERIOD_CTRL1      0x08   
#define ILI9225_FRAME_CYCLE_CTRL        0x0B   
#define ILI9225_INTERFACE_CTRL          0x0C   
#define ILI9225_OSC_CTRL                0x0F   
#define ILI9225_POWER_CTRL1             0x10   
#define ILI9225_POWER_CTRL2             0x11   
#define ILI9225_POWER_CTRL3             0x12   
#define ILI9225_POWER_CTRL4             0x13   
#define ILI9225_POWER_CTRL5             0x14   
#define ILI9225_VCI_RECYCLING           0x15   
#define ILI9225_RAM_ADDR_SET1           0x20   
#define ILI9225_RAM_ADDR_SET2           0x21   
#define ILI9225_GRAM_DATA_REG           0x22   
#define ILI9225_GATE_SCAN_CTRL          0x30   
#define ILI9225_VERTICAL_SCROLL_CTRL1   0x31   
#define ILI9225_VERTICAL_SCROLL_CTRL2   0x32   
#define ILI9225_VERTICAL_SCROLL_CTRL3   0x33   
#define ILI9225_PARTIAL_DRIVING_POS1    0x34   
#define ILI9225_PARTIAL_DRIVING_POS2    0x35   
#define ILI9225_HORIZONTAL_WINDOW_ADDR1 0x36   
#define ILI9225_HORIZONTAL_WINDOW_ADDR2 0x37   
#define ILI9225_VERTICAL_WINDOW_ADDR1   0x38   
#define ILI9225_VERTICAL_WINDOW_ADDR2   0x39   
#define ILI9225_GAMMA_CTRL1             0x50   
#define ILI9225_GAMMA_CTRL2             0x51   
#define ILI9225_GAMMA_CTRL3             0x52   
#define ILI9225_GAMMA_CTRL4             0x53   
#define ILI9225_GAMMA_CTRL5             0x54   
#define ILI9225_GAMMA_CTRL6             0x55   
#define ILI9225_GAMMA_CTRL7             0x56   
#define ILI9225_GAMMA_CTRL8             0x57   
#define ILI9225_GAMMA_CTRL9             0x58   
#define ILI9225_GAMMA_CTRL10            0x59   

 
#define TFT_INIT_DELAY 0x00  

