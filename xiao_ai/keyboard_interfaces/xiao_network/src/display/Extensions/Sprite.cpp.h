















TFT_eSprite::TFT_eSprite(TFT_eSPI *tft)
{
  _tft = tft;      

  _iwidth    = 0;  
  _iheight   = 0;
  _bpp = 16;
  _swapBytes = false;    

  _created = false;
  _vpOoB   = true;

  _xs = 0;   
  _ys = 0;
  _xe = 0;
  _ye = 0;

  _xptr = 0;  
  _yptr = 0;

  _colorMap = nullptr;

  _psram_enable = true;
  
   
  lockTransaction = true;
}






 
void* TFT_eSprite::createSprite(int16_t w, int16_t h, uint8_t frames)
{

  if ( _created ) return _img8_1;

  if ( w < 1 || h < 1 ) return nullptr;

  _iwidth  = _dwidth  = _bitwidth = w;
  _iheight = _dheight = h;

  cursor_x = 0;
  cursor_y = 0;

   
  _sx = 0;
  _sy = 0;
  _sw = w;
  _sh = h;
  _scolor = TFT_BLACK;

  _img8   = (uint8_t*) callocSprite(w, h, frames);
  _img8_1 = _img8;
  _img8_2 = _img8;
  _img    = (uint16_t*) _img8;
  _img4   = _img8;

  if ( (_bpp == 16) && (frames > 1) ) {
    _img8_2 = _img8 + (w * h * 2 + 1);
  }

   
   
   
   
   

  if ( (_bpp == 8) && (frames > 1) ) {
    _img8_2 = _img8 + (w * h + 1);
  }

   
   
  if ( (_bpp == 1) && (frames > 1) )
  {
    w = (w+7) & 0xFFF8;
    _img8_2 = _img8 + ( (w>>3) * h + 1 );
  }

  if (_img8)
  {
    _created = true;
    if ( (_bpp == 4) && (_colorMap == nullptr)) createPalette(default_4bit_palette);

    rotation = 0;
    setViewport(0, 0, _dwidth, _dheight);
    setPivot(_iwidth/2, _iheight/2);
    return _img8_1;
  }

  return nullptr;
}






void* TFT_eSprite::getPointer(void)
{
  if (!_created) return nullptr;
  return _img8_1;
}






bool TFT_eSprite::created(void)
{
  return _created;
}






TFT_eSprite::~TFT_eSprite(void)
{
  deleteSprite();

#ifdef SMOOTH_FONT
  if(fontLoaded) unloadFont();
#endif
}






void* TFT_eSprite::callocSprite(int16_t w, int16_t h, uint8_t frames)
{
   
   
   
  uint8_t* ptr8 = nullptr;

  if (frames > 2) frames = 2;  
  if (frames < 1) frames = 1;

  if (_bpp == 16)
  {
#if defined (ESP32) && defined (CONFIG_SPIRAM_SUPPORT)
    if ( psramFound() && _psram_enable && !_tft->DMA_Enabled)
    {
      ptr8 = ( uint8_t*) ps_calloc(frames * w * h + frames, sizeof(uint16_t));
       
    }
    else
#endif
    {
      ptr8 = ( uint8_t*) calloc(frames * w * h + frames, sizeof(uint16_t));
       
    }
  }

  else if (_bpp == 8)
  {
#if defined (ESP32) && defined (CONFIG_SPIRAM_SUPPORT)
    if ( psramFound() && _psram_enable ) ptr8 = ( uint8_t*) ps_calloc(frames * w * h + frames, sizeof(uint8_t));
    else
#endif
    ptr8 = ( uint8_t*) calloc(frames * w * h + frames, sizeof(uint8_t));
  }

  else if (_bpp == 4)
  {
    w = (w+1) & 0xFFFE;  
    _iwidth = w;
#if defined (ESP32) && defined (CONFIG_SPIRAM_SUPPORT)
    if ( psramFound() && _psram_enable ) ptr8 = ( uint8_t*) ps_calloc(((frames * w * h) >> 1) + frames, sizeof(uint8_t));
    else
#endif
    ptr8 = ( uint8_t*) calloc(((frames * w * h) >> 1) + frames, sizeof(uint8_t));
  }

  else  
  {
     
     
     

    w =  (w+7) & 0xFFF8;  
    _iwidth = w;          
    _bitwidth = w;        

#if defined (ESP32) && defined (CONFIG_SPIRAM_SUPPORT)
    if ( psramFound() && _psram_enable ) ptr8 = ( uint8_t*) ps_calloc(frames * (w>>3) * h + frames, sizeof(uint8_t));
    else
#endif
    ptr8 = ( uint8_t*) calloc(frames * (w>>3) * h + frames, sizeof(uint8_t));
  }

  return ptr8;
}






void TFT_eSprite::createPalette(uint16_t colorMap[], uint8_t colors)
{
  if (!_created) return;

  if (colorMap == nullptr)
  {
     
    createPalette(default_4bit_palette);
    return;
  }

   
  if (_colorMap == nullptr) _colorMap = (uint16_t *)calloc(16, sizeof(uint16_t));

  if (colors > 16) colors = 16;

   
  for (uint8_t i = 0; i < colors; i++)
  {
    _colorMap[i] = colorMap[i];
  }
}






void TFT_eSprite::createPalette(const uint16_t colorMap[], uint8_t colors)
{
  if (!_created) return;

  if (colorMap == nullptr)
  {
     
    colorMap = default_4bit_palette;
  }

   
  if (_colorMap == nullptr) _colorMap = (uint16_t *)calloc(16, sizeof(uint16_t));

  if (colors > 16) colors = 16;

   
  for (uint8_t i = 0; i < colors; i++)
  {
    _colorMap[i] = pgm_read_word(colorMap++);
  }
}






 
void* TFT_eSprite::frameBuffer(int8_t f)
{
  if (!_created) return nullptr;

  if ( f == 2 ) _img8 = _img8_2;
  else          _img8 = _img8_1;

  if (_bpp == 16) _img = (uint16_t*)_img8;

   

  if (_bpp == 4) _img4 = _img8;

  return _img8;
}






void* TFT_eSprite::setColorDepth(int8_t b)
{
   
  if (_bpp == b) return _img8_1;

   
  if ( b > 8 ) _bpp = 16;   
  else if ( b > 4 ) _bpp = 8;
  else if ( b > 1 ) _bpp = 4;
  else _bpp = 1;

   
  if (_created) {
    deleteSprite();
    return createSprite(_dwidth, _dheight);
  }

  return nullptr;
}






int8_t TFT_eSprite::getColorDepth(void)
{
  if (_created) return _bpp;
  else return 0;
}






void TFT_eSprite::setBitmapColor(uint16_t c, uint16_t b)
{
  if (c == b) b = ~c;
  _tft->bitmap_fg = c;
  _tft->bitmap_bg = b;
}






void TFT_eSprite::setPaletteColor(uint8_t index, uint16_t color)
{
  if (_colorMap == nullptr || index > 15) return;  

  _colorMap[index] = color;
}






uint16_t TFT_eSprite::getPaletteColor(uint8_t index)
{
  if (_colorMap == nullptr || index > 15) return 0;  

  return _colorMap[index];
}






void TFT_eSprite::deleteSprite(void)
{
  if (_colorMap != nullptr)
  {
    free(_colorMap);
    _colorMap = nullptr;
  }

  if (_created)
  {
    free(_img8_1);
    _img8 = nullptr;
    _created = false;
    _vpOoB   = true;   
  }
}






