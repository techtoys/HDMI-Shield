/**
 * @file    LcdParam.h
 * @author  John Leung @ TechToys Co. (www.TechToys.com.hk)
 * @license BSD license, all text above and below must be included in any redistribution
 * 
 * This header file describes timing information of a LCD panel in RGB mode.
 * 
 * @section HISTORY
 * 
 * v1.0 - First release on 14-07-2017
 */

#ifndef _LCDPARAM_H
#define _LCDPARAM_H

#include "stdint.h"
#include "stdbool.h"

/**
 * @note  Timing parameters of a LCD panel.<br>
 *        A structure with CEA_ prefix below implies a timing in compliance with CEA Standard (CEA-861-D).<br>
 *        CEA-861-D establishes protocols for consumer electronics devices such as digital TVs, set-top boxes,
 *        DVD players, and other related source or sink devices (a HDTV in our case).<br>
 *        Some information specific for RA8876 is that, although there are over 40 standards defined in CEA-861-D
 *        there are not all of them can be used in RA8876.
 *        Generation of a pixel clock is limited by the SPLL clock (SCAN_FREQ) and core clock (CORE_FREQ) speed 
 *        of RA8876. RA8876's CORE_FREQ without internal font is 133MHz. SCAN_FREQ generated should be smaller
 *        than CORE_FREQ / 1.5, as a result, the max. pixel clock = 88.67MHz. 
 *        Therefore, generation of a 1080p HDTV resolution with 60Hz frme rate is not possible because pixel clock
 *        of 1080p@60Hz requires 148MHz.<br>
 *        Another restriction is that max horizontal display width of 2049 or higher is not allowed with RA8876.<br>
 *        Max horizontal blanking period in RA8876 = h back porch + h front porch + h pulse width<br>
 *        = (REG[16]+1)*8+REG[17]+(REG[18]+1)*8+(REG[19]+1)*8<br>
 *        = (31+1)*8 + 16 + (31+1)*8 + (31+1)*8<br>
 *        = 784<br>
 *        As a result, VIC code 32 of 1920x1080p @ 24Hz is not supported either. 
 *        Because max horizontal front porch that can be set in RA8876 is 256 pclk, 
 *        VIC code 31 for 1920x1080p @50Hz is not supported again as its front porch value required
 *        to be 440 pclk.<br>
 *        With such restrictions, it would make sense to use a HDMI encoder with frame scaler embedded such as CH703x
 *        to boost a lower resolution (720p@60Hz in our standard demo) to 1080p@60Hz.
 */
typedef struct LCDParam {
  /// Name of panel
  const char*  name;  
  /// Panel width in pixels
  uint16_t  width;
  /// Panel height in pixels 
  uint16_t  height;  
  /// Horizontal blanking period in pixel clock (pclk) = pulse width + back porch + front porch
  uint16_t  hblank;    
  /// Horizontal front porch in pclk
  uint16_t  hfporch;  
  /// Horizontal pulse width in pclk
  uint16_t  hpulse; 
  /// Vertical blanking period in lines = pulse width + back porch + front porch.
  /// Blanking period for MCU to write data takes vpulse+vbackporch=vblank-vfporch
  uint16_t  vblank;  
  /// Vertical front porch in lines 
  uint16_t  vfporch; 
  /// Vertical pulse with in lines
  uint16_t  vpulse; 
  /// Pixel clock uint in MHz. This will be changed to kHz in future version.
  uint32_t  pclk;     
  /// Vsync polarity : 1=positive vsync, 0=negative vsync
  bool    vsyncPolarity;   
  /// Hsync polarity : 1=positive hsync, 0=negative hsync   
  bool    hsyncPolarity;  
  /// Pixel clock polarity : 1=falling edge, 0=rising edge    
  bool    pclkPolarity;     
  /// DE pulse polarity : 1=negative DE,  0=positive DE   
  bool    dePolarity;         
} LCDParam;

 /**
  * @note This timing is based on the timing in VESA Monitor Timings Spec. available only in a 4:3 aspect ratio.<br>
  *       Resolution 640x480 @ 60Hz. This resolution can be generated directly by RA8876 alone.
  */
