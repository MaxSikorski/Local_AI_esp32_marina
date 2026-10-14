         
         
         

 

 
 
 

 
#if !defined (TFT_PARALLEL_8_BIT)
  #ifdef CONFIG_IDF_TARGET_ESP32
    #ifdef USE_HSPI_PORT
      SPIClass spi = SPIClass(HSPI);
    #elif defined(USE_FSPI_PORT)
      SPIClass spi = SPIClass(FSPI);
    #else  
      SPIClass spi = SPIClass(VSPI);
    #endif
  #else
    #ifdef USE_HSPI_PORT
      SPIClass spi = SPIClass(HSPI);
    #elif defined(USE_FSPI_PORT)
      SPIClass spi = SPIClass(FSPI);
    #else  
      SPIClass& spi = SPI;
    #endif
  #endif
#endif

#ifdef ESP32_DMA
   
  spi_device_handle_t dmaHAL;
  #ifdef CONFIG_IDF_TARGET_ESP32
    #define DMA_CHANNEL 1
    #ifdef USE_HSPI_PORT
      spi_host_device_t spi_host = HSPI_HOST;
    #elif defined(USE_FSPI_PORT)
      spi_host_device_t spi_host = SPI_HOST;
    #else  
      spi_host_device_t spi_host = VSPI_HOST;
    #endif
  #else
    #ifdef USE_HSPI_PORT
      #define DMA_CHANNEL SPI_DMA_CH_AUTO
      spi_host_device_t spi_host = SPI3_HOST;
    #else  
      #define DMA_CHANNEL SPI_DMA_CH_AUTO
      spi_host_device_t spi_host = SPI2_HOST;
    #endif
  #endif
#endif

 
#if defined (TFT_SDA_READ) && !defined (TFT_PARALLEL_8_BIT)
 





void TFT_eSPI::begin_SDA_Read(void)
{
  gpio_set_direction((gpio_num_t)TFT_MOSI, GPIO_MODE_INPUT);
  pinMatrixInAttach(TFT_MOSI, FSPIQ_IN_IDX, false);
  SET_BUS_READ_MODE;
}





void TFT_eSPI::end_SDA_Read(void)
{
  gpio_set_direction((gpio_num_t)TFT_MOSI, GPIO_MODE_OUTPUT);
  pinMatrixOutAttach(TFT_MOSI, FSPID_OUT_IDX, false, false);
  SET_BUS_WRITE_MODE;
}
 
#endif  
 






 
uint8_t TFT_eSPI::readByte(void)
{
  uint8_t b = 0xAA;

#if defined (TFT_PARALLEL_8_BIT)
  RD_L;
  b  = gpio_get_level((gpio_num_t)TFT_D0);  
  b  = gpio_get_level((gpio_num_t)TFT_D0);
  b  = gpio_get_level((gpio_num_t)TFT_D0);  

   
  b  = (gpio_get_level((gpio_num_t)TFT_D0) << 0);
  b |= (gpio_get_level((gpio_num_t)TFT_D1) << 1);
  b |= (gpio_get_level((gpio_num_t)TFT_D2) << 2);
  b |= (gpio_get_level((gpio_num_t)TFT_D3) << 3);
  b |= (gpio_get_level((gpio_num_t)TFT_D4) << 4);
  b |= (gpio_get_level((gpio_num_t)TFT_D5) << 5);
  b |= (gpio_get_level((gpio_num_t)TFT_D6) << 6);
  b |= (gpio_get_level((gpio_num_t)TFT_D7) << 7);
  RD_H;
#endif

  return b;
}

 
#ifdef TFT_PARALLEL_8_BIT
 





void TFT_eSPI::busDir(uint32_t mask, uint8_t mode)
{
   
  pinMode(TFT_D0, mode);
  pinMode(TFT_D1, mode);
  pinMode(TFT_D2, mode);
  pinMode(TFT_D3, mode);
  pinMode(TFT_D4, mode);
  pinMode(TFT_D5, mode);
  pinMode(TFT_D6, mode);
  pinMode(TFT_D7, mode);
}





void TFT_eSPI::gpioMode(uint8_t gpio, uint8_t mode)
{
  pinMode(gpio, mode);
  digitalWrite(gpio, HIGH);
}
 
#endif  
 


 
#if defined (RPI_WRITE_STROBE) && !defined (TFT_PARALLEL_8_BIT)  
 





