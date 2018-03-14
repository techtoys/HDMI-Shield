/**
 * @file    Ra8876_Lite.h
 * @author  RAiO Application Team .
 * @license BSD license, all text above and below must be included in any redistribution
 * 
 * 
 * @section HISTORY
 * 
 * Date 29-12-2015  v1.0 - First release<br>
 * Date 14-07-2017  Modify by John Leung @ TechToys for CEA861-D monitors compatible<br>
 * 
 * @note  Pinout summary::<br>
 *        3.3V Arduino incl. Arduino 101 (direct stack), Arduino M0/M0 PRO (direct stack or via adapter Due Zipper), Arduino Due(via adapter Due Zipper)<br>
 *        RA8876-SCK/SD-SCK         <--- D13<br>
 *        RA8876-MISO/SD-MISO       ---> D12<br>
 *        RA8876-MOSI/SD-MOSI       <--- D11<br>
 *        RA8876-SS                 <--- D10<br>
 *        RA8876-RESET              <--- D8<br>
 *		  RA8876-XnINTR				---> D3<br>
 *        SD-CS                     <--- D4<br>
 *
 * @note  Teensy 3.2/3.5 (via adapter Teensy Stacker DTE20171030)<br>
 *        RA8876-SCK/SD-SCK         <--- D14<br>
 *        RA8876-MISO/SD-MISO       ---> D12<br>
 *        RA8876-MOSI/SD-MOSI       <--- D7<br>
 *        RA8876-SS                 <--- D20<br>
 *        RA8876-RESET              <--- D8<br>
 *		  RA8876-XnINTR				---> D2<br>
 *        SD-CS                     <--- D10<br>
 */
 
 /**
  * @note Add support for ESP8266 with hardware SPI, host board is the Due Zipper board DTE20171024 with jumpers set to P12<br>
  *       RA8876-XNSCS        <--- D15<br>
  *       RA8876-XNRESET      <--- D16<br>
  *       RA8876-MOSI/SD-MOSI <--- D13<br>
  *       RA8876-MISO/SD-MISO ---> D12<br>
  *       RA8876-SCK/SD-SCK   <--- D14<br>
  *		  RA8876-XnINTR		  ---> D0<br>
  *       SD-CS               <--- D2<br>
  */

/**
 * @note Add support for ESP32 (ESP32-PICO-D4) with hardware SPI
 *		VSPI for RA8876<br>
 *		===============<br>
 *		RA8876-XNSCS    		<--- GPIO5<br>
 *		RA8876-XNRESET			<--- GPIO10<br>
 *		RA8876-MOSI 			<--- GPIO23<br>
 *		RA8876-MISO 			---> GPIO19<br>
 *		RA8876-SCK   			<--- GPIO18<br>
 *		RA8876-XnINTR		  	---> GPIO35	//it is an input pin, good for interrupt pin here<br>
 *		HSPI for SDCARD<br>
 *		===============<br>
 *		SD-CS					<--- GPIO15<br>	
 *		SDCARD-MOSI				<--- GPIO13<br>
 *		SDCARD-MISO				---> GPIO4	//Cannot use IO12 somehow...<br>
 *		SDCARD-SCK				<--- GPIO14<br>
 */
  
/**
 *
 * @note	Known bug: When bfc_DrawChar_RowRowUnpacked is repeatly called, SD.open(pFilename) sometimes fail even if the file exists.
 *			It is advised to copy all font data to SDRAM prior to main loop or move binary data to external serial Flash.
 */
  
#ifndef _RA8876_LITE_H
#define _RA8876_LITE_H

#include "Arduino.h"
#include <SPI.h>
#include "UserConfig.h"
#include "Ra8876_Registers.h"
#include "stdio.h" 
#include "HDMI/LcdParam.h"
#include "edid/edid.h"
#include "Color/Color.h"
#include "util/printf.h"
#include "hw_font/hw_font.h"

#if defined (LOAD_BFC_FONT)
	#include "bfc/bfcFontMgr.h"
