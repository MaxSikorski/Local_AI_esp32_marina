#if defined (SSD1963_480_DRIVER)

  writecommand(0xE2);    
  writedata(0x23);       
  writedata(0x02);
  writedata(0x54);
  writecommand(0xE0);    
  writedata(0x01);

  delay(10);

  writecommand(0xE0);
  writedata(0x03);

  delay(10);

  writecommand(0x01);    

  delay(100);

  writecommand(0xE6);    
  writedata(0x01);
  writedata(0x1F);
  writedata(0xFF);

  writecommand(0xB0);    
  writedata(0x20);
  writedata(0x00);
  writedata(0x01);     
  writedata(0xDF);
  writedata(0x01);     
  writedata(0x0F);
  writedata(0x00);

  writecommand(0xB4);    
  writedata(0x02);     
  writedata(0x13);
  writedata(0x00);     
  writedata(0x08);
  writedata(0x2B);     
  writedata(0x00);     
  writedata(0x02);
  writedata(0x00);

  writecommand(0xB6);    
  writedata(0x01);     
  writedata(0x20);
  writedata(0x00);     
  writedata(0x04);
  writedata(0x0c);     
  writedata(0x00);     
  writedata(0x02);

  writecommand(0xBA);
  writedata(0x0F);     

  writecommand(0xB8);
  writedata(0x07);       
  writedata(0x01);     

  writecommand(0x36);    
  writedata(0x21 | TFT_MAD_COLOR_ORDER);

  writecommand(0xF0);    
  writedata(0x00);       

  delay(1);

  writecommand(0xB8);
  writedata(0x0f);     
  writedata(0x01);     

  writecommand(0xBA);
  writedata(0x01);     

  writecommand(0x2A); 
  writedata(0);
  writedata(0);
  writedata((271 & 0xFF00)>>8);
  writedata(271 & 0xFF);

  writecommand(0x2B); 
  writedata(0);
  writedata(0);
  writedata((479 & 0xFF00)>>8);
  writedata(479 & 0xFF);

  writecommand(0x2C);

  writecommand(0x29);    

  writecommand(0xBE);    
  writedata(0x06);
  writedata(0xf0);
  writedata(0x01);
  writedata(0xf0);
  writedata(0x00);
  writedata(0x00);

  writecommand(0xd0); 
  writedata(0x0d);  

  writecommand(0x2C); 

#elif defined (SSD1963_800_DRIVER)

  writecommand(0xE2);    
  writedata(0x1E);       
  writedata(0x02);
  writedata(0x54);
  writecommand(0xE0);    
  writedata(0x01);

  delay(10);

  writecommand(0xE0);
  writedata(0x03);

  delay(10);

  writecommand(0x01);    

  delay(100);

  writecommand(0xE6);    
  writedata(0x03);
  writedata(0xFF);
  writedata(0xFF);

  writecommand(0xB0);    
  writedata(0x20);
  writedata(0x00);
  writedata(0x03);     
  writedata(0x1F);
  writedata(0x01);     
  writedata(0xDF);
  writedata(0x00);

  writecommand(0xB4);    
  writedata(0x03);     
  writedata(0xA0);
  writedata(0x00);     
  writedata(0x2E);
  writedata(0x30);     
  writedata(0x00);     
  writedata(0x0F);
  writedata(0x00);

  writecommand(0xB6);    
  writedata(0x02);     
  writedata(0x0D);
  writedata(0x00);     
  writedata(0x10);
  writedata(0x10);     
  writedata(0x00);     
  writedata(0x08);

  writecommand(0xBA);
  writedata(0x0F);     

  writecommand(0xB8);
  writedata(0x07);       
  writedata(0x01);     

  writecommand(0x36);    
  writedata(0x21 | TFT_MAD_COLOR_ORDER);

  writecommand(0xF0);    
  writedata(0x00);       

  delay(1);

  writecommand(0xB8);
  writedata(0x0f);     
  writedata(0x01);     

  writecommand(0xBA);
  writedata(0x01);     

  writecommand(0x2A); 
  writedata(0);
  writedata(0);
  writedata((479 & 0xFF00)>>8);
  writedata(479 & 0xFF);

  writecommand(0x2B); 
  writedata(0);
  writedata(0);
  writedata((799 & 0xFF00)>>8);
  writedata(799 & 0xFF);

  writecommand(0x2C);

  writecommand(0x29);    

  writecommand(0xBE);    
  writedata(0x06);
  writedata(0xf0);
  writedata(0x01);
  writedata(0xf0);
  writedata(0x00);
  writedata(0x00);

  writecommand(0xd0); 
  writedata(0x0d);  

  writecommand(0x2C); 