void TFT_eSPI::pushBlock(uint16_t color, uint32_t len)
{
  uint8_t colorBin[] = { (uint8_t) (color >> 8), (uint8_t) color };
  if(len) spi.writePattern(&colorBin[0], 2, 1); len--;
  while(len--) {WR_L; WR_H;}
}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len)
{
  uint8_t *data = (uint8_t*)data_in;

  if(_swapBytes) {
      while ( len-- ) {tft_Write_16(*data); data++;}
      return;
  }

  while ( len >=64 ) {spi.writePattern(data, 64, 1); data += 64; len -= 64; }
  if (len) spi.writePattern(data, len, 1);
}

 
#elif !defined (SPI_18BIT_DRIVER) && !defined (TFT_PARALLEL_8_BIT)  
 




















































 
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){

  volatile uint32_t* spi_w = _spi_w;
  uint32_t color32 = (color<<8 | color >>8)<<16 | (color<<8 | color >>8);
  uint32_t i = 0;
  uint32_t rem = len & 0x1F;
  len =  len - rem;

   
  if (rem)
  {
    while (*_spi_cmd&SPI_USR);
    for (i=0; i < rem; i+=2) *spi_w++ = color32;
    *_spi_mosi_dlen = (rem << 4) - 1;
#if CONFIG_IDF_TARGET_ESP32S3
    *_spi_cmd = SPI_UPDATE;
    while (*_spi_cmd & SPI_UPDATE);
#endif
    *_spi_cmd = SPI_USR;
    if (!len) return;  
    i = i>>1; while(i++<16) *spi_w++ = color32;
  }

  while (*_spi_cmd&SPI_USR);
  if (!rem) while (i++<16) *spi_w++ = color32;
  *_spi_mosi_dlen =  511;

   
  while(len)
  {
    while (*_spi_cmd&SPI_USR);
#if CONFIG_IDF_TARGET_ESP32S3
    *_spi_cmd = SPI_UPDATE;
    while (*_spi_cmd & SPI_UPDATE);
#endif
    *_spi_cmd = SPI_USR;
    len -= 32;
  }

   
   
}
 




void TFT_eSPI::pushSwapBytePixels(const void* data_in, uint32_t len){

  uint8_t* data = (uint8_t*)data_in;
  uint32_t color[16];

  if (len > 31)
  {
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), 511);
    while(len>31)
    {
      uint32_t i = 0;
      while(i<16)
      {
        color[i++] = DAT8TO32(data);
        data+=4;
      }
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
      WRITE_PERI_REG(SPI_W0_REG(SPI_PORT),  color[0]);
      WRITE_PERI_REG(SPI_W1_REG(SPI_PORT),  color[1]);
      WRITE_PERI_REG(SPI_W2_REG(SPI_PORT),  color[2]);
      WRITE_PERI_REG(SPI_W3_REG(SPI_PORT),  color[3]);
      WRITE_PERI_REG(SPI_W4_REG(SPI_PORT),  color[4]);
      WRITE_PERI_REG(SPI_W5_REG(SPI_PORT),  color[5]);
      WRITE_PERI_REG(SPI_W6_REG(SPI_PORT),  color[6]);
      WRITE_PERI_REG(SPI_W7_REG(SPI_PORT),  color[7]);
      WRITE_PERI_REG(SPI_W8_REG(SPI_PORT),  color[8]);
      WRITE_PERI_REG(SPI_W9_REG(SPI_PORT),  color[9]);
      WRITE_PERI_REG(SPI_W10_REG(SPI_PORT), color[10]);
      WRITE_PERI_REG(SPI_W11_REG(SPI_PORT), color[11]);
      WRITE_PERI_REG(SPI_W12_REG(SPI_PORT), color[12]);
      WRITE_PERI_REG(SPI_W13_REG(SPI_PORT), color[13]);
      WRITE_PERI_REG(SPI_W14_REG(SPI_PORT), color[14]);
      WRITE_PERI_REG(SPI_W15_REG(SPI_PORT), color[15]);
#if CONFIG_IDF_TARGET_ESP32S3
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_UPDATE);
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_UPDATE);
#endif
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
      len -= 32;
    }
  }

  if (len > 15)
  {
    uint32_t i = 0;
    while(i<8)
    {
      color[i++] = DAT8TO32(data);
      data+=4;
    }
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), 255);
    WRITE_PERI_REG(SPI_W0_REG(SPI_PORT),  color[0]);
    WRITE_PERI_REG(SPI_W1_REG(SPI_PORT),  color[1]);
    WRITE_PERI_REG(SPI_W2_REG(SPI_PORT),  color[2]);
    WRITE_PERI_REG(SPI_W3_REG(SPI_PORT),  color[3]);
    WRITE_PERI_REG(SPI_W4_REG(SPI_PORT),  color[4]);
    WRITE_PERI_REG(SPI_W5_REG(SPI_PORT),  color[5]);
    WRITE_PERI_REG(SPI_W6_REG(SPI_PORT),  color[6]);
    WRITE_PERI_REG(SPI_W7_REG(SPI_PORT),  color[7]);