#endif

#if defined (LOAD_SD_LIBRARY)
	#include <SD.h>
#endif	

/**
 * @note  More about Canvas : <br>
 * Graphic contents on the LCD(or HDTV) are updated by data in SDRAM which is divided into several image buffers limited by the memory size.<br>
 * In our case, it is a Winbond 256Mbits SDRAM (W9825G6JH) representing a storage of 32,000,000 bytes.<br>
 * This is also equivalent to more than a storage of 1280(w)*720(h)*(17 pages) in 16-bit pixel depth.<br>
 * When a pixel is written to the Canvas area that overlaps with the display window of the LCD, that pixel will be visible.<br>
 * There are several attributes of Canvas including the starting address, width, active window starting coordinates, and finally its color depth.<br>
 * (1) Starting address of the Canvas is the physical address location in SDRAM. This parameter should be programmed to registers at REG[50h] - REG[53h] of RA8876.<br>
 * (2) Width of the Canvas in pixels is a parameter to be programmed to registers REG[54h] - REG[55h].<br>
 * This is a 13-bit value (with bits[1:0] '0')  therefore the max. Canvas width possible is 8188.<br>
 * Please notice that the unit is in pixels therefore even if we are setting a 16BPP color depth the Canvas Width for a 1280*720*RGB output is still 1280 but not 1280*2.<br>
 * The Canvas width can be larger than the physical width of the LCD screen.<br>
 * In this case, we may set a different starting coordinates of the Display Main Display to view different part of the Canvas area leading to a scrolling effect.<br>
 * (3) Active Window is the area to write pixels. Its upper-left corner coordinates (x,y) control where to start to write pixels with the Canvas address as the reference.<br>
 * (4) Active Window Width (unit:pixels) defines the width for auto line feed when data hits the right border.<br>
 * (5) Active Window Height (uint: lines) defines the height of the area in Canvas for data update.<br>
 * (6) Color depth in bit-per-pixel (BPP) of Canvas and Active Window. This parameter should be programmed to register REG[5Eh] at bit[1:0]. Three values are possible: 8BPP, 16BPP, & 24BPP.<br>
 */
namespace {
	const uint8_t RA8876_SPI_CMDWRITE 	= 0x00;
	const uint8_t RA8876_SPI_DATAWRITE 	= 0x80;
	const uint8_t RA8876_SPI_DATAREAD 	= 0xc0;
	const uint8_t RA8876_SPI_STATUSREAD = 0x40;
	const uint8_t OSC_FREQ 				= 12;	///Crystal frequency onboard (12MHz)
	const uint8_t DRAM_FREQ 			= 166;	///Max SD RAM freq
	const uint8_t SPLL_FREQ_MAX 		= 148;	///Max PLL freq. hence it is the max pixel clock frequency that can be generated
	const uint8_t CORE_FREQ 			= 120;	///Core freq. of RA8876
	const uint32_t CANVAS_OFFSET		= 0;	///Default SDRAM start address, known as the Canvas offset. Refer to note above for details
	const uint32_t CANVAS_CACHE			= 7200l;///line offset for cache
	const uint32_t MAIN_WINDOW_OFFSET	= 0;	///Starting address for display window. This is the starting address for visible area on the LCD
	const uint16_t MAIN_WINDOW_STARTX	= 0;	///Default Main Window (the area visible) start x with Canvas Start Address as the reference
	const uint16_t MAIN_WINDOW_STARTY	= 0;	///Default Main Window start y with Canvas Start Address as the reference
	const uint32_t MEM_SIZE_MAX			= 32l*1024l*1024l;	///Max. size in byte of SDRAM
	const uint16_t ACTIVE_WINDOW_STARTX = 0;	///Default Active window (the area to update) start x with Canvas Start Address as the reference
	const uint16_t ACTIVE_WINDOW_STARTY = 0;	///Default Active window start y with Canvas Start Address as the reference
	const uint16_t VSYNC_TIMEOUT_MS		= 50;	///Maximum timeout in millisec in function Ra8876_Lite::vsyncWait()
}


