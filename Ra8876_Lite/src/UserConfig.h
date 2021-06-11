/**
 * Add support for board version 2, for Serial Flash 128Mbit. Directive is : #define BOARD_VERSION_2
 * Date: 2019-08-10
 */

 /**
  * Add support for RA8876M with 64Mbit RAM
  * Date: 2021-06-11
  * Add #ifdef RA8876M...#endif in UserConfig.h
  */

////#define RA8876M

#ifndef _USER_CONFIG_H
#define _USER_CONFIG_H

/**
 * @note	This option allows debug information for RA8876 print-out via Serial Monitor (Arduino)<br>
 *			Pre-requisite : printf(args) should be available to use it.
 */
#define DEBUG_LLD_RA8876

/**
 * @note	Use font rendering functions with *.c or *.bin files created by BitFontCreator
 */
#define LOAD_BFC_FONT

/**
 * @note	Legacy Arduino SD library. This required for BitFontCreator fonts.
 *			Use either the legacy Arduino library
 */ 
#define LOAD_SD_LIBRARY
//#define LOAD_SDFAT_LIBRARY	//SdFat not supported yet


///@note	This option allows debug information for memory module (SDRAM) from Serial Monitor(Arduino) when Allegro is used.
//#define DEBUG_LLD_MEMORY

#define BOARD_VERSION_2

#if defined (TEENSYDUINO)
const int RA8876_XNSCS = 20;
const int RA8876_XNRESET = 8;
const int RA8876_MOSI = 7;
const int RA8876_MISO = 12;
const int RA8876_SCK = 14;
const int SDCARD_CS_PIN = 10;
const int SDCARD_MOSI_PIN = RA8876_MOSI;
const int SDCARD_SCK_PIN = RA8876_SCK;
const int RA8876_XNINTR = 2;
#elif defined (ESP8266)
const int RA8876_XNSCS = 15;
const int RA8876_XNRESET = 16;
const int RA8876_MOSI = 13;
const int RA8876_MISO = 12;
const int RA8876_SCK = 14;
const int SDCARD_CS_PIN = 2;
const int SDCARD_MOSI_PIN = RA8876_MOSI;
const int SDCARD_SCK_PIN = RA8876_SCK;
const int RA8876_XNINTR = 0;
#elif defined (ESP32)
const int RA8876_XNSCS = 5;
const int RA8876_XNRESET = 10;
const int RA8876_MOSI = 23;
const int RA8876_MISO = 19;
const int RA8876_SCK = 18;
const int RA8876_XNINTR = 35;	//GPIO35 is an input pin, good for interrupt pin here

const int SDCARD_CS_PIN = 15;	//SD_CS on HSPI CS=15
const int SDCARD_MOSI_PIN = 13;	//HSPI MOSI
const int SDCARD_MISO_PIN = 4;	//Don't use HSPI's native MISO (12) otherwise boot problem.
const int SDCARD_SCK_PIN = 14;	//HSPI SCK

const int CH7035_SDA = 32;
const int CH7035_SCL = 33;
#else
//Arduino Due, M0, or Arduino 101
const int RA8876_XNSCS = 10;
const int RA8876_XNRESET = 8;
const int RA8876_MOSI = 11;
const int RA8876_MISO = 12;
const int RA8876_SCK = 13;
const int SDCARD_CS_PIN = 4;
const int SDCARD_MOSI_PIN = RA8876_MOSI;
const int SDCARD_SCK_PIN = RA8876_SCK;
const int RA8876_XNINTR = 3;
#endif


#endif	//_USER_CONFIG_H