#define FP_SCALE 10
bool TFT_eSprite::pushRotated(int16_t angle, uint32_t transp)
{
  if ( !_created || _tft->_vpOoB) return false;

   
  int16_t min_x;
  int16_t min_y;
  int16_t max_x;
  int16_t max_y;

   
  if ( !getRotatedBounds(angle, &min_x, &min_y, &max_x, &max_y) ) return false;

  uint16_t sline_buffer[max_x - min_x + 1];

  int32_t xt = min_x - _tft->_xPivot;
  int32_t yt = min_y - _tft->_yPivot;
  uint32_t xe = _dwidth << FP_SCALE;
  uint32_t ye = _dheight << FP_SCALE;
  uint16_t tpcolor = (uint16_t)transp;

  if (transp != 0x00FFFFFF) {
    if (_bpp == 4) tpcolor = _colorMap[transp & 0x0F];
    tpcolor = tpcolor>>8 | tpcolor<<8;  
  }
  _tft->startWrite();  

   
  for (int32_t y = min_y; y <= max_y; y++, yt++) {
    int32_t x = min_x;
    uint32_t xs = (_cosra * xt - (_sinra * yt - (_xPivot << FP_SCALE)) + (1 << (FP_SCALE - 1)));
    uint32_t ys = (_sinra * xt + (_cosra * yt + (_yPivot << FP_SCALE)) + (1 << (FP_SCALE - 1)));

    while ((xs >= xe || ys >= ye) && x < max_x) { x++; xs += _cosra; ys += _sinra; }
    if (x == max_x) continue;

    uint32_t pixel_count = 0;
    do {
      uint32_t rp;
      int32_t xp = xs >> FP_SCALE;
      int32_t yp = ys >> FP_SCALE;
      if (_bpp == 16) {rp = _img[xp + yp * _iwidth]; }
      else { rp = readPixel(xp, yp); rp = (uint16_t)(rp>>8 | rp<<8); }
      if (transp != 0x00FFFFFF && tpcolor == rp) {
        if (pixel_count) {
           
          _tft->setWindow(x - pixel_count, y, x - 1, y);
          _tft->pushPixels(sline_buffer, pixel_count);
          pixel_count = 0;
        }
      }
      else {
        sline_buffer[pixel_count++] = rp;
      }
    } while (++x < max_x && (xs += _cosra) < xe && (ys += _sinra) < ye);
    if (pixel_count) {
       
      _tft->setWindow(x - pixel_count, y, x - 1, y);
      _tft->pushPixels(sline_buffer, pixel_count);
    }
  }

  _tft->endWrite();  

  return true;
}






 
bool TFT_eSprite::pushRotated(TFT_eSprite *spr, int16_t angle, uint32_t transp)
{
  if ( !_created  || _bpp == 4) return false;  
  if ( !spr->_created  || spr->_bpp == 4) return false;   

   
  int16_t min_x;
  int16_t min_y;
  int16_t max_x;
  int16_t max_y;

   
  if ( !getRotatedBounds(spr, angle, &min_x, &min_y, &max_x, &max_y) ) return false;

  uint16_t sline_buffer[max_x - min_x + 1];

  int32_t xt = min_x - spr->_xPivot;
  int32_t yt = min_y - spr->_yPivot;
  uint32_t xe = _dwidth << FP_SCALE;
  uint32_t ye = _dheight << FP_SCALE;
  uint16_t tpcolor = (uint16_t)transp;
  
  if (transp != 0x00FFFFFF) {
    if (_bpp == 4) tpcolor = _colorMap[transp & 0x0F];
    tpcolor = tpcolor>>8 | tpcolor<<8;  
  }

  bool oldSwapBytes = spr->getSwapBytes();
  spr->setSwapBytes(false);

   
  for (int32_t y = min_y; y <= max_y; y++, yt++) {
    int32_t x = min_x;
    uint32_t xs = (_cosra * xt - (_sinra * yt - (_xPivot << FP_SCALE)) + (1 << (FP_SCALE - 1)));
    uint32_t ys = (_sinra * xt + (_cosra * yt + (_yPivot << FP_SCALE)) + (1 << (FP_SCALE - 1)));

    while ((xs >= xe || ys >= ye) && x < max_x) { x++; xs += _cosra; ys += _sinra; }
    if (x == max_x) continue;

    uint32_t pixel_count = 0;
    do {
      uint32_t rp;
      int32_t xp = xs >> FP_SCALE;
      int32_t yp = ys >> FP_SCALE;
      if (_bpp == 16) rp = _img[xp + yp * _iwidth];
      else { rp = readPixel(xp, yp); rp = (uint16_t)(rp>>8 | rp<<8); }
      if (transp != 0x00FFFFFF && tpcolor == rp) {
        if (pixel_count) {
          spr->pushImage(x - pixel_count, y, pixel_count, 1, sline_buffer);
          pixel_count = 0;
        }
      }
      else {
        sline_buffer[pixel_count++] = rp;
      }
    } while (++x < max_x && (xs += _cosra) < xe && (ys += _sinra) < ye);
    if (pixel_count) spr->pushImage(x - pixel_count, y, pixel_count, 1, sline_buffer);
  }
  spr->setSwapBytes(oldSwapBytes);
  return true;
}






bool TFT_eSprite::getRotatedBounds(int16_t angle, int16_t *min_x, int16_t *min_y,
                                                  int16_t *max_x, int16_t *max_y)
{
   
  getRotatedBounds(angle, width(), height(), _xPivot, _yPivot, min_x, min_y, max_x, max_y);

   
  *min_x += _tft->_xPivot;
  *max_x += _tft->_xPivot;
  *min_y += _tft->_yPivot;
  *max_y += _tft->_yPivot;

   
  if (*min_x > _tft->_vpW) return false;
  if (*min_y > _tft->_vpH) return false;
  if (*max_x < _tft->_vpX) return false;
  if (*max_y < _tft->_vpY) return false;

   
  if (*min_x < _tft->_vpX) *min_x = _tft->_vpX;
  if (*min_y < _tft->_vpY) *min_y = _tft->_vpY;
  if (*max_x > _tft->_vpW) *max_x = _tft->_vpW;
  if (*max_y > _tft->_vpH) *max_y = _tft->_vpH;

  return true;
}






bool TFT_eSprite::getRotatedBounds(TFT_eSprite *spr, int16_t angle, int16_t *min_x, int16_t *min_y,
                                                                    int16_t *max_x, int16_t *max_y)
{
   
  getRotatedBounds(angle, width(), height(), _xPivot, _yPivot, min_x, min_y, max_x, max_y);

   
  *min_x += spr->_xPivot;
  *max_x += spr->_xPivot;
  *min_y += spr->_yPivot;
  *max_y += spr->_yPivot;

   
   
   

   
  if (*min_x > spr->width()) return true;
  if (*min_y > spr->height()) return true;
  if (*max_x < 0) return true;
  if (*max_y < 0) return true;

   
   
  if (*min_x < 0) min_x = 0;
  if (*min_y < 0) min_y = 0;
  if (*max_x > spr->width())  *max_x = spr->width();
  if (*max_y > spr->height()) *max_y = spr->height();

  return true;
}






