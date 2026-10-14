
 
 
 
 
 

#define ILI9481_INIT_1  
 
 
 
 
 
 
 

 
#ifdef ILI9481_INIT_1
 

    writecommand(TFT_SLPOUT);
    delay(20);

    writecommand(0xD0);
    writedata(0x07);
    writedata(0x42);
    writedata(0x18);

    writecommand(0xD1);
    writedata(0x00);
    writedata(0x07);
    writedata(0x10);

    writecommand(0xD2);
    writedata(0x01);
    writedata(0x02);

    writecommand(0xC0);
    writedata(0x10);
    writedata(0x3B);
    writedata(0x00);
    writedata(0x02);
    writedata(0x11);

    writecommand(0xC5);
    writedata(0x03);

    writecommand(0xC8);
    writedata(0x00);
    writedata(0x32);
    writedata(0x36);
    writedata(0x45);
    writedata(0x06);
    writedata(0x16);
    writedata(0x37);
    writedata(0x75);
    writedata(0x77);
    writedata(0x54);
    writedata(0x0C);
    writedata(0x00);

    writecommand(TFT_MADCTL);
    writedata(0x0A);

    writecommand(0x3A);
    #if defined (TFT_PARALLEL_8_BIT) || defined (TFT_PARALLEL_16_BIT) || defined (RPI_DISPLAY_TYPE)
      writedata(0x55);            
    #else
      writedata(0x66);            
    #endif
    
    #if !defined (TFT_PARALLEL_8_BIT) && !defined (TFT_PARALLEL_16_BIT)
      writecommand(TFT_INVON);
    #endif

    writecommand(TFT_CASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0x3F);

    writecommand(TFT_PASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0xDF);

    delay(120);
    writecommand(TFT_DISPON);

    delay(25);
 
 
#elif defined (ILI9481_INIT_2)
 

    writecommand(TFT_SLPOUT);
    delay(20);

    writecommand(0xD0);
    writedata(0x07);
    writedata(0x41);
    writedata(0x1D);

    writecommand(0xD1);
    writedata(0x00);
    writedata(0x2B);
    writedata(0x1F);

    writecommand(0xD2);
    writedata(0x01);
    writedata(0x11);

    writecommand(0xC0);
    writedata(0x10);
    writedata(0x3B);
    writedata(0x00);
    writedata(0x02);
    writedata(0x11);

    writecommand(0xC5);
    writedata(0x03);

    writecommand(0xC8);
    writedata(0x00);
    writedata(0x14);
    writedata(0x33);
    writedata(0x10);
    writedata(0x00);
    writedata(0x16);
    writedata(0x44);
    writedata(0x36);
    writedata(0x77);
    writedata(0x00);
    writedata(0x0F);
    writedata(0x00);

    writecommand(0xB0);
    writedata(0x00);

    writecommand(0xE4);
    writedata(0xA0);
    
    writecommand(0xF0);
    writedata(0x01);

    writecommand(0xF3);
    writedata(0x02);
    writedata(0x1A);

    writecommand(TFT_MADCTL);
    writedata(0x0A);

    writecommand(0x3A);
    #if defined (TFT_PARALLEL_8_BIT) || defined (TFT_PARALLEL_16_BIT) || defined (RPI_DISPLAY_TYPE)
      writedata(0x55);            
    #else
      writedata(0x66);            
    #endif
    
    #if !defined (TFT_PARALLEL_8_BIT) && !defined (TFT_PARALLEL_16_BIT)
      writecommand(TFT_INVON);
    #endif

    writecommand(TFT_CASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0x3F);

    writecommand(TFT_PASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0xDF);

    delay(120);
    writecommand(TFT_DISPON);

    delay(25);
 
 