#elif defined (SSD1963_800ALT_DRIVER)

  writecommand(0xE2);    
  writedata(0x23);       
  writedata(0x02);
  writedata(0x04);
  writecommand(0xE0);    
  writedata(0x01);

  delay(10);

  writecommand(0xE0);
  writedata(0x03);

  delay(10);

  writecommand(0x01);    

  delay(100);

  writecommand(0xE6);    
  writedata(0x04);
  writedata(0x93);
  writedata(0xE0);

  writecommand(0xB0);    
  writedata(0x00);   
  writedata(0x00);
  writedata(0x03);     
  writedata(0x1F);
  writedata(0x01);     
  writedata(0xDF);
  writedata(0x00);

  writecommand(0xB4);    
  writedata(0x03);     
  writedata(0xA0);
  writedata(0x00);     
  writedata(0x2E);
  writedata(0x30);     
  writedata(0x00);     
  writedata(0x0F);
  writedata(0x00);

  writecommand(0xB6);    
  writedata(0x02);     
  writedata(0x0D);
  writedata(0x00);     
  writedata(0x10);
  writedata(0x10);     
  writedata(0x00);     
  writedata(0x08);

  writecommand(0xBA);
  writedata(0x05);     

  writecommand(0xB8);
  writedata(0x07);       
  writedata(0x01);     

  writecommand(0x36);    
  writedata(0x21 | TFT_MAD_COLOR_ORDER);     

  writecommand(0xF0);    
  writedata(0x00);       

  delay(10);

  writecommand(0x2A); 
  writedata(0);
  writedata(0);
  writedata((479 & 0xFF00)>>8);
  writedata(479 & 0xFF);

  writecommand(0x2B); 
  writedata(0);
  writedata(0);
  writedata((799 & 0xFF00)>>8);
  writedata(799 & 0xFF);

  writecommand(0x2C);

  writecommand(0x29);    

  writecommand(0xBE);    
  writedata(0x06);
  writedata(0xF0);
  writedata(0x01);
  writedata(0xF0);
  writedata(0x00);
  writedata(0x00);

  writecommand(0xD0); 
  writedata(0x0D);  

  writecommand(0x2C); 

#elif defined (SSD1963_800BD_DRIVER)  

  writecommand(0xE2);    
  writedata(0x23);       
  writedata(0x02);
  writedata(0x54);

  writecommand(0xE0);    
  writedata(0x01);

  delay(10);

  writecommand(0xE0);
  writedata(0x03);

  delay(10);

  writecommand(0x01);    

  delay(100);

  writecommand(0xE6);    
  writedata(0x03);
  writedata(0x33);
  writedata(0x33);

  writecommand(0xB0);     
  writedata(0x20);
  writedata(0x00);
  writedata(799 >> 8);    
  writedata(799 & 0xFF);
  writedata(479 >> 8);    
  writedata(479 & 0xFF);
  writedata(0x00);

  writecommand(0xB4);    
  writedata(0x04);       
  writedata(0x1F);
  writedata(0x00);       
  writedata(0xD2);
  writedata(0x00);       
  writedata(0x00);       
  writedata(0x00);
  writedata(0x00);

  writecommand(0xB6);    
  writedata(0x02);     
  writedata(0x0C);
  writedata(0x00);     
  writedata(0x22);
  writedata(0x00);     
  writedata(0x00);     
  writedata(0x00);

  writecommand(0xB8);
  writedata(0x0F);       
  writedata(0x01);       

  writecommand(0xBA);
  writedata(0x01);     

  writecommand(0x36);    
  writedata(0x21 | TFT_MAD_COLOR_ORDER);       

	 
	 

  writecommand(0xF0);    
  writedata(0x00);       

  writecommand(0xBC);
  writedata(0x40);      
  writedata(0x80);      
  writedata(0x40);      
  writedata(0x01);      


  delay(10);

  writecommand(0x29);    

  writecommand(0xBE);    
  writedata(0x06);
  writedata(0x80);
  writedata(0x01);
  writedata(0xF0);
  writedata(0x00);
  writedata(0x00);

  writecommand(0xD0); 
  writedata(0x0D);  

#endif