#if CONFIG_IDF_TARGET_ESP32S3
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_UPDATE);
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_UPDATE);
#endif
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
    len -= 16;
  }

  if (len)
  {
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), (len << 4) - 1);
    for (uint32_t i=0; i <= (len<<1); i+=4) {
      WRITE_PERI_REG(SPI_W0_REG(SPI_PORT)+i, DAT8TO32(data)); data+=4;
    }
#if CONFIG_IDF_TARGET_ESP32S3
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_UPDATE);
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_UPDATE);
#endif
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
  }
  while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);

}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  if(_swapBytes) {
    pushSwapBytePixels(data_in, len);
    return;
  }

  uint32_t *data = (uint32_t*)data_in;

  if (len > 31)
  {
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), 511);
    while(len>31)
    {
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
      WRITE_PERI_REG(SPI_W0_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W1_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W2_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W3_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W4_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W5_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W6_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W7_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W8_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W9_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W10_REG(SPI_PORT), *data++);
      WRITE_PERI_REG(SPI_W11_REG(SPI_PORT), *data++);
      WRITE_PERI_REG(SPI_W12_REG(SPI_PORT), *data++);
      WRITE_PERI_REG(SPI_W13_REG(SPI_PORT), *data++);
      WRITE_PERI_REG(SPI_W14_REG(SPI_PORT), *data++);
      WRITE_PERI_REG(SPI_W15_REG(SPI_PORT), *data++);
#if CONFIG_IDF_TARGET_ESP32S3
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_UPDATE);
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_UPDATE);
#endif
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
      len -= 32;
    }
  }

  if (len)
  {
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), (len << 4) - 1);
    for (uint32_t i=0; i <= (len<<1); i+=4) WRITE_PERI_REG((SPI_W0_REG(SPI_PORT) + i), *data++);
#if CONFIG_IDF_TARGET_ESP32S3
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_UPDATE);
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_UPDATE);
#endif
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
  }
  while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
}

 
#elif defined (SPI_18BIT_DRIVER)  
 





void TFT_eSPI::pushBlock(uint16_t color, uint32_t len)
{
   
  uint32_t r = (color & 0xF800)>>8;
  uint32_t g = (color & 0x07E0)<<5;
  uint32_t b = (color & 0x001F)<<19;
   
  uint32_t r0 = r<<24 | b | g | r;
  uint32_t r1 = r0>>8 | g<<16;
  uint32_t r2 = r1>>8 | b<<8;

  if (len > 19)
  {
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), 479);

    while(len>19)
    {
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
      WRITE_PERI_REG(SPI_W0_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W1_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W2_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W3_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W4_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W5_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W6_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W7_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W8_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W9_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W10_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W11_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W12_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W13_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W14_REG(SPI_PORT), r2);
#if CONFIG_IDF_TARGET_ESP32S3
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_UPDATE);
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_UPDATE);
#endif
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
      len -= 20;
    }
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
  }

  if (len)
  {
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), (len * 24) - 1);
    WRITE_PERI_REG(SPI_W0_REG(SPI_PORT), r0);
    WRITE_PERI_REG(SPI_W1_REG(SPI_PORT), r1);
    WRITE_PERI_REG(SPI_W2_REG(SPI_PORT), r2);
    WRITE_PERI_REG(SPI_W3_REG(SPI_PORT), r0);
    WRITE_PERI_REG(SPI_W4_REG(SPI_PORT), r1);
    WRITE_PERI_REG(SPI_W5_REG(SPI_PORT), r2);
    if (len > 8 )
    {
      WRITE_PERI_REG(SPI_W6_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W7_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W8_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W9_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W10_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W11_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W12_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W13_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W14_REG(SPI_PORT), r2);
    }
#if CONFIG_IDF_TARGET_ESP32S3
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_UPDATE);
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_UPDATE);
#endif
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
  }
}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
   
  if(!_swapBytes) { while ( len-- ) {tft_Write_16S(*data); data++;} }
  else { while ( len-- ) {tft_Write_16(*data); data++;} }
}





