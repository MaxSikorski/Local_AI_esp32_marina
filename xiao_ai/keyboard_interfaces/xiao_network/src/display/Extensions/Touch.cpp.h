 
 

 

 

 
 

 

 
#ifndef Z_THRESHOLD
  #define Z_THRESHOLD 350  
#endif





 
inline void TFT_eSPI::begin_touch_read_write(void){
  DMA_BUSY_CHECK;
  CS_H;  
  #if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS)
    if (locked) {locked = false; spi.beginTransaction(SPISettings(SPI_TOUCH_FREQUENCY, MSBFIRST, SPI_MODE0));}
  #else
    spi.setFrequency(SPI_TOUCH_FREQUENCY);
  #endif
  SET_BUS_READ_MODE;
  T_CS_L;
}





inline void TFT_eSPI::end_touch_read_write(void){
  T_CS_H;
  #if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS)
    if(!inTransaction) {if (!locked) {locked = true; spi.endTransaction();}}
  #else
    spi.setFrequency(SPI_FREQUENCY);
  #endif
   
}





void TFT_eSPI::spi_begin_touch() {begin_touch_read_write();}
void TFT_eSPI::spi_end_touch()   {  end_touch_read_write();}





uint8_t TFT_eSPI::getTouchRaw(uint16_t *x, uint16_t *y){
  uint16_t tmp;

  begin_touch_read_write();
  
   
  spi.transfer(0xd0);                     
  spi.transfer(0);                        
  spi.transfer(0xd0);                     
  spi.transfer(0);                        
  spi.transfer(0xd0);                     
  spi.transfer(0);                        
  spi.transfer(0xd0);                     

  tmp = spi.transfer(0);                    
  tmp = tmp <<5;
  tmp |= 0x1f & (spi.transfer(0x90)>>3);    

  *x = tmp;

   
  spi.transfer(0);                        
  spi.transfer(0x90);                     
  spi.transfer(0);                        
  spi.transfer(0x90);                     
  spi.transfer(0);                        
  spi.transfer(0x90);                     

  tmp = spi.transfer(0);                  
  tmp = tmp <<5;
  tmp |= 0x1f & (spi.transfer(0)>>3);     

  *y = tmp;

  end_touch_read_write();

  return true;
}





uint16_t TFT_eSPI::getTouchRawZ(void){

  begin_touch_read_write();

   
  int16_t tz = 0xFFF;
  spi.transfer(0xb0);                
  tz += spi.transfer16(0xc0) >> 3;   
  tz -= spi.transfer16(0x00) >> 3;   

  end_touch_read_write();

  if (tz == 4095) tz = 0;

  return (uint16_t)tz;
}





#define _RAWERR 20  
uint8_t TFT_eSPI::validTouch(uint16_t *x, uint16_t *y, uint16_t threshold){
  uint16_t x_tmp, y_tmp, x_tmp2, y_tmp2;

   
  uint16_t z1 = 1;
  uint16_t z2 = 0;
  while (z1 > z2)
  {
    z2 = z1;
    z1 = getTouchRawZ();
    delay(1);
  }

   

  if (z1 <= threshold) return false;
    
  getTouchRaw(&x_tmp,&y_tmp);

   
   

  delay(1);  
  if (getTouchRawZ() <= threshold) return false;

  delay(2);  
  getTouchRaw(&x_tmp2,&y_tmp2);
  
   
   

  if (abs(x_tmp - x_tmp2) > _RAWERR) return false;
  if (abs(y_tmp - y_tmp2) > _RAWERR) return false;
  
  *x = x_tmp;
  *y = y_tmp;
  
  return true;
}
  




uint8_t TFT_eSPI::getTouch(uint16_t *x, uint16_t *y, uint16_t threshold){
  uint16_t x_tmp, y_tmp;
  
  if (threshold<20) threshold = 20;
  if (_pressTime > millis()) threshold=20;

  uint8_t n = 5;
  uint8_t valid = 0;
  while (n--)
  {
    if (validTouch(&x_tmp, &y_tmp, threshold)) valid++;;
  }

  if (valid<1) { _pressTime = 0; return false; }
  
  _pressTime = millis() + 50;

  convertRawXY(&x_tmp, &y_tmp);

  if (x_tmp >= _width || y_tmp >= _height) return false;

  _pressX = x_tmp;
  _pressY = y_tmp;
  *x = _pressX;
  *y = _pressY;
  return valid;
}





void TFT_eSPI::convertRawXY(uint16_t *x, uint16_t *y)
{
  uint16_t x_tmp = *x, y_tmp = *y, xx, yy;

  if(!touchCalibration_rotate){
    xx=(x_tmp-touchCalibration_x0)*_width/touchCalibration_x1;
    yy=(y_tmp-touchCalibration_y0)*_height/touchCalibration_y1;
    if(touchCalibration_invert_x)
      xx = _width - xx;
    if(touchCalibration_invert_y)
      yy = _height - yy;
  } else {
    xx=(y_tmp-touchCalibration_x0)*_width/touchCalibration_x1;
    yy=(x_tmp-touchCalibration_y0)*_height/touchCalibration_y1;
    if(touchCalibration_invert_x)
      xx = _width - xx;
    if(touchCalibration_invert_y)
      yy = _height - yy;
  }
  *x = xx;
  *y = yy;
}