void TFT_eSprite::getRotatedBounds(int16_t angle, int16_t w, int16_t h, int16_t xp, int16_t yp,
                                   int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
{
   
  float radAngle = -angle * 0.0174532925;  
  float sina = sin(radAngle);
  float cosa = cos(radAngle);

  w -= xp;  
  h -= yp;  

   
  int16_t x0 = -xp * cosa - yp * sina;
  int16_t y0 =  xp * sina - yp * cosa;

  int16_t x1 =  w * cosa - yp * sina;
  int16_t y1 = -w * sina - yp * cosa;

  int16_t x2 =  h * sina + w * cosa;
  int16_t y2 =  h * cosa - w * sina;

  int16_t x3 =  h * sina - xp * cosa;
  int16_t y3 =  h * cosa + xp * sina;

   
  *min_x = x0-2;
  if (x1 < *min_x) *min_x = x1-2;
  if (x2 < *min_x) *min_x = x2-2;
  if (x3 < *min_x) *min_x = x3-2;

  *max_x = x0+2;
  if (x1 > *max_x) *max_x = x1+2;
  if (x2 > *max_x) *max_x = x2+2;
  if (x3 > *max_x) *max_x = x3+2;

  *min_y = y0-2;
  if (y1 < *min_y) *min_y = y1-2;
  if (y2 < *min_y) *min_y = y2-2;
  if (y3 < *min_y) *min_y = y3-2;

  *max_y = y0+2;
  if (y1 > *max_y) *max_y = y1+2;
  if (y2 > *max_y) *max_y = y2+2;
  if (y3 > *max_y) *max_y = y3+2;

  _sinra = round(sina * (1<<FP_SCALE));
  _cosra = round(cosa * (1<<FP_SCALE));
}






void TFT_eSprite::pushSprite(int32_t x, int32_t y)
{
  if (!_created) return;

  if (_bpp == 16)
  {
    bool oldSwapBytes = _tft->getSwapBytes();
    _tft->setSwapBytes(false);
    _tft->pushImage(x, y, _dwidth, _dheight, _img );
    _tft->setSwapBytes(oldSwapBytes);
  }
  else if (_bpp == 4)
  {
    _tft->pushImage(x, y, _dwidth, _dheight, _img4, false, _colorMap);
  }
  else _tft->pushImage(x, y, _dwidth, _dheight, _img8, (bool)(_bpp == 8));
}






void TFT_eSprite::pushSprite(int32_t x, int32_t y, uint16_t transp)
{
  if (!_created) return;

  if (_bpp == 16)
  {
    bool oldSwapBytes = _tft->getSwapBytes();
    _tft->setSwapBytes(false);
    _tft->pushImage(x, y, _dwidth, _dheight, _img, transp );
    _tft->setSwapBytes(oldSwapBytes);
  }
  else if (_bpp == 8)
  {
    transp = (uint8_t)((transp & 0xE000)>>8 | (transp & 0x0700)>>6 | (transp & 0x0018)>>3);
    _tft->pushImage(x, y, _dwidth, _dheight, _img8, (uint8_t)transp, (bool)true);
  }
  else if (_bpp == 4)
  {
    _tft->pushImage(x, y, _dwidth, _dheight, _img4, (uint8_t)(transp & 0x0F), false, _colorMap);
  }
  else _tft->pushImage(x, y, _dwidth, _dheight, _img8, 0, (bool)false);
}






 
 
 
 
 
 
 

bool TFT_eSprite::pushToSprite(TFT_eSprite *dspr, int32_t x, int32_t y)
{
  if (!_created) return false;
  if (!dspr->created()) return false;

   
  int8_t ds_bpp = dspr->getColorDepth();
  if (_bpp == 16 && ds_bpp != 16 && ds_bpp !=  8) return false;
  if (_bpp ==  8 && ds_bpp !=  8) return false;
  if (_bpp ==  4 && ds_bpp !=  4) return false;
  if (_bpp ==  1 && ds_bpp !=  1) return false;

  bool oldSwapBytes = dspr->getSwapBytes();
  dspr->setSwapBytes(false);
  dspr->pushImage(x, y, _dwidth, _dheight, _img, _bpp);
  dspr->setSwapBytes(oldSwapBytes);

  return true;
}






 
 
 
 
 
 

bool TFT_eSprite::pushToSprite(TFT_eSprite *dspr, int32_t x, int32_t y, uint16_t transp)
{
  if ( !_created  || !dspr->_created) return false;  

   
  int8_t ds_bpp = dspr->getColorDepth();
  if (_bpp == 16 && ds_bpp != 16 && ds_bpp !=  8) return false;
  if (_bpp ==  8 && ds_bpp !=  8) return false;
  if (_bpp ==  4 || ds_bpp ==  4) return false;
  if (_bpp ==  1 && ds_bpp !=  1) return false;

  bool oldSwapBytes = dspr->getSwapBytes();
  uint16_t sline_buffer[width()];

  transp = transp>>8 | transp<<8;

   
  for (int32_t ys = 0; ys < height(); ys++) {
    int32_t ox = x;
    uint32_t pixel_count = 0;

    for (int32_t xs = 0; xs < width(); xs++) {
      uint16_t rp = 0;
      if (_bpp == 16) rp = _img[xs + ys * width()];
      else { rp = readPixel(xs, ys); rp = rp>>8 | rp<<8; }
       

      if (transp == rp) {
        if (pixel_count) {
          dspr->pushImage(ox, y, pixel_count, 1, sline_buffer);
          ox += pixel_count;
          pixel_count = 0;
        }
        ox++;
      }
      else {
        sline_buffer[pixel_count++] = rp;
      }
    }
    if (pixel_count) dspr->pushImage(ox, y, pixel_count, 1, sline_buffer);
    y++;
  }
  dspr->setSwapBytes(oldSwapBytes);
  return true;
}






bool TFT_eSprite::pushSprite(int32_t tx, int32_t ty, int32_t sx, int32_t sy, int32_t sw, int32_t sh)
{
  if (!_created) return false;

   
  setWindow(sx, sy, sx + sw - 1, sy + sh - 1);

  






   
  sw = _xe - _xs + 1;
  sh = _ye - _ys + 1;

  if (_ys >= _iheight) return false;

  if (_bpp == 16)
  {
    bool oldSwapBytes = _tft->getSwapBytes();
    _tft->setSwapBytes(false);

     
    if ( sx == 0 && sw == _dwidth)
      _tft->pushImage(tx, ty, sw, sh, _img + _iwidth * _ys );
    else  
      while (sh--)
        _tft->pushImage(tx, ty++, sw, 1, _img + _xs + _iwidth * _ys++ );

    _tft->setSwapBytes(oldSwapBytes);
  }
  else if (_bpp == 8)
  {
     
    if ( sx == 0 && sw == _dwidth)
      _tft->pushImage(tx, ty, sw, sh, _img8 + _iwidth * _ys, (bool)true );
    else  
    while (sh--)
      _tft->pushImage(tx, ty++, sw, 1, _img8 + _xs + _iwidth * _ys++, (bool)true );
  }
  else if (_bpp == 4)
  {
     
    if ( sx == 0 && sw == _dwidth)
      _tft->pushImage(tx, ty, sw, sh, _img4 + (_iwidth>>1) * _ys, false, _colorMap );
    else  
    {
      int32_t ds = _xs&1;  

      int32_t de = 0;      
      if ((sw > ds) && (_xe&1)) de = 1;

      uint32_t dm = 0;      
      if (sw > (ds+de)) dm = sw - ds - de;
      sw--;

      uint32_t yp = (_xs + ds + _iwidth * _ys)>>1;
      _tft->startWrite();
      while (sh--)
      {
        if (ds) _tft->drawPixel(tx, ty, readPixel(_xs, _ys) );
        if (dm) _tft->pushImage(tx + ds, ty, dm, 1, _img4 + yp, false, _colorMap );
        if (de) _tft->drawPixel(tx + sw, ty, readPixel(_xe, _ys) );
        _ys++;
        ty++;
        yp += (_iwidth>>1);
      }
      _tft->endWrite();
    }
  }
  else  
  {
     
    if ( sx == 0 && sw == _dwidth)
      _tft->pushImage(tx, ty, sw, sh, _img8 + (_bitwidth>>3) * _ys, (bool)false );
    else  
    {
      _tft->startWrite();
      while (sh--)
      {
        _tft->pushImage(tx, ty++, sw, 1, _img8 + (_bitwidth>>3) * _ys++, (bool)false );
      }
      _tft->endWrite();
    }
  }

  return true;
}






uint16_t TFT_eSprite::readPixelValue(int32_t x, int32_t y)
{
  if (_vpOoB  || !_created) return 0xFF;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((x < _vpX) || (y < _vpY) ||(x >= _vpW) || (y >= _vpH)) return 0xFF;

  if (_bpp == 16)
  {
     
    return readPixel(x - _xDatum, y - _yDatum);
  }

  if (_bpp == 8)
  {
     
    return _img8[x + y * _iwidth];
  }

  if (_bpp == 4)
  {
    if (x >= _dwidth) return 0xFF;
    if ((x & 0x01) == 0)
      return _img4[((x+y*_iwidth)>>1)] >> 4;    
    else
      return _img4[((x+y*_iwidth)>>1)] & 0x0F;  
  }

  if (_bpp == 1)
  {
     
    if (rotation == 1)
    {
      uint16_t tx = x;
      x = _dheight - y - 1;
      y = tx;
    }
    else if (rotation == 2)
    {
      x = _dwidth - x - 1;
      y = _dheight - y - 1;
    }
    else if (rotation == 3)
    {
      uint16_t tx = x;
      x = y;
      y = _dwidth - tx - 1;
    }
     
    return (_img8[(x + y * _bitwidth)>>3] >> (7-(x & 0x7))) & 0x01;
  }

  return 0;
}





uint16_t TFT_eSprite::readPixel(int32_t x, int32_t y)
{
  if (_vpOoB  || !_created) return 0xFFFF;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((x < _vpX) || (y < _vpY) ||(x >= _vpW) || (y >= _vpH)) return 0xFFFF;

  if (_bpp == 16)
  {
    uint16_t color = _img[x + y * _iwidth];
    return (color >> 8) | (color << 8);
  }

  if (_bpp == 8)
  {
    uint16_t color = _img8[x + y * _iwidth];
    if (color != 0)
    {
    uint8_t  blue[] = {0, 11, 21, 31};
      color =   (color & 0xE0)<<8 | (color & 0xC0)<<5
              | (color & 0x1C)<<6 | (color & 0x1C)<<3
              | blue[color & 0x03];
    }
    return color;
  }

  if (_bpp == 4)
  {
    if (x >= _dwidth) return 0xFFFF;
    uint16_t color;
    if ((x & 0x01) == 0)
      color = _colorMap[_img4[((x+y*_iwidth)>>1)] >> 4];    
    else
      color = _colorMap[_img4[((x+y*_iwidth)>>1)] & 0x0F];  
    return color;
  }

   
   
  if (rotation == 1)
  {
    uint16_t tx = x;
    x = _dheight - y - 1;
    y = tx;
  }
  else if (rotation == 2)
  {
    x = _dwidth - x - 1;
    y = _dheight - y - 1;
  }
  else if (rotation == 3)
  {
    uint16_t tx = x;
    x = y;
    y = _dwidth - tx - 1;
  }

  uint16_t color = (_img8[(x + y * _bitwidth)>>3] << (x & 0x7)) & 0x80;

  if (color) return _tft->bitmap_fg;
  else       return _tft->bitmap_bg;
}






void  TFT_eSprite::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint8_t sbpp)
{
  if (data == nullptr || !_created) return;

  PI_CLIP;

  if (_bpp == 16)  
  {
     
    uint8_t *ptro = (uint8_t *)data + ((dx + dy * w) << 1);
     
    uint8_t *ptrs = (uint8_t *)_img + ((x + y * _iwidth) << 1);

    if(_swapBytes)
    {
      while (dh--)
      {
         
        memcpy(ptrs+1, ptro, (dw<<1) - 1);
         
        for (int32_t xp = 0; xp < (dw<<1); xp+=2)
        {
          ptrs[xp] = ptro[xp+1];;
        }
        ptro += w<<1;
        ptrs += _iwidth<<1;
      }
    }
    else
    {
      while (dh--)
      {
        memcpy(ptrs, ptro, dw<<1);
        ptro += w << 1;
        ptrs += _iwidth << 1;
      }
    }
  }
  else if (_bpp == 8 && sbpp == 8)  
  {
     
    uint8_t *ptro = (uint8_t *)data + (dx + dy * w);
     
    uint8_t *ptrs = (uint8_t *)_img + (x + y * _iwidth);

    while (dh--)
    {
      memcpy(ptrs, ptro, dw);
      ptro += w;
      ptrs += _iwidth;
    }
  }
  else if (_bpp == 8)  
  {
    uint16_t lastColor = 0;
    uint8_t  color8    = 0;
    for (int32_t yp = dy; yp < dy + dh; yp++)
    {
      int32_t xyw = x + y * _iwidth;
      int32_t dxypw = dx + yp * w;
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        uint16_t color = data[dxypw++];
        if (color != lastColor) {
           
          if(!_swapBytes) color8 = (uint8_t)((color & 0xE0) | (color & 0x07)<<2 | (color & 0x1800)>>11);
          else color8 = (uint8_t)((color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3);
        }
        lastColor = color;
        _img8[xyw++] = color8;
      }
      y++;
    }
  }
  else if (_bpp == 4)
  {
     
     
     

    int sWidth = (_iwidth >> 1);
    uint8_t *ptr = (uint8_t *)data;

    if ((x & 0x01) == 0 && (dx & 0x01) == 0 && (dw & 0x01) == 0)
    {
      x = (x >> 1) + y * sWidth;
      dw = (dw >> 1);
      dx = (dx >> 1) + dy * (w>>1);
      while (dh--)
      {
        memcpy(_img4 + x, ptr + dx, dw);
        dx += (w >> 1);
        x += sWidth;
      }
    }
    else   
    {
      for (int32_t yp = dy; yp < dy + dh; yp++)
      {
        int32_t ox = x;
        for (int32_t xp = dx; xp < dx + dw; xp++)
        {
          uint32_t color;
          if ((xp & 0x01) == 0)
            color = (ptr[((xp+yp*w)>>1)] & 0xF0) >> 4;  
          else
            color = ptr[((xp-1+yp*w)>>1)] & 0x0F;       
          drawPixel(ox, y, color);
          ox++;
        }
        y++;
      }
    }
  }

  else  
  {
     
    uint32_t ww =  (w+7)>>3;  
    uint8_t *ptr = (uint8_t *)data;
    for (int32_t yp = dy;  yp < dy + dh; yp++)
    {
      uint32_t yw = yp * ww;               
      int32_t ox = x;
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        uint16_t readPixel = (ptr[(xp>>3) + yw] & (0x80 >> (xp & 0x7)) );
        drawPixel(ox++, y, readPixel);
      }
      y++;
    }
  }
}