#elif defined (ILI9481_INIT_3)
 

    writecommand(TFT_SLPOUT);
    delay(20);

    writecommand(0xD0);
    writedata(0x07);
    writedata(0x41);
    writedata(0x1D);

    writecommand(0xD1);
    writedata(0x00);
    writedata(0x2B);
    writedata(0x1F);

    writecommand(0xD2);
    writedata(0x01);
    writedata(0x11);

    writecommand(0xC0);
    writedata(0x10);
    writedata(0x3B);
    writedata(0x00);
    writedata(0x02);
    writedata(0x11);

    writecommand(0xC5);
    writedata(0x03);

    writecommand(0xC8);
    writedata(0x00);
    writedata(0x14);
    writedata(0x33);
    writedata(0x10);
    writedata(0x00);
    writedata(0x16);
    writedata(0x44);
    writedata(0x36);
    writedata(0x77);
    writedata(0x00);
    writedata(0x0F);
    writedata(0x00);

    writecommand(0xB0);
    writedata(0x00);

    writecommand(0xE4);
    writedata(0xA0);
    
    writecommand(0xF0);
    writedata(0x01);

    writecommand(0xF3);
    writedata(0x40);
    writedata(0x0A);

    writecommand(TFT_MADCTL);
    writedata(0x0A);

    writecommand(0x3A);
    #if defined (TFT_PARALLEL_8_BIT) || defined (TFT_PARALLEL_16_BIT) || defined (RPI_DISPLAY_TYPE)
      writedata(0x55);            
    #else
      writedata(0x66);            
    #endif
    
    #if !defined (TFT_PARALLEL_8_BIT) && !defined (TFT_PARALLEL_16_BIT)
      writecommand(TFT_INVON);
    #endif

    writecommand(TFT_CASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0x3F);

    writecommand(TFT_PASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0xDF);

    delay(120);
    writecommand(TFT_DISPON);

    delay(25);
 
 
#elif defined (ILI9481_INIT_4)
 

    writecommand(TFT_SLPOUT);
    delay(20);

    writecommand(0xD0);
    writedata(0x07);
    writedata(0x40);
    writedata(0x1D);

    writecommand(0xD1);
    writedata(0x00);
    writedata(0x18);
    writedata(0x13);

    writecommand(0xD2);
    writedata(0x01);
    writedata(0x11);

    writecommand(0xC0);
    writedata(0x10);
    writedata(0x3B);
    writedata(0x00);
    writedata(0x02);
    writedata(0x11);

    writecommand(0xC5);
    writedata(0x03);

    writecommand(0xC8);
    writedata(0x00);
    writedata(0x44);
    writedata(0x06);
    writedata(0x44);
    writedata(0x0A);
    writedata(0x08);
    writedata(0x17);
    writedata(0x33);
    writedata(0x77);
    writedata(0x44);
    writedata(0x08);
    writedata(0x0C);

    writecommand(0xB0);
    writedata(0x00);

    writecommand(0xE4);
    writedata(0xA0);
    
    writecommand(0xF0);
    writedata(0x01);

    writecommand(TFT_MADCTL);
    writedata(0x0A);

    writecommand(0x3A);
    #if defined (TFT_PARALLEL_8_BIT) || defined (TFT_PARALLEL_16_BIT) || defined (RPI_DISPLAY_TYPE)
      writedata(0x55);            
    #else
      writedata(0x66);            
    #endif
    
    #if !defined (TFT_PARALLEL_8_BIT) && !defined (TFT_PARALLEL_16_BIT)
      writecommand(TFT_INVON);
    #endif

    writecommand(TFT_CASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0x3F);

    writecommand(TFT_PASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0xDF);

    delay(120);
    writecommand(TFT_DISPON);

    delay(25);
 
 
