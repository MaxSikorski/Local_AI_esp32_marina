   

  writecommand(TFT_MADCTL);
  rotation = m % 8;
  switch (rotation) {
   case 0:  
     writedata(TFT_MAD_BGR | TFT_MAD_MX);
     _width  = _init_width;
     _height = _init_height;
     break;
   case 1:  
     writedata(TFT_MAD_BGR | TFT_MAD_MV);
     _width  = _init_height;
     _height = _init_width;
     break;
   case 2:  
     writedata( TFT_MAD_BGR | TFT_MAD_MY);
     _width  = _init_width;
     _height = _init_height;
    break;
   case 3:  
     writedata(TFT_MAD_BGR | TFT_MAD_MV | TFT_MAD_MX | TFT_MAD_MY);
     _width  = _init_height;
     _height = _init_width;
     break;
   case 4:  
     writedata(TFT_MAD_BGR | TFT_MAD_MX | TFT_MAD_MY);
     _width  = _init_width;
     _height = _init_height;
     break;
   case 5:  
     writedata(TFT_MAD_BGR | TFT_MAD_MV | TFT_MAD_MX);
     _width  = _init_height;
     _height = _init_width;
     break;
   case 6:  
     writedata( TFT_MAD_BGR);
     _width  = _init_width;
     _height = _init_height;
     break;
   case 7:  
     writedata(TFT_MAD_BGR | TFT_MAD_MV | TFT_MAD_MY);
     _width  = _init_height;
     _height = _init_width;
     break;
  }
  