void  TFT_eSprite::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data)
{
#ifdef ESP32
  pushImage(x, y, w, h, (uint16_t*) data);
#else
   
  if (data == nullptr || !_created) return;

  PI_CLIP;

  if (_bpp == 16)  
  {
    for (int32_t yp = dy; yp < dy + dh; yp++)
    {
      int32_t ox = x;
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        uint16_t color = pgm_read_word(data + xp + yp * w);
        if(_swapBytes) color = color<<8 | color>>8;
        _img[ox + y * _iwidth] = color;
        ox++;
      }
      y++;
    }
  }

  else if (_bpp == 8)  
  {
    for (int32_t yp = dy; yp < dy + dh; yp++)
    {
      int32_t ox = x;
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        uint16_t color = pgm_read_word(data + xp + yp * w);
        if(_swapBytes) color = color<<8 | color>>8;
        _img8[ox + y * _iwidth] = (uint8_t)((color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3);
        ox++;
      }
      y++;
    }
  }

  else if (_bpp == 4)
  {
    #ifdef TFT_eSPI_DEBUG
    Serial.println("TFT_eSprite::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data) not implemented");
    #endif
    return;
  }

  else  
  {
    x-= _xDatum;    
    y-= _yDatum;
    uint16_t bsw =  (w+7) >> 3;  
    uint8_t *ptr = ((uint8_t*)data) + dy * bsw;
    
    while (dh--) {
      int32_t odx = dx;
      int32_t ox  = x;
      while (odx < dx + dw) {
        uint8_t pbyte = pgm_read_byte(ptr + (odx>>3));
        uint8_t mask = 0x80 >> (odx & 7);
        while (mask) {
          uint8_t p = pbyte & mask;
          mask = mask >> 1;
          drawPixel(ox++, y, p);
          odx++;
        }
      }
      ptr += bsw;
      y++;
    }
  }
#endif  
}






 
void TFT_eSprite::setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
  if (x0 > x1) transpose(x0, x1);
  if (y0 > y1) transpose(y0, y1);
  
  int32_t w = width();
  int32_t h = height();

  if ((x0 >= w) || (x1 < 0) || (y0 >= h) || (y1 < 0))
  {  
    _xs = 0;
    _ys = _dheight;
    _xe = 0;
    _ye = _dheight;
  }
  else
  {
    if (x0 < 0) x0 = 0;
    if (x1 >= w) x1 = w - 1;
    if (y0 < 0) y0 = 0;
    if (y1 >= h) y1 = h - 1;

    _xs = x0;
    _ys = y0;
    _xe = x1;
    _ye = y1;
  }

  _xptr = _xs;
  _yptr = _ys;
}






void TFT_eSprite::pushColor(uint16_t color)
{
  if (!_created ) return;

   
  if (_bpp == 16)
    _img [_xptr + _yptr * _iwidth] = (uint16_t) (color >> 8) | (color << 8);

  else  if (_bpp == 8)
    _img8[_xptr + _yptr * _iwidth] = (uint8_t )((color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3);

  else if (_bpp == 4)
  {
    uint8_t c = (uint8_t)color & 0x0F;
    if ((_xptr & 0x01) == 0) {
      _img4[(_xptr + _yptr * _iwidth)>>1] = (c << 4) | (_img4[(_xptr + _yptr * _iwidth)>>1] & 0x0F);   
    }
    else {
      _img4[(_xptr + _yptr * _iwidth)>>1] = (_img4[(_xptr + _yptr * _iwidth)>>1] & 0xF0) | c;  
    }
  }

  else drawPixel(_xptr, _yptr, color);

   
  _xptr++;

   
  if (_xptr > _xe)
  {
    _xptr = _xs;
    _yptr++;
    if (_yptr > _ye) _yptr = _ys;
  }

}






void TFT_eSprite::pushColor(uint16_t color, uint32_t len)
{
  if (!_created ) return;

  uint16_t pixelColor;

  if (_bpp == 16)
    pixelColor = (uint16_t) (color >> 8) | (color << 8);

  else  if (_bpp == 8)
    pixelColor = (color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3;

  else pixelColor = (uint16_t) color;  

  while(len--) writeColor(pixelColor);
}






void TFT_eSprite::writeColor(uint16_t color)
{
  if (!_created ) return;

   
  if (_bpp == 16) _img [_xptr + _yptr * _iwidth] = color;

   
  else if (_bpp == 8) _img8[_xptr + _yptr * _iwidth] = (uint8_t) color;

  else if (_bpp == 4)
  {
    uint8_t c = (uint8_t)color & 0x0F;
    if ((_xptr & 0x01) == 0)
      _img4[(_xptr + _yptr * _iwidth)>>1] = (c << 4) | (_img4[(_xptr + _yptr * _iwidth)>>1] & 0x0F);   
    else
      _img4[(_xptr + _yptr * _iwidth)>>1] = (_img4[(_xptr + _yptr * _iwidth)>>1] & 0xF0) | c;  
  }

  else drawPixel(_xptr, _yptr, color);

   
  _xptr++;

   
  if (_xptr > _xe)
  {
    _xptr = _xs;
    _yptr++;
    if (_yptr > _ye) _yptr = _ys;
  }
}






 
void TFT_eSprite::setScrollRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  if ((x >= _iwidth) || (y >= _iheight) || !_created ) return;

  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }

  if ((x + w) > _iwidth ) w = _iwidth  - x;
  if ((y + h) > _iheight) h = _iheight - y;

  if ( w < 1 || h < 1) return;

  _sx = x;
  _sy = y;
  _sw = w;
  _sh = h;

  _scolor = color;
}






