         
         
         

 
 
 

#if defined (TFT_PARALLEL_8_BIT)
   
#else
   
  #if !defined (TFT_MOSI) || !defined (TFT_MISO) || !defined (TFT_SCLK)
    SPIClass& spi = SPI;
  #else
    SPIClass spi(TFT_MOSI, TFT_MISO, TFT_SCLK);
  #endif
   
  SPI_HandleTypeDef spiHal;
#endif

#ifdef STM32_DMA
   
  DMA_HandleTypeDef dmaHal;
#endif

 
#if defined (TFT_SDA_READ) && !defined (TFT_PARALLEL_8_BIT)
 





uint8_t TFT_eSPI::tft_Read_8(void)
{
  uint8_t  ret = 0;
  uint32_t reg = 0;

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
     
    while (len>31) {
    #if !defined (SSD1963_DRIVER)
       
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
    #else
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
    #endif
      len-=32;
    }

    while (len>7) {
    #if !defined (SSD1963_DRIVER)
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
    #else
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
    #endif
      len-=8;
    }

  while (len--) {tft_Write_16(color);}
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
#if defined (STM_PORTA_DATA_BUS)
  #if defined (STM32F1xx)
    if (mode == OUTPUT) GPIOA->CRL = 0x33333333;
    else GPIOA->CRL = 0x88888888;
  #else
    if (mode == OUTPUT) GPIOA->MODER = (GPIOA->MODER & 0xFFFF0000) | 0x00005555;
    else GPIOA->MODER &= 0xFFFF0000;
  #endif
#elif defined (STM_PORTB_DATA_BUS)
  #if defined (STM32F1xx)
    if (mode == OUTPUT) GPIOB->CRL = 0x33333333;
    else GPIOB->CRL = 0x88888888;
  #else
    if (mode == OUTPUT) GPIOB->MODER = (GPIOB->MODER & 0xFFFF0000) | 0x00005555;
    else GPIOB->MODER &= 0xFFFF0000;
  #endif
#elif defined (STM_PORTC_DATA_BUS)
  #if defined (STM32F1xx)
    if (mode == OUTPUT) GPIOC->CRL = 0x33333333;
    else GPIOC->CRL = 0x88888888;
  #else
    if (mode == OUTPUT) GPIOC->MODER = (GPIOC->MODER & 0xFFFF0000) | 0x00005555;
    else GPIOC->MODER &= 0xFFFF0000;
  #endif
#elif defined (STM_PORTD_DATA_BUS)
  #if defined (STM32F1xx)
    if (mode == OUTPUT) GPIOD->CRL = 0x33333333;
    else GPIOD->CRL = 0x88888888;
  #else
    if (mode == OUTPUT) GPIOD->MODER = (GPIOD->MODER & 0xFFFF0000) | 0x00005555;
    else GPIOD->MODER &= 0xFFFF0000;
  #endif
#else
  if (mode == OUTPUT) {
    LL_GPIO_SetPinMode(D0_PIN_PORT, D0_PIN_MASK, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(D1_PIN_PORT, D1_PIN_MASK, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(D2_PIN_PORT, D2_PIN_MASK, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(D3_PIN_PORT, D3_PIN_MASK, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(D4_PIN_PORT, D4_PIN_MASK, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(D5_PIN_PORT, D5_PIN_MASK, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(D6_PIN_PORT, D6_PIN_MASK, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(D7_PIN_PORT, D7_PIN_MASK, LL_GPIO_MODE_OUTPUT);
  }
  else {
    LL_GPIO_SetPinMode(D0_PIN_PORT, D0_PIN_MASK, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(D1_PIN_PORT, D1_PIN_MASK, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(D2_PIN_PORT, D2_PIN_MASK, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(D3_PIN_PORT, D3_PIN_MASK, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(D4_PIN_PORT, D4_PIN_MASK, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(D5_PIN_PORT, D5_PIN_MASK, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(D6_PIN_PORT, D6_PIN_MASK, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(D7_PIN_PORT, D7_PIN_MASK, LL_GPIO_MODE_INPUT);
  }
#endif
}






void TFT_eSPI::gpioMode(uint8_t gpio, uint8_t mode)
{
  PinName pn = digitalPinToPinName(gpio);
   
  if (mode == OUTPUT) pin_function(pn, STM_PIN_DATA(STM_MODE_OUTPUT_PP, GPIO_NOPULL, 0));
   
  else pin_function(pn, STM_PIN_DATA(STM_MODE_INPUT, GPIO_PULLUP, 0));
}





uint8_t TFT_eSPI::readByte(void)
{
  uint8_t b = 0;

  RD_L;
#if defined (STM_PORTA_DATA_BUS)
  b = GPIOA->IDR;
  b = GPIOA->IDR;
  b = GPIOA->IDR;
  b = (GPIOA->IDR) & 0xFF;
#elif defined (STM_PORTB_DATA_BUS)
  b = GPIOB->IDR;
  b = GPIOB->IDR;
  b = GPIOB->IDR;
  b = (GPIOB->IDR) & 0xFF;
#elif defined (STM_PORTC_DATA_BUS)
  b = GPIOC->IDR;
  b = GPIOC->IDR;
  b = GPIOC->IDR;
  b = (GPIOC->IDR) & 0xFF;
#elif defined (STM_PORTD_DATA_BUS)
  b = GPIOD->IDR;
  b = GPIOD->IDR;
  b = GPIOD->IDR;
  b = (GPIOD->IDR) & 0xFF;
#else
  b  = RD_TFT_D0 | RD_TFT_D0 | RD_TFT_D0 | RD_TFT_D0;  

  b  = RD_TFT_D0 | RD_TFT_D1 | RD_TFT_D2 | RD_TFT_D3;
  b |= RD_TFT_D4 | RD_TFT_D5 | RD_TFT_D6 | RD_TFT_D7;
#endif
  RD_H;

  return b;
}

 
#elif defined (RPI_WRITE_STROBE)   
 





void TFT_eSPI::pushBlock(uint16_t color, uint32_t len)
{
  if(len) { tft_Write_16(color); len--; }
  while(len--) {WR_L; WR_H;}
}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len)
{
  uint16_t *data = (uint16_t*)data_in;

  if (_swapBytes) while ( len-- ) { tft_Write_16S(*data); data++;}
  else while ( len-- ) {tft_Write_16(*data); data++;}
}

 
#elif defined (SPI_18BIT_DRIVER)  
 





#define BUF_SIZE 240*3
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len)
{
   
   
   
  uint8_t r = (color & 0xF800)>>8;  
  uint8_t g = (color & 0x07E0)>>3;  
  uint8_t b = (color & 0x001F)<<3;  

  while(len--) {
    TX_FIFO = (r);
    TX_FIFO = (g);
    TX_FIFO = (b);
  }
   SPI_BUSY_CHECK;
}




void TFT_eSPI::pushPixels(const void* data_in, uint32_t len)
{
  uint16_t *data = (uint16_t*)data_in;

  if(!_swapBytes) {
    while ( len-- ) {
       
      TX_FIFO = (*data & 0xF8);  
      TX_FIFO = (*data & 0xE000)>>11 | (*data & 0x07)<<5;  
      TX_FIFO = (*data & 0x1F00)>>5;  
      data++;
    }
  }
  else {
    while ( len-- ) {
       
      TX_FIFO = (*data & 0xF800)>>8;  
      TX_FIFO = (*data & 0x07E0)>>3;  
      TX_FIFO = (*data & 0x001F)<<3;  
      data++;
    }
  }
  SPI_BUSY_CHECK;
}

 
#else  
 





#define BUF_SIZE 480





















void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){
     
    while (len>31) {
    #if !defined (SSD1963_DRIVER)
       
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
    #else
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
    #endif
      len-=32;
    }

    while (len>7) {
    #if !defined (SSD1963_DRIVER)
      tft_Write_32D(color); tft_Write_32D(color);
      tft_Write_32D(color); tft_Write_32D(color);
    #else
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
      tft_Write_16(color); tft_Write_16(color); tft_Write_16(color); tft_Write_16(color);
    #endif
      len-=8;
    }

  while (len--) {tft_Write_16(color);}
}






void TFT_eSPI::pushPixels(const void* data_in, uint32_t len)
{
  uint16_t *data = (uint16_t*)data_in;

  if(_swapBytes) {
    while ( len-- ) {
      TX_FIFO = (uint8_t)(*data>>8);
      TX_FIFO = (uint8_t)(*data);
      data++;
    }
  }
  else {
    while ( len-- ) {
       
      TX_FIFO = (uint8_t)(*data);
      TX_FIFO = (uint8_t)(*data>>8);
      data++;
    }
  }
  SPI_BUSY_CHECK;
}

 
#endif  
 


 
#if defined STM32_DMA && !defined (TFT_PARALLEL_8_BIT)  
 





 
bool TFT_eSPI::dmaBusy(void)
{
   
  return (spiHal.State == HAL_SPI_STATE_BUSY_TX);  
}






void TFT_eSPI::dmaWait(void)
{
   
  while (spiHal.State == HAL_SPI_STATE_BUSY_TX);  
}






 
void TFT_eSPI::pushPixelsDMA(uint16_t* image, uint32_t len)
{
  if (len == 0) return;

   
  while (spiHal.State == HAL_SPI_STATE_BUSY_TX);  

  if(_swapBytes) {
    for (uint32_t i = 0; i < len; i++) (image[i] = image[i] << 8 | image[i] >> 8);
  }

  HAL_SPI_Transmit_DMA(&spiHal, (uint8_t*)image, len << 1);
}






 
void TFT_eSPI::pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t* image, uint16_t* buffer)
{
  if ((x >= _vpW) || (y >= _vpH)) return;

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
    while (spiHal.State == HAL_SPI_STATE_BUSY_TX);  
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

  setWindow(x, y, x + dw - 1, y + dh - 1);

   
   
   
   
  while(len>0x7FFF) {  
    HAL_SPI_Transmit(&spiHal, (uint8_t*)buffer, 0x800<<1, HAL_MAX_DELAY);
    len -= 0x800; buffer+= 0x800;  
  }
   
  HAL_SPI_Transmit_DMA(&spiHal, (uint8_t*)buffer, len << 1);
}

 
 
 

 
#if defined (STM32F2xx) || defined (STM32F4xx) || defined (STM32F7xx)




  #if (TFT_SPI_PORT == 1)
    extern "C" void DMA2_Stream3_IRQHandler();
    void DMA2_Stream3_IRQHandler(void)
  #elif (TFT_SPI_PORT == 2)
    extern "C" void DMA1_Stream4_IRQHandler();
    void DMA1_Stream4_IRQHandler(void)
  #elif (TFT_SPI_PORT == 3)
    extern "C" void DMA1_Stream5_IRQHandler();
    void DMA1_Stream5_IRQHandler(void)
  #endif
  {
     
    HAL_DMA_IRQHandler(&dmaHal);
  }





 
 
 
bool TFT_eSPI::initDMA(bool ctrl_cs)
{
  ctrl_cs = ctrl_cs;  

  #if (TFT_SPI_PORT == 1)
    __HAL_RCC_DMA2_CLK_ENABLE();                            
    dmaHal.Init.Channel = DMA_CHANNEL_3;                    
  #elif (TFT_SPI_PORT == 2)
    __HAL_RCC_DMA1_CLK_ENABLE();                            
    dmaHal.Init.Channel = DMA_CHANNEL_0;                    
  #elif (TFT_SPI_PORT == 3)
    __HAL_RCC_DMA1_CLK_ENABLE();                            
    dmaHal.Init.Channel = DMA_CHANNEL_0;                    
  
  #endif

  dmaHal.Init.Mode =  DMA_NORMAL;  
  dmaHal.Init.Direction = DMA_MEMORY_TO_PERIPH;           
  dmaHal.Init.PeriphInc = DMA_PINC_DISABLE;               
  dmaHal.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;  
  dmaHal.Init.MemInc = DMA_MINC_ENABLE;                   
  dmaHal.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;     

  if (HAL_DMA_Init(&dmaHal) != HAL_OK){                   
     
    return DMA_Enabled = false;
  };
  #if (TFT_SPI_PORT == 1)
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);   
  #elif (TFT_SPI_PORT == 2)
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);   
  #elif (TFT_SPI_PORT == 3)
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  #endif

  __HAL_LINKDMA(&spiHal, hdmatx, dmaHal);    

  return DMA_Enabled = true;
}

#elif defined (STM32F1xx)  




  #if (TFT_SPI_PORT == 1)
    extern "C" void DMA1_Channel3_IRQHandler();
    void DMA1_Channel3_IRQHandler(void)
  #elif (TFT_SPI_PORT == 2)
    extern "C" void DMA1_Channel5_IRQHandler();
    void DMA1_Channel5_IRQHandler(void)
  #endif
  {
     
    HAL_DMA_IRQHandler(&dmaHal);
  }

 




bool TFT_eSPI::initDMA(bool ctrl_cs)
{
  ctrl_cs = ctrl_cs;  

  __HAL_RCC_DMA1_CLK_ENABLE();                            

  dmaHal.Init.Mode =  DMA_NORMAL;  
  dmaHal.Init.Direction = DMA_MEMORY_TO_PERIPH;           
  dmaHal.Init.PeriphInc = DMA_PINC_DISABLE;               
  dmaHal.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;  
  dmaHal.Init.MemInc = DMA_MINC_ENABLE;                   
  dmaHal.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;     
  dmaHal.Init.Priority = DMA_PRIORITY_LOW;                

  __HAL_LINKDMA(&spiHal, hdmatx, dmaHal);  

  if (HAL_DMA_Init(&dmaHal) != HAL_OK){                   
     
    return DMA_Enabled = false;
  };

  #if (TFT_SPI_PORT == 1)
    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);   
  #elif (TFT_SPI_PORT == 2)
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);   
  #endif

  return DMA_Enabled = true;
}
#endif  





void TFT_eSPI::deInitDMA(void)
{
  HAL_DMA_DeInit(&dmaHal);
  DMA_Enabled = false;
}

 
#endif  
 
