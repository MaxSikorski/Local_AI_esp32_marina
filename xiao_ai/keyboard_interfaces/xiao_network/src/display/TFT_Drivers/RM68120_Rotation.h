
 

  rotation = m % 4;  
  uint8_t reg = 0;

  switch (rotation) {
    case 0:
      reg = TFT_MAD_COLOR_ORDER;
      _width  = _init_width;
      _height = _init_height;
      break;
    case 1:
      reg = TFT_MAD_MV | TFT_MAD_MX | TFT_MAD_COLOR_ORDER;
      _width  = _init_height;
      _height = _init_width;
      break;
    case 2:
      reg = TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_COLOR_ORDER;
      _width  = _init_width;
      _height = _init_height;
      break;
    case 3:
      reg = TFT_MAD_MV | TFT_MAD_MY | TFT_MAD_COLOR_ORDER;
      _width  = _init_height;
      _height = _init_width;
      break;
  }
  writeRegister16(TFT_MADCTL, reg);