void TFT_eSprite::scroll(int16_t dx, int16_t dy)
{
  if (abs(dx) >= _sw || abs(dy) >= _sh)
  {
    fillRect (_sx, _sy, _sw, _sh, _scolor);
    return;
  }

   
  uint32_t w  = _sw - abs(dx);  
  uint32_t h  = _sh - abs(dy);  
  int32_t iw  = _iwidth;        

   
  uint32_t tx = _sx;  
  uint32_t fx = _sx;  
  uint32_t ty = _sy;  
  uint32_t fy = _sy;  

   
  if (dx <= 0) fx -= dx;
  else tx += dx;

   
  if (dy <= 0) fy -= dy;
  else
  {  
    ty = ty + _sh - 1;  
    iw = -iw;           
    fy = ty - dy;       
  }

   
  uint32_t fyp = fx + fy * _iwidth;
  uint32_t typ = tx + ty * _iwidth;

   
  if (_bpp == 16)
  {
    while (h--)
    {  
      memmove( _img + typ, _img + fyp, w<<1);
      typ += iw;
      fyp += iw;
    }
  }
  else if (_bpp == 8)
  {
    while (h--)
    {  
      memmove( _img8 + typ, _img8 + fyp, w);
      typ += iw;
      fyp += iw;
    }
  }
  else if (_bpp == 4)
  {
     
    if (dx >  0) { tx += w; fx += w; }  
    while (h--)
    {  
      for (uint16_t xp = 0; xp < w; xp++)
      {
        if (dx <= 0) drawPixel(tx + xp, ty, readPixelValue(fx + xp, fy));
        if (dx >  0) drawPixel(tx - xp, ty, readPixelValue(fx - xp, fy));
      }
      if (dy <= 0)  { ty++; fy++; }
      else  { ty--; fy--; }
    }
  }
  else if (_bpp == 1 )
  {
    if (dx >  0) { tx += w; fx += w; }  
    while (h--)
    {  
      for (uint16_t xp = 0; xp < w; xp++)
      {
        if (dx <= 0) drawPixel(tx + xp, ty, readPixelValue(fx + xp, fy));
        if (dx >  0) drawPixel(tx - xp, ty, readPixelValue(fx - xp, fy));
      }
      if (dy <= 0)  { ty++; fy++; }
      else  { ty--; fy--; }
    }
  }
  else return;  

   
  if (dx > 0) fillRect(_sx, _sy, dx, _sh, _scolor);
  if (dx < 0) fillRect(_sx + _sw + dx, _sy, -dx, _sh, _scolor);
  if (dy > 0) fillRect(_sx, _sy, _sw, dy, _scolor);
  if (dy < 0) fillRect(_sx, _sy + _sh + dy, _sw, -dy, _scolor);
}






void TFT_eSprite::fillSprite(uint32_t color)
{
  if (!_created || _vpOoB) return;

   
  if(_xDatum == 0 && _yDatum == 0  &&  _xWidth == width())
  {
    if(_bpp == 16) {
      if ( (uint8_t)color == (uint8_t)(color>>8) ) {
        memset(_img,  (uint8_t)color, _iwidth * _yHeight * 2);
      }
      else fillRect(_vpX, _vpY, _xWidth, _yHeight, color);
    }
    else if (_bpp == 8)
    {
      color = (color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3;
      memset(_img8, (uint8_t)color, _iwidth * _yHeight);
    }
    else if (_bpp == 4)
    {
      uint8_t c = ((color & 0x0F) | (((color & 0x0F) << 4) & 0xF0));
      memset(_img4, c, (_iwidth * _yHeight) >> 1);
    }
    else if (_bpp == 1)
    {
      if(color) memset(_img8, 0xFF, (_bitwidth>>3) * _dheight + 1);
      else      memset(_img8, 0x00, (_bitwidth>>3) * _dheight + 1);
    }
  }
  else fillRect(_vpX - _xDatum, _vpY - _yDatum, _xWidth, _yHeight, color);
}






 
int16_t TFT_eSprite::width(void)
{
  if (!_created ) return 0;

  if (_bpp > 1) {
    if (_vpDatum) return _xWidth;
    return _dwidth;
  }

  if (rotation & 1) {
    if (_vpDatum) return _xWidth;
    return _dheight;
  }

  if (_vpDatum) return _xWidth;
  return _dwidth;
}






int16_t TFT_eSprite::height(void)
{
  if (!_created ) return 0;

  if (_bpp > 1) {
    if (_vpDatum) return _yHeight;
    return _dheight;
  }

  if (rotation & 1) {
    if (_vpDatum) return _yHeight;
    return _dwidth;
  }

  if (_vpDatum) return _yHeight;
  return _dheight;
}






 
void TFT_eSprite::setRotation(uint8_t r)
{
  if (_bpp != 1) return;

  rotation = r;
  
  if (rotation&1) {
    resetViewport();
  }
  else {
    resetViewport();
  }
}






uint8_t TFT_eSprite::getRotation(void)
{
  return rotation;
}






void TFT_eSprite::drawPixel(int32_t x, int32_t y, uint32_t color)
{
  if (!_created || _vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((x < _vpX) || (y < _vpY) ||(x >= _vpW) || (y >= _vpH)) return;

  if (_bpp == 16)
  {
    color = (color >> 8) | (color << 8);
    _img[x+y*_iwidth] = (uint16_t) color;
  }
  else if (_bpp == 8)
  {
    _img8[x+y*_iwidth] = (uint8_t)((color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3);
  }
  else if (_bpp == 4)
  {
    uint8_t c = color & 0x0F;
    int index = (x+y*_iwidth)>>1;;
    if ((x & 0x01) == 0) {
      _img4[index] = (uint8_t)((c << 4) | (_img4[index] & 0x0F));
    }
    else {
      _img4[index] =  (uint8_t)(c | (_img4[index] & 0xF0));
    }
  }
  else  
  {
    if (rotation == 1)
    {
      uint16_t tx = x;
      x = _dwidth - y - 1;
      y = tx;
    }
    else if (rotation == 2)
    {
      x = _dwidth - x - 1;
      y = _dheight - y - 1;
    }
    else if (rotation == 3)
    {
      uint16_t tx = x;
      x = y;
      y = _dheight - tx - 1;
    }

    if (color) _img8[(x + y * _bitwidth)>>3] |=  (0x80 >> (x & 0x7));
    else       _img8[(x + y * _bitwidth)>>3] &= ~(0x80 >> (x & 0x7));
  }
}






void TFT_eSprite::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
  if (!_created || _vpOoB) return;

   

  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    transpose(x0, y0);
    transpose(x1, y1);
  }

  if (x0 > x1) {
    transpose(x0, x1);
    transpose(y0, y1);
  }

  int32_t dx = x1 - x0, dy = abs(y1 - y0);;

  int32_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

  if (y0 < y1) ystep = 1;

   
  if (steep) {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) drawPixel(y0, xs, color);
        else drawFastVLine(y0, xs, dlen, color);
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) drawFastVLine(y0, xs, dlen, color);
  }
  else
  {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) drawPixel(xs, y0, color);
        else drawFastHLine(xs, y0, dlen, color);
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) drawFastHLine(xs, y0, dlen, color);
  }
}