/**
 * @note  Display input data format. Correlate to SDRAM Data Structure (Section 8.2) on RA8876_DS_V13.pdf
 */
enum COLOR_MODE {  
  COLOR_8BPP_RGB332=1,    	//8BPP in R[3]G[3]B[2], input data format=R[7:5]G[7:5]B[7:6]
  COLOR_16BPP_RGB565=2,   	//16BPP in R[5]G[6]B[5], input data format=G[4:2]B[7:3], R[7:3]G[7:5]
  COLOR_24BPP_RGB888=3,   	//24BPP in B[7]G[7]R[7], input data format=B[7:0]G[7:0]R[7:0]
  COLOR_6BPP_ARGB2222=4,	//Index display with opacity (aRGB 2:2:2:2)
  COLOR_12BPP_ARGB4444=5  	//12BPP with ARGB of 4bits each, input data format=G[7:4]B[7:4], A[3:0]R[7:4]
  };

/**
 * @note  RA8876 class for Arduino/mbed
 */
class Ra8876_Lite
{
 private:
  uint8_t _xnscs, _xnreset, _mosi, _miso, _sck;
  LCDParam lcd;
  SPIClass *_SPI;
  bool _initialised = false;
  COLOR_MODE _colorMode;
  //This irq flagto be set in isr() function in main.
  volatile bool _irqEventTrigger = false;
   
  ///@note Canvas width & height, and they can be larger than the LCD dimensions
  uint16_t _canvasWidth;
  uint16_t _canvasHeight;
    
  void     hal_bsp_init(void);
  void     hal_gpio_write(uint8_t pin, bool level);
  void     hal_delayMs(uint32_t ms);
  inline   void hal_di(void);
  inline   void hal_ei(void);
  inline   uint16_t hal_spi_write16(uint16_t val);
  inline   void hal_spi_write(const uint8_t  *buf, uint32_t byte_count);
  inline   void hal_spi_write(const uint16_t *buf, uint32_t word_count);
  inline   void hal_spi_read (uint8_t  *buf, uint32_t byte_count);
 
  
  void     lcdRegWrite(uint8_t reg);
  uint8_t  lcdDataRead(void);                
  uint8_t  lcdStatusRead(void);  
  
  void     	lcdRegDataWrite(uint8_t reg, uint8_t data);
  uint8_t  	lcdRegDataRead(uint8_t reg);
  void		lcdDataWrite(uint8_t data);
  void 		lcdDataWrite16bpp(uint16_t data); 
  
  bool     ra8876PllInitial (uint16_t pclk);  
  bool     ra8876SdramInitial(void);  
  
  /**
   * @note Functions in checkXXX(args) are status polling routines
   */
  bool  checkSdramReady(uint32_t timeout);
  bool  checkIcReady(uint32_t timeout);
  void  check2dBusy(void);
  bool  check2dBusy(uint32_t timeout);  
  void  checkWriteFifoNotFull(void);
  bool  checkWriteFifoNotFull(uint32_t timeout);
  void  checkWriteFifoEmpty(void);
  bool  checkWriteFifoEmpty(uint32_t timeout);
  void  checkReadFifoNotFull(void);
  void  checkReadFifoNotEmpty(void);

  void  lcdHorizontalWidthVerticalHeight(uint16_t width,uint16_t height);
  void  lcdHorizontalBackPorch(uint16_t numbers);
  void  lcdHsyncStartPosition(uint16_t numbers);
  void  lcdHsyncPulseWidth(uint16_t numbers);
  void  lcdVerticalBackPorch(uint16_t numbers);
  void  lcdVsyncStartPosition(uint16_t numbers);
  void  lcdVsyncPulseWidth(uint16_t numbers);
  
  inline void ramAccessPrepare(void);

