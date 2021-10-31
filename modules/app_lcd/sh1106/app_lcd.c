// $Id$
/*

 * This is a modified Version of ../ssd1322/app_lcd.c and ../universal/app_lcd.c

 * Application specific OLED driver for up to 8 * SH1106 (every display
 * provides a resolution of 182x64)
 * Referenced from MIOS32_LCD routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <glcd_font.h>

#include "app_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <glcd_font.h>

#include "app_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// 0: J15 pins are configured in Push Pull Mode (3.3V)
// 1: J15 pins are configured in Open Drain mode (perfect for 3.3V->5V levelshifting)
#ifndef APP_LCD_OUTPUT_MODE
#define APP_LCD_OUTPUT_MODE  0
#endif

// for LPC17 only: should J10 be used for CS lines
// This option is nice if no J15 shiftregister is connected to a LPCXPRESSO.
// This shiftregister is available on the MBHP_CORE_LPC17 module
#ifndef APP_LCD_USE_J10_FOR_CS
#define APP_LCD_USE_J10_FOR_CS 0
#endif

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 display_available = 0;


/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  if( MIOS32_BOARD_J15_PortInit(APP_LCD_OUTPUT_MODE) < 0 )
    return -2; // failed to initialize J15

#if APP_LCD_USE_J10_FOR_CS
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, APP_LCD_OUTPUT_MODE ? MIOS32_BOARD_PIN_MODE_OUTPUT_OD : MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
#endif

  // set LCD type
  mios32_lcd_parameters.lcd_type = MIOS32_LCD_TYPE_GLCD_CUSTOM;
  mios32_lcd_parameters.num_x = APP_LCD_NUM_X;
  mios32_lcd_parameters.width = APP_LCD_WIDTH;
  mios32_lcd_parameters.num_x = APP_LCD_NUM_Y;
  mios32_lcd_parameters.height = APP_LCD_HEIGHT;
  mios32_lcd_parameters.colour_depth = APP_LCD_COLOUR_DEPTH;

  u16 ctr;
  for (ctr=0; ctr<300; ++ctr)
    MIOS32_DELAY_Wait_uS(1000);

  u8 vccstate = SH1106_SWITCHCAPVCC;

  // Initialize LCD
  // Init sequence for 128x64 OLED module
    APP_LCD_Cmd(SH1106_DISPLAYOFF);                    // 0xAE
    APP_LCD_Cmd(SH1106_SETDISPLAYCLOCKDIV);            // 0xD5
    APP_LCD_Cmd(0x80);                                  // the suggested ratio 0x80
    APP_LCD_Cmd(SH1106_SETMULTIPLEX);                  // 0xA8
    APP_LCD_Cmd(0x3F);
    APP_LCD_Cmd(SH1106_SETDISPLAYOFFSET);              // 0xD3
    APP_LCD_Cmd(0x00);                                   // no offset
	
    APP_LCD_Cmd(SH1106_SETSTARTLINE | 0x0);            // line #0 0x40
    APP_LCD_Cmd(SH1106_CHARGEPUMP);                    // 0x8D
    if (vccstate == SH1106_EXTERNALVCC) 
      { APP_LCD_Cmd(0x10); }
    else 
      { APP_LCD_Cmd(0x14); }
    APP_LCD_Cmd(SH1106_MEMORYMODE);                    // 0x20
    APP_LCD_Cmd(0x00);                                  // 0x0 act like ks0108
    APP_LCD_Cmd(SH1106_SEGREMAP | 0x1);
    APP_LCD_Cmd(SH1106_COMSCANDEC);
    APP_LCD_Cmd(SH1106_SETCOMPINS);                    // 0xDA
    APP_LCD_Cmd(0x12);
    APP_LCD_Cmd(SH1106_SETCONTRAST);                   // 0x81
    if (vccstate == SH1106_EXTERNALVCC) 
      { APP_LCD_Cmd(0x9F); }
    else 
      { APP_LCD_Cmd(0xCF); }
    APP_LCD_Cmd(SH1106_SETPRECHARGE);                  // 0xd9
    if (vccstate == SH1106_EXTERNALVCC) 
      { APP_LCD_Cmd(0x22); }
    else 
      { APP_LCD_Cmd(0xF1); }
    APP_LCD_Cmd(SH1106_SETVCOMDETECT);                 // 0xDB
    APP_LCD_Cmd(0x40);
    APP_LCD_Cmd(SH1106_DISPLAYALLON_RESUME);           // 0xA4
    APP_LCD_Cmd(SH1106_NORMALDISPLAY);                 // 0xA6
    
  APP_LCD_Clear();

  APP_LCD_Cmd(SH1106_DISPLAYON);//--turn on oled panel
  //APP_LCD_Cmd(0xae | 1); // Set_Display_On_Off(0x01);

  return (display_available & (1 << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
#if 0  // TODO
  // select LCD depending on current cursor position
  // THIS PART COULD BE CHANGED TO ARRANGE THE 8 DISPLAYS ON ANOTHER WAY
  u8 cs = mios32_lcd_y / APP_LCD_HEIGHT;

  if( cs >= 8 )
    return -1; // invalid CS line
#endif

  u8 cs=0;

  // chip select and DC
#if APP_LCD_USE_J10_FOR_CS
  MIOS32_BOARD_J10_Set(~(1 << cs));
#else
  MIOS32_BOARD_J15_DataSet(~(1 << cs));
#endif
  MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC

  // send data
  MIOS32_BOARD_J15_SerDataShift(data);

  // increment graphical cursor
  ++mios32_lcd_x;

#if 0
  // if end of display segment reached: set X position of all segments to 0
  if( (mios32_lcd_x % APP_LCD_WIDTH) == 0 ) {
    APP_LCD_Cmd(0x75); // set X=0
    APP_LCD_Data(0x00);
    APP_LCD_Data(0x00);
  }
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  // select all LCDs
#if APP_LCD_USE_J10_FOR_CS
  MIOS32_BOARD_J10_Set(0x00);
#else
  MIOS32_BOARD_J15_DataSet(0x00);
#endif
  MIOS32_BOARD_J15_RS_Set(0); // RS pin used to control DC

  MIOS32_BOARD_J15_SerDataShift(cmd);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void)
{ 
  uint8_t i,j;
  for (i=0;i<APP_LCD_HEIGHT;i+=8){
    APP_LCD_GCursorSet(0, i);
    for (j=0;j<APP_LCD_WIDTH;j++){
      APP_LCD_Data(0x0);
    }

  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line)
{
  // mios32_lcd_x/y set by MIOS32_LCD_CursorSet() function
  return APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
   s32 error = 0;

    // set X position
    error |= APP_LCD_Cmd(0x00 | (x & 0xf));
    error |= APP_LCD_Cmd(0x10 | ((x>>4) & 0xf));

    // set Y position
    error |= APP_LCD_Cmd(0xb0 | ((y>>3) & 7));

    return error;
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a single special character
// IN: character number (0-7) in <num>, pattern in <table[8]>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8])
{
 // CLCD
    s32 i;

    // send character number
    APP_LCD_Cmd(((num&7)<<3) | 0x40);

    // send 8 data bytes
    for(i=0; i<8; ++i)
      if( APP_LCD_Data(table[i]) < 0 )
	return -1; // error during sending character

    // set cursor to original position
    return APP_LCD_CursorSet(mios32_lcd_column, mios32_lcd_line);
 
  return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Sets the background colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BColourSet(u32 rgb)
{
  return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Sets the foreground colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_FColourSet(u32 rgb)
{
  return -3; // not supported
}



/////////////////////////////////////////////////////////////////////////////
// Sets a pixel in the bitmap
// IN: bitmap, x/y position and colour value (value range depends on APP_LCD_COLOUR_DEPTH)
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour)
{
  if( x >= bitmap.width || y >= bitmap.height )
    return -1; // pixel is outside bitmap

  // all GLCDs support the same bitmap scrambling
  u8 *pixel = (u8 *)&bitmap.memory[bitmap.line_offset*(y / 8) + x];
  u8 mask = 1 << (y % 8);

  *pixel &= ~mask;
  if( colour )
    *pixel |= mask;

  return -3; // not supported
}

/////////////////////////////////////////////////////////////////////////////
// Transfers a Bitmap within given boundaries to the LCD
// IN: bitmap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap)
{
  
  // abort if max. width reached
  if( mios32_lcd_x >= mios32_lcd_parameters.width )
    return -2;

  // all GLCDs support the same bitmap scrambling
  int line;
  int y_lines = (bitmap.height >> 3);

  u16 initial_x = mios32_lcd_x;
  u16 initial_y = mios32_lcd_y;
  for(line=0; line<y_lines; ++line) {

    // calculate pointer to bitmap line
    u8 *memory_ptr = bitmap.memory + line * bitmap.line_offset;

    // set graphical cursor after second line has reached
    if( line > 0 ) {
      mios32_lcd_x = initial_x;
      mios32_lcd_y += 8;
      APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    }

    // transfer character
    int x;
    for(x=0; x<bitmap.width; ++x)
      APP_LCD_Data(*memory_ptr++);
  }

  // fix graphical cursor if more than one line has been print
  if( y_lines >= 1 ) {
    mios32_lcd_y = initial_y;
    APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
  }

  return 0; // no error
}