void TFT_eSPI::pushSwapBytePixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
   
  while ( len-- ) {tft_Write_16(*data); data++;}
}

 
#elif defined (TFT_PARALLEL_8_BIT)  
 





void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){
  if ( (color >> 8) == (color & 0x00FF) )
  { if (!len) return;
    tft_Write_16(color);
  #if defined (SSD1963_DRIVER)
    while (--len) {WR_L; WR_H; WR_L; WR_H; WR_L; WR_H;}
  #else
    #ifdef PSEUDO_16_BIT
      while (--len) {WR_L; WR_H;}
    #else
      while (--len) {WR_L; WR_H; WR_L; WR_H;}
    #endif
  #endif
  }
  else while (len--) {tft_Write_16(color);}
}





void TFT_eSPI::pushSwapBytePixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  while ( len-- ) {tft_Write_16(*data); data++;}
}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  if(_swapBytes) { while ( len-- ) {tft_Write_16(*data); data++; } }
  else { while ( len-- ) {tft_Write_16S(*data); data++;} }
}

 
#endif  
 


 
#if defined (ESP32_DMA) && !defined (TFT_PARALLEL_8_BIT)  
 





bool TFT_eSPI::dmaBusy(void)
{
  if (!DMA_Enabled || !spiBusyCheck) return false;

  spi_transaction_t *rtrans;
  esp_err_t ret;
  uint8_t checks = spiBusyCheck;
  for (int i = 0; i < checks; ++i)
  {
    ret = spi_device_get_trans_result(dmaHAL, &rtrans, 0);
    if (ret == ESP_OK) spiBusyCheck--;
  }

   
  if (spiBusyCheck ==0) return false;
  return true;
}






void TFT_eSPI::dmaWait(void)
{
  if (!DMA_Enabled || !spiBusyCheck) return;
  spi_transaction_t *rtrans;
  esp_err_t ret;
  for (int i = 0; i < spiBusyCheck; ++i)
  {
    ret = spi_device_get_trans_result(dmaHAL, &rtrans, portMAX_DELAY);
    assert(ret == ESP_OK);
  }
  spiBusyCheck = 0;
}






 
void TFT_eSPI::pushPixelsDMA(uint16_t* image, uint32_t len)
{
  if ((len == 0) || (!DMA_Enabled)) return;

  dmaWait();

  if(_swapBytes) {
    for (uint32_t i = 0; i < len; i++) (image[i] = image[i] << 8 | image[i] >> 8);
  }

   
   
   
   
  bool temp = _swapBytes;
  _swapBytes = false;
  while(len>0x4000) {  
    pushPixels(image, 0x400);
    len -= 0x400; image+= 0x400;  
  }
  _swapBytes = temp;

  esp_err_t ret;
  static spi_transaction_t trans;

  memset(&trans, 0, sizeof(spi_transaction_t));

  trans.user = (void *)1;
  trans.tx_buffer = image;   
  trans.length = len * 16;         
  trans.flags = 0;                 

  ret = spi_device_queue_trans(dmaHAL, &trans, portMAX_DELAY);
  assert(ret == ESP_OK);

  spiBusyCheck++;
}






 
void TFT_eSPI::pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t const* image)
{
  if ((w == 0) || (h == 0) || (!DMA_Enabled)) return;

  uint16_t *buffer = (uint16_t*)image;
  uint32_t len = w*h;

  dmaWait();

  setAddrWindow(x, y, w, h);
   
   
   
   
  bool temp = _swapBytes;
  _swapBytes = false;
  while(len>0x4000) {  
    pushPixels(buffer, 0x400);
    len -= 0x400; buffer+= 0x400;  
  }
  _swapBytes = temp;

  esp_err_t ret;
  static spi_transaction_t trans;

  memset(&trans, 0, sizeof(spi_transaction_t));

  trans.user = (void *)1;
  trans.tx_buffer = buffer;    
  trans.length = len * 16;    
  trans.flags = 0;            

  ret = spi_device_queue_trans(dmaHAL, &trans, portMAX_DELAY);
  assert(ret == ESP_OK);

  spiBusyCheck++;
}






 
void TFT_eSPI::pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t* image, uint16_t* buffer)
{
  if ((x >= _vpW) || (y >= _vpH) || (!DMA_Enabled)) return;

  int32_t dx = 0;
  int32_t dy = 0;
  int32_t dw = w;
  int32_t dh = h;

  if (x < _vpX) { dx = _vpX - x; dw -= dx; x = _vpX; }
  if (y < _vpY) { dy = _vpY - y; dh -= dy; y = _vpY; }

  if ((x + dw) > _vpW ) dw = _vpW - x;
  if ((y + dh) > _vpH ) dh = _vpH - y;

  if (dw < 1 || dh < 1) return;

  uint32_t len = dw*dh;

  if (buffer == nullptr) {
    buffer = image;
    dmaWait();
  }

   
  if ( (dw != w) || (dh != h) ) {
    if(_swapBytes) {
      for (int32_t yb = 0; yb < dh; yb++) {
        for (int32_t xb = 0; xb < dw; xb++) {
          uint32_t src = xb + dx + w * (yb + dy);
          (buffer[xb + yb * dw] = image[src] << 8 | image[src] >> 8);
        }
      }
    }
    else {
      for (int32_t yb = 0; yb < dh; yb++) {
        memcpy((uint8_t*) (buffer + yb * dw), (uint8_t*) (image + dx + w * (yb + dy)), dw << 1);
      }
    }
  }
   
  else if (buffer != image || _swapBytes) {
    if(_swapBytes) {
      for (uint32_t i = 0; i < len; i++) (buffer[i] = image[i] << 8 | image[i] >> 8);
    }
    else {
      memcpy(buffer, image, len*2);
    }
  }

  if (spiBusyCheck) dmaWait();  

  setAddrWindow(x, y, dw, dh);

   
   
   
   
  bool temp = _swapBytes;
  _swapBytes = false;
  while(len>0x4000) {  
    pushPixels(buffer, 0x400);
    len -= 0x400; buffer+= 0x400;  
  }
  _swapBytes = temp;

  esp_err_t ret;
  static spi_transaction_t trans;

  memset(&trans, 0, sizeof(spi_transaction_t));

  trans.user = (void *)1;
  trans.tx_buffer = buffer;   
  trans.length = len * 16;    
  trans.flags = 0;            

  ret = spi_device_queue_trans(dmaHAL, &trans, portMAX_DELAY);
  assert(ret == ESP_OK);

  spiBusyCheck++;
}

 
 
 

 