  /* SDRAM addressing */
  void 		putPicture_set_frame(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t lnOffset=CANVAS_OFFSET);
  int32_t 	canvasAddress_from_lnOffset(uint32_t lnOffset);
  
  /* Display Window (Main Window) setup */
  void displayImageStartAddress(uint32_t addr);
  void displayImageWidth(uint16_t width);
  void displayWindowStartXY(uint16_t x0,uint16_t y0);
  
  /**
   * @note BTE related functions
   */
  void bte_Source0_MemoryStartAddr(uint32_t addr);
  void bte_Source0_ImageWidth(uint16_t width);
  void bte_Source0_WindowStartXY(uint16_t x0,uint16_t y0);
  void bte_Source1_MemoryStartAddr(uint32_t addr);
  void bte_Source1_ImageWidth(uint16_t width);
  void bte_Source1_WindowStartXY(uint16_t x0,uint16_t y0);
  void bte_DestinationMemoryStartAddr(uint32_t addr);
  void bte_DestinationImageWidth(uint16_t width);
  void bte_DestinationWindowStartXY(uint16_t x0,uint16_t y0);
  void bte_WindowSize(uint16_t width, uint16_t height);
  
  /* BFC font related functions */
#if defined (LOAD_BFC_FONT)
  Color bfc_GetColorBasedPixel(uint8_t pixel, uint8_t bpp, Color src_color, Color bg);
  int bfc_DrawChar_RowRowUnpacked(
  uint16_t x0, uint16_t y0, 
  const BFC_FONT *pFont, 
  uint16_t ch, 
  Color color, 
  Color bg, 
  bool rotate_ccw90=false, 
  uint32_t lnOffset=CANVAS_OFFSET);
  
#if defined (LOAD_SD_LIBRARY)
  int bfc_DrawChar_RowRowUnpacked(
  uint16_t x0, uint16_t y0, 
  const char* pFilename, 
  uint16_t ch, 
  Color color, 
  Color bg, 
  bool rotate_ccw90=false,
  uint32_t lnOffset=CANVAS_OFFSET);
#endif
#endif

  /* Switch between Text(hardware) vs Graphic mode */
  void textMode(bool on);
  void setHwTextParameter1(FONT_SRC source_select, FONT_HEIGHT size_select, FONT_CODE iso_select);//cch
  void setHwTextParameter2(uint8_t align, bool chroma_key, uint8_t width_enlarge, uint8_t height_enlarge, bool rotate_ccw90=false);//cdh 
  void genitopCharacterRomParameter(FONT_CODE coding, uint8_t gt_width, GT_FONT_ROM part_no=FONT_ROM_GT21L16T1W, uint8_t scs_select=RA8876_SERIAL_FLASH_SELECT0);//b7h,bbh,ceh,cfh
  
  /* Pixel-wise rotation */
  void rotateCw90(uint16_t *x, uint16_t *y);
  void rotateCcw90(uint16_t *x, uint16_t *y);
  
  /* Serial Flash and Genitop font ROM, DMA related */
  void setSerialFlash(uint8_t scs_select=RA8876_SERIAL_FLASH_SELECT1);
  
public:

  Ra8876_Lite(uint8_t xnscs, uint8_t xnreset, uint8_t mosi, uint8_t miso, uint8_t sck);
  bool  begin(const LCDParam *timing=&CEA_1280x720p_60Hz, MonitorInfo *edid=NULL, bool automatic = false);
  bool  initialised(void) {return _initialised;}; 
  void  displayOn(bool on);
  
  COLOR_MODE  getColorMode(void);
  uint8_t	  getColorDepth(void);
  
  void setForegroundColor(Color color);
  void setBackgroundColor(Color color);
  
  /// Interrupt related functions
  void		irqEventHandler(void);
  void		irqEventSet(uint8_t event, bool en);
  uint8_t 	irqEventQuery(void);
  void		irqEventFlagReset(uint8_t event);
  void		vsyncWait(void);
  