#elif defined (ILI9481_INIT_5)
 

    writecommand(TFT_SLPOUT);
    delay(20);

    writecommand(0xD0);
    writedata(0x07);
    writedata(0x41);
    writedata(0x1D);

    writecommand(0xD1);
    writedata(0x00);
    writedata(0x1C);
    writedata(0x1F);

    writecommand(0xD2);
    writedata(0x01);
    writedata(0x11);

    writecommand(0xC0);
    writedata(0x10);
    writedata(0x3B);
    writedata(0x00);
    writedata(0x02);
    writedata(0x11);

    writecommand(0xC5);
    writedata(0x03);

    writecommand(0xC6);
    writedata(0x83);

    writecommand(0xC8);
    writedata(0x00);
    writedata(0x26);
    writedata(0x21);
    writedata(0x00);
    writedata(0x00);
    writedata(0x1F);
    writedata(0x65);
    writedata(0x23);
    writedata(0x77);
    writedata(0x00);
    writedata(0x0F);
    writedata(0x00);

    writecommand(0xB0);
    writedata(0x00);

    writecommand(0xE4);
    writedata(0xA0);
    
    writecommand(0xF0);
    writedata(0x01);

    writecommand(TFT_MADCTL);
    writedata(0x0A);

    writecommand(0x3A);
    #if defined (TFT_PARALLEL_8_BIT) || defined (TFT_PARALLEL_16_BIT) || defined (RPI_DISPLAY_TYPE)
      writedata(0x55);            
    #else
      writedata(0x66);            
    #endif
    
    #if !defined (TFT_PARALLEL_8_BIT) && !defined (TFT_PARALLEL_16_BIT)
      writecommand(TFT_INVON);
    #endif

    writecommand(TFT_CASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0x3F);

    writecommand(TFT_PASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0xDF);

    delay(120);
    writecommand(TFT_DISPON);

    delay(25);
 
 
#elif defined (ILI9481_INIT_6)
 

    writecommand(TFT_SLPOUT);
    delay(20);

    writecommand(0xD0);
    writedata(0x07);
    writedata(0x41);
    writedata(0x1D);

    writecommand(0xD1);
    writedata(0x00);
    writedata(0x2B);
    writedata(0x1F);

    writecommand(0xD2);
    writedata(0x01);
    writedata(0x11);

    writecommand(0xC0);
    writedata(0x10);
    writedata(0x3B);
    writedata(0x00);
    writedata(0x02);
    writedata(0x11);
    writedata(0x00);

    writecommand(0xC5);
    writedata(0x03);

    writecommand(0xC6);
    writedata(0x80);

    writecommand(0xC8);
    writedata(0x00);
    writedata(0x14);
    writedata(0x33);
    writedata(0x10);
    writedata(0x00);
    writedata(0x16);
    writedata(0x44);
    writedata(0x36);
    writedata(0x77);
    writedata(0x00);
    writedata(0x0F);
    writedata(0x00);

    writecommand(0xB0);
    writedata(0x00);

    writecommand(0xE4);
    writedata(0xA0);
    
    writecommand(0xF0);
    writedata(0x08);

    writecommand(0xF3);
    writedata(0x40);
    writedata(0x0A);

    writecommand(0xF6);
    writedata(0x84);

    writecommand(0xF7);
    writedata(0x80);

    writecommand(0xB3);
    writedata(0x00);
    writedata(0x01);
    writedata(0x06);
    writedata(0x30);

    writecommand(0xB4);
    writedata(0x00);

    writecommand(0x0C);
    writedata(0x00);
    writedata(0x55);

    writecommand(TFT_MADCTL);
    writedata(0x0A);

    writecommand(0x3A);
    #if defined (TFT_PARALLEL_8_BIT) || defined (TFT_PARALLEL_16_BIT) || defined (RPI_DISPLAY_TYPE)
      writedata(0x55);            
    #else
      writedata(0x66);            
    #endif
    
    #if !defined (TFT_PARALLEL_8_BIT) && !defined (TFT_PARALLEL_16_BIT)
      writecommand(TFT_INVON);
    #endif

    writecommand(TFT_CASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0x3F);

    writecommand(TFT_PASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0xDF);

    delay(120);
    writecommand(TFT_DISPON);

    delay(25);
 
 


 
