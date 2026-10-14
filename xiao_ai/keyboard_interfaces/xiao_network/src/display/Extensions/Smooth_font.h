  
  

 public:

   
  void     loadFont(const uint8_t array[]);
#ifdef FONT_FS_AVAILABLE
  void     loadFont(String fontName, fs::FS &ffs);
#endif
  void     loadFont(String fontName, bool flash = true);
  void     unloadFont( void );
  bool     getUnicodeIndex(uint16_t unicode, uint16_t *index);

  virtual void drawGlyph(uint16_t code);

  void     showFont(uint32_t td);

  
  typedef struct
  {
    const uint8_t* gArray;            
    uint16_t gCount;                  
    uint16_t yAdvance;                
    uint16_t spaceWidth;              
    int16_t  ascent;                  
    int16_t  descent;                 
    uint16_t maxAscent;               
    uint16_t maxDescent;              
  } fontMetrics;

fontMetrics gFont = { nullptr, 0, 0, 0, 0, 0, 0, 0 };

   
  uint16_t* gUnicode = NULL;   
  uint8_t*  gHeight = NULL;    
  uint8_t*  gWidth = NULL;     
  uint8_t*  gxAdvance = NULL;  
  int16_t*  gdY = NULL;        
  int8_t*   gdX = NULL;        
  uint32_t* gBitmap = NULL;    

  bool     fontLoaded = false;  

#ifdef FONT_FS_AVAILABLE
  fs::File fontFile;
  fs::FS   &fontFS  = SPIFFS;
  bool     spiffs   = true;
  bool     fs_font = false;     

#else
  bool     fontFile = true;
#endif

  private:

  void     loadMetrics(void);
  uint32_t readInt32(void);

  uint8_t* fontPtr = nullptr;

