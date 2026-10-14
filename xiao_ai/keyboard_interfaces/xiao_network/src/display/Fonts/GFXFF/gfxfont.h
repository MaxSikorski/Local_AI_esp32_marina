 

 
 
 
 
 

#ifndef _GFXFONT_H_
#define _GFXFONT_H_

#ifdef LOAD_GFXFF

typedef struct {  
	uint32_t bitmapOffset;      
	uint8_t  width, height;     
	uint8_t  xAdvance;          
	int8_t   xOffset, yOffset;  
} GFXglyph;

typedef struct {  
	uint8_t  *bitmap;       
	GFXglyph *glyph;        
	uint16_t  first, last;  
	uint8_t   yAdvance;     
} GFXfont;

   
  #include <Fonts/GFXFF/TomThumb.h>   

  #include <Fonts/GFXFF/FreeMono9pt7b.h>   
  #include <Fonts/GFXFF/FreeMono12pt7b.h>  
  #include <Fonts/GFXFF/FreeMono18pt7b.h>  
  #include <Fonts/GFXFF/FreeMono24pt7b.h>  

  #include <Fonts/GFXFF/FreeMonoOblique9pt7b.h>   
  #include <Fonts/GFXFF/FreeMonoOblique12pt7b.h>  
  #include <Fonts/GFXFF/FreeMonoOblique18pt7b.h>  
  #include <Fonts/GFXFF/FreeMonoOblique24pt7b.h>  
  
  #include <Fonts/GFXFF/FreeMonoBold9pt7b.h>   
  #include <Fonts/GFXFF/FreeMonoBold12pt7b.h>  
  #include <Fonts/GFXFF/FreeMonoBold18pt7b.h>  
  #include <Fonts/GFXFF/FreeMonoBold24pt7b.h>  
  
  #include <Fonts/GFXFF/FreeMonoBoldOblique9pt7b.h>   
  #include <Fonts/GFXFF/FreeMonoBoldOblique12pt7b.h>  
  #include <Fonts/GFXFF/FreeMonoBoldOblique18pt7b.h>  
  #include <Fonts/GFXFF/FreeMonoBoldOblique24pt7b.h>  
  
   
  #include <Fonts/GFXFF/FreeSans9pt7b.h>   
  #include <Fonts/GFXFF/FreeSans12pt7b.h>  
  #include <Fonts/GFXFF/FreeSans18pt7b.h>  
  #include <Fonts/GFXFF/FreeSans24pt7b.h>  
  
  #include <Fonts/GFXFF/FreeSansOblique9pt7b.h>   
  #include <Fonts/GFXFF/FreeSansOblique12pt7b.h>  
  #include <Fonts/GFXFF/FreeSansOblique18pt7b.h>  
  #include <Fonts/GFXFF/FreeSansOblique24pt7b.h>  
  
  #include <Fonts/GFXFF/FreeSansBold9pt7b.h>   
  #include <Fonts/GFXFF/FreeSansBold12pt7b.h>  
  #include <Fonts/GFXFF/FreeSansBold18pt7b.h>  
  #include <Fonts/GFXFF/FreeSansBold24pt7b.h>  
  
  #include <Fonts/GFXFF/FreeSansBoldOblique9pt7b.h>   
  #include <Fonts/GFXFF/FreeSansBoldOblique12pt7b.h>  
  #include <Fonts/GFXFF/FreeSansBoldOblique18pt7b.h>  
  #include <Fonts/GFXFF/FreeSansBoldOblique24pt7b.h>  
  
   
  #include <Fonts/GFXFF/FreeSerif9pt7b.h>   
  #include <Fonts/GFXFF/FreeSerif12pt7b.h>  
  #include <Fonts/GFXFF/FreeSerif18pt7b.h>  
  #include <Fonts/GFXFF/FreeSerif24pt7b.h>  
  
  #include <Fonts/GFXFF/FreeSerifItalic9pt7b.h>   
  #include <Fonts/GFXFF/FreeSerifItalic12pt7b.h>  
  #include <Fonts/GFXFF/FreeSerifItalic18pt7b.h>  
  #include <Fonts/GFXFF/FreeSerifItalic24pt7b.h>  
  
  #include <Fonts/GFXFF/FreeSerifBold9pt7b.h>   
  #include <Fonts/GFXFF/FreeSerifBold12pt7b.h>  
  #include <Fonts/GFXFF/FreeSerifBold18pt7b.h>  
  #include <Fonts/GFXFF/FreeSerifBold24pt7b.h>  
  
  #include <Fonts/GFXFF/FreeSerifBoldItalic9pt7b.h>   
  #include <Fonts/GFXFF/FreeSerifBoldItalic12pt7b.h>  
  #include <Fonts/GFXFF/FreeSerifBoldItalic18pt7b.h>  
  #include <Fonts/GFXFF/FreeSerifBoldItalic24pt7b.h>  

#endif  

#endif  
