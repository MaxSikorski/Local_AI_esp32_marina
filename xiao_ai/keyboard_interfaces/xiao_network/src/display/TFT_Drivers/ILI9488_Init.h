
 
 
 
 
 


 

    writecommand(0xE0);  
    writedata(0x00);
    writedata(0x03);
    writedata(0x09);
    writedata(0x08);
    writedata(0x16);
    writedata(0x0A);
    writedata(0x3F);
    writedata(0x78);
    writedata(0x4C);
    writedata(0x09);
    writedata(0x0A);
    writedata(0x08);
    writedata(0x16);
    writedata(0x1A);
    writedata(0x0F);

    writecommand(0XE1);  
    writedata(0x00);
    writedata(0x16);
    writedata(0x19);
    writedata(0x03);
    writedata(0x0F);
    writedata(0x05);
    writedata(0x32);
    writedata(0x45);
    writedata(0x46);
    writedata(0x04);
    writedata(0x0E);
    writedata(0x0D);
    writedata(0x35);
    writedata(0x37);
    writedata(0x0F);

    writecommand(0XC0);  
    writedata(0x17);
    writedata(0x15);

    writecommand(0xC1);  
    writedata(0x41);

    writecommand(0xC5);  
    writedata(0x00);
    writedata(0x12);
    writedata(0x80);

    writecommand(TFT_MADCTL);  
    writedata(0x48);           

    writecommand(0x3A);  
#if defined (TFT_PARALLEL_8_BIT) || defined (TFT_PARALLEL_16_BIT) || defined (RPI_DISPLAY_TYPE)
    writedata(0x55);   
#else
    writedata(0x66);   
#endif

    writecommand(0xB0);  
    writedata(0x00);

    writecommand(0xB1);  
    writedata(0xA0);

    writecommand(0xB4);  
    writedata(0x02);

    writecommand(0xB6);  
    writedata(0x02);
    writedata(0x02);
    writedata(0x3B);

    writecommand(0xB7);  
    writedata(0xC6);

    writecommand(0xF7);  
    writedata(0xA9);
    writedata(0x51);
    writedata(0x2C);
    writedata(0x82);

    writecommand(TFT_SLPOUT);   
delay(120);

    writecommand(TFT_DISPON);   
delay(25);

 