void TFT_eSprite::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color)
{
  if (!_created || _vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((x < _vpX) || (x >= _vpW) || (y >= _vpH)) return;

  if (y < _vpY) { h += y - _vpY; y = _vpY; }

  if ((y + h) > _vpH) h = _vpH - y;

  if (h < 1) return;

  if (_bpp == 16)
  {
    color = (color >> 8) | (color << 8);
    int32_t yp = x + _iwidth * y;
    while (h--) {_img[yp] = (uint16_t) color; yp += _iwidth;}
  }
  else if (_bpp == 8)
  {
    color = (color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3;
    while (h--) _img8[x + _iwidth * y++] = (uint8_t) color;
  }
  else if (_bpp == 4)
  {
    if ((x & 0x01) == 0)
    {
      uint8_t c = (uint8_t) (color & 0xF) << 4;
      while (h--) {
        _img4[(x + _iwidth * y)>>1] = (uint8_t) (c | (_img4[(x + _iwidth * y)>>1] & 0x0F));
        y++;
      }
    }
    else {
      uint8_t c = (uint8_t)color & 0xF;
      while (h--) {
        _img4[(x + _iwidth * y)>>1] = (uint8_t) (c | (_img4[(x + _iwidth * y)>>1] & 0xF0));  
        y++;
      }
    }
  }
  else
  {
    x -= _xDatum;  
    y -= _yDatum;
    while (h--)
    {
      drawPixel(x, y++, color);
    }
  }
}






void TFT_eSprite::drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color)
{
  if (!_created || _vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((y < _vpY) || (x >= _vpW) || (y >= _vpH)) return;

  if (x < _vpX) { w += x - _vpX; x = _vpX; }

  if ((x + w) > _vpW) w = _vpW - x;

  if (w < 1) return;

  if (_bpp == 16)
  {
    color = (color >> 8) | (color << 8);
    while (w--) _img[_iwidth * y + x++] = (uint16_t) color;
  }
  else if (_bpp == 8)
  {
    color = (color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3;
    memset(_img8+_iwidth * y + x, (uint8_t)color, w);
  }
  else if (_bpp == 4)
  {
    uint8_t c = (uint8_t)color & 0x0F;
    uint8_t c2 = (c | ((c << 4) & 0xF0));
    if ((x & 0x01) == 1)
    {
      drawPixel(x - _xDatum, y - _yDatum, color);
      x++; w--;
      if (w < 1)
        return;
    }

    if (((w + x) & 0x01) == 1)
    {
       
      drawPixel(x - _xDatum + w - 1, y - _yDatum, color);
      w--;
      if (w < 1) return;
    }
    memset(_img4 + ((_iwidth * y + x) >> 1), c2, (w >> 1));
  }
  else {
    x -= _xDatum;  
    y -= _yDatum;

    while (w--)
    {
      drawPixel(x++, y, color);
    }
  }
}






void TFT_eSprite::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
  if (!_created || _vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

   
  if ((x >= _vpW) || (y >= _vpH)) return;

  if (x < _vpX) { w += x - _vpX; x = _vpX; }
  if (y < _vpY) { h += y - _vpY; y = _vpY; }

  if ((x + w) > _vpW) w = _vpW - x;
  if ((y + h) > _vpH) h = _vpH - y;

  if ((w < 1) || (h < 1)) return;

  int32_t yp = _iwidth * y + x;

  if (_bpp == 16)
  {
    color = (color >> 8) | (color << 8);
    uint32_t iw = w;
    int32_t ys = yp;
    if(h--)  {while (iw--) _img[yp++] = (uint16_t) color;}
    yp = ys;
    while (h--)
    {
      yp += _iwidth;
      memcpy( _img+yp, _img+ys, w<<1);
    }
  }
  else if (_bpp == 8)
  {
    color = (color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3;
    while (h--)
    {
      memset(_img8 + yp, (uint8_t)color, w);
      yp += _iwidth;
    }
  }
  else if (_bpp == 4)
  {
    uint8_t c1 = (uint8_t)color & 0x0F;
    uint8_t c2 = c1 | ((c1 << 4) & 0xF0);
    if ((x & 0x01) == 0 && (w & 0x01) == 0)
    {
      yp = (yp >> 1);
      while (h--)
      {
        memset(_img4 + yp, c2, (w>>1));
        yp += (_iwidth >> 1);
      }
    }
    else if ((x & 0x01) == 0)
    {

       
      yp = (yp >> 1);
      while (h--)
      {
        if (w > 1)
          memset(_img4 + yp, c2, (w-1)>>1);
         
        drawPixel(x+w-1-_xDatum, y+h-_yDatum, c1);
        yp += (_iwidth >> 1);
      }
    }
    else if ((w & 0x01) == 1)
    {
      yp = (yp + 1) >> 1;
      while (h--) {
        drawPixel(x-_xDatum, y+h-_yDatum, color & 0x0F);
        if (w > 1)
          memset(_img4 + yp, c2, (w-1)>>1);
         
        yp += (_iwidth >> 1);
      }
    }
    else
    {
      yp = (yp + 1) >> 1;
      while (h--) {
        drawPixel(x-_xDatum, y+h-_yDatum, color & 0x0F);
        if (w > 1) drawPixel(x+w-1-_xDatum, y+h-_yDatum, color & 0x0F);
        if (w > 2)
          memset(_img4 + yp, c2, (w-2)>>1);
         
        yp += (_iwidth >> 1);
      }
    }
  }
  else
  {
    x -= _xDatum;
    y -= _yDatum;
    while (h--)
    {
      int32_t ww = w;
      int32_t xx = x;
      while (ww--) drawPixel(xx++, y, color);
      y++;
    }
  }
}






void TFT_eSprite::drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size)
{
  if ( _vpOoB || !_created ) return;

  if (c < 32) return;
#ifdef LOAD_GLCD
 
#ifdef LOAD_GFXFF
  if(!gfxFont) {  
#endif
 

  if ((x >= _vpW - _xDatum) ||  
      (y >= _vpH - _yDatum))    
    return;

  if (((x + 6 * size - 1) < (_vpX - _xDatum)) ||  
      ((y + 8 * size - 1) < (_vpY - _yDatum)))    
    return;

  if (c > 255) return;
  if (!_cp437 && c > 175) c++;

  bool fillbg = (bg != color);

  if ((size==1) && fillbg)
  {
    uint8_t column[6];
    uint8_t mask = 0x1;

    for (int8_t i = 0; i < 5; i++ ) column[i] = pgm_read_byte(font + (c * 5) + i);
    column[5] = 0;

    int8_t j, k;
    for (j = 0; j < 8; j++) {
      for (k = 0; k < 5; k++ ) {
        if (column[k] & mask) {
          drawPixel(x + k, y + j, color);
        }
        else {
          drawPixel(x + k, y + j, bg);
        }
      }

      mask <<= 1;

      drawPixel(x + k, y + j, bg);
    }
  }
  else
  {
    for (int8_t i = 0; i < 6; i++ ) {
      uint8_t line;
      if (i == 5)
        line = 0x0;
      else
        line = pgm_read_byte(font + (c * 5) + i);

      if (size == 1)  
      {
        for (int8_t j = 0; j < 8; j++) {
          if (line & 0x1) drawPixel(x + i, y + j, color);
          line >>= 1;
        }
      }
      else {   
        for (int8_t j = 0; j < 8; j++) {
          if (line & 0x1) fillRect(x + (i * size), y + (j * size), size, size, color);
          else if (fillbg) fillRect(x + i * size, y + j * size, size, size, bg);
          line >>= 1;
        }
      }
    }
  }

 
#ifdef LOAD_GFXFF
  } else {  
#endif
 
#endif  

#ifdef LOAD_GFXFF
     
    if ((c >= pgm_read_word(&gfxFont->first)) && (c <= pgm_read_word(&gfxFont->last )))
    {
 

      c -= pgm_read_word(&gfxFont->first);
      GFXglyph *glyph  = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c]);

      uint8_t  w  = pgm_read_byte(&glyph->width),
               h  = pgm_read_byte(&glyph->height);
      int8_t   xo = pgm_read_byte(&glyph->xOffset),
               yo = pgm_read_byte(&glyph->yOffset);

      if (((x + xo + w * size - 1) < (_vpX - _xDatum)) ||  
          ((y + yo + h * size - 1) < (_vpY - _yDatum)))    
        return;

      uint8_t  *bitmap = (uint8_t *)pgm_read_dword(&gfxFont->bitmap);
      uint32_t bo = pgm_read_word(&glyph->bitmapOffset);

      uint8_t  xx, yy, bits=0, bit=0;
       
      int16_t  xo16 = 0, yo16 = 0;

      if(size > 1) {
        xo16 = xo;
        yo16 = yo;
      }

      uint16_t hpc = 0;  
      for(yy=0; yy<h; yy++) {
        for(xx=0; xx<w; xx++) {
          if(bit == 0) {
            bits = pgm_read_byte(&bitmap[bo++]);
            bit  = 0x80;
          }
          if(bits & bit) hpc++;
          else {
            if (hpc) {
              if(size == 1) drawFastHLine(x+xo+xx-hpc, y+yo+yy, hpc, color);
              else fillRect(x+(xo16+xx-hpc)*size, y+(yo16+yy)*size, size*hpc, size, color);
              hpc=0;
            }
          }
          bit >>= 1;
        }
         
        if (hpc) {
          if(size == 1) drawFastHLine(x+xo+xx-hpc, y+yo+yy, hpc, color);
          else fillRect(x+(xo16+xx-hpc)*size, y+(yo16+yy)*size, size*hpc, size, color);
          hpc=0;
        }
      }
    }
#endif


#ifdef LOAD_GLCD
  #ifdef LOAD_GFXFF
  }  
  #endif
#else
  #ifndef LOAD_GFXFF
    color = color;
    bg = bg;
    size = size;
  #endif
#endif

}






   
   
int16_t TFT_eSprite::drawChar(uint16_t uniCode, int32_t x, int32_t y)
{
  return drawChar(uniCode, x, y, textfont);
}

   
int16_t TFT_eSprite::drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font)
{
  if (_vpOoB || !uniCode) return 0;

  if (font==1) {
#ifdef LOAD_GLCD
  #ifndef LOAD_GFXFF
    drawChar(x, y, uniCode, textcolor, textbgcolor, textsize);
    return 6 * textsize;
  #endif
#else
  #ifndef LOAD_GFXFF
    return 0;
  #endif
#endif

#ifdef LOAD_GFXFF
    drawChar(x, y, uniCode, textcolor, textbgcolor, textsize);
    if(!gfxFont) {  
    #ifdef LOAD_GLCD
      return 6 * textsize;
    #else
      return 0;
    #endif
    }
    else {
      if((uniCode >= pgm_read_word(&gfxFont->first)) && (uniCode <= pgm_read_word(&gfxFont->last) )) {
        uint16_t   c2    = uniCode - pgm_read_word(&gfxFont->first);
        GFXglyph *glyph = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c2]);
        return pgm_read_byte(&glyph->xAdvance) * textsize;
      }
      else {
        return 0;
      }
    }