const LCDParam CEA_640x480p_60Hz =
{
    "CEA 640x480p@60Hz 4:3 VIC#1",
    640, 480,
    160,  //h blanking total
    16,   //h front porch
    96,   //h pulse width
    45,   //v blanking total
    10,   //v front porch
    2,    //v pulse width
    25,   //pclk in MHz (chg to 25200kHz in new version)
    0,    //-ve vsync
    0,    //-ve vsync
    0,
    0
};

/**
 * @note  This timing is based on CEA-770.2-C [19], with one difference. CEA-770.2-C has a composite sync while
          CEA-861-D uses separate sync signals, thus eliminating the need for serrations during vertical sync. This
          format timing can use either 4:3 or 16:9 aspect ratio.<br>
          Resolution 720x480 @ 60Hz. This resolution can be generated directly by RA8876 alone.
 */
const LCDParam CEA_720x480p_60Hz =
{
    "CEA 720x480p@60Hz 4:3 VIC#2",
    720, 480,
    138,  //h blanking total
    16,   //h front porch
    62,   //h pulse width
    45,   //v blanking total
    9,    //v front porch
    6,    //v pulse width
    27,   //pclk
    0,
    0,
    0,
    0    
};

/**
 * @note  This format is available only in a 16:9 aspect ratio. This timing is based on CEA-770.3-D [20], but there
          are two differences. First, CEA-770.3-C uses tri-level sync, while CEA-861-D uses bi-level. Bi-level sync
          timing is accomplished using the second half of the CEA-770.3-C tri-level sync, defining the actual sync
          time to be the rising edge of that pulse.<br>
          Resolution 1280x720 @ 60Hz. This resolution can be generated directly by RA8876 alone.
 */
const LCDParam CEA_1280x720p_60Hz = 
{
    "CEA 1280x720p@60Hz 16:9 VIC#4",
    1280, 720,    //width, height
    (220+110+40), //horizontal blanking total
    110,      //horizontal front porch
    40,       //horizontal pulse width
    (20+5+5), //vertical blanking total
    5,        //vertical front porch
    5,        //vertical pulse width
    74,       //pixel clock 74MHz
    1,        //+ve vsync
    1,        //+ve hsync
    0,        //rising edge pclk
    0         //+ve de
};

/**
 * @note  This format is available only in a 16:9 aspect ratio. It is likely that non-HDMI source devices may not
          recognize this format in a Detailed Timing Descriptor.<br>
          Resolution 1920x1080 @ 60Hz. This resolution can NOT be generated directly by RA8876 alone.
 */
const LCDParam CEA_1920x1080p_60Hz = 
{
    "CEA 1920x1080P@60Hz 16:9 VIC#16",  //CEA861D format 16
    1920, 1080,
    280,
    88,
    44,
    45,
    4,
    5,
    148,
    1,
    1,
    0,
    0
};

/**
 * @note  This timing is based on ITU-R BT.1358 [41]. This format timing can use either 4:3 or 16:9 aspect ratio.<br>
 *        Resolution 720x576 @ 50Hz. This resolution can be generated directly by RA8876 alone.
 */
const LCDParam CEA_720x576p_50Hz = 
{
    "CEA 720x576p@50Hz 4:3 VIC#17",
    720, 576,
    144,  //h blanking total
    12,   //h front porch
    64,   //h pulse width
    49,   //v blanking total
    5,    //v front porch
    5,    //v pulse width
    27,   //pclk
    0,
    0,
    0,
    0  
};

/**
 * @note    Example of a non-CEA TFT panel (7" TFT).<br>
 *          Resolution 800x600 @ 60Hz. This resolution can be generated directly by RA8876 alone.
 */
