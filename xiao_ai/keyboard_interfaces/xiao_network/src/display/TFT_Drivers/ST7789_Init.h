
 
 
 
 
 

#ifndef INIT_SEQUENCE_3
{
  fillScreen(TFT_RED);
  writecommand(ST7789_SLPOUT);    
  delay(120);

  writecommand(ST7789_NORON);     

   
  writecommand(ST7789_MADCTL);
   
  writedata(TFT_MAD_COLOR_ORDER);

   
  writecommand(0xB6);
  writedata(0x0A);
  writedata(0x82);

  writecommand(ST7789_RAMCTRL);
  writedata(0x00);
  writedata(0xE0);  

  writecommand(ST7789_COLMOD);
  writedata(0x55);
  delay(10);

   
  writecommand(ST7789_PORCTRL);
  writedata(0x0c);
  writedata(0x0c);
  writedata(0x00);
  writedata(0x33);
  writedata(0x33);

  writecommand(ST7789_GCTRL);       
  writedata(0x35);

   
  writecommand(ST7789_VCOMS);
  writedata(0x28);		 

  writecommand(ST7789_LCMCTRL);
  writedata(0x0C);

  writecommand(ST7789_VDVVRHEN);
  writedata(0x01);
  writedata(0xFF);

  writecommand(ST7789_VRHS);        
  writedata(0x10);

  writecommand(ST7789_VDVSET);
  writedata(0x20);

  writecommand(ST7789_FRCTR2);
  writedata(0x0f);

  writecommand(ST7789_PWCTRL1);
  writedata(0xa4);
  writedata(0xa1);

   
  writecommand(ST7789_PVGAMCTRL);
  writedata(0xd0);
  writedata(0x00);
  writedata(0x02);
  writedata(0x07);
  writedata(0x0a);
  writedata(0x28);
  writedata(0x32);
  writedata(0x44);
  writedata(0x42);
  writedata(0x06);
  writedata(0x0e);
  writedata(0x12);
  writedata(0x14);
  writedata(0x17);

  writecommand(ST7789_NVGAMCTRL);
  writedata(0xd0);
  writedata(0x00);
  writedata(0x02);
  writedata(0x07);
  writedata(0x0a);
  writedata(0x28);
  writedata(0x31);
  writedata(0x54);
  writedata(0x47);
  writedata(0x0e);
  writedata(0x1c);
  writedata(0x17);
  writedata(0x1b);
  writedata(0x1e);

  writecommand(ST7789_INVON);

  writecommand(ST7789_CASET);     
  writedata(0x00);
  writedata(0x00);
  writedata(0x00);
  writedata(0xEF);     

  writecommand(ST7789_RASET);     
  writedata(0x00);
  writedata(0x00);
  writedata(0x01);
  writedata(0x3F);     

 

  end_tft_write();
  delay(120);
  begin_tft_write();

  writecommand(ST7789_DISPON);     
  delay(120);

#ifdef TFT_BL
   
  digitalWrite(TFT_BL, HIGH);
  pinMode(TFT_BL, OUTPUT);
#endif
}


#else
 
{
  writecommand(ST7789_SLPOUT);    
  delay(120);

  writecommand(ST7789_NORON);     

   
  writecommand(ST7789_MADCTL);
  writedata(TFT_MAD_COLOR_ORDER);

  
  
  

  writecommand(ST7789_COLMOD);
  writedata(0x55);
  delay(10);

   
  writecommand(ST7789_PORCTRL);
  writedata(0x0b);
  writedata(0x0b);
  writedata(0x00);
  writedata(0x33);
  writedata(0x33);

  writecommand(ST7789_GCTRL);       
  writedata(0x75);

   
  writecommand(ST7789_VCOMS);
  writedata(0x28);		 

  writecommand(ST7789_LCMCTRL);
  writedata(0x2C);

  writecommand(ST7789_VDVVRHEN);
  writedata(0x01);

  writecommand(ST7789_VRHS);        
  writedata(0x1F);

  writecommand(ST7789_FRCTR2);
  writedata(0x13);

  writecommand(ST7789_PWCTRL1);
  writedata(0xa7);

  writecommand(ST7789_PWCTRL1);
  writedata(0xa4);
  writedata(0xa1);

  writecommand(0xD6);
  writedata(0xa1);

   
  writecommand(ST7789_PVGAMCTRL);
  writedata(0xf0);
  writedata(0x05);
  writedata(0x0a);
  writedata(0x06);
  writedata(0x06);
  writedata(0x03);
  writedata(0x2b);
  writedata(0x32);
  writedata(0x43);
  writedata(0x36);
  writedata(0x11);
  writedata(0x10);
  writedata(0x2b);
  writedata(0x32);

  writecommand(ST7789_NVGAMCTRL);
  writedata(0xf0);
  writedata(0x08);
  writedata(0x0c);
  writedata(0x0b);
  writedata(0x09);
  writedata(0x24);
  writedata(0x2b);
  writedata(0x22);
  writedata(0x43);
  writedata(0x38);
  writedata(0x15);
  writedata(0x16);
  writedata(0x2f);
  writedata(0x37);

 

  end_tft_write();
  delay(120);
  begin_tft_write();

  writecommand(ST7789_DISPON);     
  delay(120);

#ifdef TFT_BL
   
  digitalWrite(TFT_BL, HIGH);
  pinMode(TFT_BL, OUTPUT);
#endif
}
#endif