#endif
  }

  if ((font>1) && (font<9) && ((uniCode < 32) || (uniCode > 127))) return 0;

  int32_t width  = 0;
  int32_t height = 0;
  uint32_t flash_address = 0;
  uniCode -= 32;

#ifdef LOAD_FONT2
  if (font == 2) {
    flash_address = pgm_read_dword(&chrtbl_f16[uniCode]);
    width = pgm_read_byte(widtbl_f16 + uniCode);
    height = chr_hgt_f16;
  }
  #ifdef LOAD_RLE
  else
  #endif
#endif

#ifdef LOAD_RLE
  {
    if ((font>2) && (font<9)) {
      flash_address = pgm_read_dword( (const void*)(pgm_read_dword( &(fontdata[font].chartbl ) ) + uniCode*sizeof(void *)) );
      width = pgm_read_byte( (uint8_t *)pgm_read_dword( &(fontdata[font].widthtbl ) ) + uniCode );
      height= pgm_read_byte( &fontdata[font].height );
    }
  }
#endif

  int32_t xd = x + _xDatum;
  int32_t yd = y + _yDatum;

  if ((xd + width * textsize < _vpX || xd >= _vpW) && (yd + height * textsize < _vpY || yd >= _vpH)) return width * textsize ;

  int32_t w = width;
  int32_t pX      = 0;
  int32_t pY      = y;
  uint8_t line = 0;
  bool clip = xd < _vpX || xd + width  * textsize >= _vpW || yd < _vpY || yd + height * textsize >= _vpH;

#ifdef LOAD_FONT2  
  if (font == 2) {
    w = w + 6;  
    w = w / 8;

    for (int32_t i = 0; i < height; i++)
    {
      if (textcolor != textbgcolor) fillRect(x, pY, width * textsize, textsize, textbgcolor);

      for (int32_t k = 0; k < w; k++)
      {
        line = pgm_read_byte((uint8_t *)flash_address + w * i + k);
        if (line) {
          if (textsize == 1) {
            pX = x + k * 8;
            if (line & 0x80) drawPixel(pX, pY, textcolor);
            if (line & 0x40) drawPixel(pX + 1, pY, textcolor);
            if (line & 0x20) drawPixel(pX + 2, pY, textcolor);
            if (line & 0x10) drawPixel(pX + 3, pY, textcolor);
            if (line & 0x08) drawPixel(pX + 4, pY, textcolor);
            if (line & 0x04) drawPixel(pX + 5, pY, textcolor);
            if (line & 0x02) drawPixel(pX + 6, pY, textcolor);
            if (line & 0x01) drawPixel(pX + 7, pY, textcolor);
          }
          else {
            pX = x + k * 8 * textsize;
            if (line & 0x80) fillRect(pX, pY, textsize, textsize, textcolor);
            if (line & 0x40) fillRect(pX + textsize, pY, textsize, textsize, textcolor);
            if (line & 0x20) fillRect(pX + 2 * textsize, pY, textsize, textsize, textcolor);
            if (line & 0x10) fillRect(pX + 3 * textsize, pY, textsize, textsize, textcolor);
            if (line & 0x08) fillRect(pX + 4 * textsize, pY, textsize, textsize, textcolor);
            if (line & 0x04) fillRect(pX + 5 * textsize, pY, textsize, textsize, textcolor);
            if (line & 0x02) fillRect(pX + 6 * textsize, pY, textsize, textsize, textcolor);
            if (line & 0x01) fillRect(pX + 7 * textsize, pY, textsize, textsize, textcolor);
          }
        }
      }
      pY += textsize;
    }
  }

  #ifdef LOAD_RLE
  else
  #endif
#endif   

#ifdef LOAD_RLE   
   
  {
    w *= height;  
    int16_t color = textcolor;
    if (_bpp == 16) color = (textcolor >> 8) | (textcolor << 8);
    else if (_bpp == 8) color = ((textcolor & 0xE000)>>8 | (textcolor & 0x0700)>>6 | (textcolor & 0x0018)>>3);

    int16_t bgcolor = textbgcolor;
    if (_bpp == 16) bgcolor = (textbgcolor >> 8) | (textbgcolor << 8);
    else if (_bpp == 8) bgcolor = ((textbgcolor & 0xE000)>>8 | (textbgcolor & 0x0700)>>6 | (textbgcolor & 0x0018)>>3);

    if (textcolor == textbgcolor && !clip && _bpp != 1) {
      int32_t px = 0, py = pY;  
      int32_t pc = 0;  
      uint8_t np = textsize * textsize;  

      uint8_t tnp = 0;  
      uint8_t ts = textsize - 1;  
       
       
      while (pc < w) {
        line = pgm_read_byte((uint8_t *)flash_address);
        flash_address++;
        if (line & 0x80) {
          line &= 0x7F;
          line++;
          if (ts) {
            px = xd + textsize * (pc % width);  
            py = yd + textsize * (pc / width);
          }
          else {
            px = xd + pc % width;  
            py = yd + pc / width;
          }
          while (line--) {  
            pc++;  
            setWindow(px, py, px + ts, py + ts);

            if (ts) {
              tnp = np;
              while (tnp--) writeColor(color);
            }
            else writeColor(color);

            px += textsize;

            if (px >= (xd + width * textsize)) {
              px = xd;
              py += textsize;
            }
          }
        }
        else {
          line++;
          pc += line;
        }
      }
    }
    else {
       
       
      if (textcolor != textbgcolor && textsize == 1 && !clip && _bpp != 1)
      {
        setWindow(xd, yd, xd + width - 1, yd + height - 1);

         
        while (w > 0) {
          line = pgm_read_byte((uint8_t *)flash_address++);  
          if (line & 0x80) {
            line &= 0x7F;
            line++; w -= line;
            while (line--) writeColor(color);
          }
          else {
            line++; w -= line;
            while (line--) writeColor(bgcolor);
          }
        }
      }
      else
      {
        int32_t px = 0, py = 0;   
        int32_t tx = 0, ty = 0;   
        int32_t pc = 0;           
        int32_t pl = 0;           
        uint16_t pcol = 0;        
        bool     pf = true;       
        while (pc < w) {
          line = pgm_read_byte((uint8_t *)flash_address);
          flash_address++;
          if (line & 0x80) { pcol = textcolor; line &= 0x7F; pf = true;}
          else { pcol = textbgcolor; if (textcolor == textbgcolor) pf = false;}
          line++;
          px = pc % width;
          tx = x + textsize * px;
          py = pc / width;
          ty = y + textsize * py;

          pl = 0;
          pc += line;
          while (line--) {
            pl++;
            if ((px+pl) >= width) {
              if (pf) fillRect(tx, ty, pl * textsize, textsize, pcol);
              pl = 0;
              px = 0;
              tx = x;
              py ++;
              ty += textsize;
            }
          }
          if (pl && pf) fillRect(tx, ty, pl * textsize, textsize, pcol);
        }
      }
    }
  }
   