extern "C" void dc_callback();

void IRAM_ATTR dc_callback(spi_transaction_t *spi_tx)
{
  if ((bool)spi_tx->user) {DC_D;}
  else {DC_C;}
}





extern "C" void dma_end_callback();

void IRAM_ATTR dma_end_callback(spi_transaction_t *spi_tx)
{
  WRITE_PERI_REG(SPI_DMA_CONF_REG(spi_host), 0);
}





bool TFT_eSPI::initDMA(bool ctrl_cs)
{
  if (DMA_Enabled) return false;

  esp_err_t ret;
  spi_bus_config_t buscfg = {
    .mosi_io_num = TFT_MOSI,
    .miso_io_num = TFT_MISO,
    .sclk_io_num = TFT_SCLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .data4_io_num = -1,
    .data5_io_num = -1,
    .data6_io_num = -1,
    .data7_io_num = -1,
    .max_transfer_sz = 65536,  
    .flags = 0,
    .intr_flags = 0
  };

  int8_t pin = -1;
  if (ctrl_cs) pin = TFT_CS;

  spi_device_interface_config_t devcfg = {
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .mode = TFT_SPI_MODE,
    .duty_cycle_pos = 0,
    .cs_ena_pretrans = 0,
    .cs_ena_posttrans = 0,
    .clock_speed_hz = SPI_FREQUENCY,
    .input_delay_ns = 0,
    .spics_io_num = pin,
    .flags = SPI_DEVICE_NO_DUMMY,  
    .queue_size = 1,             
    .pre_cb = 0,  
    .post_cb = dma_end_callback  
  };
  ret = spi_bus_initialize(spi_host, &buscfg, DMA_CHANNEL);
  ESP_ERROR_CHECK(ret);
  ret = spi_bus_add_device(spi_host, &devcfg, &dmaHAL);
  ESP_ERROR_CHECK(ret);

  DMA_Enabled = true;
  spiBusyCheck = 0;
  return true;
}





void TFT_eSPI::deInitDMA(void)
{
  if (!DMA_Enabled) return;
  spi_bus_remove_device(dmaHAL);
  spi_bus_free(spi_host);
  DMA_Enabled = false;
}

 
#endif  
 
