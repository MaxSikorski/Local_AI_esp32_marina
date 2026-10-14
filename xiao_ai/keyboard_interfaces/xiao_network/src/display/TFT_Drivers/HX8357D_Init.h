
 
 
 
 
 


 

     
    writecommand(HX8357D_SETC);
    writedata(0xFF);
    writedata(0x83);
    writedata(0x57);
    delay(300);

     
    writecommand(HX8357_SETRGB); 
    writedata(0x80);   
 
    writedata(0x0);
    writedata(0x06);
    writedata(0x06);

    writecommand(HX8357D_SETCOM);
    writedata(0x25);   
    
    writecommand(HX8357_SETOSC);
    writedata(0x68);   
    
    writecommand(HX8357_SETPANEL);  
    writedata(0x05);   
    
    writecommand(HX8357_SETPWR1);
    writedata(0x00);   
    writedata(0x15);   
    writedata(0x1C);   
    writedata(0x1C);   
    writedata(0x83);   
    writedata(0xAA);   

    writecommand(HX8357D_SETSTBA);  
    writedata(0x50);   
    writedata(0x50);   
    writedata(0x01);   
    writedata(0x3C);   
    writedata(0x1E);   
    writedata(0x08);   
    
    writecommand(HX8357D_SETCYC);  
    writedata(0x02);   
    writedata(0x40);   
    writedata(0x00);   
    writedata(0x2A);   
    writedata(0x2A);   
    writedata(0x0D);   
    writedata(0x78);   

    writecommand(HX8357D_SETGAMMA); 
    writedata(0x02);
    writedata(0x0A);
    writedata(0x11);
    writedata(0x1d);
    writedata(0x23);
    writedata(0x35);
    writedata(0x41);
    writedata(0x4b);
    writedata(0x4b);
    writedata(0x42);
    writedata(0x3A);
    writedata(0x27);
    writedata(0x1B);
    writedata(0x08);
    writedata(0x09);
    writedata(0x03);
    writedata(0x02);
    writedata(0x0A);
    writedata(0x11);
    writedata(0x1d);
    writedata(0x23);
    writedata(0x35);
    writedata(0x41);
    writedata(0x4b);
    writedata(0x4b);
    writedata(0x42);
    writedata(0x3A);
    writedata(0x27);
    writedata(0x1B);
    writedata(0x08);
    writedata(0x09);
    writedata(0x03);
    writedata(0x00);
    writedata(0x01);
    
    writecommand(HX8357_COLMOD);
    writedata(0x55);   
    
    writecommand(HX8357_MADCTL);  
    writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_COLOR_ORDER); 
    
    writecommand(HX8357_TEON);   
    writedata(0x00); 
    
    writecommand(HX8357_TEARLINE);   
    writedata(0x00); 
    writedata(0x02);

    writecommand(HX8357_SLPOUT);   
    delay(150);
    
    writecommand(HX8357_DISPON);   
    delay(50);

 



