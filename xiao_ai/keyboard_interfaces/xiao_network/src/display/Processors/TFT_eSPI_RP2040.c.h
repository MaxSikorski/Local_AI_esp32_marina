         
         
         

 
 
 

#if !defined (RP2040_PIO_INTERFACE)  

   
  #ifdef ARDUINO_ARCH_MBED
     
    MbedSPI spi = MbedSPI(TFT_MISO, TFT_MOSI, TFT_SCLK);
  #else
     
     
    SPIClassRP2040 spi = SPIClassRP2040(SPI_X, TFT_MISO, -1, TFT_SCLK, TFT_MOSI);
  #endif

#else  

  #ifdef RP2040_PIO_SPI
    #if  defined (SPI_18BIT_DRIVER)
       
      #include "pio_SPI_18bit.pio.h"
    #else
       
      #include "pio_SPI.pio.h"
    #endif
  #elif defined (TFT_PARALLEL_8_BIT)
    #if defined (SSD1963_DRIVER)
       
      #include "pio_8bit_parallel_18bpp.pio.h"
    #else
       
      #include "pio_8bit_parallel.pio.h"
    #endif
  #else  
     
    #include "pio_16bit_parallel.pio.h"
  #endif

   
  #ifdef ARDUINO_ARCH_MBED
     
    #error The Arduino RP2040 MBED board package is not supported when PIO is used. Use the community package by Earle Philhower.
  #endif

   
  PIO tft_pio = pio0;      
  int8_t pio_sm = 0;   
   
  uint32_t program_offset  = 0;

   
  uint32_t pull_stall_mask = 0;

   
  uint32_t pio_instr_jmp8  = 0;
  uint32_t pio_instr_fill  = 0;
  uint32_t pio_instr_addr  = 0;

   
  uint32_t pio_instr_set_dc = 0;
  uint32_t pio_instr_clr_dc = 0;

#endif

#ifdef RP2040_DMA
  int32_t            dma_tx_channel;
  dma_channel_config dma_tx_config;
#endif

 
#if defined (TFT_SDA_READ) && !defined (RP2040_PIO_INTERFACE)
 





uint8_t TFT_eSPI::tft_Read_8(void)
{
  uint8_t  ret = 0;

  







  
  ret = spi.transfer(0x00);

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
 

 
#if defined (RP2040_PIO_INTERFACE)
 
#ifdef RP2040_PIO_SPI
void pioinit(uint32_t clock_freq) {

   
  tft_pio = pio0;
  if (!pio_can_add_program(tft_pio, &tft_io_program)) {
    tft_pio = pio1;
    if (!pio_can_add_program(tft_pio, &tft_io_program)) {
       
      return;
    }
  }

  pio_sm = pio_claim_unused_sm(tft_pio, false);

   
  program_offset = pio_add_program(tft_pio, &tft_io_program);

   
  pio_gpio_init(tft_pio, TFT_DC);
  pio_gpio_init(tft_pio, TFT_SCLK);
  pio_gpio_init(tft_pio, TFT_MOSI);

   
  pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, TFT_DC, 1, true);
  pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, TFT_SCLK, 1, true);
  pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, TFT_MOSI, 1, true);

   
  pio_sm_config c = tft_io_program_get_default_config(program_offset);

  sm_config_set_set_pins(&c, TFT_DC, 1);
   
  sm_config_set_sideset_pins(&c, TFT_SCLK);
   
  sm_config_set_out_pins(&c, TFT_MOSI, 1);
   
  uint16_t clock_div = 0.98 + clock_get_hz(clk_sys) / (clock_freq * 2.0);  
  sm_config_set_clkdiv(&c, clock_div);
   
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
   
  sm_config_set_out_shift(&c, false, false, 0);
   
  pio_sm_init(tft_pio, pio_sm, program_offset + tft_io_offset_start_tx, &c);

   
  pio_sm_set_enabled(tft_pio, pio_sm, true);

   
  pull_stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + pio_sm);

   
  pio_instr_jmp8  = pio_encode_jmp(program_offset + tft_io_offset_start_8);
  pio_instr_fill  = pio_encode_jmp(program_offset + tft_io_offset_block_fill);
  pio_instr_addr  = pio_encode_jmp(program_offset + tft_io_offset_set_addr_window);

  pio_instr_set_dc = pio_encode_set((pio_src_dest)0, 1);
  pio_instr_clr_dc = pio_encode_set((pio_src_dest)0, 0);
}
#else  
void pioinit(uint16_t clock_div, uint16_t fract_div) {

   
  tft_pio = pio0;
  if (!pio_can_add_program(tft_pio, &tft_io_program)) {
    tft_pio = pio1;
    if (!pio_can_add_program(tft_pio, &tft_io_program)) {
       
      return;
    }
  }

  pio_sm = pio_claim_unused_sm(tft_pio, false);

  #if defined (TFT_PARALLEL_8_BIT)
    uint8_t bits = 8;
  #else  
    uint8_t bits = 16;
  #endif
  
   
  program_offset = pio_add_program(tft_pio, &tft_io_program);

   
  pio_gpio_init(tft_pio, TFT_DC);
  pio_gpio_init(tft_pio, TFT_WR);

  for (int i = 0; i < bits; i++) {
    pio_gpio_init(tft_pio, TFT_D0 + i);
  }

   
  pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, TFT_DC, 1, true);
  pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, TFT_WR, 1, true);
  pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, TFT_D0, bits, true);

   
  pio_sm_config c = tft_io_program_get_default_config(program_offset);
   
  sm_config_set_set_pins(&c, TFT_DC, 1);
   
  sm_config_set_sideset_pins(&c, TFT_WR);
   
  sm_config_set_out_pins(&c, TFT_D0, bits);
   
  sm_config_set_clkdiv_int_frac(&c, clock_div, fract_div);
   
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
   
  sm_config_set_out_shift(&c, false, false, 0);
   
  pio_sm_init(tft_pio, pio_sm, program_offset + tft_io_offset_start_tx, &c);

   
  pio_sm_set_enabled(tft_pio, pio_sm, true);

   
  pull_stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + pio_sm);

   
  pio_instr_jmp8  = pio_encode_jmp(program_offset + tft_io_offset_start_8);
  pio_instr_fill  = pio_encode_jmp(program_offset + tft_io_offset_block_fill);
  pio_instr_addr  = pio_encode_jmp(program_offset + tft_io_offset_set_addr_window);

   
  pio_instr_set_dc = pio_encode_set((pio_src_dest)0, 1);
  pio_instr_clr_dc = pio_encode_set((pio_src_dest)0, 0);
}
#endif





