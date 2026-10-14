
 

  rotation = m % 8;  

  writecommand(TFT_MADCTL);
  switch (rotation) {
    case 0:
      writedata(TFT_MAD_MX | TFT_MAD_COLOR_ORDER);
      _width  = _init_width;
      _height = _init_height;
#ifdef CGRAM_OFFSET
      if (_init_width == 222)
      {
        colstart = 49;
        rowstart = 0;
      }
#endif
      break;
    case 1:
      writedata(TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
      _width  = _init_height;
      _height = _init_width;
#ifdef CGRAM_OFFSET
      if (_init_width == 222)
      {
        colstart = 0;
        rowstart = 49;
      }
#endif
      break;
    case 2:
      writedata(TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
      _width  = _init_width;
      _height = _init_height;
#ifdef CGRAM_OFFSET
      if (_init_width == 222)
      {
        colstart = 49;
        rowstart = 0;
      }
#endif
      break;
    case 3:
      writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
      _width  = _init_height;
      _height = _init_width;
#ifdef CGRAM_OFFSET
      if (_init_width == 222)
      {
        colstart = 0;
        rowstart = 49;
      }
#endif
      break;
   
    case 4:
      writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
      _width  = _init_width;
      _height = _init_height;
      break;
    case 5:
      writedata(TFT_MAD_MV | TFT_MAD_MX | TFT_MAD_COLOR_ORDER);
      _width  = _init_height;
      _height = _init_width;
      break;
    case 6:
      writedata(TFT_MAD_COLOR_ORDER);
      _width  = _init_width;
      _height = _init_height;
      break;
    case 7:
      writedata(TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
      _width  = _init_height;
      _height = _init_width;
      break;

  }