 /**
  * @brief   Return monitor width in pixels. <br>
  *			 This parameter will be used to initialize RA8876 in lcdHorizontalWidthVerticalHeight().<br>
  *			 This parameter will also be used to initialize the width of Canvas Window and Active Window<br>
  *			 if no overloading in canvasImageBuffer() is applied.
  * @return  width of the monitor
  */
  uint16_t  getLCDWidth(void) {return lcd.width;}

  /**
   * @brief 	Return monitor height in lines. <br>
   *			This parameter will be used to initialize RA8876 in lcdHorizontalWidthVerticalHeight().<br>
   *			This parameter will also be used to initialize the height of Canvas Window and Active Window<br>
   *			if no overloading in canvasImageBuffer() is applied.
   * @return 	height of the monitor
   */
  uint16_t  getLCDHeight(void) {return lcd.height;}
  
  /**
   * @brief   Return canvas width in pixels
   * @return  width of canvas in pixel count
   */
  uint16_t  getCanvasWidth(void) {return _canvasWidth;}
  
  /**
   * @brief   Return height of active window in lines
   * @return  Height of active window. 
   */
  uint16_t  getCanvasHeight(void) {return _canvasHeight;}
  
  /* SDRAM addressing */
  void canvasLinearModeSet(void);
  void canvasBlockModeSet(void);
  
  void canvasImageBuffer(
  uint16_t width, 
  uint16_t height, 
  uint16_t x0=ACTIVE_WINDOW_STARTX, 
  uint16_t y0=ACTIVE_WINDOW_STARTY, 
  COLOR_MODE mode=COLOR_16BPP_RGB565, 
  uint32_t offset=CANVAS_OFFSET);
  
  void canvasClear(
  Color color, 
  uint16_t x0=ACTIVE_WINDOW_STARTX, 
  uint16_t y0=ACTIVE_WINDOW_STARTY, 
  uint32_t lnOffset=CANVAS_OFFSET);
  
  void canvasWrite(const void *data, uint32_t lnOffset, size_t byte_count);
  
  #if defined (LOAD_SD_LIBRARY)
  void canvasWrite(uint16_t width, uint16_t height, const char *pFilename, uint32_t lnOffset);
  #endif
  void canvasRead (void  *data, uint32_t lnOffset, size_t data_count); 
  
  void canvasImageStartAddress(uint32_t addr);
  void canvasImageWidth(uint16_t width, uint16_t height);
  void activeWindowXY(uint16_t x0,uint16_t y0);
  void activeWindowWH(uint16_t width,uint16_t height); 
  
  /* Set Display Window, need to call canvasImageBuffer() before calling this fcn */
  void displayMainWindow(
  uint16_t x0=MAIN_WINDOW_STARTX, 
  uint16_t y0=MAIN_WINDOW_STARTY, 
  uint32_t offset=MAIN_WINDOW_OFFSET);
  
  /*graphic function*/
  void graphicMode(bool on);
  void setPixelCursor(uint16_t x, uint16_t y, uint32_t lnOffset=CANVAS_OFFSET);
  void putPixel(uint16_t x, uint16_t y, Color color, uint32_t lnOffset=CANVAS_OFFSET);
  void putPicture(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* data, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET);
  ///If no SD card is available, const data stored in MCU's Flash or ext. Flash is the only option.
  ///For small system (Arduino or mbed) it is not advised.
  #if defined (LOAD_SD_LIBRARY)
  void putPicture(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char *pFilename,   bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET);
  void putPicture(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const String& pFilename, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET){
  putPicture(x,y,width,height,pFilename.c_str(), rotate_ccw90, lnOffset);}
  #endif
 
  /* Hardware text function*/
  void setHwTextColor(Color text_color);
  void setHwTextColor(Color foreground_color, Color background_color);
  void setHwTextCursor(uint16_t x, uint16_t y, uint32_t lnOffset=CANVAS_OFFSET);
  void setHwTextParam(Color background_color, uint8_t width_enlarge=1, uint8_t height_enlarge=1, bool rotate_ccw90=false);
  
