
 
 
 
 
 
 

{
	 
  static const uint8_t PROGMEM
  Bcmd[] = {                   
    18,                        
    ST7735_SWRESET,   TFT_INIT_DELAY,   
      50,                      
    ST7735_SLPOUT ,   TFT_INIT_DELAY,   
      255,                     
    ST7735_COLMOD , 1+TFT_INIT_DELAY,   
      0x05,                    
      10,                      
    ST7735_FRMCTR1, 3+TFT_INIT_DELAY,   
      0x00,                    
      0x06,                    
      0x03,                    
      10,                      
    ST7735_MADCTL , 1      ,   
      0x40 | TFT_MAD_COLOR_ORDER,  
    ST7735_DISSET5, 2      ,   
      0x15,                    
                               
      0x02,                    
    ST7735_INVCTR , 1      ,   
      0x0,                     
    ST7735_PWCTR1 , 2+TFT_INIT_DELAY,   
      0x02,                    
      0x70,                    
      10,                      
    ST7735_PWCTR2 , 1      ,   
      0x05,                    
    ST7735_PWCTR3 , 2      ,   
      0x01,                    
      0x02,                    
    ST7735_VMCTR1 , 2+TFT_INIT_DELAY,   
      0x3C,                    
      0x38,                    
      10,                      
    ST7735_PWCTR6 , 2      ,   
      0x11, 0x15,
    ST7735_GMCTRP1,16      ,   
      0x09, 0x16, 0x09, 0x20,  
      0x21, 0x1B, 0x13, 0x19,  
      0x17, 0x15, 0x1E, 0x2B,
      0x04, 0x05, 0x02, 0x0E,
    ST7735_GMCTRN1,16+TFT_INIT_DELAY,   
      0x0B, 0x14, 0x08, 0x1E,  
      0x22, 0x1D, 0x18, 0x1E,
      0x1B, 0x1A, 0x24, 0x2B,
      0x06, 0x06, 0x02, 0x0F,
      10,                      
    ST7735_CASET  , 4      ,   
      0x00, 0x02,              
      0x00, 0x81,              
    ST7735_RASET  , 4      ,   
      0x00, 0x02,              
      0x00, 0x81,              
    ST7735_NORON  ,   TFT_INIT_DELAY,   
      10,                      
    ST7735_DISPON ,   TFT_INIT_DELAY,   
      255 },                   

  Rcmd1[] = {                  
    15,                        
    ST7735_SWRESET,   TFT_INIT_DELAY,   
      150,                     
    ST7735_SLPOUT ,   TFT_INIT_DELAY,   
      255,                     
    ST7735_FRMCTR1, 3      ,   
      0x01, 0x2C, 0x2D,        
    ST7735_FRMCTR2, 3      ,   
      0x01, 0x2C, 0x2D,        
    ST7735_FRMCTR3, 6      ,   
      0x01, 0x2C, 0x2D,        
      0x01, 0x2C, 0x2D,        
    ST7735_INVCTR , 1      ,   
      0x07,                    
    ST7735_PWCTR1 , 3      ,   
      0xA2,
      0x02,                    
      0x84,                    
    ST7735_PWCTR2 , 1      ,   
      0xC5,                    
    ST7735_PWCTR3 , 2      ,   
      0x0A,                    
      0x00,                    
    ST7735_PWCTR4 , 2      ,   
      0x8A,                    
      0x2A,  
    ST7735_PWCTR5 , 2      ,   
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,   
      0x0E,
    ST7735_INVOFF , 0      ,   
    ST7735_MADCTL , 1      ,   
      0xC0 | TFT_MAD_COLOR_ORDER,  
    ST7735_COLMOD , 1      ,   
      0x05 },                  

  Rcmd2green[] = {             
    2,                         
    ST7735_CASET  , 4      ,   
      0x00, 0x02,              
      0x00, 0x7F+0x02,         
    ST7735_RASET  , 4      ,   
      0x00, 0x01,              
      0x00, 0x9F+0x01 },       

  Rcmd2red[] = {               
    2,                         
    ST7735_CASET  , 4      ,   
      0x00, 0x00,              
      0x00, 0x7F,              
    ST7735_RASET  , 4      ,   
      0x00, 0x00,              
      0x00, 0x9F },            

   
  Rcmd3RobotLCD[] = {
      3,
      ST7735_FRMCTR1, 2    ,   
        0x0B, 0x14,
      ST7735_FRMCTR2, 2    ,   
        0x0B, 0x14,
      ST7735_FRMCTR3, 4    ,   
        0x0B, 0x14,
        0x0B, 0x14 },

  Rcmd3[] = {                  
    4,                         
    ST7735_GMCTRP1, 16      ,  
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      ,  
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    TFT_INIT_DELAY,  
      10,                      
    ST7735_DISPON ,    TFT_INIT_DELAY,  
      100 };                   

     if (tabcolor == INITB)
     {
       commandList(Bcmd);
     }
     else 
     {
	     commandList(Rcmd1);

       if (tabcolor == INITR_GREENTAB)
       {
         commandList(Rcmd2green);
         colstart = 2;
         rowstart = 1;
       }
       else if (tabcolor == INITR_GREENTAB2)
       {
         commandList(Rcmd2green);
         writecommand(ST7735_MADCTL);
         writedata(0xC0 | TFT_MAD_COLOR_ORDER);
         colstart = 2;
         rowstart = 1;
       }
       else if (tabcolor == INITR_GREENTAB3)
       {
         commandList(Rcmd2green);
         colstart = 2;
         rowstart = 3;
       }
       else if (tabcolor == INITR_GREENTAB128)
       {
         commandList(Rcmd2green);
         colstart = 0;
         rowstart = 32;
       }
       else if (tabcolor == INITR_GREENTAB160x80)
       {
         commandList(Rcmd2green);
         writecommand(TFT_INVON);
         colstart = 26;
         rowstart = 1;
       }
       else if (tabcolor == INITR_ROBOTLCD)
       {
         commandList(Rcmd2green);
         commandList(Rcmd3RobotLCD);
       }
       else if (tabcolor == INITR_REDTAB160x80)
       {
         commandList(Rcmd2green);
         colstart = 24;
         rowstart = 0;
       }
       else if (tabcolor == INITR_REDTAB)
       {
         commandList(Rcmd2red);
       }
       else if (tabcolor == INITR_BLACKTAB)
       {
         writecommand(ST7735_MADCTL);
         writedata(0xC0 | TFT_MAD_COLOR_ORDER);
       }

       commandList(Rcmd3);
     }
}
