  
  
 

 
 
 





void TFT_eSPI::loadFont(const uint8_t array[])
{
  if (array == nullptr) return;
  fontPtr = (uint8_t*) array;
  loadFont("", false);
}

#ifdef FONT_FS_AVAILABLE




void TFT_eSPI::loadFont(String fontName, fs::FS &ffs)
{
  fontFS = ffs;
  loadFont(fontName, false);
}
#endif





void TFT_eSPI::loadFont(String fontName, bool flash)
{
  


























































  if (fontLoaded) unloadFont();

#ifdef FONT_FS_AVAILABLE
  if (fontName == "") fs_font = false;
  else { fontPtr = nullptr; fs_font = true; }

  if (fs_font) {
    spiffs = flash;  

    if(spiffs) fontFS = SPIFFS;

     
    if (fontFS.exists("/" + fontName + ".vlw") == false) {
      Serial.println("Font file " + fontName + " not found!");
      return;
    }

    fontFile = fontFS.open( "/" + fontName + ".vlw", "r");

    if(!fontFile) return;

    fontFile.seek(0, fs::SeekSet);
  }
#else
   
  fontName = fontName;
  flash = flash;
#endif

  gFont.gArray   = (const uint8_t*)fontPtr;

  gFont.gCount   = (uint16_t)readInt32();  
                             readInt32();  
  gFont.yAdvance = (uint16_t)readInt32();  
                             readInt32();  
  gFont.ascent   = (uint16_t)readInt32();  
  gFont.descent  = (uint16_t)readInt32();  

   
  gFont.maxAscent  = gFont.ascent;    
  gFont.maxDescent = gFont.descent;   
  gFont.yAdvance   = gFont.ascent + gFont.descent;
  gFont.spaceWidth = gFont.yAdvance / 4;   

  fontLoaded = true;

   
  loadMetrics();
}






 
void TFT_eSPI::loadMetrics(void)
{
  uint32_t headerPtr = 24;
  uint32_t bitmapPtr = headerPtr + gFont.gCount * 28;

#if defined (ESP32) && defined (CONFIG_SPIRAM_SUPPORT)
  if ( psramFound() )
  {
    gUnicode  = (uint16_t*)ps_malloc( gFont.gCount * 2);  
    gHeight   =  (uint8_t*)ps_malloc( gFont.gCount );     
    gWidth    =  (uint8_t*)ps_malloc( gFont.gCount );     
    gxAdvance =  (uint8_t*)ps_malloc( gFont.gCount );     
    gdY       =  (int16_t*)ps_malloc( gFont.gCount * 2);  
    gdX       =   (int8_t*)ps_malloc( gFont.gCount );     
    gBitmap   = (uint32_t*)ps_malloc( gFont.gCount * 4);  
  }
  else
#endif
  {
    gUnicode  = (uint16_t*)malloc( gFont.gCount * 2);  
    gHeight   =  (uint8_t*)malloc( gFont.gCount );     
    gWidth    =  (uint8_t*)malloc( gFont.gCount );     
    gxAdvance =  (uint8_t*)malloc( gFont.gCount );     
    gdY       =  (int16_t*)malloc( gFont.gCount * 2);  
    gdX       =   (int8_t*)malloc( gFont.gCount );     
    gBitmap   = (uint32_t*)malloc( gFont.gCount * 4);  
  }

#ifdef SHOW_ASCENT_DESCENT
  Serial.print("ascent  = "); Serial.println(gFont.ascent);
  Serial.print("descent = "); Serial.println(gFont.descent);
#endif

#ifdef FONT_FS_AVAILABLE
  if (fs_font) fontFile.seek(headerPtr, fs::SeekSet);
#endif

  uint16_t gNum = 0;

  while (gNum < gFont.gCount)
  {
    gUnicode[gNum]  = (uint16_t)readInt32();  
    gHeight[gNum]   =  (uint8_t)readInt32();  
    gWidth[gNum]    =  (uint8_t)readInt32();  
    gxAdvance[gNum] =  (uint8_t)readInt32();  
    gdY[gNum]       =  (int16_t)readInt32();  
    gdX[gNum]       =   (int8_t)readInt32();  
    readInt32();  

     
     
     
     

     
     
     
    













     
    if (((int16_t)gHeight[gNum] - (int16_t)gdY[gNum]) > gFont.maxDescent)
    {
       
      if (((gUnicode[gNum] > 0x20) && (gUnicode[gNum] < 0xA0) && (gUnicode[gNum] != 0x7F)) || (gUnicode[gNum] > 0xFF))
      {
        gFont.maxDescent   = gHeight[gNum] - gdY[gNum];
#ifdef SHOW_ASCENT_DESCENT
        Serial.print("Unicode = 0x"); Serial.print(gUnicode[gNum], HEX); Serial.print(", maxDescent = "); Serial.println(gHeight[gNum] - gdY[gNum]);
#endif
      }
    }

    gBitmap[gNum] = bitmapPtr;

    bitmapPtr += gWidth[gNum] * gHeight[gNum];

    gNum++;
    yield();
  }

  gFont.yAdvance = gFont.maxAscent + gFont.maxDescent;

  gFont.spaceWidth = (gFont.ascent + gFont.descent) * 2/7;   
}






