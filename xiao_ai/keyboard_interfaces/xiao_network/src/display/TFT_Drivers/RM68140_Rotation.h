   


  writecommand(TFT_MADCTL);
  rotation = m % 4;
  switch (rotation) {
   case 0:  
     writedata(TFT_MAD_BGR);
     writecommand(0xB6);
     writedata(0);
     writedata(0x22);
     writedata(0x3B);
      _width  = TFT_WIDTH;
      _height = TFT_HEIGHT;
     break;
   case 1:  
     writedata(TFT_MAD_MV | TFT_MAD_BGR);
     writecommand(0xB6);
     writedata(0);
     writedata(0x02);
     writedata(0x3B);
      _width  = TFT_HEIGHT;
      _height = TFT_WIDTH;
     break;
   case 2:  
     writedata(TFT_MAD_BGR);
     writecommand(0xB6);
     writedata(0);
     writedata(0x42);
     writedata(0x3B);
      _width  = TFT_WIDTH;
      _height = TFT_HEIGHT;
     break;
   case 3:  
     writedata(TFT_MAD_MV | TFT_MAD_BGR);
     writecommand(0xB6);
     writedata(0);
     writedata(0x62);
     writedata(0x3B);
      _width  = TFT_HEIGHT;
      _height = TFT_WIDTH;
     break;
  }
   
