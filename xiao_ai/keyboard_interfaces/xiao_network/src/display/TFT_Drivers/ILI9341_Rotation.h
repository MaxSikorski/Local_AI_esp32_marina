
 

  rotation = m % 8;  

  writecommand(TFT_MADCTL);
  switch (rotation) {
    case 0:
#ifdef M5STACK
      writedata(TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
#else
      writedata(TFT_MAD_MX | TFT_MAD_COLOR_ORDER);
#endif
      _width  = _init_width;
      _height = _init_height;
      break;
    case 1:
#ifdef M5STACK
      writedata(TFT_MAD_COLOR_ORDER);
#else
      writedata(TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
#endif
      _width  = _init_height;
      _height = _init_width;
      break;
    case 2:
#ifdef M5STACK
      writedata(TFT_MAD_MV | TFT_MAD_MX | TFT_MAD_COLOR_ORDER);
#else
      writedata(TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
#endif
      _width  = _init_width;
      _height = _init_height;
      break;
    case 3:
#ifdef M5STACK
      writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
#else
      writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
#endif
      _width  = _init_height;
      _height = _init_width;
      break;
   
    case 4:
#ifdef M5STACK
      writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
#else
      writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
#endif
      _width  = _init_width;
      _height = _init_height;
      break;
    case 5:
#ifdef M5STACK
      writedata(TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
#else
      writedata(TFT_MAD_MV | TFT_MAD_MX | TFT_MAD_COLOR_ORDER);
#endif
      _width  = _init_height;
      _height = _init_width;
      break;
    case 6:
#ifdef M5STACK
      writedata(TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
#else
      writedata(TFT_MAD_COLOR_ORDER);
#endif
      _width  = _init_width;
      _height = _init_height;
      break;
    case 7:
#ifdef M5STACK
      writedata(TFT_MAD_MX | TFT_MAD_COLOR_ORDER);
#else
      writedata(TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
#endif
      _width  = _init_height;
      _height = _init_width;
      break;

  }