#ifdef RP2040_PIO_PUSHBLOCK
 
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len)
{
#if  defined (SPI_18BIT_DRIVER) || (defined (SSD1963_DRIVER) && defined (TFT_PARALLEL_8_BIT))
  uint32_t col = ((color & 0xF800)<<8) | ((color & 0x07E0)<<5) | ((color & 0x001F)<<3);
  if (len) {
    WAIT_FOR_STALL;
    tft_pio->sm[pio_sm].instr = pio_instr_fill;

    TX_FIFO = col;
    TX_FIFO = --len;  
  }
#else
  if (len) {
    WAIT_FOR_STALL;
    tft_pio->sm[pio_sm].instr = pio_instr_fill;

    TX_FIFO = color;
    TX_FIFO = --len;  
  }
#endif
}

#else
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){

  while (len > 4) {
     
    WAIT_FOR_FIFO_FREE(5);
    TX_FIFO = color;
    TX_FIFO = color;
    TX_FIFO = color;
    TX_FIFO = color;
    TX_FIFO = color;

    len -= 5;
  }

  if (len) {
     
    WAIT_FOR_FIFO_FREE(4);
    while (len--) TX_FIFO = color;
  }
}
#endif





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){
#if  defined (SPI_18BIT_DRIVER) || (defined (SSD1963_DRIVER) && defined (TFT_PARALLEL_8_BIT))
  uint16_t *data = (uint16_t*)data_in;
  if (_swapBytes) {
    while ( len-- ) {
      uint32_t col = *data++;
      tft_Write_16(col);
    }
  }
  else {
    while ( len-- ) {
      uint32_t col = *data++;
      tft_Write_16S(col);
    }
  }
#else
  const uint16_t *data = (uint16_t*)data_in;

   
  if(_swapBytes) {
    while (len > 4) {
      WAIT_FOR_FIFO_FREE(5);
      TX_FIFO = data[0];
      TX_FIFO = data[1];
      TX_FIFO = data[2];
      TX_FIFO = data[3];
      TX_FIFO = data[4];
      data += 5;
      len  -= 5;
    }

    if (len) {
      WAIT_FOR_FIFO_FREE(4);
      while(len--) TX_FIFO = *data++;
    }
  }
  else {
    while (len > 4) {
      WAIT_FOR_FIFO_FREE(5);
      TX_FIFO = data[0] << 8 | data[0] >> 8;
      TX_FIFO = data[1] << 8 | data[1] >> 8;
      TX_FIFO = data[2] << 8 | data[2] >> 8;
      TX_FIFO = data[3] << 8 | data[3] >> 8;
      TX_FIFO = data[4] << 8 | data[4] >> 8;
      data += 5;
      len  -= 5;
    }

    if (len) {
      WAIT_FOR_FIFO_FREE(4);
      while(len--) {
        TX_FIFO = *data << 8 | *data >> 8;
        data++;
      }
    }
  }
#endif
}





void TFT_eSPI::busDir(uint32_t mask, uint8_t mode)
{
   
  mask = mask;
  mode = mode;












}





void TFT_eSPI::gpioMode(uint8_t gpio, uint8_t mode)
{
   
  gpio = gpio;
  mode = mode;

}