void TFT_eSPI::unloadFont( void )
{
  if (gUnicode)
  {
    free(gUnicode);
    gUnicode = NULL;
  }

  if (gHeight)
  {
    free(gHeight);
    gHeight = NULL;
  }

  if (gWidth)
  {
    free(gWidth);
    gWidth = NULL;
  }

  if (gxAdvance)
  {
    free(gxAdvance);
    gxAdvance = NULL;
  }

  if (gdY)
  {
    free(gdY);
    gdY = NULL;
  }

  if (gdX)
  {
    free(gdX);
    gdX = NULL;
  }

  if (gBitmap)
  {
    free(gBitmap);
    gBitmap = NULL;
  }

  gFont.gArray = nullptr;

#ifdef FONT_FS_AVAILABLE
  if (fs_font && fontFile) fontFile.close();
#endif

  fontLoaded = false;
}






uint32_t TFT_eSPI::readInt32(void)
{
  uint32_t val = 0;

#ifdef FONT_FS_AVAILABLE
  if (fs_font) {
    val  = (uint32_t)fontFile.read() << 24;
    val |= (uint32_t)fontFile.read() << 16;
    val |= (uint32_t)fontFile.read() << 8;
    val |= (uint32_t)fontFile.read();
  }
  else
#endif
  {
    val  = (uint32_t)pgm_read_byte(fontPtr++) << 24;
    val |= (uint32_t)pgm_read_byte(fontPtr++) << 16;
    val |= (uint32_t)pgm_read_byte(fontPtr++) << 8;
    val |= (uint32_t)pgm_read_byte(fontPtr++);
  }

  return val;
}






bool TFT_eSPI::getUnicodeIndex(uint16_t unicode, uint16_t *index)
{
  for (uint16_t i = 0; i < gFont.gCount; i++)
  {
    if (gUnicode[i] == unicode)
    {
      *index = i;
      return true;
    }
  }
  return false;
}






 
void TFT_eSPI::drawGlyph(uint16_t code)
{
  uint16_t fg = textcolor;
  uint16_t bg = textbgcolor;

   
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

    if (textwrapX && (cursor_x + gWidth[gNum] + gdX[gNum] > width()))
    {
      cursor_y += gFont.yAdvance;
      cursor_x = 0;
      bg_cursor_x = 0;
    }
    if (textwrapY && ((cursor_y + gFont.yAdvance) >= height())) cursor_y = 0;
    if (cursor_x == 0) cursor_x -= gdX[gNum];

    uint8_t* pbuffer = nullptr;
    const uint8_t* gPtr = (const uint8_t*) gFont.gArray;

#ifdef FONT_FS_AVAILABLE
    if (fs_font)
    {
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
    uint8_t pixel;

    startWrite();  

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
        if (spiffs)
        {
          fontFile.read(pbuffer, gWidth[gNum]);
           
        }
        else
        {
          endWrite();     
          fontFile.read(pbuffer, gWidth[gNum]);
          startWrite();   
           
        }
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
            if (getColor) bg = getColor(x + cx, y + cy);
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
    endWrite();
  }
  else
  {
     
    drawRect(cursor_x, cursor_y + gFont.maxAscent - gFont.ascent, gFont.spaceWidth, gFont.ascent, fg);
    cursor_x += gFont.spaceWidth + 1;
  }
  bg_cursor_x = cursor_x;
  last_cursor_x = cursor_x;
}





void TFT_eSPI::showFont(uint32_t td)
{
  if(!fontLoaded) return;

  int16_t cursorX = width();  
  int16_t cursorY = height(); 
  uint32_t timeDelay = 0;     

  fillScreen(textbgcolor);
  
  for (uint16_t i = 0; i < gFont.gCount; i++)
  {
     
    if (cursorX + gdX[i] + gWidth[i] >= width())  {
      cursorX = -gdX[i];

      cursorY += gFont.yAdvance;
      if (cursorY + gFont.maxAscent + gFont.descent >= height()) {
        cursorX = -gdX[i];
        cursorY = 0;
        delay(timeDelay);
        timeDelay = td;
        fillScreen(textbgcolor);
      }
    }

    setCursor(cursorX, cursorY);
    drawGlyph(gUnicode[i]);
    cursorX += gxAdvance[i];
    yield();
  }

  delay(timeDelay);
  fillScreen(textbgcolor);
}