  void putHwChar(const HW_FONT *pFont, const char ch); 
  void putHwChar(const HW_FONT *pFont, const uint16_t ch);
  void putHwString(const HW_FONT *pFont, const char *str);
  void putHwString(const HW_FONT *pFont, const uint16_t *str);
  void putHwString(const HW_FONT *pFont, const String &str) 
  {putHwString(pFont, str.c_str());}

#if defined (LOAD_BFC_FONT)
	uint16_t putBfcChar  (uint16_t x0,uint16_t y0, const BFC_FONT *pFont, const uint16_t ch, Color color, Color bg, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET);
	uint16_t putBfcString(uint16_t x0,uint16_t y0, const BFC_FONT *pFont, const uint16_t *str, Color color, Color bg, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET);
	uint16_t putBfcString(uint16_t x0,uint16_t y0, const BFC_FONT *pFont, const char *str, Color color, Color bg, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET);
	uint16_t putBfcString(uint16_t x0,uint16_t y0, const BFC_FONT *pFont, const String &str, Color color, Color bg, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET) 
	{uint16_t width = putBfcString(x0,y0,pFont, str.c_str(), color, bg, rotate_ccw90, lnOffset); return width;}
	
	uint16_t getBfcStringWidth(const BFC_FONT *pFont, const char *str);
	uint16_t getBfcStringWidth(const BFC_FONT *pFont, const uint16_t *str);
	uint16_t getBfcStringWidth(const BFC_FONT *pFont, const String &str)
	{uint16_t width = getBfcStringWidth(pFont, str.c_str()); return width;}
	uint16_t getBfcFontHeight(const BFC_FONT *pFont);
	///If no SD card is available, const data stored in MCU's Flash or ext. Flash is the only option.
	///For small system (Arduino or mbed) it is not advised.
	#if defined (LOAD_SD_LIBRARY)
	uint16_t putBfcChar  (uint16_t x0,uint16_t y0, const char *pFilename, const uint16_t ch, Color color, Color bg, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET);
	uint16_t blitBfcChar (uint16_t x0,uint16_t y0, const char *pFilename, const uint16_t ch, Color color, Color bg, bool rotate_ccw90=false);
	uint16_t putBfcString(uint16_t x0,uint16_t y0, const char *pFilename, const uint16_t *str, Color color, Color bg, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET);
	uint16_t blitBfcString(uint16_t x0, uint16_t y0, const char *pFilename, const uint16_t *str, Color color, Color bg, bool rotate_ccw90=false);
	uint16_t putBfcString(uint16_t x0,uint16_t y0, const char *pFilename, const char *str, Color color, Color bg, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET);
	uint16_t blitBfcString(uint16_t x0, uint16_t y0, const char *pFilename, const char *str, Color color, Color bg, bool rotate_ccw90=false);
	///@note	This function is to support const String as the argument.
	uint16_t putBfcString(uint16_t x0,uint16_t y0, const char *pFilename, const String &str, Color color, Color bg, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET) 
	{uint16_t width = putBfcString(x0,y0,pFilename, str.c_str(), color, bg, rotate_ccw90, lnOffset); return width;}  
	uint16_t blitBfcString(uint16_t x0,uint16_t y0, const char *pFilename, const String &str, Color color, Color bg, bool rotate_ccw90=false, uint32_t lnOffset=CANVAS_OFFSET) 
	{uint16_t width = blitBfcString(x0,y0,pFilename, str.c_str(), color, bg, rotate_ccw90); return width;}  	
	
	uint16_t getBfcCharWidth(const char *pFilename, const uint16_t ch);
	uint16_t getBfcStringWidth(const char *pFilename, const char *str);
	uint16_t getBfcStringWidth(const char *pFilename, const String &str)
	{uint16_t width = getBfcStringWidth(pFilename, str.c_str()); return width;}
	uint16_t getBfcStringWidth(const char *pFilename, const uint16_t *str);
	uint16_t getBfcFontHeight(const char *pFilename);
	#endif
#endif
  