#elif defined (ILI9481_INIT_7)
  
					 
					writecommand(0x11);
					delay(20);
					writecommand(0xD0);
					writedata(0x07);
					writedata(0x42);
					writedata(0x1B);

					writecommand(0xD1);
					writedata(0x00);
					writedata(0x14);
					writedata(0x1B);

					writecommand(0xD2);
					writedata(0x01);
					writedata(0x12);

					writecommand(0xC0);
					writedata(0x10);
					writedata(0x3B);
					writedata(0x00);
					writedata(0x02);
					writedata(0x01);

					writecommand(0xC5);
					writedata(0x03);

					writecommand(0xC8);
					writedata(0x00);
					writedata(0x46);
					writedata(0x44);
					writedata(0x50);
					writedata(0x04);
					writedata(0x16);
					writedata(0x33);
					writedata(0x13);
					writedata(0x77);
					writedata(0x05);
					writedata(0x0F);
					writedata(0x00);

					writecommand(0x36);
					writedata(0x0A);

					writecommand(0x3A);
    #if defined (TFT_PARALLEL_8_BIT) || defined (TFT_PARALLEL_16_BIT) || defined (RPI_DISPLAY_TYPE)
          writedata(0x55);            
    #else
          writedata(0x66);            
    #endif
    
    #if !defined (TFT_PARALLEL_8_BIT) && !defined (TFT_PARALLEL_16_BIT)
      writecommand(TFT_INVON);
    #endif

					writecommand(0x22);
					writedata(0x00);
					writedata(0x00);
					writedata(0x01);
					writedata(0x3F);

					writecommand(0x2B);
					writedata(0x00);
					writedata(0x00);
					writedata(0x01);
					writedata(0xE0);
					delay(120);
					writecommand(0x29);

#elif defined (ILI9481_INIT_8)

					 
					writecommand(0x01);  
					delay(220);

					writecommand(0x11);
					delay(280);

					writecommand(0xd0);  
					writedata(0x07); 
					writedata(0x44); 
					writedata(0x1E); 
					delay(220);

					writecommand(0xd1);  
					writedata(0x00); 
					writedata(0x0C); 
					writedata(0x1A); 

					writecommand(0xC5);   
					writedata(0x03);  

					writecommand(0xd2);   
					writedata(0x01);   
					writedata(0x11);   

					writecommand(0xE4);   
					writedata(0xa0);
					writecommand(0xf3);
					writedata(0x00);
					writedata(0x2a);

					 
					writecommand(0xc8);
					writedata(0x00);
					writedata(0x26);
					writedata(0x21);
					writedata(0x00);
					writedata(0x00);
					writedata(0x1f);
					writedata(0x65);
					writedata(0x23);
					writedata(0x77);
					writedata(0x00);
					writedata(0x0f);
					writedata(0x00);
					 

					writecommand(0xC0);	 
					writedata(0x00);  
					writedata(0x3B);  
					writedata(0x00);  
					writedata(0x02);  
					writedata(0x11);  

					writecommand(0xc6);  
					writedata(0x83);
					 

					writecommand(0xf0);  
					writedata(0x01);

					writecommand(0xE4); 
					writedata(0xa0);

					 
					writecommand(0x36);   
					writedata(0x0A);  

					writecommand(0x3a);
    #if defined (TFT_PARALLEL_8_BIT) || defined (TFT_PARALLEL_16_BIT) || defined (RPI_DISPLAY_TYPE)
          writedata(0x55);            
    #else
          writedata(0x66);            
    #endif
    
    #if defined (TFT_PARALLEL_8_BIT) || defined (TFT_PARALLEL_16_BIT)
          writecommand(TFT_INVON);
    #endif

					writecommand(0xb4); 
					writedata(0x02);
					writedata(0x00);  
					writedata(0x00);
					writedata(0x01);

					delay(280);

					writecommand(0x2a);
					writedata(0x00);
					writedata(0x00);
					writedata(0x01);
					writedata(0x3F);  

					writecommand(0x2b);
					writedata(0x00);
					writedata(0x00);
					writedata(0x01);
					writedata(0xDf);  

					 
					writecommand(0x29);	
					writecommand(0x2c);

#endif