const LCDParam AT080TN52 =
{  "Innolux AT080TN52",
    800, 600,
    256, //h blanking total
    210, //h front porch
    8,  //h pulse width
    35, //v blanking total
    12, //v front porch
    8,  //v pulse width
    40, //pclk
    0,  //-ve vsync
    0,  //-ve hsync
    1,  //falling edge pclk
    0
  };

/**
 * @note    Example of a non-CEA TFT panel (Common 4.3" WQVGA TFT of 480*272 RGB).<br>
 *          Resolution 480x272 @ 60Hz. This resolution can be generated directly by RA8876 alone.
 */  
const LCDParam WQVGA_480x272_60Hz = 
{
  "WQVGA 480x272 TFT",
  480, 272,
  45, //h blanking
  2,  //h front porch
  41, //h pulse width
  14, //v blanking 
  2,  //v front porch
  10, //v pulse width
  9,  //9MHz pixel clock
  0,  //-ve vsync
  0,  //-ve hsync
  0,  //rising edge pclk
  0,  //+ve de
};

/**
 * @note    Example of a non-CEA TFT panel (7" TFT).<br>
 *          Resolution 800x480 @ 60Hz. This resolution can be generated directly by RA8876 alone.
 *			Reference: NewHaven 5" 800xRGBx480 TFT (NHD-5.0-800480TF-ATXI#)
 */
const LCDParam WVGA_800x480_60Hz = 
{
  "WVGA 800x480 TFT",
  800, 480,
  128,  //h blanking total
  40,   //h front porch
  48,   //h pulse width
  45,   //v blanking
  13,   //v front porch
  3,    //v pulse width
  30,   //30Mhz pclk
  0,    //-ve vsync
  0,    //-ve hsync
  0,    //rising edge pclk
  0,    //+ve de
};

/**
 * @note    VESA compliance 1024x768 @60Hz frame rate.<br>
 *          Resolution 1024x768 @ 60Hz. This resolution can be generated directly by RA8876 alone.
 */
const LCDParam VESA_1024x768_60Hz = 
{
  "VESA 1024x768 @ 60Hz",
  1024, 768,
  320,  //h blanking
  24,   //h front porch
  136,  //h pulse width
  38,   //v blanking
  3,    //v front porch
  6,    //v pulse width
  65,   //65MHz pixel clk
  0,    //-ve vsync
  0,    //-ve hsync
  0,    //rising edge pclk
  0     //+ve de
};

/**
 * @note    VESA compliance 1366x768 @60Hz frame rate.<br>
 *          This resolution can be generated directly by RA8876 alone.
 */
const LCDParam VESA_1366x768_60Hz = 
{
  "VESA 1366x768 @ 60Hz",
  1366, 768,
  194,  //h blanking
  32,   //h front porch
  64,   //h pulse width
  38,   //v blanking
  6,    //v front porch
  12,   //v pulse width
  75,   //75MHz pixel clk
  0,    //-ve vsync
  0,    //-ve hsync
  0,    //rising edge pclk
  0     //+ve de
};

/**
 * @note    VESA SVGA 800x600 @ 60Hz <br>
 */
const LCDParam SVGA_800x600_60Hz =
{  "VESA 800x600 @ 60Hz",
    800, 600,
    256, 	//h blanking total
    40, 	//h front porch
    128,  	//h pulse width
    28, 	//v blanking total
	1, 		//v front porch
    4,  	//v pulse width
    40, 	//pclk
    1,  	//+ve vsync
    1,  	//+ve hsync
    0,  	//rising edge pclk
    0
  };

/**
 * @note	VESA FWVGA 848x480 @ 60Hz
 */
const LCDParam FWVGA_848x480_60Hz = 
{
  "VESA FWVGA 848x480 @ 60Hz",
  848, 480,
  240,	//h blanking
  16,   //h front porch
  112,	//h pulse width
  37,   //v blanking
  6,   	//v front porch
  8,    //v pulse width
  34,   //34Mhz pclk
  1,    //+ve vsync
  1,    //+ve hsync
  0,    //rising edge pclk
  0,    //+ve de
};
  
#endif