uint8_t TFT_eSPI::readByte(void)
{
  uint8_t b = 0;
















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
  uint16_t r = (color & 0xF800)>>8;
  uint16_t g = (color & 0x07E0)>>3;
  uint16_t b = (color & 0x001F)<<3;

   
  if (len > 32) {
    uint32_t rg = r<<8 | g;
    uint32_t br = b<<8 | r;
    uint32_t gb = g<<8 | b;
     
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
    hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
    while ( len > 1 ) {
      while (!spi_is_writable(SPI_X)){}; spi_get_hw(SPI_X)->dr = rg;
      while (!spi_is_writable(SPI_X)){}; spi_get_hw(SPI_X)->dr = br;
      while (!spi_is_writable(SPI_X)){}; spi_get_hw(SPI_X)->dr = gb;
      len -= 2;
    }
     
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
    hw_write_masked(&spi_get_hw(SPI_X)->cr0, (8 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
  }

   
  while ( len-- ) {tft_Write_8N(r);tft_Write_8N(g);tft_Write_8N(b);}
}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  if (_swapBytes) {
    while ( len-- ) {
      uint32_t col = *data++;
      tft_Write_16(col);
    }
  }
  else {
    while ( len-- ) {
      uint32_t col = *data++;
      tft_Write_16S(col);
    }
  }
}

 
#else  
 





void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){
  while(len--)
  {
    while (!spi_is_writable(SPI_X)){};
    spi_get_hw(SPI_X)->dr = (uint32_t)color;
  }
}





void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){
  uint16_t *data = (uint16_t*)data_in;
  if (_swapBytes) {
    while(len--)
    {
      while (!spi_is_writable(SPI_X)){};
      spi_get_hw(SPI_X)->dr = (uint32_t)(*data++);
    }
  }
  else
  {
    while(len--)
    {
      uint16_t color = *data++;
      color = color >> 8 | color << 8;
      while (!spi_is_writable(SPI_X)){};
      spi_get_hw(SPI_X)->dr = (uint32_t)color;
    }
  }
}

 
#endif  
 


 
#ifdef RP2040_DMA  
 










bool TFT_eSPI::dmaBusy(void) {
  if (!DMA_Enabled) return false;

  if (dma_channel_is_busy(dma_tx_channel)) return true;

#if !defined (RP2040_PIO_INTERFACE)
   
  while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
  hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
#endif

  return false;
}





void TFT_eSPI::dmaWait(void)
{
  while (dma_channel_is_busy(dma_tx_channel));

#if !defined (RP2040_PIO_INTERFACE)
   
  while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
  hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
#endif
}





void TFT_eSPI::pushPixelsDMA(uint16_t* image, uint32_t len)
{
  if ((len == 0) || (!DMA_Enabled)) return;

  dmaWait();

  channel_config_set_bswap(&dma_tx_config, !_swapBytes);

#if !defined (RP2040_PIO_INTERFACE)
  dma_channel_configure(dma_tx_channel, &dma_tx_config, &spi_get_hw(SPI_X)->dr, (uint16_t*)image, len, true);
#else
  dma_channel_configure(dma_tx_channel, &dma_tx_config, &tft_pio->txf[pio_sm], (uint16_t*)image, len, true);
#endif
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
    for (int32_t yb = 0; yb < dh; yb++) {
      memmove((uint8_t*) (buffer + yb * dw), (uint8_t*) (image + dx + w * (yb + dy)), dw << 1);
    }
  }
   
  else if (buffer != image) {
    memcpy(buffer, image, len*2);
  }

  dmaWait();  

  setAddrWindow(x, y, dw, dh);

  channel_config_set_bswap(&dma_tx_config, !_swapBytes);

#if !defined (RP2040_PIO_INTERFACE)
  dma_channel_configure(dma_tx_channel, &dma_tx_config, &spi_get_hw(SPI_X)->dr, (uint16_t*)buffer, len, true);
#else
  dma_channel_configure(dma_tx_channel, &dma_tx_config, &tft_pio->txf[pio_sm], (uint16_t*)buffer, len, true);
#endif
}





bool TFT_eSPI::initDMA(bool ctrl_cs)
{
  if (DMA_Enabled) return false;

  ctrl_cs = ctrl_cs;  

  dma_tx_channel = dma_claim_unused_channel(false);
  
  if (dma_tx_channel < 0) return false;

  dma_tx_config = dma_channel_get_default_config(dma_tx_channel);

  channel_config_set_transfer_data_size(&dma_tx_config, DMA_SIZE_16);
#if !defined (RP2040_PIO_INTERFACE)
  channel_config_set_dreq(&dma_tx_config, spi_get_index(SPI_X) ? DREQ_SPI1_TX : DREQ_SPI0_TX);
#else
  channel_config_set_dreq(&dma_tx_config, pio_get_dreq(tft_pio, pio_sm, true));
#endif

  DMA_Enabled = true;
  return true;
}





void TFT_eSPI::deInitDMA(void)
{
  if (!DMA_Enabled) return;
  dma_channel_unclaim(dma_tx_channel);
  DMA_Enabled = false;
}

 
#endif  
 