#endif

#if !defined (LOAD_FONT2) && !defined (LOAD_RLE)
   
  flash_address = flash_address;
  w = w;
  pX = pX;
  pY = pY;
  line = line;
  clip = clip;
#endif

  return width * textsize;     
}


#ifdef SMOOTH_FONT




 
void TFT_eSprite::drawGlyph(uint16_t code)
{
  uint16_t fg = textcolor;
  uint16_t bg = textbgcolor;
  bool getBG  = false;
  if (fg == bg) getBG = true;

   
  if (last_cursor_x != cursor_x)
  {
    bg_cursor_x = cursor_x;
    last_cursor_x = cursor_x;
  }

  if (code < 0x21)
  {
    if (code == 0x20) {
      if (_fillbg) fillRect(bg_cursor_x, cursor_y, (cursor_x + gFont.spaceWidth) - bg_cursor_x, gFont.yAdvance, bg);
      cursor_x += gFont.spaceWidth;
      bg_cursor_x = cursor_x;
      last_cursor_x = cursor_x;
      return;
    }

    if (code == '\n') {
      cursor_x = 0;
      bg_cursor_x = 0;
      last_cursor_x = 0;
      cursor_y += gFont.yAdvance;
      if (textwrapY && (cursor_y >= height())) cursor_y = 0;
      return;
    }
  }

  uint16_t gNum = 0;
  bool found = getUnicodeIndex(code, &gNum);

  if (found)
  {

    bool newSprite = !_created;

    if (newSprite)
    {
      createSprite(gWidth[gNum], gFont.yAdvance);
      if(fg != bg) fillSprite(bg);
      cursor_x = -gdX[gNum];
      bg_cursor_x = cursor_x;
      last_cursor_x = cursor_x;
      cursor_y = 0;
    }
    else
    {
      if( textwrapX && ((cursor_x + gWidth[gNum] + gdX[gNum]) > width())) {
        cursor_y += gFont.yAdvance;
        cursor_x = 0;
        bg_cursor_x = 0;
        last_cursor_x = 0;
      }

      if( textwrapY && ((cursor_y + gFont.yAdvance) > height())) cursor_y = 0;
      if ( cursor_x == 0) cursor_x -= gdX[gNum];
    }

    uint8_t* pbuffer = nullptr;
    const uint8_t* gPtr = (const uint8_t*) gFont.gArray;

#ifdef FONT_FS_AVAILABLE
    if (fs_font) {
      fontFile.seek(gBitmap[gNum], fs::SeekSet);  
      pbuffer =  (uint8_t*)malloc(gWidth[gNum]);
    }
#endif

    int16_t cy = cursor_y + gFont.maxAscent - gdY[gNum];
    int16_t cx = cursor_x + gdX[gNum];

     
     

    int16_t  fxs = cx;
    uint32_t fl = 0;
    int16_t  bxs = cx;
    uint32_t bl = 0;
    int16_t  bx = 0;
    uint8_t pixel = 0;

    int16_t fillwidth  = 0;
    int16_t fillheight = 0;

     
    if (_fillbg) {
      fillwidth  = (cursor_x + gxAdvance[gNum]) - bg_cursor_x;
      if (fillwidth > 0) {
        fillheight = gFont.maxAscent - gdY[gNum];
        if (fillheight > 0) {
          fillRect(bg_cursor_x, cursor_y, fillwidth, fillheight, textbgcolor);
        }
      }
      else {
         
        fillwidth = 0;
      }

       
      if (bg_cursor_x < cx) fillRect(bg_cursor_x, cy, cx - bg_cursor_x, gHeight[gNum], textbgcolor);
       
      if (bg_cursor_x > cx) bx = bg_cursor_x - cx;
       
      if (cx + gWidth[gNum] < cursor_x + gxAdvance[gNum]) {
        fillRect(cx + gWidth[gNum], cy, (cursor_x + gxAdvance[gNum]) - (cx + gWidth[gNum]), gHeight[gNum], textbgcolor);
      }
    }

    for (int32_t y = 0; y < gHeight[gNum]; y++)
    {
#ifdef FONT_FS_AVAILABLE
      if (fs_font) {
        fontFile.read(pbuffer, gWidth[gNum]);
      }
#endif

      for (int32_t x = 0; x < gWidth[gNum]; x++)
      {
#ifdef FONT_FS_AVAILABLE
        if (fs_font) pixel = pbuffer[x];
        else
#endif
        pixel = pgm_read_byte(gPtr + gBitmap[gNum] + x + gWidth[gNum] * y);

        if (pixel)
        {
          if (bl) { drawFastHLine( bxs, y + cy, bl, bg); bl = 0; }
          if (pixel != 0xFF)
          {
            if (fl) {
              if (fl==1) drawPixel(fxs, y + cy, fg);
              else drawFastHLine( fxs, y + cy, fl, fg);
              fl = 0;
            }
            if (getBG) bg = readPixel(x + cx, y + cy);
            drawPixel(x + cx, y + cy, alphaBlend(pixel, fg, bg));
          }
          else
          {
            if (fl==0) fxs = x + cx;
            fl++;
          }
        }
        else
        {
          if (fl) { drawFastHLine( fxs, y + cy, fl, fg); fl = 0; }
          if (_fillbg) {
            if (x >= bx) {
              if (bl==0) bxs = x + cx;
              bl++;
            }
          }
        }
      }
      if (fl) { drawFastHLine( fxs, y + cy, fl, fg); fl = 0; }
      if (bl) { drawFastHLine( bxs, y + cy, bl, bg); bl = 0; }
    }

     
    if (fillwidth > 0) {
      fillheight = (cursor_y + gFont.yAdvance) - (cy + gHeight[gNum]);
      if (fillheight > 0) {
        fillRect(bg_cursor_x, cy + gHeight[gNum], fillwidth, fillheight, textbgcolor);
      }
    }

    if (pbuffer) free(pbuffer);
    cursor_x += gxAdvance[gNum];

    if (newSprite)
    {
      pushSprite(cx, cursor_y);
      deleteSprite();
    }
  }
  else
  {
     
    drawRect(cursor_x, cursor_y + gFont.maxAscent - gFont.ascent, gFont.spaceWidth, gFont.ascent, fg);
    cursor_x += gFont.spaceWidth + 1;
  }
  bg_cursor_x = cursor_x;
  last_cursor_x = cursor_x;
}






void TFT_eSprite::printToSprite(String string)
{
  if(!fontLoaded) return;
  printToSprite((char*)string.c_str(), string.length());
}






void TFT_eSprite::printToSprite(char *cbuffer, uint16_t len)  
{
  if(!fontLoaded) return;

  uint16_t n = 0;
  bool newSprite = !_created;
  int16_t  cursorX = _tft->cursor_x;

  if (newSprite)
  {
    int16_t sWidth = 0;
    uint16_t index = 0;
    bool     first = true;
    while (n < len)
    {
      uint16_t unicode = decodeUTF8((uint8_t*)cbuffer, &n, len - n);
      if (getUnicodeIndex(unicode, &index))
      {
        if (first) {
          first = false;
          sWidth -= gdX[index];
          cursorX += gdX[index];
        }
        if (n == len) sWidth += ( gWidth[index] + gdX[index]);
        else sWidth += gxAdvance[index];
      }
      else sWidth += gFont.spaceWidth + 1;
    }

    createSprite(sWidth, gFont.yAdvance);

    if (textcolor != textbgcolor) fillSprite(textbgcolor);
  }

  n = 0;

  while (n < len)
  {
    uint16_t unicode = decodeUTF8((uint8_t*)cbuffer, &n, len - n);
     
     
    drawGlyph(unicode);
  }

  if (newSprite)
  {  
    pushSprite(cursorX, _tft->cursor_y);
    deleteSprite();
  }
}






int16_t TFT_eSprite::printToSprite(int16_t x, int16_t y, uint16_t index)
{
  bool newSprite = !_created;
  int16_t sWidth = gWidth[index];

  if (newSprite)
  {
    createSprite(sWidth, gFont.yAdvance);

    if (textcolor != textbgcolor) fillSprite(textbgcolor);

    drawGlyph(gUnicode[index]);

    pushSprite(x + gdX[index], y, textbgcolor);
    deleteSprite();
  }

  else drawGlyph(gUnicode[index]);

  return gxAdvance[index];
}
#endif
