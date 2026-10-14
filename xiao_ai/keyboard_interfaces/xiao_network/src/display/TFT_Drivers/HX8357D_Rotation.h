   

  writecommand(TFT_MADCTL);
  rotation = m % 4;
  switch (rotation) {
   case 0:  
     writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
      _width  = _init_width;
      _height = _init_height;
     break;
   case 1:  
     writedata(TFT_MAD_MV | TFT_MAD_MY | TFT_MAD_COLOR_ORDER);
      _width  = _init_height;
      _height = _init_width;
     break;
   case 2:  
     writedata(TFT_MAD_COLOR_ORDER);
      _width  = _init_width;
      _height = _init_height;
     break;
   case 3:  
     writedata(TFT_MAD_MX | TFT_MAD_MV | TFT_MAD_COLOR_ORDER);
      _width  = _init_height;
      _height = _init_width;
     break;
  }
