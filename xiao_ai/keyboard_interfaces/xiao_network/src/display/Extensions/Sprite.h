






class TFT_eSprite : public TFT_eSPI {

 public:

  explicit TFT_eSprite(TFT_eSPI *tft);
  ~TFT_eSprite(void);

            
            
            
            
            
            
            
  void*    createSprite(int16_t width, int16_t height, uint8_t frames = 1);

            
  void*    getPointer(void);

            
  bool     created(void);

            
  void     deleteSprite(void);

            
            
  void*    frameBuffer(int8_t f);
  
            
            
  void*    setColorDepth(int8_t b);
  int8_t   getColorDepth(void);

            
  void     createPalette(uint16_t *palette = nullptr, uint8_t colors = 16);        
  void     createPalette(const uint16_t *palette = nullptr, uint8_t colors = 16);  

            
  void     setPaletteColor(uint8_t index, uint16_t color);

            
  uint16_t getPaletteColor(uint8_t index);

            
  void     setBitmapColor(uint16_t fg, uint16_t bg);

            
  void     drawPixel(int32_t x, int32_t y, uint32_t color);

            
  void     drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size),

            
           fillSprite(uint32_t color),

            
            
           setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1),
            
           pushColor(uint16_t color),
            
           pushColor(uint16_t color, uint32_t len),
            
           writeColor(uint16_t color),

            
            
           setScrollRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color = TFT_BLACK),
            
            
            
           scroll(int16_t dx, int16_t dy = 0),

            
           drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color),
           drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color),
           drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color),

            
           fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

            
            
            
  void     setRotation(uint8_t rotation);
  uint8_t  getRotation(void);

            
  bool     pushRotated(int16_t angle, uint32_t transp = 0x00FFFFFF);
            
  bool     pushRotated(TFT_eSprite *spr, int16_t angle, uint32_t transp = 0x00FFFFFF);

            
  bool     getRotatedBounds(int16_t angle, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y);
            
  bool     getRotatedBounds(TFT_eSprite *spr, int16_t angle, int16_t *min_x, int16_t *min_y,
                                                             int16_t *max_x, int16_t *max_y);
            
  void     getRotatedBounds(int16_t angle, int16_t w, int16_t h, int16_t xp, int16_t yp,
                            int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y);

            
  uint16_t readPixel(int32_t x0, int32_t y0);

            
            
  uint16_t readPixelValue(int32_t x, int32_t y);

            
  void     pushImage(int32_t x0, int32_t y0, int32_t w, int32_t h, uint16_t *data, uint8_t sbpp = 0);
  void     pushImage(int32_t x0, int32_t y0, int32_t w, int32_t h, const uint16_t *data);

            
            
  void     pushSprite(int32_t x, int32_t y);
  void     pushSprite(int32_t x, int32_t y, uint16_t transparent);

            
  bool     pushSprite(int32_t tx, int32_t ty, int32_t sx, int32_t sy, int32_t sw, int32_t sh);

            
  bool     pushToSprite(TFT_eSprite *dspr, int32_t x, int32_t y);
  bool     pushToSprite(TFT_eSprite *dspr, int32_t x, int32_t y, uint16_t transparent);

            
  int16_t  drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font),
           drawChar(uint16_t uniCode, int32_t x, int32_t y);

            
  int16_t  width(void),
           height(void);

            
            
  void     drawGlyph(uint16_t code);
            
  void     printToSprite(String string);
            
  void     printToSprite(char *cbuffer, uint16_t len);
            
  int16_t  printToSprite(int16_t x, int16_t y, uint16_t index);

 private:

  TFT_eSPI *_tft;

            
  void*    callocSprite(int16_t width, int16_t height, uint8_t frames = 1);

            
  void     begin_nin_write(void) { ; }
  void     end_nin_write(void) { ; }

 protected:

  uint8_t  _bpp;      
  uint16_t *_img;     
  uint8_t  *_img8;    
  uint8_t  *_img4;    
  uint8_t  *_img8_1;  
  uint8_t  *_img8_2;  

  uint16_t *_colorMap;  

  int32_t  _sinra;    
  int32_t  _cosra;    

  bool     _created;  
  bool     _gFont = false; 

  int32_t  _xs, _ys, _xe, _ye, _xptr, _yptr;  
  int32_t  _sx, _sy;  
  uint32_t _sw, _sh;  
  uint32_t _scolor;   

  int32_t  _iwidth, _iheight;  
  int32_t  _dwidth, _dheight;  
  int32_t  _bitwidth;          

};
