  
  

 public:
            
  uint8_t  getTouchRaw(uint16_t *x, uint16_t *y);
            
  uint16_t getTouchRawZ(void);
            
  void     convertRawXY(uint16_t *x, uint16_t *y);
            
            
            
            
            
            
            
  uint8_t  getTouch(uint16_t *x, uint16_t *y, uint16_t threshold = 600);

            
  void     calibrateTouch(uint16_t *data, uint32_t color_fg, uint32_t color_bg, uint8_t size);
            
  void     setTouch(uint16_t *data);

 private:
            
  void     spi_begin_touch();
  void     spi_end_touch();

            
  inline void begin_touch_read_write() __attribute__((always_inline));
  inline void end_touch_read_write()   __attribute__((always_inline));

            
  uint8_t  validTouch(uint16_t *x, uint16_t *y, uint16_t threshold = 600);

            
  uint16_t touchCalibration_x0 = 300, touchCalibration_x1 = 3600, touchCalibration_y0 = 300, touchCalibration_y1 = 3600;
  uint8_t  touchCalibration_rotate = 1, touchCalibration_invert_x = 2, touchCalibration_invert_y = 0;

  uint32_t _pressTime;         
  uint16_t _pressX, _pressY;   