  /*draw function*/
  void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, Color color);
  void drawSquare(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, Color color);
  void drawSquareFill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, Color color);
  void drawCircleSquare(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t xr, uint16_t yr, Color color);
  void drawCircleSquareFill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t xr, uint16_t yr, Color color);
  void drawTriangle(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2, Color color);
  void drawTriangleFill(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2, Color color);
  void drawCircle(uint16_t x0,uint16_t y0,uint16_t r,Color color);
  void drawCircleFill(uint16_t x0,uint16_t y0,uint16_t r,Color color);
  void drawEllipse(uint16_t x0,uint16_t y0,uint16_t xr,uint16_t yr,Color color);
  void drawEllipseFill(uint16_t x0,uint16_t y0,uint16_t xr,uint16_t yr,Color color);
 
  /*BTE function*/                   
  void bteMemoryCopyWithROP(  uint32_t s0_addr,
                              uint16_t s0_image_width,
                              uint16_t s0_x,uint16_t s0_y,
                              uint32_t s1_addr,
                              uint16_t s1_image_width,
                              uint16_t s1_x,uint16_t s1_y,
                              uint32_t des_addr,
                              uint16_t des_image_width,
                              uint16_t des_x,uint16_t des_y,
                              uint16_t copy_width,uint16_t copy_height,
                              uint8_t  rop_code);
                            
  void bteMemoryCopyWithChromaKey(  uint32_t s0_addr,
                                    uint16_t s0_image_width,
                                    uint16_t s0_x,uint16_t s0_y,
                                    uint32_t des_addr,
                                    uint16_t des_image_width, 
                                    uint16_t des_x,uint16_t des_y,
                                    uint16_t copy_width,uint16_t copy_height,
                                    Color chromakey_color);
                                  
  void bteMpuWriteWithROP(uint32_t s1_addr,
                          uint16_t s1_image_width,
                          uint16_t s1_x,uint16_t s1_y,
                          uint32_t des_addr,
                          uint16_t des_image_width,
                          uint16_t des_x,uint16_t des_y,
                          uint16_t width,uint16_t height,
                          uint8_t  rop_code,
                          const uint8_t *data);
                          
  void bteMpuWriteWithROP(uint32_t s1_addr,
                          uint16_t s1_image_width,
                          uint16_t s1_x,uint16_t s1_y,
                          uint32_t des_addr,
                          uint16_t des_image_width,uint16_t des_x,uint16_t des_y,
                          uint16_t width,uint16_t height,
                          uint8_t  rop_code,
                          const uint16_t *data);     
                                         
  void bteMpuWriteWithChromaKey(uint32_t des_addr,
                                uint16_t des_image_width, 
                                uint16_t des_x,uint16_t des_y,
                                uint16_t width,uint16_t height,
                                Color chromakey_color,
                                const uint8_t *data);
                                
  void bteMpuWriteWithChromaKey(uint32_t des_addr,
                                uint16_t des_image_width,
                                uint16_t des_x,uint16_t des_y,
                                uint16_t width,uint16_t height,
                                Color chromakey_color,
                                const uint16_t *data);
                                
  void bteMpuWriteColorExpansion( uint32_t des_addr,
                                  uint16_t des_image_width, 
                                  uint16_t des_x,uint16_t des_y,
                                  uint16_t width,uint16_t height,
                                  Color foreground_color,
                                  Color background_color,
                                  const uint8_t *data);
                                  
  void bteMpuWriteColorExpansionWithChromaKey(uint32_t des_addr,
                                              uint16_t des_image_width,
                                              uint16_t des_x,uint16_t des_y,
                                              uint16_t width,uint16_t height,
                                              Color foreground_color,
                                              Color background_color,
                                              const uint8_t *data);
                                              
  void btePatternFill(uint8_t p8x8or16x16, 
                      uint32_t s0_addr,
                      uint16_t s0_image_width,
                      uint16_t s0_x,uint16_t s0_y,
                      uint32_t des_addr,
                      uint16_t des_image_width, 
                      uint16_t des_x,uint16_t des_y,
                      uint16_t copy_width,uint16_t copy_height);
                      
  void btePatternFillWithChromaKey( uint8_t p8x8or16x16,
                                    uint32_t s0_addr,
                                    uint16_t s0_image_width,
                                    uint16_t s0_x,uint16_t s0_y,
                                    uint32_t des_addr,
                                    uint16_t des_image_width, 
                                    uint16_t des_x,uint16_t des_y,
                                    uint16_t copy_width,uint16_t copy_height,
                                    Color chromakey_color);

  void bteMemoryCopyWithOpacity(uint32_t s0_addr,
                                uint16_t s0_image_width,
                                uint16_t s0_x, uint16_t s0_y,
                                uint32_t s1_addr,
                                uint16_t s1_image_width,
                                uint16_t s1_x, uint16_t s1_y,
                                uint32_t des_addr,
                                uint16_t des_image_width,
                                uint16_t des_x, uint16_t des_y,
                                uint16_t copy_width, uint16_t copy_height,
                                uint8_t  alpha);
								
  void bteMemoryCopyWithOpacity(  uint32_t s0_addr,
                                  uint16_t s0_image_width,
                                  uint16_t s0_x, uint16_t s0_y,
                                  uint32_t s1_addr,
                                  uint16_t s1_image_width,
                                  uint16_t s1_x, uint16_t s1_y,
                                  uint32_t des_addr,
                                  uint16_t des_image_width,
                                  uint16_t des_x, uint16_t des_y,
                                  uint16_t copy_width, uint16_t copy_height,
                                  uint8_t  alpha,
                                  bool     mode);

  void bteMpuWriteWithOpacity(    uint32_t s1_addr,
                                  uint16_t s1_image_width,
                                  uint16_t s1_x, uint16_t s1_y,
                                  uint32_t des_addr,
                                  uint16_t des_image_width,
                                  uint16_t des_x, uint16_t des_y,
                                  uint16_t copy_width, uint16_t copy_height,
                                  uint8_t  alpha,
                                  bool     mode,
                                  const uint8_t* data);

  void bteSolidFill ( uint32_t des_addr,
                      uint16_t des_x,uint16_t des_y,
                      uint16_t bte_width,uint16_t bte_height,
                      Color foreground_color);
                                                        
  /*DMA function*/
  //DMA image transfer in Block Mode
  void dmaDataBlockTransfer( 	uint16_t x0,uint16_t y0,
								uint16_t copy_width, uint16_t copy_height,
								uint16_t picture_width,
								uint32_t src_addr);
						
  //DMA image transfer in Linear Mode
  void dmaDataLinearTransfer( 	uint32_t des_addr, 
								uint16_t picture_width, uint16_t picture_height,
								uint32_t src_addr);
  
  /*  PWM functions */
  void pwm1BacklightControl(void);
  void pwm_Prescaler(uint8_t prescaler);
  void pwm_ClockMuxReg(uint8_t pwm1_clk_div, uint8_t pwm0_clk_div, uint8_t xpwm1_ctrl, uint8_t xpwm0_ctrl);
  void pwm_Configuration( uint8_t pwm1_inverter,
                          uint8_t pwm1_auto_reload,
                          uint8_t pwm1_start,
                          uint8_t pwm0_dead_zone, 
                          uint8_t pwm0_inverter, 
                          uint8_t pwm0_auto_reload,
                          uint8_t pwm0_start);
  void pwm0_ClocksPerPeriod(uint16_t clocks_per_period);
  void pwm0_Duty(uint16_t duty);
  void pwm1_ClocksPerPeriod(uint16_t clocks_per_period);
  void pwm1_Duty(uint16_t duty);

  /* Debug functions */
  void parser(char *msg);
  void displayColorBar(bool on);
};

extern Ra8876_Lite ra8876lite;

#endif