void TFT_eSPI::calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size){
  int16_t values[] = {0,0,0,0,0,0,0,0};
  uint16_t x_tmp, y_tmp;



  for(uint8_t i = 0; i<4; i++){
    fillRect(0, 0, size+1, size+1, color_bg);
    fillRect(0, _height-size-1, size+1, size+1, color_bg);
    fillRect(_width-size-1, 0, size+1, size+1, color_bg);
    fillRect(_width-size-1, _height-size-1, size+1, size+1, color_bg);

    if (i == 5) break;  
    
    switch (i) {
      case 0:  
        drawLine(0, 0, 0, size, color_fg);
        drawLine(0, 0, size, 0, color_fg);
        drawLine(0, 0, size , size, color_fg);
        break;
      case 1:  
        drawLine(0, _height-size-1, 0, _height-1, color_fg);
        drawLine(0, _height-1, size, _height-1, color_fg);
        drawLine(size, _height-size-1, 0, _height-1 , color_fg);
        break;
      case 2:  
        drawLine(_width-size-1, 0, _width-1, 0, color_fg);
        drawLine(_width-size-1, size, _width-1, 0, color_fg);
        drawLine(_width-1, size, _width-1, 0, color_fg);
        break;
      case 3:  
        drawLine(_width-size-1, _height-size-1, _width-1, _height-1, color_fg);
        drawLine(_width-1, _height-1-size, _width-1, _height-1, color_fg);
        drawLine(_width-1-size, _height-1, _width-1, _height-1, color_fg);
        break;
      }

     
    if(i>0) delay(1000);

    for(uint8_t j= 0; j<8; j++){
       
      while(!validTouch(&x_tmp, &y_tmp, Z_THRESHOLD/2));
      values[i*2  ] += x_tmp;
      values[i*2+1] += y_tmp;
      }
    values[i*2  ] /= 8;
    values[i*2+1] /= 8;
  }


   
   
  touchCalibration_rotate = false;
  if(abs(values[0]-values[2]) > abs(values[1]-values[3])){
    touchCalibration_rotate = true;
    touchCalibration_x0 = (values[1] + values[3])/2;  
    touchCalibration_x1 = (values[5] + values[7])/2;  
    touchCalibration_y0 = (values[0] + values[4])/2;  
    touchCalibration_y1 = (values[2] + values[6])/2;  
  } else {
    touchCalibration_x0 = (values[0] + values[2])/2;  
    touchCalibration_x1 = (values[4] + values[6])/2;  
    touchCalibration_y0 = (values[1] + values[5])/2;  
    touchCalibration_y1 = (values[3] + values[7])/2;  
  }

   
  touchCalibration_invert_x = false;
  if(touchCalibration_x0 > touchCalibration_x1){
    values[0]=touchCalibration_x0;
    touchCalibration_x0 = touchCalibration_x1;
    touchCalibration_x1 = values[0];
    touchCalibration_invert_x = true;
  }
  touchCalibration_invert_y = false;
  if(touchCalibration_y0 > touchCalibration_y1){
    values[0]=touchCalibration_y0;
    touchCalibration_y0 = touchCalibration_y1;
    touchCalibration_y1 = values[0];
    touchCalibration_invert_y = true;
  }

   
  touchCalibration_x1 -= touchCalibration_x0;
  touchCalibration_y1 -= touchCalibration_y0;

  if(touchCalibration_x0 == 0) touchCalibration_x0 = 1;
  if(touchCalibration_x1 == 0) touchCalibration_x1 = 1;
  if(touchCalibration_y0 == 0) touchCalibration_y0 = 1;
  if(touchCalibration_y1 == 0) touchCalibration_y1 = 1;

   
  if(parameters != NULL){
    parameters[0] = touchCalibration_x0;
    parameters[1] = touchCalibration_x1;
    parameters[2] = touchCalibration_y0;
    parameters[3] = touchCalibration_y1;
    parameters[4] = touchCalibration_rotate | (touchCalibration_invert_x <<1) | (touchCalibration_invert_y <<2);
  }
}






void TFT_eSPI::setTouch(uint16_t *parameters){
  touchCalibration_x0 = parameters[0];
  touchCalibration_x1 = parameters[1];
  touchCalibration_y0 = parameters[2];
  touchCalibration_y1 = parameters[3];

  if(touchCalibration_x0 == 0) touchCalibration_x0 = 1;
  if(touchCalibration_x1 == 0) touchCalibration_x1 = 1;
  if(touchCalibration_y0 == 0) touchCalibration_y0 = 1;
  if(touchCalibration_y1 == 0) touchCalibration_y1 = 1;

  touchCalibration_rotate = parameters[4] & 0x01;
  touchCalibration_invert_x = parameters[4] & 0x02;
  touchCalibration_invert_y = parameters[4] & 0x04;
}
