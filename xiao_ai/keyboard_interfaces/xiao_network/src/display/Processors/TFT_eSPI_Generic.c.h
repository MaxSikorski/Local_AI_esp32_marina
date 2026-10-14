         
         
         

 
 
 

 
#ifdef TFT_SPI_PORT
  SPIClass& spi = TFT_SPI_PORT;
#else
  SPIClass& spi = SPI;
#endif

 
#if defined (TFT_SDA_READ) && !defined (TFT_PARALLEL_8_BIT)
 





uint8_t TFT_eSPI::tft_Read_8(void)
{
  uint8_t  ret = 0;

  for (uint8_t i = 0; i < 8; i++) {   
    ret <<= 1;
    SCLK_L;
    if (digitalRead(TFT_MOSI)) ret |= 1;
    SCLK_H;
  }

  return ret;
}





void TFT_eSPI::begin_SDA_Read(void)
{
   
  spi.end();
}





void TFT_eSPI::end_SDA_Read(void)
{
   
  spi.begin();
}

 
#endif  
 


 
#if defined (TFT_PARALLEL_8_BIT)  
 





void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){

  while (len>1) {tft_Write_32D(color); len-=2;}
  if (len) {tft_Write_16(color);}
}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  if(_swapBytes) {
    while (len>1) {tft_Write_16(*data); data++; tft_Write_16(*data); data++; len -=2;}
    if (len) {tft_Write_16(*data);}
    return;
  }

  while (len>1) {tft_Write_16S(*data); data++; tft_Write_16S(*data); data++; len -=2;}
  if (len) {tft_Write_16S(*data);}
}





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
  return;
}





void TFT_eSPI::gpioMode(uint8_t gpio, uint8_t mode)
{
   
}





uint8_t TFT_eSPI::readByte(void)
{
  uint8_t b = 0;

  busDir(0, INPUT);
  digitalWrite(TFT_RD, LOW);

  b |= digitalRead(TFT_D0) << 0;
  b |= digitalRead(TFT_D1) << 1;
  b |= digitalRead(TFT_D2) << 2;
  b |= digitalRead(TFT_D3) << 3;
  b |= digitalRead(TFT_D4) << 4;
  b |= digitalRead(TFT_D5) << 5;
  b |= digitalRead(TFT_D6) << 6;
  b |= digitalRead(TFT_D7) << 7;

  digitalWrite(TFT_RD, HIGH);
  busDir(0, OUTPUT); 

  return b;
}

 
#elif defined (RPI_WRITE_STROBE)   
 





void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){

  if(len) { tft_Write_16(color); len--; }
  while(len--) {WR_L; WR_H;}
}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len)
{
  uint16_t *data = (uint16_t*)data_in;

  if (_swapBytes) while ( len-- ) {tft_Write_16S(*data); data++;}
  else while ( len-- ) {tft_Write_16(*data); data++;}
}

 
#elif defined (SPI_18BIT_DRIVER)  
 





void TFT_eSPI::pushBlock(uint16_t color, uint32_t len)
{
   
  uint8_t r = (color & 0xF800)>>8;
  uint8_t g = (color & 0x07E0)>>3;
  uint8_t b = (color & 0x001F)<<3;

  while ( len-- ) {tft_Write_8(r); tft_Write_8(g); tft_Write_8(b);}
}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  if (_swapBytes) {
    while ( len-- ) {
      uint16_t color = *data >> 8 | *data << 8;
      tft_Write_8((color & 0xF800)>>8);
      tft_Write_8((color & 0x07E0)>>3);
      tft_Write_8((color & 0x001F)<<3);
      data++;
    }
  }
  else {
    while ( len-- ) {
      tft_Write_8((*data & 0xF800)>>8);
      tft_Write_8((*data & 0x07E0)>>3);
      tft_Write_8((*data & 0x001F)<<3);
      data++;
    }
  }
}

 
#else  
 





void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){

  while ( len-- ) {tft_Write_16(color);}
}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;

  if (_swapBytes) while ( len-- ) {tft_Write_16(*data); data++;}
  else while ( len-- ) {tft_Write_16S(*data); data++;}
}

 
#endif  
 


 
 
 

 











