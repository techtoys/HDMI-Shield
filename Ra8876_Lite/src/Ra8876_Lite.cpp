/**
 * @file    Ra8876_Lite.cpp
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
 *        Arduino 101, Arduino M0/M0 PRO, Arduino Due(via adapter), Arduino Mega2560(via adapter)<br>
 *        RA8876-SCK/SD-SCK         <--- D13<br>
 *        RA8876-MISO/SD-MISO       ---> D12<br>
 *        RA8876-MOSI/SD-MOSI       <--- D11<br>
 *        RA8876-SS                 <--- D10<br>
 *        RA8876-RESET              <--- D8<br>
 *		  RA8876-XnINTR				---> D3<br>
 *        SD-CS                     <--- D4<br>
 *
 * @note  Teensy 3.2/3.5 (via adapter)<br>
 *        RA8876-SCK/SD-SCK         <--- D14<br>
 *        RA8876-MISO/SD-MISO       ---> D12<br>
 *        RA8876-MOSI/SD-MOSI       <--- D7<br>
 *        RA8876-SS                 <--- D20<br>
 *        RA8876-RESET              <--- D8<br>
 *		  RA8876-XnINTR				---> D15<br>
 *        SD-CS                     <--- D10<br>    
 *		   
 */
 
 /**
  * @note Add support for ESP8266 with hardware SPI <br>
  *       keyword ESP8266<br>
  *       Pinout <br>
  *       RA8876_XNSCS        <--- D15<br>
  *       RA8876_XNRESET      <--- D16<br>
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
 * Date 13-10-2017	modify bte_DestinationMemoryStartAddr bug for 
 * lcdRegDataWrite(RA8876_DT_STR3, addr>>16), it should be lcdRegDataWrite(RA8876_DT_STR2, addr>>16).
 */
 
/**
 * Date 17-10-2017	Note on using Vsync interrupt from XnINTR pin.
 *					(1) Leave REG[03h] bit7=0 for XnINTR pin active low
 *					(2) Enable Vsync time base interrupt by writing 0x10 to REG[0Bh]
 *					(3) wire an MCU pin to XnINTR, set external interrupt trigger with high->low transition
 *					(4) On an ISR mark a Vsync event
 *					(5) Process Vsync event, finally write 0x10 to REG[0Ch] to clear the interrupt flag to start all over
 *
 * 					*** Vsync frequency varies with framerate. At 60Hz, Vsync frequency is around 17ms. ***
 *					DSO waveforms of XnINTR pin and Vsync pin look like figures below (w/ 800x480 60Hz generated).
 *					Top trace is XnINTR, down trace is Vsync (-ve active)
 ------
       |
	   |
	   |______________________________
	   
 ------		   -----------------------
	   |	  |
	   |	  |
	   |______|
 */
 
#include "Ra8876_Lite.h"

/**
 * @note	Some remarks on SPI settings:
 *			(1) Although it is stated that SPI mode 3 is required for accessing RA8876 on datasheet, 
 *			PI mode 0 is working fine too from experience. The difference is an idle clock state.
 *			SPI mode 3 uses a high SCK when idle whereas SPI mode 0 uses a low SCK when idle.
 *			Both use the same rising SCK edge for sampling. We are using Mode 0 here because SD card onboard is sharing 
 *			the same SPI bus except for the chip select pin. SD card with SPI is using mode 0. Better compatibility 
 *			is acheived with SPI_MODE0.<br>
 *			(2) RA8876 can handle max 50MHz SPI clock but this clock is limited by individual Arduino platform leading
 *			to different SPI clock speeds.
 */
#if defined (_VARIANT_ARDUINO_DUE_X_)
  SPISettings _param(42000000, MSBFIRST, SPI_MODE0);  //Due
#elif defined (_VARIANT_ARDUINO_101_X_)
  SPISettings _param(16000000, MSBFIRST, SPI_MODE0);  //Genuino 101 setup parameters
#elif defined (_VARIANT_ARDUINO_ZERO_)
#include "wiring_private.h"
  SPISettings _param(12000000, MSBFIRST, SPI_MODE0);  //Arduino M0/M0 PRO setup parameters
#elif defined (TEENSYDUINO)
  SPISettings _param(50000000, MSBFIRST, SPI_MODE0);  //Arduino Teensy 3.1/3.2/3.5/3.6
#elif defined (ESP8266)
  SPISettings _param(48000000, MSBFIRST, SPI_MODE0);  //ESP8266
#else
  SPISettings _param(4000000, MSBFIRST,  SPI_MODE0);  //Others
#endif

/**
 * @brief HAL level board support package setup for SPI and GPIO
 */
void Ra8876_Lite::hal_bsp_init(void)
{
  pinMode(_xnreset, OUTPUT); digitalWrite(_xnreset, LOW); //RA8876 in reset mode by default
	
  #if defined (_VARIANT_ARDUINO_DUE_X_)
    _SPI = &SPI;  //legacy SPI port in use with adapter
    _SPI->begin(_xnscs);  
  #elif defined (TEENSYDUINO)
    pinMode(_xnscs, OUTPUT); digitalWrite(_xnscs, HIGH);    //RA8876 deselect by default
    _SPI = &SPI;
    //need to use a SPI port separate from I2S pinout
    //initialize the bus for Teensy
    _SPI->setMOSI(_mosi);
    _SPI->setMISO(_miso);
    _SPI->setSCK(_sck); 
    _SPI->begin();
  #elif defined (_VARIANT_ARDUINO_ZERO_)
    pinMode(_xnscs, OUTPUT); digitalWrite(_xnscs, HIGH);    //RA8876 deselect by default
    //need to map SPI to pins D11-D13, declare raSPI static in global space for its a hardware resource
    static SPIClass raSPI(&sercom1, _miso, _sck, _mosi, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_3);
    _SPI = &raSPI;
    _SPI->begin();
    pinPeripheral(_mosi, PIO_SERCOM);
    pinPeripheral(_miso, PIO_SERCOM);
    pinPeripheral(_sck, PIO_SERCOM);
  #elif defined (ESP8266)
    //With ESP8266 it is possible to use Hw CS for RA8876 SPI access; however, it won't be possible
    //to use SD Card anymore because the RA8876 & SD card are sharing the same SPI with a different CS line.
    //ESP8266 fail to connect SD card when IO15 is set as the hardware CS with _SPI->setCs(true)
    pinMode(_xnscs, OUTPUT); digitalWrite(_xnscs, HIGH);   //ESP8266 uses Sw CS for SPI
    _SPI = &SPI;          //default SPI pinout from ESP-12S
    //_SPI->setHwCs(true);//using IO15 as SS pin for RA8876
    _SPI->begin();	
  #elif defined (ESP32)
	_SPI = &SPI;
	_SPI->begin(RA8876_SCK,RA8876_MISO,RA8876_MOSI,RA8876_XNSCS);
	pinMode(_xnscs, OUTPUT); digitalWrite(_xnscs, HIGH);    //RA8876 deselect by default
	//_SPI->setHwCs(true);
	//Only 24MHz or slower SPI clock working for some unknown reason(s)...
	_SPI->setFrequency(24000000);
  #else
    //default initialization for Arduino 101
    pinMode(_xnscs, OUTPUT); digitalWrite(_xnscs, HIGH);
    _SPI = &SPI;
    _SPI->begin();
  #endif

}

  
/**
 * @brief HAL level GPIO write. The pin to write should be set an output in hal_bsp_init().
 * @param pin is the pin number
 * @param level is '1' or '0'
 */
void Ra8876_Lite::hal_gpio_write(uint8_t pin, bool level)
{
  digitalWrite(pin, level);
}

/**
 * @brief HAL level delay function
 * @param ms is the amount of delay (uint32_t) in milliseconds
 */
void Ra8876_Lite::hal_delayMs(uint32_t ms)
{
  delay(ms);
}

/**
 * @brief HAL level SPI write function in 16-bit
 * @param val is a 16-bit value to write
 */
inline uint16_t Ra8876_Lite::hal_spi_write16(uint16_t val)
{
  uint16_t d;
  
  #if defined (_VARIANT_ARDUINO_DUE_X_)
	_SPI->beginTransaction(_xnscs, _param);
    d = _SPI->transfer16(_xnscs,val);
	_SPI->endTransaction();
  #elif defined (ESP8266)
	uint16_t msb, lsb;
    _SPI->beginTransaction(_param);
    digitalWrite(_xnscs, LOW);  //comment this line if _SPI->setCs(true) is used in hal_bsp_init()
    _SPI->write16(val);
    msb=SPI1W0&0xff;
    lsb = (SPI1W0>>8);
    d = ((uint16_t)msb<<8) | lsb;
    digitalWrite(_xnscs, HIGH);//comment this line if _SPI->setCs(true) is used in hal_bsp_init()
    // _SPI->endTransaction();   there is no need to put endTransaction() here for ESP8266. This fcn doing nothing in ESPClass::SPI.cpp  	
  #elif defined (ESP32)
	digitalWrite(_xnscs, LOW);
    d = _SPI->transfer16(val);
	digitalWrite(_xnscs, HIGH);
  #else
    _SPI->beginTransaction(_param);
    digitalWrite(_xnscs, LOW);
    d = _SPI->transfer16(val);
    digitalWrite(_xnscs, HIGH);
    _SPI->endTransaction();
  #endif  

  return d;
}

/**
 * @brief HAL level burst data write
 * @param *buf points to 8-bit data buffer
 * @param byte_count is the number of data in byte
 */
inline void Ra8876_Lite::hal_spi_write(const uint8_t *buf, uint32_t byte_count)
{
	if(!byte_count) return;
	
	#if defined (_VARIANT_ARDUINO_DUE_X_)
		_SPI->beginTransaction(_xnscs, _param);
		_SPI->transfer(_xnscs,RA8876_SPI_DATAWRITE, SPI_CONTINUE);
		byte_count=byte_count-1;
		while(byte_count--){
			_SPI->transfer(_xnscs, *buf++, SPI_CONTINUE);
		}
		_SPI->transfer(_xnscs, *buf, SPI_LAST);
		_SPI->endTransaction();
	#elif defined (ESP8266)
		_SPI->beginTransaction(_param);
		digitalWrite(_xnscs, LOW);
		_SPI->transfer(RA8876_SPI_DATAWRITE);
		_SPI->transferBytes((uint8_t *)buf, NULL, byte_count);
		digitalWrite(_xnscs, HIGH);
	#elif defined (ESP32)
		digitalWrite(_xnscs, LOW);
		_SPI->transfer(RA8876_SPI_DATAWRITE);
		_SPI->writeBytes((uint8_t *)buf, byte_count);
		digitalWrite(_xnscs, HIGH);
	#else 
		_SPI->beginTransaction(_param);//set _param should execute above digitalWrite(_xnscs, LOW) for some platform e.g. Arduino M0
		digitalWrite(_xnscs, LOW);
		_SPI->transfer(RA8876_SPI_DATAWRITE);
		while(byte_count--){     		
		_SPI->transfer(*buf++);  
		}  
		digitalWrite(_xnscs, HIGH);
		_SPI->endTransaction();  	//endTransaction() should execute after digitalWrite(_xnscs, HIGH) for some platform e.g. Arduino M0	
	#endif
}

/**
 * @brief HAL level burst data read
 * @param *buf points to 8-bit data buffer storing data return
 * @param byte_count is the byte count
 * @note  This function is never called after a comparison on the stability with lcdDataRead().
 *		  Running this function sometimes return false values.
 */
inline void Ra8876_Lite::hal_spi_read(uint8_t *buf, uint32_t byte_count)
{
	if(!byte_count) return;
	
	#if defined (_VARIANT_ARDUINO_DUE_X_)
		_SPI->beginTransaction(_xnscs, _param);
		_SPI->transfer(_xnscs,RA8876_SPI_DATAREAD, SPI_CONTINUE);
		byte_count=byte_count-1;
		while(byte_count--){
			*buf++ = _SPI->transfer(_xnscs, 0x00, SPI_CONTINUE);
		}
		*buf = _SPI->transfer(_xnscs, 0x00, SPI_LAST);
		_SPI->endTransaction();
	#elif defined (ESP8266)
		_SPI->beginTransaction(_param);
		digitalWrite(_xnscs, LOW);
		_SPI->transfer(RA8876_SPI_DATAREAD);
		_SPI->transferBytes(NULL, buf, byte_count);
		digitalWrite(_xnscs, HIGH);
	#elif defined (ESP32)
		digitalWrite(_xnscs, LOW);
		_SPI->transfer(RA8876_SPI_DATAREAD);
		_SPI->transferBytes(NULL, buf, byte_count);
		digitalWrite(_xnscs, HIGH);
	#else 
		_SPI->beginTransaction(_param);//set _param should execute above digitalWrite(_xnscs, LOW) for some platform e.g. Arduino M0
		digitalWrite(_xnscs, LOW);
		_SPI->transfer(RA8876_SPI_DATAREAD);
		while(byte_count--){    	
		*buf++ = _SPI->transfer(0x00);  
		}  
		digitalWrite(_xnscs, HIGH);
		_SPI->endTransaction();  	//endTransaction() should execute after digitalWrite(_xnscs, HIGH) for some platform e.g. Arduino M0	
	#endif
}

/**
 * @brief HAL level burst data write
 * @param *buf points to 16-bit data buffer
 * @param word_count is the data count in uint16_t
 */
inline void Ra8876_Lite::hal_spi_write(const uint16_t *buf, uint32_t word_count)
{
	if(!word_count) return;
	
	#if defined (_VARIANT_ARDUINO_DUE_X_)
		_SPI->beginTransaction(_xnscs, _param);
		_SPI->transfer(_xnscs,RA8876_SPI_DATAWRITE, SPI_CONTINUE);
		word_count=word_count-1;
		while(word_count--){  
		_SPI->transfer(_xnscs, (uint8_t)(*buf), SPI_CONTINUE);
		_SPI->transfer(_xnscs, (uint8_t)(*buf>>8), SPI_CONTINUE);
		buf++;
		}  	
		_SPI->transfer(_xnscs, (uint8_t)(*buf), SPI_CONTINUE);
		_SPI->transfer(_xnscs, (uint8_t)(*buf>>8), SPI_LAST);	
		_SPI->endTransaction();
	#elif defined (ESP8266)
		_SPI->beginTransaction(_param);
		digitalWrite(_xnscs, LOW);
		_SPI->transfer(RA8876_SPI_DATAWRITE);
		while(word_count--)
		{
		_SPI->write((uint8_t)(*buf));
		_SPI->write((uint8_t)(*buf>>8));
		buf++;
		}
		digitalWrite(_xnscs, HIGH);
	#elif defined (ESP32)
		digitalWrite(_xnscs, LOW);
		_SPI->transfer(RA8876_SPI_DATAWRITE);
		_SPI->writePixels(buf, word_count<<1);	//word_count<<1 chg to byte count for SPI.writePixels()
		digitalWrite(_xnscs, HIGH);
	#else
		_SPI->beginTransaction(_param);	//set _param should execute above digitalWrite(_xnscs, LOW) for some platform e.g. Arduino M0
		digitalWrite(_xnscs, LOW); 		
		_SPI->transfer(RA8876_SPI_DATAWRITE);
		while(word_count--){  
		_SPI->transfer((uint8_t)(*buf));
		_SPI->transfer((uint8_t)(*buf>>8));
		buf++;
		}  
		digitalWrite(_xnscs, HIGH);		 
		_SPI->endTransaction();	//endTransaction() should execute after digitalWrite(_xnscs, HIGH) for some platform e.g. Arduino M0
	#endif
}

/**
 * @brief	HAL level disable global interrupts
 */
inline void Ra8876_Lite::hal_di(void)
{
	noInterrupts();
}

/**
 * @brief	HAL level enable global interrupts
 */
inline void Ra8876_Lite::hal_ei(void)
{
	interrupts();
}

/**
 * @brief   Class constructor
 * @param   xnscs is the CS pin for SPI
 * @param   xnreset is hardware reset pin
 * @param   mosi is the MOSI pin for SPI
 * @param   miso is the MOSI pin for SPI
 * @param   sck is the serial clock pin for SPI
 */
Ra8876_Lite::Ra8876_Lite(uint8_t xnscs, uint8_t xnreset, uint8_t mosi, uint8_t miso, uint8_t sck):
_xnscs(xnscs), _xnreset(xnreset), _mosi(mosi), _miso(miso), _sck(sck){}

/**
 * @brief Initialize RA8876 with either predefined LCD timing parameters(manual) or EDID information(automatic).
 * @param *timing points to a LCDParam structure when no EDID information is available from the monitor.<br>
 *        Manual timing parameters default to 720P for 9904 Boot ROM
 *        Some common LCD parameters defined in LcdParam.h header file.
 * @param *edid points to an EDID structure when it is available from the monitor.<br>
 *        EDID is a data structure to allow a display to pass its identity and display parameters to the host.<br>
 *        In CH703x, EDID is read from the monitor through DDC_SCL & DDC_SD pins (I2C) into a buffer
 *        which can be accessed by the host.
 * @param automatic is a boolean flag indicating if EDID is available. It defaults to false.<br>
 * @return  true if successful.<br>
 *          false if failed.<br>
 * @note  Example to use
 *        Ra8876_Lite ra8876lite;
 *        ra8876lite.begin();  //assume no EDID is required, manually setup RA8876 for default 720P HDTV resolution
 */

bool Ra8876_Lite::begin(const LCDParam *timing, MonitorInfo *edid, bool automatic)
{
    _initialised = false;
  
#ifdef DEBUG_LLD_RA8876
  printf("Running ra8876_Lite::begin(args)...\n");
#endif
	
    hal_bsp_init();
	//Hard reset RA8876
    hal_gpio_write(_xnreset, 1);
    hal_delayMs(1);
    hal_gpio_write(_xnreset, 0);
    hal_delayMs(1);
    hal_gpio_write(_xnreset, 1);
    hal_delayMs(10);
   
  if(!checkIcReady(10))
  {return _initialised;}
  
  //read ID code must disable pll, 01h bit7 set 0
  lcdRegDataWrite(0x01,0x00);
  hal_delayMs(1);

  if ((lcdRegDataRead(0xff) != 0x76)&&(lcdRegDataRead(0xff) != 0x77))
  {
    #ifdef DEBUG_LLD_RA8876
    printf("RA8876 not found!\n");
    #endif
    return _initialised;
  }
  else
  {
    #ifdef DEBUG_LLD_RA8876
    printf("RA8876 connect pass!\n");
    #endif
  }

  if(automatic) //EDID information from monitor
  {
    lcd = 
    {
      edid->dsc_product_name,
      (uint16_t)edid->detailed_timings[0].h_addr, (uint16_t)edid->detailed_timings[0].v_addr, //width, height
      (uint16_t)edid->detailed_timings[0].h_blank,      //horizontal blanking
      (uint16_t)edid->detailed_timings[0].h_front_porch,  //horizontal front porch
      (uint16_t)edid->detailed_timings[0].h_sync,     //horizontal pulse width
      (uint16_t)edid->detailed_timings[0].v_blank,      //vertical blanking
      (uint16_t)edid->detailed_timings[0].v_front_porch,  //vertical front porch
      (uint16_t)edid->detailed_timings[0].v_sync,       //vertical pulse width
      (uint32_t)edid->detailed_timings[0].pixel_clock/1000000UL,    //pixel clock
      !(edid->detailed_timings[0].digital.negative_vsync),  //vsync polarity
      !(edid->detailed_timings[0].digital.negative_hsync),  //hsync polarity
      0,      //rising edge pclk
      0,      //+ve de
    };   
  }
  else          //manual LCD timing parameters
  {
    lcd=
    {
      timing->name,
      timing->width, timing->height,
      timing->hblank,
      timing->hfporch,
      timing->hpulse,
      timing->vblank,
      timing->vfporch,
      timing->vpulse,
      timing->pclk,
      timing->vsyncPolarity,
      timing->hsyncPolarity,
      timing->pclkPolarity,
      timing->dePolarity,
    };    
  }
  
  if(!ra8876PllInitial(lcd.pclk))
  {
    #ifdef DEBUG_LLD_RA8876
    printf("PLL init failed!\n");
    #endif
    return _initialised;
  }
  
  if(!ra8876SdramInitial())
  {
    #ifdef DEBUG_LLD_RA8876
    printf("SDRAM init failed!\n");
    #endif    
    return _initialised; 
  }
 
  //REG[01h]
  lcdRegDataWrite(RA8876_CCR,RA8876_PLL_ENABLE<<7|RA8876_WAIT_NO_MASK<<6|RA8876_KEY_SCAN_DISABLE<<5|RA8876_TFT_OUTPUT24<<3
  |RA8876_I2C_MASTER_DISABLE<<2|RA8876_SERIAL_IF_ENABLE<<1|RA8876_HOST_DATA_BUS_SERIAL);

  //REG[02h]
  lcdRegDataWrite(RA8876_MACR,RA8876_DIRECT_WRITE<<6|RA8876_READ_MEMORY_LRTB<<4|RA8876_WRITE_MEMORY_LRTB<<1);

  //REG[03h]
  lcdRegDataWrite(RA8876_ICR,RA8876_GRAPHIC_MODE<<2|RA8876_MEMORY_SELECT_IMAGE);

  lcdRegDataWrite(RA8876_DPCR,(lcd.pclkPolarity<<7)|(RA8876_DISPLAY_OFF<<6)|(RA8876_OUTPUT_RGB)); //REG[12h]
  //edid->detailed_timings[0].digital.negative_vsync = 1 if polarity is negative
  //REG[13h] bit7 XHSYNC polarity 0: Low active, 1: High active
  //edid->detailed_timings[0].digital.negative_hsync = 1 if polarity is negative
  //REG[13h] bit6 XVSYNC polarity 0: Low active, 1: High active
  //All CEA-861-D video signals are DE high active
  lcdRegDataWrite(RA8876_PCSR, lcd.hsyncPolarity<<7|lcd.vsyncPolarity<<6|lcd.dePolarity<<5); //REG[13h]
  lcdHorizontalWidthVerticalHeight(lcd.width, lcd.height);
  lcdHorizontalBackPorch(lcd.hblank-lcd.hfporch-lcd.hpulse);
  lcdHsyncStartPosition(lcd.hfporch);
  lcdHsyncPulseWidth(lcd.hpulse);
  lcdVerticalBackPorch(lcd.vblank-lcd.vfporch-lcd.vpulse);
  lcdVsyncStartPosition(lcd.vfporch);
  lcdVsyncPulseWidth(lcd.vpulse);

  //REG[B9h]: enable XnSFCS1 (pin 38) as chip select for Serial Flash W25Q256FV and enter 4-Byte mode for it
  setSerialFlash();
  
  #ifdef DEBUG_LLD_RA8876
    printf("Name of LCD: %s\n", lcd.name);
    printf("LCD width: %d\n", lcd.width);
    printf("LCD height: %d\n", lcd.height);
    printf("H blanking: %d\n", lcd.hblank);
    printf("H front porch: %d\n", lcd.hfporch);
    printf("H pulse width: %d\n", lcd.hpulse);
    printf("V blanking: %d\n", lcd.vblank);
    printf("V front porch: %d\n", lcd.vfporch);
    printf("V pulse width: %d\n", lcd.vpulse);
    printf("Pixel clock: %d\n", lcd.pclk);
    printf("Pclk Polarity: %d\n", lcd.pclkPolarity);
    printf("Vsync Polarity: %d\n", lcd.vsyncPolarity);
    printf("Hsync Polarity: %d\n", lcd.hsyncPolarity);
    printf("DE Polarity: %d\n", lcd.dePolarity);
  #endif
  
	
	canvasImageWidth(lcd.width, lcd.height);
	activeWindowWH(lcd.width,lcd.height);
	
  _initialised = true;
  
  return _initialised;
}

/**
 * @brief   Register write function.
 * @param   reg is the register to write.
 */
void Ra8876_Lite::lcdRegWrite(uint8_t reg) 
{
  uint16_t _data = ((uint16_t)RA8876_SPI_CMDWRITE<<8 | reg);
  hal_spi_write16((uint16_t)_data);
}

/**
 * @brief   Data write function.
 * @param   data is the value to write in 8-bit width.
 */
void Ra8876_Lite::lcdDataWrite(uint8_t data) 
{
  uint16_t _data = ((uint16_t)RA8876_SPI_DATAWRITE<<8 | data);
  hal_spi_write16((uint16_t)_data);
}

/**
 * @brief   Data read function with register location predefined.
 * @return  data return
 */
uint8_t Ra8876_Lite::lcdDataRead(void) 
{
  uint8_t vret;
  uint16_t _data = ((uint16_t)RA8876_SPI_DATAREAD<<8 | 0xFF);
  vret = (uint8_t)hal_spi_write16((uint16_t)_data);

  return (vret);
}

/**
 * @brief   Status read function from the STATUS register.
 * @return  value returned from the STATUS register
 */
uint8_t Ra8876_Lite::lcdStatusRead(void) 
{
  uint8_t sret;
  uint16_t _data = ((uint16_t)RA8876_SPI_STATUSREAD<<8 | 0xFF);
  sret = (uint8_t)hal_spi_write16((uint16_t)_data);

  return (sret);
}

/**
 * @brief Write 16-bit data to RA8876.
 * @note  This is to support SPI interface to write 16bpp data after Regwrite 04h
 * @param data is the value to write, often used in pixel write in 5-6-5 color mode
 */
void Ra8876_Lite::lcdDataWrite16bpp(uint16_t data) 
{
  uint8_t buf[2];
  buf[0] = (uint8_t)data;
  buf[1] = (uint8_t)(data>>8);
  
  hal_spi_write((const uint8_t *)buf, 2);
}

 /**
  * @brief  Make function call to lcdRegDataWrite(), lcdRegDataRead(), & lcdStatusRead() from a serial terminal.
  * @param	*msg is a pointer to incoming message in ASCII characters.
  * @note   Prerequisite: printf(args) should be available<br>
  *			Example to use:<br>
  *         In Arduino IDE, open Serial Monitor and type in following commands and click <send><br>
  *         spi,w,0x12,0x60 //SPI write to RA8876 at register 0x12 a value of 0x60 to turn on color bar test.<br>
  *         spi,w,0x12,0x40 //SPI write to return to normal operation.<br>
  *         spi,r,0x12 		//SPI read from register 0x12.<br>
  *         spi,r,status	//SPI status read from the STATUS register. <br>
  */
 void Ra8876_Lite::parser(char *msg)
 {
  char *token=NULL;
  bool isWriteCommand = false;
  const char delimiters[] = ", ";
  uint8_t reg, val; 

  token = strtok(msg, delimiters);
  if(!strcmp(token,"SPI") || !strcmp(token, "spi")) 
  {
    token = strtok(NULL, delimiters);  //get r or w
    if(!strcmp(token,"w") || !strcmp(token, "W")){
      isWriteCommand=true;
    }
    else if (!strcmp(token,"r") || !strcmp(token, "R")){
      isWriteCommand=false;
    }
    else{
      printf("It is not a valid operation.\n");
      return;
    }
  }

  if(isWriteCommand)
  {
    token = strtok(NULL, delimiters); //get the register address to write
    reg = strtol(token, NULL, 16);    //convert char to hex

    token = strtok(NULL, delimiters); //get the value to write
    val = strtol(token, NULL, 16);

    lcdRegDataWrite(reg, val);
    printf("Write to RA8876@%2Xh, val=%2Xh OK.\n", reg, val);
  }
  else
  {
    token = strtok(NULL, delimiters); //get the next token to see if it is a "status"
    if(!strncmp(token,"status",6) || !strncmp(token,"STATUS",6))
    {
      val = lcdStatusRead();
      printf("Status read from RA8876=%2Xh OK.\n", val);
    }
    else
    {
      reg = strtol(token, NULL, 16); 
      val = lcdRegDataRead(reg);
      printf("Value read from RA8876@%2Xh, val=%2Xh\n", reg, val);
    }
  }
 }

/**
 * @brief Return color mode
 * @return COLOR_MODE enum<br>
	enum {  
	  COLOR_8BPP_RGB332=1,    	//8BPP in R[3]G[3]B[2], input data format=R[7:5]G[7:5]B[7:6]
	  COLOR_16BPP_RGB565=2,   	//16BPP in R[5]G[6]B[5], input data format=G[4:2]B[7:3], [7:3]G[7:5]
	  COLOR_24BPP_RGB888=3,   	//24BPP in B[7]G[7]R[7], input data format=B[7:0]G[7:0]R[7:0]
	  COLOR_6BPP_ARGB2222=4,	//Index display with opacity (aRGB 2:2:2:2)
	  COLOR_12BPP_ARGB4444=5  	//12BPP with ARGB of 4bits each, input data format=G[7:4]B[7:4], A[3:0]R[7:4]
	  } COLOR_MODE;
 */
COLOR_MODE Ra8876_Lite::getColorMode(void)
{
  return _colorMode;
}

/**
 * @brief 	Return color depth in bit-per-pixel (bpp)
 * @return	Color depth in bpp = 1 for RGB332/ARGB2222, bpp=2 for RGB565/ARGB4444, bpp=3 for RGB888, bpp=0 for error
 */
uint8_t Ra8876_Lite::getColorDepth(void)
{
	COLOR_MODE _mode = getColorMode();
	
	uint8_t bpp = 2;
	
	switch(_mode)
	{
		case COLOR_8BPP_RGB332:
		case COLOR_6BPP_ARGB2222:
			bpp = 1;
			break;
		case COLOR_16BPP_RGB565:
		case COLOR_12BPP_ARGB4444:
			bpp = 2;
			break;
		case COLOR_24BPP_RGB888:
			bpp = 3;
			break;
	}
	return bpp;
}

/**
 * @brief This function write 8-bit data to a register
 * @param reg is the register address to write
 * @param data is the data to write
 */
void Ra8876_Lite::lcdRegDataWrite(uint8_t reg,uint8_t data)
{
  lcdRegWrite(reg);
  lcdDataWrite(data);
}

/**
 * @brief This function read a 8-bit value from a register
 * @param reg is the register address to read from
 * @return content at the register
 */
uint8_t Ra8876_Lite::lcdRegDataRead(uint8_t reg)
{
  lcdRegWrite(reg);
  return (lcdDataRead());
}

/**
 * @brief Polling until memory write FIFO buffer is not full [Status Register] bit7
 */
void Ra8876_Lite::checkWriteFifoNotFull(void)
{
  uint16_t i;
  for(i=0;i<10000;i++)   //Please according to your usage to modify i value.
  {
    if( (lcdStatusRead()&RA8876_STSR_WR_FIFO_FULL)==0 ){break;}
  }
}

/**
 * @brief Polling for memory write FIFO buffer full [Status Register] bit7
 * @note  Only when Memory Write FIFO is not full, MPU may write another one pixel
 * @param timeout is the number of read status function to run before a timeout error
 * @return true Memory Write FIFO is not full<br>
 *         false Memory Write FIFO is full<br>
 */

bool Ra8876_Lite::checkWriteFifoNotFull(uint32_t timeout)
{  
  while(timeout--)
  {
    if( (lcdStatusRead()&RA8876_STSR_WR_FIFO_FULL)==0 )
    return true;
  }

  #ifdef DEBUG_LLD_RA8876
    printf("checkWriteFifoNotFull() timeout error, Write FIFO is full!\n");
  #endif
  
  return false;
}


/**
 * @brief This function polls until memory write FIFO is empty
 */
void Ra8876_Lite::checkWriteFifoEmpty(void)
{
  uint16_t i;
  for(i=0;i<10000;i++)
  {
    if( (lcdStatusRead()&RA8876_STSR_WR_FIFO_EMPTY)==RA8876_STSR_WR_FIFO_EMPTY ){break;}
  }
}

/**
 * @brief This function polls for Memory Write FIFO empty [Status Register] bit6
 * @param timeout measured in number of status read cycle to run 
 * @return false Memory Write FIFO is not empty.
 *         true Memory Write FIFO is empty.
 */
bool Ra8876_Lite::checkWriteFifoEmpty(uint32_t timeout)
{
  while(timeout--)
  {
    if( (lcdStatusRead()&RA8876_STSR_WR_FIFO_EMPTY)==RA8876_STSR_WR_FIFO_EMPTY )
    return true;
  }
  #ifdef DEBUG_LLD_RA8876
    printf("checkWriteFifoEmpty() timeout error, Memory Write FIFO is not empty!\n");
  #endif  

  return false;
}

/**
 * @brief This function polls until Memory Read FIFO not full
 * @note  [Status Register] bit5
 *        [Status Register] bit5 =0: Memory Read FIFO is not full<br>
 *        [Status Register] bit5 =1: Memory Read FIFO is full<br>
 */
void Ra8876_Lite::checkReadFifoNotFull(void)
{
  uint16_t i;
  for(i=0;i<10000;i++)  //Please according to your usage to modify i value.
  {if( (lcdStatusRead()&RA8876_STSR_RD_FIFO_FULL)==0x00 ){break;}}
}

/**
 * @brief This function polls until Memory Read FIFO not empty
 * @note  [Status Register] bit4
 *        [Status Register] bit4=0: Memory Read FIFO is not empty<br>
 *        [Status Register] bit4=1: Memory Read FIFO is empty.
 */
void Ra8876_Lite::checkReadFifoNotEmpty(void)
{
  uint16_t i;
  for(i=0;i<10000;i++)// //Please according to your usage to modify i value. 
  {if( (lcdStatusRead()&RA8876_STSR_RD_FIFO_EMPTY)==0x00 ){break;}}
}


/**
 * @brief Polling for core task status until it is not busy [Status Register] bit3
 * @note  Tasks incl. BTE, Geometry engine, Serial flash DMA, Text write or Graphic write
 */
void Ra8876_Lite::check2dBusy(void)
{
  uint32_t i;
  for(i=0;i<1000000;i++)   //Please according to your usage to modify i value.
  {
    if( (lcdStatusRead()&RA8876_STSR_CORE_BUSY)==0x00 )
    {break;}
  }
}

/**
 * @brief Polling for core task status [Status Register] bit3
 * @note  Tasks incl. BTE, Geometry engine, Serial flash DMA, Text write or Graphic write
 * @param timeout measured in number of status read cycle to run
 * @return  true if core is still busy with a timeout error 
 *          false if core is not busy
 */
bool Ra8876_Lite::check2dBusy(uint32_t timeout)
{
  while(timeout--)
  {
    if((lcdStatusRead()&RA8876_STSR_CORE_BUSY)==0x00)
    return false;
  }
  
  #ifdef DEBUG_LLD_RA8876
    printf("check2dBusy() timeout error, core is still busy!\n");
  #endif

  return true;
}

/**
* @brief  Polling for [Status Register] bit2 for SDRAM status
* @param  timeout in number of status read cycle
* @return true if SDRAM is ready for access<br>
*         false if SDRAM is not ready for access<br>
*/
bool Ra8876_Lite::checkSdramReady(uint32_t timeout)
{
  while(timeout--)
  { 
   if((lcdStatusRead()&RA8876_STSR_SDRAM_READY)==RA8876_STSR_SDRAM_READY)
    return true;
  }
  
  #ifdef DEBUG_LLD_RA8876
    printf("checkSdramReady() timeout error!\n");
  #endif
  
  return false;
}

/**
* @brief  Polling for RA8876 operation status [Status Register]:bit1
* @note   Inhibit operation state means internal reset event keep running or
*         initial display still running or chip enter power saving state.
* @param  timeout in number of status read cycle
* @return true: Normal operation state<br>
*         false: Inhibit operation state<br>
*/
bool Ra8876_Lite::checkIcReady(uint32_t timeout)
{
  while(timeout--)
  {
     if( (lcdStatusRead()&RA8876_STSR_OP_MODE_INHIBIT)==0x00 )
     {return true;}     
  }
   
  #ifdef DEBUG_LLD_RA8876
      printf("checkIcReady() timeout error!\n");
  #endif
  
  return false;
}

/**
* @brief  PLL initialization function.
* @note   PLL output is calculated from this formula<br>
*         PLL = OSC_FREQ*(PLLDIVN+1)/2^PLLDIVM, with conditions 100MHz<=PLL<=600MHz with 10MHz<=OSC_FREQ/2^PLLDIVM<=40MHz<br>
*         Clock generated (e.g. Pclk) = PLL/2^PLLDIVK<br>
*         OSC_FREQ is the crystal frequency onboard (12MHz)<br>
*         PLLDIVM = 0 ~ 1<br>
*         PLLDIVK = 0 ~ 3<br>
*         PLLDIVN = 1 ~ 63<br>
*         Example, we want to output a pixel clock of 74250000Hz,<br>
*         set PLLDIVM = 0, PLLDIVN=49, PLL = 12*(49+1)/(2^0) = 600<br>
*         set PLLDIVK = 3, PCLK = 600/2^3 = 75MHz (~74.25MHz)<br>
* @param  pclk is the pixel clock with unit in MHz<br>
* @return true if successful<br>
*         false if not successful. Reason can be a too high pclk frequency<br>
* 
*/
bool Ra8876_Lite::ra8876PllInitial(uint16_t pclk)
{
  #ifdef DEBUG_LLD_RA8876
    printf("Ra8876_Lite::ra8876PllInitial(pclk)...\n");
  #endif
  
  uint8_t xPLLDIVN;
  uint8_t xPLLDIVK;
  //disable PLL prior to parameter changes
  uint8_t CCR = lcdRegDataRead(RA8876_CCR);

  lcdRegDataWrite(RA8876_CCR, CCR&0x7F);  //PLL_EN @ bit[7]

  if ((pclk>54)&&(pclk<=SPLL_FREQ_MAX))
  {
    //e.g.  VID code 35,36  2880x480 @59.940Hz pclk=108MHz  
    //xPLLDIVM=0, xPLLDIVK=2
    xPLLDIVK = 2;
    //to round off to nearest 0.5, pclk*4/OSC_FREQ-1+0.5 become pclk*4/OSC_FREQ-0.5.
    //this round 48.5 to 49 for a higher frame rate.
    //xPLLDIVN = round((float)(pclk*4/OSC_FREQ)-0.5);
    xPLLDIVN = (pclk*4/OSC_FREQ)-1;
  }
  else if ((pclk<=54) || (pclk>SPLL_FREQ_MAX))  //pixel clock > 148MHz is not supported. Bound any frequency > 148 to 74MHz
  {
    //e.g.  VID code 4, 1280x720 @60Hz pclk=74.250MHz (round off to 74MHz)
    //      VID code 31, 1920x1080 @50Hz pclk=148.5MHz (round off to 148MHz)
    //xPLLDIVM=0, xPLLDIVK=3
    if(pclk>SPLL_FREQ_MAX) {pclk=74;} //bound any pixel clock higher than 108MHz to 74MHz for low field rate 
    xPLLDIVK = 3;
    //xPLLDIVN = round((float)(pclk*8/OSC_FREQ)-0.5);
    xPLLDIVN = (pclk*8/OSC_FREQ)-1;
  }
/*
  #ifdef DEBUG_LLD_RA8876
    printf("Setting pixel clock with xPLLDIVN=%d, xPLLDIVK=%d\n", xPLLDIVN, xPLLDIVK);
  #endif
*/  
  if((xPLLDIVN>63)||(xPLLDIVN==0))
  {
  #ifdef DEBUG_LLD_RA8876
    printf("xPLLDIVN out of range. Check video source.\n");
  #endif
    return false;
  }
  lcdRegDataWrite(0x05, xPLLDIVK<<1); //set SCLK PLL control register 1
  lcdRegDataWrite(0x06, xPLLDIVN);    //set SCLK PLL control register 2 
  
  lcdRegDataWrite(0x07,0x02);     //PLL Divided by 2
  lcdRegDataWrite(0x08,(DRAM_FREQ*2/OSC_FREQ)-1); //DRAM_FREQ*2/OSC_FREQ=166*2/12=27
  
  lcdRegDataWrite(0x09,0x02);       //PLL Divided by 2
  lcdRegDataWrite(0x0A,(CORE_FREQ*2/OSC_FREQ)-1); //CORE_FREQ*2/OSC_FREQ=144*2/12=24
  
  lcdRegDataWrite(RA8876_CCR, RA8876_PLL_ENABLE<<7);  //enable PLL
  hal_delayMs(20);  //wait PLL stable
   
  if((lcdRegDataRead(RA8876_CCR)&0x80)==0x80)
  {
#ifdef DEBUG_LLD_RA8876
    printf("PLL initialization successful\n");
#endif
    return true;
  }
  else
  {
#ifdef DEBUG_LLD_RA8876
    printf("PLL initialization failed\n");
#endif
    return false;  
  }
}

/**
 * @brief SDRAM initialization assuming an external SDRAM WINBOND W9825G6KH-6
 * @return true if SDRAM initialization is successful<br>
 *         false if SDRAM initialization failed<br>
 * 
 */
bool Ra8876_Lite::ra8876SdramInitial(void)
{
uint8_t	  	CAS_Latency;
uint16_t	Auto_Refresh;

  CAS_Latency=3;
  Auto_Refresh=(64*DRAM_FREQ*1000L)/(4096L);
  lcdRegDataWrite(0xe0,0x31);      
  lcdRegDataWrite(0xe1,CAS_Latency);      //CAS:2=0x02锛孋AS:3=0x03
  lcdRegDataWrite(0xe2,Auto_Refresh);
  lcdRegDataWrite(0xe3,Auto_Refresh>>8);
  lcdRegDataWrite(0xe4,0x01);

  return (checkSdramReady(10));
}

/**
 * @brief This function switch LCD ON or OFF
 * @param true (1) to turn ON<br>
 *        false (0) to turn OFF<br>
 */
 void Ra8876_Lite::displayOn(bool on)
 {
    uint8_t DPCR = lcdRegDataRead(RA8876_DPCR); //read REG[12h]
    
    if(on)
      DPCR|=(RA8876_DISPLAY_ON<<6);
    else
      DPCR&=~(RA8876_DISPLAY_ON<<6);
    
    lcdRegDataWrite(RA8876_DPCR, DPCR);
    hal_delayMs(20);
 }

/**
 * @brief	Interrupt handler routine, should be called by isr() function in main
 * @note	Example in Arduino: <br>
 *			void isr(void)
			{
				ra8876lite.irqEventHandler();
			}
			void setup()
			{
				pinMode(RA8876_VSYNC, INPUT_PULLUP);
				attachInterrupt(digitalPinToInterrupt(RA8876_VSYNC), isr, FALLING);
			}
			
 */
void Ra8876_Lite::irqEventHandler(void)
{
	_irqEventTrigger = true;
}

/**
 * @brief	Function to enable/disable interrupt event(s)
 * @param	event is the argument to write to Interrupt Enable Register REG[0Bh].<br>
 *			Choose from constants under RA8876_INTEN in Ra8876_Registers.h as below<br>
 * 			RA8876_WAKEUP_IRQ_ENABLE<br>
 *			RA8876_XPS0_IRQ_ENABLE<br>
 *			RA8876_IIC_IRQ_ENABLE<br>
 *			RA8876_VSYNC_IRQ_ENABLE<br>
 *			RA8876_KEYSCAN_IRQ_ENABLE<br>
 *			RA8876_CORETASK_IRQ_ENABLE<br>
 *			RA8876_PWM1_IRQ_ENABLE<br>
 *			RA8876_PWM0_IRQ_ENABLE<br>
 *
 * @param	en is an enable or disable argument. Write '1' to enable, '0' to disable
 * @note	Example: <br>
 *			//enable Vsync interrupt
 *			ra8876lite.irqEventSet(RA8876_VSYNC_IRQ_ENABLE, 1);	
 *			//disable Vsync interrupt
 *			ra8876lite.irqEventSet(RA8876_VSYNC_IRQ_ENABLE, 0);	
 *			//enable BTE and VSYNC interrupts
 *			ra8876lite.irqEventSet(RA8876_VSYNC_IRQ_ENABLE|RA8876_CORETASK_IRQ_ENABLE, 1);
 */
void Ra8876_Lite::irqEventSet(uint8_t event, bool en)
{
	uint8_t INTEN = lcdRegDataRead(RA8876_INTEN);
	
	if(en)
	{
		lcdRegDataWrite(RA8876_INTEN, INTEN|event);
	}
	else
	{
		lcdRegDataWrite(RA8876_INTEN, INTEN&~event);
	}
}

/**
 * @brief	Read Interrupt Event Flag Register REG[0Ch] to query the source(s) of interrupt.<br>
 *			This function also resets XnINTR pin interrupt to false.
 * @return	non-zero if there is more than one interrupt source triggered.
 *			zero if there is no interrupt event.
 */
uint8_t Ra8876_Lite::irqEventQuery(void)
{
	if(_irqEventTrigger)
	{ 
		uint8_t INTF = lcdRegDataRead(RA8876_INTF);
		hal_di();
		_irqEventTrigger = false;	//reset XnINTR pin interrupt for the next event
		hal_ei();					//enable interrupt for next event(s)		
		return (INTF);
	}	

	return 0;
}

/**
 * @brief	Reset Interrupt Event Flag for next event.
 * @param	event is the argument to write to REG[0Ch]. <br>
 *			Select from list below : <br>
 * 			RA8876_WAKEUP_EVENT<br>
 *			RA8876_XPS0_EVENT<br>
 *			RA8876_IIC_EVENT<br>
 *			RA8876_VSYNC_EVENT<br>
 *			RA8876_KEYSCAN_EVENT<br>
 *			RA8876_CORETASK_EVENT<br>
 *			RA8876_PWM1_EVENT<br>
 *			RA8876_PWM0_EVENT<br>
 * @note	Example: <br>
 * 			//Reset Vsync interrupt flag for next vsync event
 *			ra8876lite.irqEventFlagReset(RA8876_VSYNC_EVENT);
 */
void Ra8876_Lite::irqEventFlagReset(uint8_t event)
{
	lcdRegDataWrite(RA8876_INTF, event);	//reset individual event flag of INTF register REG[0Ch]
}

/**
 * @brief	Wait for vsync to avoid screen flicker.<br>
 *			This is a blocking function for a max. time of 50 msec (VSYNC_TIMEOUT_MS:constant defined in Ra8876_Lite.h ).<br>
 *			A max. wait time of 50msec is equivalent to a wait time for vsync of 20fps video.
 * @note	Example:<br>
 *			irqEventSet(RA8876_VSYNC_IRQ_ENABLE, 1);
 *			ra8876lite.vsyncWait();
 *			blit(source, dest,0,0,0,0,800,480);
 *			irqEventSet(RA8876_VSYNC_IRQ_ENABLE, 0);
 */
void Ra8876_Lite::vsyncWait(void)
{
	uint16_t timeout = VSYNC_TIMEOUT_MS;
	//irqEventSet(RA8876_VSYNC_IRQ_ENABLE, 1);
	while((irqEventQuery()&RA8876_VSYNC_EVENT) != RA8876_VSYNC_EVENT)
	{
		hal_delayMs(1);
		timeout--;
		if(timeout==0) break;
	}
	
	if(timeout>0)	//valid VSYNC_EVENT before timeout
	{
	ra8876lite.irqEventFlagReset(RA8876_VSYNC_EVENT);
	//irqEventSet(RA8876_VSYNC_IRQ_ENABLE, 0);		
	}
	else
	{
	//printf("Timer expired before a vsync can be detected.\n");
	}
}

/**
 * @brief Set physical size of the LCD in width and height
 * @param width is the width in pixels
 * @param height is the height in lines
 * @note  A Frame Scaler is available in CH703X HDMI encoder to boost 1280x720@60Hz to 1080P@60Hz.
 * As a result, the width and height parameters passed to this function is not the final resolution 
 * we are watching on the HDTV or LCD monitor. Instead, it is the original RGB signal (default 720P @60Hz) 
 * generated by RA8876.
 */
void Ra8876_Lite::lcdHorizontalWidthVerticalHeight(uint16_t width, uint16_t height)
{
   unsigned char temp;
   temp=(width/8)-1;
   lcdRegDataWrite(RA8876_HDWR,temp);
   temp=width%8;
   lcdRegDataWrite(RA8876_HDWFTR,temp);
   temp=height-1;
   lcdRegDataWrite(RA8876_VDHR0,temp);
   temp=(height-1)>>8;
   lcdRegDataWrite(RA8876_VDHR1,temp);
}

/**
 * @brief Set LCD horizontal back porch
 * @param numbers is the back porch in pixels
 */
void Ra8876_Lite::lcdHorizontalBackPorch(uint16_t numbers)
{
 uint8_t temp;
 if(numbers<8)
  {
   lcdRegDataWrite(RA8876_HNDR,0x00);
   lcdRegDataWrite(RA8876_HNDFTR,numbers);
  }
 else
  {
  temp=(numbers/8)-1;
  lcdRegDataWrite(RA8876_HNDR,temp);
  temp=numbers%8;
  lcdRegDataWrite(RA8876_HNDFTR,temp);
  }	
}

/**
 * @brief Set LCD horizontal front porch
 * @param numbers is the front porch in pixels
 */
void Ra8876_Lite::lcdHsyncStartPosition(uint16_t numbers)
{uint8_t temp;
 if(numbers<8)
  {
   lcdRegDataWrite(RA8876_HSTR,0x00);
  }
  else
  {
   temp=(numbers/8)-1;
   lcdRegDataWrite(RA8876_HSTR,temp);
  }
}

/**
 * @brief Set LCD horizontal pulse width
 * @param numbers is the pulse width in pixels
 */
void Ra8876_Lite::lcdHsyncPulseWidth(uint16_t numbers)
{uint8_t temp;
 if(numbers<8)
  {
   lcdRegDataWrite(RA8876_HPWR,0x00);
  }
  else
  {
   temp=(numbers/8)-1;
   lcdRegDataWrite(RA8876_HPWR,temp);
  }
}

/**
 * @brief Set LCD vertical back porch
 * @param numbers is the vertical porch in lines
 */
void Ra8876_Lite::lcdVerticalBackPorch(uint16_t numbers)
{uint8_t temp;
  temp=numbers-1;
  lcdRegDataWrite(RA8876_VNDR0,temp);
  lcdRegDataWrite(RA8876_VNDR1,temp>>8);
}

/**
 * @brief Set LCD vertical front porch
 * @param numbers is the front porch in lines
 */
void Ra8876_Lite::lcdVsyncStartPosition(uint16_t numbers)
{uint8_t temp;
  temp=numbers-1;
  lcdRegDataWrite(RA8876_VSTR,temp);
}

/**
 * @brief Set LCD vertical pulse width
 * @param numbers is the pulse width in lines
 */
void Ra8876_Lite::lcdVsyncPulseWidth(uint16_t numbers)
{uint8_t temp;
  temp=numbers-1;
  lcdRegDataWrite(RA8876_VPWR,temp);
}

/**
 * @brief 	Set up Display Window (Main Window or better say... this is the LCD screen you are watching).
 * @note  	The width of Display Window is set to the same width of Canvas.
 * @param 	x0 is the x-coordinate at upper-left corner (default MAIN_WINDOW_STARTX).
 * @param 	y0 is the offset y-coordinate at upper-left corner (default MAIN_WINDOW_STARTY).
 * @param 	offset is the SDRAM starting address in bytes.
 * @note  	If color mode in 16BPP, offset = pixel count *2, etc.<br>
 *			Need to call canvasImageBuffer() before calling this function; otherwise, _canvasWidth is using default
 *			lcd.width fetched from a monitor in Ra8876_Lite::begin().
 */
void Ra8876_Lite::displayMainWindow(uint16_t x0, uint16_t y0, uint32_t offset)
{
  displayImageStartAddress(offset);	//20h-23h
  displayImageWidth(_canvasWidth);	//24h-25h
  displayWindowStartXY(x0,y0);		//26h-29h
}

/**
 * @brief 	Set Display Window (also known as the Main Window) starting address.
 * @param 	addr is the starting address. It must be a multiple of 4.
 * @note  	If color mode in 16BPP, offset = pixel count *2.
 */
void Ra8876_Lite::displayImageStartAddress(uint32_t addr)	
{
  lcdRegDataWrite(RA8876_MISA0,addr);//20h
  lcdRegDataWrite(RA8876_MISA1,addr>>8);//21h 
  lcdRegDataWrite(RA8876_MISA2,addr>>16);//22h  
  lcdRegDataWrite(RA8876_MISA3,addr>>24);//23h 
}

/**
 * @brief Set Display Window width.
 * @param width in pixels in multiple of 4.
 * @note  The width of Display Window is always set to the same width of canvas.
 */
void Ra8876_Lite::displayImageWidth(uint16_t width)	
{
  lcdRegDataWrite(RA8876_MIW0,width); //24h
  lcdRegDataWrite(RA8876_MIW1,width>>8); //25h 
}

/**
 * @brief Set upper left of the Display Window relative to Canvas Window
 * @param x0 is the upper left x-coordinate in multiple of 4
 * @param y0 is the upper left y-coordinate in multiple of 4
 */
void Ra8876_Lite::displayWindowStartXY(uint16_t x0,uint16_t y0)	
{
  lcdRegDataWrite(RA8876_MWULX0,x0);//26h
  lcdRegDataWrite(RA8876_MWULX1,x0>>8);//27h
  lcdRegDataWrite(RA8876_MWULY0,y0);//28h
  lcdRegDataWrite(RA8876_MWULY1,y0>>8);//29h
}

/**
 * @brief	Set Canvas addressing to Linear mode
 */
void Ra8876_Lite::canvasLinearModeSet(void)
{
	uint8_t AW_COLOR = lcdRegDataRead(RA8876_AW_COLOR); //read REG[5Eh]	
	AW_COLOR|=(RA8876_CANVAS_LINEAR_MODE<<2);
	
	lcdRegDataWrite(RA8876_AW_COLOR, AW_COLOR);	//set to linear mode
	
}

/**
 * @brief	Set Canvas addressing to Block mode
 */
void Ra8876_Lite::canvasBlockModeSet(void)
{
	uint8_t AW_COLOR = lcdRegDataRead(RA8876_AW_COLOR); //read REG[5Eh]	
	AW_COLOR&=~(RA8876_CANVAS_LINEAR_MODE<<2);
	
	lcdRegDataWrite(RA8876_AW_COLOR, AW_COLOR);	//set to block mode
}
  
/**
 * @brief This function allocates a memory space as the Canvas Window in SDRAM
 * @param width in pixels in 4 pixels resolution.
 * @param height in line numbers
 * @param x0 the Active Window start position in x-direction (default ACTIVE_WINDOW_STARTX)
 * @param y0 the Active Window start position in y-direction (default ACTIVE_WINDOW_STARTY)
 * @param mode is the color mode from enum COLOR_MODE<br>
	COLOR_8BPP_RGB332=1<br>
	COLOR_16BPP_RGB565=2<br>
	COLOR_24BPP_RGB888=3<br>
	COLOR_6BPP_ARGB2222=4<br>
	COLOR_12BPP_ARGB4444=5<br>

 * @param offset is the physical address in SDRAM in bytes.<br>
 * @note  If color mode=COLOR_16BPP_RGB565, offset value at pixel n = n*2bytes
 */
void Ra8876_Lite::canvasImageBuffer(uint16_t width, uint16_t height, uint16_t x0, uint16_t y0, COLOR_MODE mode, uint32_t offset)
{
  _colorMode = mode;
  
  uint8_t _canvasMode = RA8876_CANVAS_BLOCK_MODE;
  
  //REG[10h], REG[11h], REG[5Eh], REG[92h]
  if(_colorMode==COLOR_8BPP_RGB332){
    lcdRegDataWrite(RA8876_MPWCTR,RA8876_PIP1_WINDOW_DISABLE<<7|RA8876_PIP2_WINDOW_DISABLE<<6|
    RA8876_SELECT_CONFIG_PIP1<<4|RA8876_IMAGE_COLOR_DEPTH_8BPP<<2|RA8876_PANEL_SYNC_MODE); //REG[10h]
    lcdRegDataWrite(RA8876_PIPCDEP,RA8876_PIP1_COLOR_DEPTH_8BPP<<2|RA8876_PIP2_COLOR_DEPTH_8BPP); //REG[11h]
    lcdRegDataWrite(RA8876_AW_COLOR,_canvasMode<<2|RA8876_CANVAS_COLOR_DEPTH_8BPP); //REG[5Eh]
    lcdRegDataWrite(RA8876_BTE_COLR,RA8876_S0_COLOR_DEPTH_8BPP<<5|RA8876_S1_COLOR_DEPTH_8BPP<<2|RA8876_DESTINATION_COLOR_DEPTH_8BPP);//REG[92h]
  }
  else if (_colorMode==COLOR_24BPP_RGB888){
    lcdRegDataWrite(RA8876_MPWCTR,RA8876_PIP1_WINDOW_DISABLE<<7|RA8876_PIP2_WINDOW_DISABLE<<6|
    RA8876_SELECT_CONFIG_PIP1<<4|RA8876_IMAGE_COLOR_DEPTH_24BPP<<2|RA8876_PANEL_SYNC_MODE); //REG[10h]
    lcdRegDataWrite(RA8876_PIPCDEP,RA8876_PIP1_COLOR_DEPTH_24BPP<<2|RA8876_PIP2_COLOR_DEPTH_24BPP); //REG[11h]
    lcdRegDataWrite(RA8876_AW_COLOR,_canvasMode<<2|RA8876_CANVAS_COLOR_DEPTH_24BPP); //REG[5Eh]
    lcdRegDataWrite(RA8876_BTE_COLR,RA8876_S0_COLOR_DEPTH_24BPP<<5|RA8876_S1_COLOR_DEPTH_24BPP<<2|RA8876_DESTINATION_COLOR_DEPTH_24BPP);//REG[92h]
  }
  else{
    lcdRegDataWrite(RA8876_MPWCTR,RA8876_PIP1_WINDOW_DISABLE<<7|RA8876_PIP2_WINDOW_DISABLE<<6|
    RA8876_SELECT_CONFIG_PIP1<<4|RA8876_IMAGE_COLOR_DEPTH_16BPP<<2|RA8876_PANEL_SYNC_MODE);  //REG[10h]
    lcdRegDataWrite(RA8876_PIPCDEP,RA8876_PIP1_COLOR_DEPTH_16BPP<<2|RA8876_PIP2_COLOR_DEPTH_16BPP); //REG[11h]
    lcdRegDataWrite(RA8876_AW_COLOR,_canvasMode<<2|RA8876_CANVAS_COLOR_DEPTH_16BPP); //REG[5Eh]
    lcdRegDataWrite(RA8876_BTE_COLR,RA8876_S0_COLOR_DEPTH_16BPP<<5|RA8876_S1_COLOR_DEPTH_16BPP<<2|RA8876_DESTINATION_COLOR_DEPTH_16BPP);//REG[92h]
  }   
   
    canvasImageStartAddress(offset);
    if(width!=lcd.width || height!=lcd.height) {
		canvasImageWidth(width, height);
		activeWindowWH(width,height);
	}
    activeWindowXY(x0,y0);  
}

/**
 * @brief Clear the canvas area with BTE solid fill
 * @param color is an object composed of 4 components: red, green, blue, alpha 
 * @param x0 is the starting x-coordinate (default ACTIVE_WINDOW_STARTX)
 * @param y0 is the starting y-coordinate (default ACTIVE_WINDOW_STARTY)
 * @param offset is the SDRAM address in bytes (default CANVAS_OFFSET)
 */
void Ra8876_Lite::canvasClear(Color color, uint16_t x0, uint16_t y0, uint32_t lnOffset)
{
  if((x0>_canvasWidth) || (y0>_canvasHeight)) return;
  
  int32_t _canvasAddress = canvasAddress_from_lnOffset(lnOffset);
  if(_canvasAddress<0) return;
  
  bteSolidFill (_canvasAddress,x0,y0,_canvasWidth,_canvasHeight, color);
}

/**
 * @brief	Write data to SDRAM.
 * @param	*data is a void pointer to the data source.
 * @param	lnOffset is the line start address, unit in line number. Data always start from x=0.	
 * @param	byte_count is the element count in byte. Therefore a pixel depth in 16BPP needs a multiply by 2.
 * @note	Example to use: <br>
			//_colorMode=COLOR_8BPP_RGB332, write a line of 8 pixels at vertical position 300 starting from x=0
			const uint8_t pixels_8bit[8] = {0xAB, 0xCD, 0xEF, 0x01, 0x02, 0x03, 0x04, 0x05};
			ra8876lite.canvasWrite(pixels_8bit,  300, 8);
			//Output : ABh CDh EFh 1h 2h 3h 4h 5h 
			
			//_colorMode=COLOR_16BPP_RGB565, write a line of 8 pixels at vertical position 300 starting from x=0
			const uint16_t pixels_16bit[8] = {0xABCD, 0xCDCD, 0xEFCD, 0x01CD, 0x02CD, 0x03CD, 0x04CD, 0x05CD};
			ra8876lite.canvasWrite(pixels_16bit, 300, sizeof(pixels_16bit));	//byte_count = 16 not 8
			//Output : CDh ABh CDh CDh CDh EFh CDh 1h CDh 2h CDh 3h CDh 4h CDh 5h 
			
			//_colorMode=COLOR_24BPP_RGB888, write 2 pixels starting from x=0 at vertical position 300.
			//An incorrect demonstration:
			const uint32_t pixels_32bit[2] = {0x00CDEF23, 0x00034567};
			ra8876lite.canvasWrite(pixels_32bit, 300, 6);
			//Output : 23h EFh CDh 0h 67h 45h . This is not right because there is no 24bit data type in stdint.h.
			
			//_colorMode=COLOR_24BPP_RGB888, write 2 pixels starting from x=0 at vertical position 300.
			//A correct demontration to write 2 pixels of 24BPP:
			const uint8_t pixels_24bit[6] = {0x23, 0xEF, 0xCD, 0x67, 0x45, 0x03};
			ra8876lite.canvasWrite(pixels_24bit, 300, 6);
			//Output : 23h EFh CDh 67h 45h 3h
			
 */
void Ra8876_Lite::canvasWrite(const void *data, uint32_t lnOffset, size_t byte_count)
{
  if(!byte_count) return;
	
  uint8_t bpp = getColorDepth();

/** 
	//no longer required here with setPixelCursor(0,0,lnOffset);
	int32_t _canvasAddress = canvasAddress_from_lnOffset(lnOffset);
	if(_canvasAddress<0) return;
  
	canvasImageStartAddress(_canvasAddress);
*/  
	activeWindowXY(0,0);
  
	if(byte_count%_canvasWidth){
		activeWindowWH(_canvasWidth,(byte_count/_canvasWidth/bpp) + 1);
	}else{
		activeWindowWH(_canvasWidth,byte_count/_canvasWidth/bpp);
	}
	setPixelCursor(0,0,lnOffset);
  
	ramAccessPrepare();
  
	hal_spi_write((const uint8_t *)data, byte_count);

	//Main window restore
	activeWindowXY(ACTIVE_WINDOW_STARTX,ACTIVE_WINDOW_STARTY);
	activeWindowWH(_canvasWidth,_canvasHeight);
	canvasImageStartAddress(CANVAS_OFFSET);
}

#if defined (LOAD_SD_LIBRARY)
/**
 * @brief	Write an image in binary format to SDRAM with image width transformed to full canvas width.
 * @note	There is no width and height information from bin file generated by RAiO tool solid so<br>
 *			we need them as argument.
 */
void Ra8876_Lite::canvasWrite(uint16_t width, uint16_t height, const char *pFilename, uint32_t lnOffset)
{
	File gfxFile = SD.open(pFilename);
	
	if(!gfxFile) {return;}
	/*
	//no longer required here with putPicture_set_frame(..., lnOffset);
    int32_t _canvasAddress = canvasAddress_from_lnOffset(lnOffset);
	if(_canvasAddress<0) return;
  
	canvasImageStartAddress(_canvasAddress);
	*/
	uint16_t _h = (((uint32_t)width*height)+_canvasWidth)/_canvasWidth;
	putPicture_set_frame(0,0,_canvasWidth,_h, lnOffset);
	
	uint16_t _ln = _canvasWidth*getColorDepth();
	uint8_t lnBuffer[_ln];

	while(gfxFile.available())
	{
		gfxFile.read((uint8_t *)lnBuffer, _ln);
		hal_spi_write(lnBuffer, _ln);
	}
	
	/*
	//This is less efficient
	uint8_t bpp = getColorDepth();
	switch(bpp)
	{
		case 1:
			while(gfxFile.available()){
			lcdDataWrite((uint8_t)gfxFile.read());	}
			break;
		case 2:
			while(gfxFile.available()){
			lcdDataWrite((uint8_t)gfxFile.read());lcdDataWrite((uint8_t)gfxFile.read()); }
			break;
		case 3:
			while(gfxFile.available()){
			lcdDataWrite((uint8_t)gfxFile.read());lcdDataWrite((uint8_t)gfxFile.read());lcdDataWrite((uint8_t)gfxFile.read());}
			break;
	}	
	*/
	gfxFile.close();
	//Main window restore
	activeWindowXY(ACTIVE_WINDOW_STARTX,ACTIVE_WINDOW_STARTY);
	activeWindowWH(_canvasWidth,_canvasHeight);
	canvasImageStartAddress(CANVAS_OFFSET);
}
#endif

/**
 * @brief	Read data from SDRAM.
 * @param	*data is a void pointer to the data buffer.
 * @param	lnOffset is the line start address, unit in line number. Data always start from x=0.	
 * @param	data_count is the number of pixels to read.
 * @note	Example to use: <br>
			if(ra8876lite.getColorMode()==COLOR_8BPP_RGB332)
			  {
				uint8_t read_buffer[384];
				ra8876lite.canvasRead(read_buffer, 300, sizeof(read_buffer));
				for(uint16_t i=0; i<sizeof(read_buffer); i++)
				printf("buffer index = %d : value = read_buffer[%Xh]\n", i, read_buffer[i]);            
			  }			
 */
void Ra8876_Lite::canvasRead (void *data, uint32_t lnOffset, size_t data_count)
{		
  if(!data_count) return;
	 
  activeWindowXY(0,0);
  
  if(data_count%_canvasWidth){
    activeWindowWH(_canvasWidth,(data_count/_canvasWidth) + 1);
  }else{
    activeWindowWH(_canvasWidth,(data_count/_canvasWidth));
  }
  setPixelCursor(0,0,lnOffset);
  
	ramAccessPrepare();
	lcdDataRead();	//dummy read is required somehow
	if(_colorMode == COLOR_8BPP_RGB332)
	{	
		uint8_t *pdata8_t = (uint8_t *)data;
		while(data_count--) 
		{
			//checkReadFifoNotEmpty();
			*pdata8_t++ = lcdDataRead();
		}
		/**
		 * @note	Ra8876_Lite::canvasRead (void  *data, uint32_t lnOffset, uint32_t data_count)<br>
		 *			This is more stable for fast SPI to use lcdDataRead() in constrast with hal_spi_read(*data, uint32_t)
		 */
	}  
	else if (_colorMode == COLOR_16BPP_RGB565)
	{
		uint16_t *pdata16_t = (uint16_t *)data;
		while(data_count--)
		{
			uint8_t loByte = lcdDataRead();
			uint16_t hiByte = (uint16_t)lcdDataRead();
			*pdata16_t++ = (hiByte<<8 | loByte);
			//pdata16_t++;
		}
		
	}
	else if (_colorMode == COLOR_24BPP_RGB888)
	{	
		uint32_t *pdata32_t = (uint32_t *)data;
		while(data_count--)
		{
			uint8_t blue   = lcdDataRead();
			uint32_t green = (uint32_t)lcdDataRead();
			uint32_t red   = (uint32_t)lcdDataRead();
			*pdata32_t++ = (red<<16 | green <<8 | blue);
			//pdata32_t++;
		}
	}
	
	//Main window restore
	activeWindowXY(ACTIVE_WINDOW_STARTX,ACTIVE_WINDOW_STARTY);
	activeWindowWH(_canvasWidth,_canvasHeight);
	canvasImageStartAddress(CANVAS_OFFSET);
}

/**
 * @brief Set canvas start address.
 * @note  This function is ignored if canvas in linear addressing mode.
 * @param addr is the address in 32-bit integer.
 */
void Ra8876_Lite::canvasImageStartAddress(uint32_t addr)	
{
  lcdRegDataWrite(RA8876_CVSSA0,addr);    //50h
  lcdRegDataWrite(RA8876_CVSSA1,addr>>8); //51h
  lcdRegDataWrite(RA8876_CVSSA2,addr>>16);//52h
  lcdRegDataWrite(RA8876_CVSSA3,addr>>24);//53h  
}

/**
 * @brief Set canvas image width.
 * @note  Max width is 12-bit (4096). This function is ignored if canvas in linear addressing mode.
 * @param width is pixels. It is 4 pixel resolutions.
 */
void Ra8876_Lite::canvasImageWidth(uint16_t width, uint16_t height)	
{
  _canvasWidth = width;
  _canvasHeight = height;
  
  lcdRegDataWrite(RA8876_CVS_IMWTH0,width);     //54h
  lcdRegDataWrite(RA8876_CVS_IMWTH1,width>>8);  //55h
}

/**
 * @brief Set Active Window upper-left corner x,y coordinates with reference to canvas image start address.
 * @note  This function is ignored if canvas in linear addressing mode.
 * @param x0 is the upper left x-coordinate. Max 8188.
 * @param y0 is the upper left y-coordinate. Max 8191.
 */
void Ra8876_Lite::activeWindowXY(uint16_t x0,uint16_t y0)	
{
  lcdRegDataWrite(RA8876_AWUL_X0,x0);//56h
  lcdRegDataWrite(RA8876_AWUL_X1,x0>>8);//57h 
  lcdRegDataWrite(RA8876_AWUL_Y0,y0);//58h
  lcdRegDataWrite(RA8876_AWUL_Y1,y0>>8);//59h 
}

/**
 * @brief Set Active Window width & height.
 * @note  This function is ignored if canvas in linear addressing mode.
 * @param width in pixels. Max 8188.
 * @param height in lines. Max 8191.
 */
void Ra8876_Lite::activeWindowWH(uint16_t width,uint16_t height)	
{
  lcdRegDataWrite(RA8876_AW_WTH0,width);//5ah
  lcdRegDataWrite(RA8876_AW_WTH1,width>>8);//5bh
  lcdRegDataWrite(RA8876_AW_HT0,height);//5ch
  lcdRegDataWrite(RA8876_AW_HT1,height>>8);//5dh  
}

/**
 * @brief Set cursor position
 * @param x,y is the coordinates
 * @param lnOffset is the canvas start address represented in line number.
 * @note  Coordinates (x,y) is relative to Canvas Start Address. User should program proper active window 
 * related parameters before configure this register. Therefore, if the Canvas Start Address has been set
 * to some non-display area in SDRAM, pixel write after setPixelCursor(x,y) will write to that non-display region.
 * Max x,y = 2^13 i.e. 8192. <br>
 */
void Ra8876_Lite:: setPixelCursor(uint16_t x,uint16_t y,uint32_t lnOffset)
{
  //uint8_t bpp = getColorDepth();
  //if(lnOffset > (uint32_t)(MEM_SIZE_MAX/(_canvasWidth*bpp)-1)) return;
  //canvasImageStartAddress(lnOffset*_canvasWidth*bpp);
  
  int32_t _canvasAddress = canvasAddress_from_lnOffset(lnOffset);
  if(_canvasAddress<0) return;
  canvasImageStartAddress(_canvasAddress);
  
  lcdRegDataWrite(RA8876_CURH0,x); //5fh
  lcdRegDataWrite(RA8876_CURH1,x>>8);//60h
  lcdRegDataWrite(RA8876_CURV0,y);//61h
  lcdRegDataWrite(RA8876_CURV1,y>>8);//62h
  
  //activeWindowXY(0,0);
  /*
  #ifdef CANVAS_IN_BLOCK_MODE
  lcdRegDataWrite(RA8876_CURH0,x); //5fh
  lcdRegDataWrite(RA8876_CURH1,x>>8);//60h
  lcdRegDataWrite(RA8876_CURV0,y);//61h
  lcdRegDataWrite(RA8876_CURV1,y>>8);//62h
  #else
  uint32_t _address = (uint32_t)(x+(y*_canvasWidth))*getColorDepth();
  lcdRegDataWrite(RA8876_CURH0,(uint8_t)_address); //5fh
  lcdRegDataWrite(RA8876_CURH1,(uint8_t)(_address>>8));//60h
  lcdRegDataWrite(RA8876_CURV0,(uint8_t)(_address>>16));//61h
  lcdRegDataWrite(RA8876_CURV1,(uint8_t)(_address>>24));//62h  
  #endif
  */
}

//**************************************************************//
//**************************************************************//
void Ra8876_Lite::bte_Source0_MemoryStartAddr(uint32_t addr)	
{
  lcdRegDataWrite(RA8876_S0_STR0,addr);//93h
  lcdRegDataWrite(RA8876_S0_STR1,addr>>8);//94h
  lcdRegDataWrite(RA8876_S0_STR2,addr>>16);//95h
  lcdRegDataWrite(RA8876_S0_STR3,addr>>24);////96h
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::bte_Source0_ImageWidth(uint16_t width)	
{
  lcdRegDataWrite(RA8876_S0_WTH0,width);//97h
  lcdRegDataWrite(RA8876_S0_WTH1,width>>8);//98h
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::bte_Source0_WindowStartXY(uint16_t x0,uint16_t y0)	
{
  lcdRegDataWrite(RA8876_S0_X0,x0);//99h
  lcdRegDataWrite(RA8876_S0_X1,x0>>8);//9ah
  lcdRegDataWrite(RA8876_S0_Y0,y0);//9bh
  lcdRegDataWrite(RA8876_S0_Y1,y0>>8);//9ch
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::bte_Source1_MemoryStartAddr(uint32_t addr)	
{
  lcdRegDataWrite(RA8876_S1_STR0,addr);//9dh
  lcdRegDataWrite(RA8876_S1_STR1,addr>>8);//9eh
  lcdRegDataWrite(RA8876_S1_STR2,addr>>16);//9fh
  lcdRegDataWrite(RA8876_S1_STR3,addr>>24);//a0h
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::bte_Source1_ImageWidth(uint16_t width)	
{
  lcdRegDataWrite(RA8876_S1_WTH0,width);//a1h
  lcdRegDataWrite(RA8876_S1_WTH1,width>>8);//a2h
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::bte_Source1_WindowStartXY(uint16_t x0,uint16_t y0)	
{
  lcdRegDataWrite(RA8876_S1_X0,x0);//a3h
  lcdRegDataWrite(RA8876_S1_X1,x0>>8);//a4h
  lcdRegDataWrite(RA8876_S1_Y0,y0);//a5h
  lcdRegDataWrite(RA8876_S1_Y1,y0>>8);//a6h
}
//**************************************************************//
//**************************************************************//
void  Ra8876_Lite::bte_DestinationMemoryStartAddr(uint32_t addr)	
{
  lcdRegDataWrite(RA8876_DT_STR0,(uint8_t)addr);//a7h
  lcdRegDataWrite(RA8876_DT_STR1,(uint8_t)(addr>>8));//a8h
  lcdRegDataWrite(RA8876_DT_STR2,(uint8_t)(addr>>16));//a9h
  lcdRegDataWrite(RA8876_DT_STR3,(uint8_t)(addr>>24));//aah
}

//**************************************************************//
//**************************************************************//
void  Ra8876_Lite::bte_DestinationImageWidth(uint16_t width)	
{
  lcdRegDataWrite(RA8876_DT_WTH0,width);//abh
  lcdRegDataWrite(RA8876_DT_WTH1,width>>8);//ach
}

//**************************************************************//
//**************************************************************//
void  Ra8876_Lite::bte_DestinationWindowStartXY(uint16_t x0,uint16_t y0)	
{
  lcdRegDataWrite(RA8876_DT_X0,x0);//adh
  lcdRegDataWrite(RA8876_DT_X1,x0>>8);//aeh
  lcdRegDataWrite(RA8876_DT_Y0,y0);//afh
  lcdRegDataWrite(RA8876_DT_Y1,y0>>8);//b0h
}

//**************************************************************//
//**************************************************************//
void  Ra8876_Lite::bte_WindowSize(uint16_t width, uint16_t height)
{
  lcdRegDataWrite(RA8876_BTE_WTH0,width);//b1h
  lcdRegDataWrite(RA8876_BTE_WTH1,width>>8);//b2h
  lcdRegDataWrite(RA8876_BTE_HIG0,height);//b3h
  lcdRegDataWrite(RA8876_BTE_HIG1,height>>8);//b4h
}

#if defined (LOAD_BFC_FONT)
/**
 * @brief	This function returns color from different bit-per-pixel setup in BitFontCreator
 * @param	pixel is a character from font data
 * @param	bpp is the bit-per-pixel 
 * @param	src_color is the font color to be drawn
 * @param	bg is the background color
 * @note	BitFontCreator allows three types of font to create. They are Monochrome (1 bpp), Antialiased in 2 bpp, & 4bpp.
 *			This function accepts input font color from argument 'src_color' and returns the destination color 'des_color'
 *			for all three font types.
 */
Color Ra8876_Lite::bfc_GetColorBasedPixel(uint8_t pixel, uint8_t bpp, Color src_color, Color bg)
{
	Color des_color;	//destination color
	
	switch(bpp)
	{
		case 1:
			des_color.r = src_color.r*pixel + bg.r*(1-pixel);
			des_color.g = src_color.g*pixel + bg.g*(1-pixel);
			des_color.b = src_color.b*pixel + bg.b*(1-pixel);
			break;
		case 2:
			des_color.r = src_color.r*pixel/3 + bg.r*(3-pixel)/3;
			des_color.g = src_color.g*pixel/3 + bg.g*(3-pixel)/3;
			des_color.b = src_color.b*pixel/3 + bg.b*(3-pixel)/3;		
			break;
		case 4:
			des_color.r = src_color.r*pixel/15 + bg.r*(15-pixel)/15;
			des_color.g = src_color.g*pixel/15 + bg.g*(15-pixel)/15;
			des_color.b = src_color.b*pixel/15 + bg.b*(15-pixel)/15;
			break;
		case 8:
			des_color.r = src_color.r*pixel/255 + bg.r*(255-pixel);
			des_color.g = src_color.g*pixel/255 + bg.g*(255-pixel);
			des_color.b = src_color.b*pixel/255 + bg.b*(255-pixel);
			break;
	}
	return des_color;
}

/*
 * @brief	This function decodes pixel data from MCU's Flash. Not prefered for small MCUs.
 */
int Ra8876_Lite::bfc_DrawChar_RowRowUnpacked(
	uint16_t x0, uint16_t y0,	//coordinates to draw a character
	const BFC_FONT *pFont,		//pointer to BFC_FONT from mcu's Flash.
	uint16_t ch, 				//character to draw
	Color color, 				//font color
	Color bg, 					//background color
	bool rotate_ccw90,			//rotate by 90deg in counter clockwise, default false
	uint32_t lnOffset			//SDRAM address offset expressed in line number
	)			
{
	// 1. find the character information first
	const BFC_CHARINFO *pCharInfo = GetCharInfo(pFont, (unsigned short)ch);
	
	if( pCharInfo != 0 )
	{
		int height = pFont->FontHeight;
		int width = pCharInfo->Width;
		//int data_size = pCharInfo->DataSize;              // # bytes of the data array
		const unsigned char *pData = pCharInfo->p.pData8;   // pointer to data array

		int bpp = GetFontBpp(pFont->FontType);              // how many bits per pixel
		int bytesPerLine = (width * bpp + 7) / 8;           // # bytes in a row
		int bLittleEndian = (GetFontEndian(pFont->FontType)==1);

		uint16_t x, y, _x, _y, col;
		unsigned char data, pixel, bit;
		
		// 2. draw all the pixels in this character
		for(y=0; y<height; y++)
		{
			for(x=0; x<width; x++)
			{
				col = (x * bpp) / 8;       // byte index in the line
				data = pData[y * bytesPerLine + col];

				// every BYTE (8 bits) data includes 8/bpp pixels,
				// we need to get each pixel color index (0,1,2,3... based on bpp) from the BYTE data
				pixel = data;
				
				// bit index in the BYTE
				// For 1-bpp: bit =  x % 8 (Big Endian),   7 -  x % 8 (Little Endian)
				// For 2-bpp: bit = 2x % 8 (Big Endian),   6 - 2x % 8 (Little Endian)
				// For 4-bpp: bit = 4x % 8 (Big Endian),   4 - 4x % 8 (Little Endian)
				bit = bLittleEndian ? (8-bpp)-(x*bpp)%8 : (x*bpp)%8;

				pixel = pixel<<bit;               // clear left pixels
				pixel = pixel>>(8/bpp-1)*bpp;     // clear right pixels
				
				Color des_color = bfc_GetColorBasedPixel(pixel, bpp, color, bg);			
				
				_x = x0+x, 
				_y = y0+y;
					
				///Rotation by 90 degrees requires transformation by software.
				///Mirror image or 180 degrees rotation can be done by RA8876 hardware.
				if(rotate_ccw90)
					rotateCcw90(&_x, &_y);
				
				if(pixel) 
				{
					Ra8876_Lite::putPixel(_x, _y, des_color, lnOffset);
				}
				else
				{
					sf::Color _color;
					if(bg!=_color.Transparent){
						Ra8876_Lite::putPixel(_x, _y, bg, lnOffset);
					}
				}
			}
		}
		
		if(lnOffset!=CANVAS_OFFSET)
			canvasImageStartAddress(CANVAS_OFFSET);
		
		return width;
	}
	return 0;
}	

/**
 * @brief	This function decodes pixel data in row-based from a *.bin file stored in SD Card. The binary file 
 *			created by BinFontCreator.
 * @note	Known bug: When this function is repeatly called, SD.open(pFilename) sometimes fail even if the file exists.
 *			It is advised to copy all font data to SDRAM prior to main loop or move binary data to external serial Flash.
 */
#if defined (LOAD_SD_LIBRARY)
int Ra8876_Lite::bfc_DrawChar_RowRowUnpacked(	
	uint16_t x0, uint16_t y0, 	//coordinates to draw a character
	const char* pFilename, 		//filename pointer
	uint16_t ch, 					//character to draw
	Color color, 				//color to draw
	Color bg, 					//background
	bool rotate_ccw90,
	uint32_t lnOffset
	)
{
	File fontFile = SD.open(pFilename);

	if(!fontFile) {
		printf("No such file exist!\n");
		return 0;
		}
	else
	{
		//printf("File open OK.\n");	
	}		
	
	uint8_t buf[12];	
	BFC_BIN_FONT bfcBinFont;
	
	//(1) Read 12 bytes from Bin file beginning for BFC_BIN_FONT first 
	fontFile.read((uint8_t *)buf, 12);	
	bfcBinFont.FontType = (uint32_t)buf[0]|(uint32_t)buf[1]<<8|(uint32_t)buf[2]<<16|(uint32_t)buf[3]<<24;
	//printf("Bin FontType = %X\n", bfcBinFont.FontType);	
	uint16_t height = bfcBinFont.FontHeight = (uint16_t)buf[4]|(uint16_t)buf[5]<<8;
	//printf("Bin FontHeight = %d\n", bfcBinFont.FontHeight);	
	bfcBinFont.Baseline = (uint16_t)buf[6]|(uint16_t)buf[7]<<8;
	//printf("Bin Baseline = %d\n", bfcBinFont.Baseline);
	bfcBinFont.Reversed = (uint16_t)buf[8]|(uint16_t)buf[9]<<8;
	//printf("Bin Reversed = %d\n", bfcBinFont.Reversed);
	bfcBinFont.NumRanges = (uint16_t)buf[10]|(uint16_t)buf[11]<<8;
	//printf("Bin NumRanges = %d\n", bfcBinFont.NumRanges);
	
	//NumRanges should be >= 1. This is the number of character-range.
	if(!bfcBinFont.NumRanges) 
	{
		fontFile.close();
		return 0;	
	}
	
	//(2) Now we know the number of character ranges by chNumRanges below.
	//Need to find out the location of "ch" from these character ranges.
	uint16_t chNumRanges = bfcBinFont.NumRanges;
	uint16_t charInfo_array_index = 0;
	uint32_t addressOffset = (uint32_t)0x0c + (4L*(uint32_t)chNumRanges);	//this is the physical address of "ch", one NumRanges occupies 4 bytes that's why 4*chNumRanges

	BFC_BIN_CHARRANGE fontxxx_Prop[chNumRanges];
	
	for(uint16_t i=0; i<chNumRanges; i++)
	{
		fontFile.read((uint8_t *)buf, 4);
		fontxxx_Prop[i].FirstChar = (uint16_t)buf[0]|(uint16_t)buf[1]<<8;
		fontxxx_Prop[i].LastChar  = (uint16_t)buf[2]|(uint16_t)buf[3]<<8;
		
		//scan it from the beginning of BFC_BIN_CHARINFO fontxxx_CharInfo[] array until ch is in range
		if(ch >= fontxxx_Prop[i].FirstChar && ch <= fontxxx_Prop[i].LastChar)
		{
			//the character "ch" is inside this ranage
			addressOffset += (uint32_t)(charInfo_array_index*8L);
			addressOffset += (uint32_t)(ch-fontxxx_Prop[i].FirstChar)*8L;
			//printf("Address offset of char 'ch' = 0x%X\n", addressOffset);
			//printf("ch index = %d\n", charInfo_array_index);
			break;	//break the loop once it has been found!
		}
		else
		{			
			charInfo_array_index += (fontxxx_Prop[i].LastChar-fontxxx_Prop[i].FirstChar+1);
		}
	}
	
	//(3) The variable <addressOffset> is the physical address of BFC_BIN_CHARINFO.
	// If it is a valid address, we may now read 8 bytes for BFC_BIN_CHARINFO structure in which
	// the character width, data size, and physical address can be read.
	if(!fontFile.seek(addressOffset)) //seek physical address of ch's BFC_BIN_CHARINFO
	{
		printf("Address of \"ch\" is not valid!\n");
		fontFile.close();
		return 0;
	}

	fontFile.read((uint8_t*)buf, 8);	//read BFC_BIN_CHARINFO at physical address of "ch"
	
	BFC_BIN_CHARINFO bfcBinFont_info;
	
	uint16_t width = bfcBinFont_info.Width = (uint16_t)buf[0]|(uint16_t)buf[1]<<8;
	uint16_t data_size = bfcBinFont_info.DataSize = (uint16_t)buf[2]|(uint16_t)buf[3]<<8;
	//bfcBinFont_info.OffData is the physical address of pixel data
	uint32_t data_address = bfcBinFont_info.OffData = (uint32_t)buf[4]|(uint32_t)buf[5]<<8|(uint32_t)buf[6]<<16|(uint32_t)buf[7]<<24;	
	//printf("BFC_BIN_CHARINFO : width = %d, size = %d, address = 0x%X\n", bfcBinFont_info.Width, bfcBinFont_info.DataSize, bfcBinFont_info.OffData);
	/*	
	//This is no good for small mcu as data_size can be very big. Instead, we read pixels from SD Card one-by-one and plot them each
	unsigned char data[data_size];
	//(4) With a valid physical address of character's pixel data, we may now read this data into a temporary array pixel[data_size]
	//	
	if(!fontFile.seek(data_address))
	{
		//printf("Data of \"ch\" is not valid!\n");
		fontFile.close();
		return 0;
	}
	
	uint16_t i=0;
	while(fontFile.available() && data_size--)
	{
		data[i]=fontFile.read();
		//printf("Index @%d, data=0x%X\n", i, data[i]);
		i++;
	}
	*/
	//(5) Now we have everything required to render a single character from a bin file. 
	// The final step is to draw each pixel with data fetched from SD card with fontFile.read();
	int bpp = GetFontBpp(bfcBinFont.FontType);
	uint16_t bytesPerLine = (width * bpp + 7)/8;
	int bLittleEndian = (GetFontEndian(bfcBinFont.FontType)==1);
	
	uint16_t x, y, _x, _y, col;
	unsigned char pixel, bit;
	
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			col = (x * bpp)/8;
			fontFile.seek(data_address + (y * bytesPerLine + col));	//addres seeking for each pixel
			pixel = fontFile.read();
			//pixel = data[y * bytesPerLine + col];
			bit = bLittleEndian ? (8-bpp)-(x*bpp)%8 : (x*bpp)%8;
			
			pixel = pixel<<bit;					// clear left pixels
			pixel = pixel>>(8/bpp-1)*bpp;     	// clear right pixels			
			Color des_color = bfc_GetColorBasedPixel(pixel, bpp, color, bg);		
			_x=x0+x, 
			_y=y0+y;				
			///Rotation by 90 degrees requires transformation by software.
			///Mirror image or 180 degrees rotation can be done by RA8876 hardware.
			if(rotate_ccw90)
				rotateCcw90(&_x, &_y);
				
			if(pixel) 
			{
				Ra8876_Lite::putPixel(_x, _y, des_color, lnOffset);
			}
			else
			{
				sf::Color _color;
				if(bg!=_color.Transparent){
					Ra8876_Lite::putPixel(_x, _y, bg, lnOffset);
				}
			}		
		}
	}
	
	if(lnOffset!=CANVAS_OFFSET)
		canvasImageStartAddress(CANVAS_OFFSET);	
	
	//(6) Finally close the file 
	fontFile.close();	
	//printf("File closed.\n");	
	return width;
}
#endif	//#if defined (LOAD_SD_LIBRARY)

/**
 * @brief	This function draws a single character from a *.c file generated by BitFontCreator
 * @param	x0 is the x-coordinate of top left corner
 * @param	y0 is the y-coordinate of top left corner
 * @param	*pFont is a pointer to BFC_FONT in MCU's Flash
 * @param	color is the font color
 * @param	bg is the background color
 * @param	rotate_ccw90 is a boolean flag to rotate a single character in counter clockwise 90 degrees
 * @return	Width of the character drawn useful for string print in putBfcString(args)
 */
uint16_t Ra8876_Lite::putBfcChar(
uint16_t x0,uint16_t y0, 
const BFC_FONT *pFont, 
const uint16_t ch, 
Color color, 
Color bg, 
bool rotate_ccw90,
uint32_t lnOffset)
{
	return (uint16_t)bfc_DrawChar_RowRowUnpacked(x0, y0, pFont, ch, color, bg, rotate_ccw90, lnOffset);
}


#if defined (LOAD_SD_LIBRARY)
/**
 * @brief	This function draws a single character from a *.bin file generated by BitFontCreator.<br>
 * @param	x0 is the x-coordinate of top left corner
 * @param	y0 is the y-coordinate of top left corner
 * @param	*pFilename points to the filename of *.bin
 * @param	color is the font color
 * @param	bg is the background color
 * @param	rotate_ccw90 is a boolean flag to rotate a single character in counter clockwise 90 degrees
 * @param	lnOffset is the Canvas Address offset in line number
 * @return	Width of the character drawn useful for string print in putBfcString(args)
 * @note	Dependency : SD.h<br>
 *	 		Example to use <br>
 *			///Font: GN_Kin_iro_SansSerif48hAA4 こんにちは in Unicode 16 = {0x3053, 0x3093, 0x306B, 0x3061, 0x306F, '\0'}
 *			///Character こ in Unicode = 0x3953
 *			ra8876lite.putBfcChar(100,100, "GN_Kin.bin", 0x3053, color.Red, color.Transparent);
 */
uint16_t Ra8876_Lite::putBfcChar(
uint16_t x0,uint16_t y0, 
const char *pFilename, 
const uint16_t ch, 
Color color, 
Color bg, 
bool rotate_ccw90,
uint32_t lnOffset)
{
	return (uint16_t)bfc_DrawChar_RowRowUnpacked(x0, y0, pFilename, ch, color, bg, rotate_ccw90, lnOffset);
}

/**
 * @brief	This function blit a single character in Unicode from a cache region to the Main Window.<br>
 * @note    For low-end microcontrollers (micros) it is very common to see a LCD drawn in slow-mo pixel-by-pixel.<br>
 *          It is because low-end micros usually come with limited memory resource that rule out the possiblity of a frame buffer.<br>
 *          On ArduoHDMI there is an external SDRAM of 256Mbit. This is equivalent to a storage space of 256*1024*1024 bit = 32Mbyte.<br>
 *          If we are using 2-byte color depth in 5-6-5 (RGB) format, this SDRAM can store up to 800*480*43 pages in 800*480 resolution.<br>
 *          Deducting the page for the Main Window (visible screen area), still we are left with 42 pages as storage space.<br>
 *          Only two procedures are involved :<br>
 *          (1) Render the character on some region outside the Main Window. No more pixel-by-pixel plotting because it is outside the visible region.<br>
 *          (2) After rendering finished, copy the rectangle that holds the character to the Main Window by 2D BTE.<br>
 *			Limitation: antialiased characters don't look good because only a single color mask is possible with bteMemoryCopyWithChromaKey()
 * @param	x0 is the x-coordinate of top left corner
 * @param	y0 is the y-coordinate of top left corner
 * @param	*pFilename points to the filename of *.bin
 * @param	color is the font color
 * @param	bg is the background color
 * @param	rotate_ccw90 is a boolean flag to rotate in counter clockwise 90 degrees
 * @return	Width of the character
 * @note	Dependency : SD.h<br>
 *	 		Example to use <br>
 *			///Font: GN_Kin_iro_SansSerif48hAA4 こんにちは in Unicode 16 = {0x3053, 0x3093, 0x306B, 0x3061, 0x306F, '\0'}
 *			///Character こ in Unicode = 0x3953
 *			ra8876lite.blitBfcChar(100,100, "GN_Kin.bin", 0x3053, color.Red, color.Black);
 */
uint16_t Ra8876_Lite::blitBfcChar(
uint16_t x0,uint16_t y0, 
const char *pFilename, 
const uint16_t ch, 
Color color, 
Color bg, 
bool rotate_ccw90)
{
  uint16_t copy_width, copy_height, w_return;
  Color _bg=bg, mask;
  
  if(bg==mask.Transparent)
    _bg=mask.Magenta;  //use Magenta as the color mask
    
  if(rotate_ccw90)
  {
    copy_height = w_return = putBfcChar(x0, y0, pFilename, ch, color, _bg, rotate_ccw90, CANVAS_CACHE);
    copy_width  = getBfcFontHeight(pFilename);  
    rotateCcw90(&x0,&y0);     
    y0-=copy_height-1;
  }
  else
  {
    copy_width = w_return = putBfcChar(x0, y0, pFilename, ch, color, _bg, rotate_ccw90, CANVAS_CACHE);
    copy_height = getBfcFontHeight(pFilename);   
  }
	
  if(bg==color.Transparent)
  {
    bteMemoryCopyWithChromaKey(
    canvasAddress_from_lnOffset(CANVAS_CACHE),
    getCanvasWidth(),
    x0,y0,
    canvasAddress_from_lnOffset(CANVAS_OFFSET),
    getCanvasWidth(),
    x0,y0,
    copy_width, copy_height,
    mask.Magenta);       
  }
  else
  {
    bteMemoryCopyWithROP(
    canvasAddress_from_lnOffset(CANVAS_CACHE),
    getCanvasWidth(),
    x0,y0,
    0,0,0,0,  //all '0' for s1 image source
    canvasAddress_from_lnOffset(CANVAS_OFFSET),
    getCanvasWidth(),
    x0,y0,
    copy_width, copy_height,
    RA8876_BTE_ROP_CODE_12);   
  }	
  
  return w_return;
}

/**
 *
 */
uint16_t Ra8876_Lite::blitBfcString(
uint16_t x0,uint16_t y0, 
const char *pFilename, 
const uint16_t *str, 
Color color, 
Color bg, 
bool rotate_ccw90)
{
  uint16_t copy_width, copy_height, w_return;
  Color _bg=bg, mask;
  
  if(bg==mask.Transparent)
    _bg=mask.Magenta;  //use Magenta as the color mask
    
  if(rotate_ccw90)
  {
    copy_height = w_return = putBfcString(x0, y0, pFilename, str, color, _bg, rotate_ccw90, CANVAS_CACHE);
    copy_width  = getBfcFontHeight(pFilename);  
    rotateCcw90(&x0,&y0);     
    y0-=copy_height-1;
  }
  else
  {
    copy_width = w_return = putBfcString(x0, y0, pFilename, str, color, _bg, rotate_ccw90, CANVAS_CACHE);
    copy_height = getBfcFontHeight(pFilename);   
  }
	
  if(bg==color.Transparent)
  {
    bteMemoryCopyWithChromaKey(
    canvasAddress_from_lnOffset(CANVAS_CACHE),
    getCanvasWidth(),
    x0,y0,
    canvasAddress_from_lnOffset(CANVAS_OFFSET),
    getCanvasWidth(),
    x0,y0,
    copy_width, copy_height,
    mask.Magenta);       
  }
  else
  {
    bteMemoryCopyWithROP(
    canvasAddress_from_lnOffset(CANVAS_CACHE),
    getCanvasWidth(),
    x0,y0,
    0,0,0,0,  //all '0' for s1 image source
    canvasAddress_from_lnOffset(CANVAS_OFFSET),
    getCanvasWidth(),
    x0,y0,
    copy_width, copy_height,
    RA8876_BTE_ROP_CODE_12);   
  }	
  
  return w_return;
}


uint16_t Ra8876_Lite::blitBfcString(
uint16_t x0,uint16_t y0, 
const char *pFilename, 
const char *str, 
Color color, 
Color bg, 
bool rotate_ccw90)
{
  uint16_t copy_width, copy_height, w_return;
  Color _bg=bg, mask;
  
  if(bg==mask.Transparent)
    _bg=mask.Magenta;  //use Magenta as the color mask
    
  if(rotate_ccw90)
  {
    copy_height = w_return = putBfcString(x0, y0, pFilename, str, color, _bg, rotate_ccw90, CANVAS_CACHE);
    copy_width  = getBfcFontHeight(pFilename);  
    rotateCcw90(&x0,&y0);     
    y0-=copy_height-1;
  }
  else
  {
    copy_width = w_return = putBfcString(x0, y0, pFilename, str, color, _bg, rotate_ccw90, CANVAS_CACHE);
    copy_height = getBfcFontHeight(pFilename);   
  }
	
  if(bg==color.Transparent)
  {
    bteMemoryCopyWithChromaKey(
    canvasAddress_from_lnOffset(CANVAS_CACHE),
    getCanvasWidth(),
    x0,y0,
    canvasAddress_from_lnOffset(CANVAS_OFFSET),
    getCanvasWidth(),
    x0,y0,
    copy_width, copy_height,
    mask.Magenta);       
  }
  else
  {
    bteMemoryCopyWithROP(
    canvasAddress_from_lnOffset(CANVAS_CACHE),
    getCanvasWidth(),
    x0,y0,
    0,0,0,0,  //all '0' for s1 image source
    canvasAddress_from_lnOffset(CANVAS_OFFSET),
    getCanvasWidth(),
    x0,y0,
    copy_width, copy_height,
    RA8876_BTE_ROP_CODE_12);   
  }	
  
  return w_return;
}
#endif

/**
 * @brief	This function draws a string from a *.c font file generated by BitFontCreator. 
 * @param	x0 is the x-coordinate of top left corner
 * @param	y0 is the y-coordinate of top left corner
 * @param	*pFont is a pointer to BFC_FONT in MCU's Flash
 * @param	*str is a pointer to a string of ASCII
 * @param	color is the font color
 * @param	bg is the background color
 * @param	rotate_ccw90 is a boolean flag to rotate the string in counter clockwise 90 degrees
 * @return	width of string to draw
 * @note	Example to use <br>
 *			//...
 *			extern const BFC_FONT fontLucida_Sans_Unicode16h_rowrowBig;
 *			
 *			//Draw a string "Hello World!" with font color black on a white background
 *			ra8876lite.putBfcString(200,100, &fontLucida_Sans_Unicode16h_rowrowBig, "Hello World!", color.Black, color.White);
 */ 
uint16_t Ra8876_Lite::putBfcString(
uint16_t x0,uint16_t y0, 
const BFC_FONT *pFont, 
const char *str, 
Color color, 
Color bg, 
bool rotate_ccw90,
uint32_t lnOffset)
{
	int x = x0;
	int y = y0;
	int width = 0;
	char ch = 0;

	if( pFont == 0 || str == 0 )
		return 0;

	while(*str != '\0')
	{
		ch = *str;
		width = putBfcChar(x, y, pFont, ch, color, bg, rotate_ccw90, lnOffset);
		str++;
		x += width;
	}  
	return (uint16_t)(x-x0);
}

/**
 * @brief	This function draws a string from a *.c font file generated by BitFontCreator. 
 * @param	x0 is the x-coordinate of top left corner
 * @param	y0 is the y-coordinate of top left corner
 * @param	*pFont is a pointer to BFC_FONT in MCU's Flash
 * @param	*str is a pointer to an array 2-byte fonts
 * @param	color is the font color
 * @param	bg is the background color
 * @param	rotate_ccw90 is a boolean flag to rotate the string in counter clockwise 90 degrees
 * @return	width of string to draw
 * @note	Example to use <br>
 *			//...
 *			extern const BFC_FONT fontLucida_Sans_Unicode16h_rowrowBig;
 *			
 *			unsigned short chinese_string[]={0x4F60, 0x597D, 0x4E16, 0x754C, 0x676F, '\0'};	//2-byte font is supported
 *			//Draw some Chinese characters with font color black on a white background
 *			ra8876lite.putBfcString(200,100, &fontLucida_Sans_Unicode16h_rowrowBig, chinese_string, color.Black, color.White);
 */
uint16_t Ra8876_Lite::putBfcString(
uint16_t x0,
uint16_t y0, 
const BFC_FONT *pFont,
const uint16_t *str, 
Color color, 
Color bg, 
bool rotate_ccw90,
uint32_t lnOffset)
{
	int x = x0;
	int y = y0;
	int width = 0;
	uint16_t ch = 0;
	
	if( pFont == 0 || str == 0 )
		return 0;

	while(*str != '\0')
	{
		ch = *str;
		width = putBfcChar(x, y, pFont, ch, color, bg, rotate_ccw90, lnOffset);
		str++;
		x += width;
	}  	
	
	return (uint16_t)(x-x0);
}

#if defined (LOAD_SD_LIBRARY)
/**
 * @brief	This function draws a string from a *.c font file generated by BitFontCreator. 
 * @param	x0 is the x-coordinate of top left corner
 * @param	y0 is the y-coordinate of top left corner
 * @param	*pFilename is a pointer to the binary file name
 * @param	*str is a pointer to a string of ASCII, 1 byte font
 * @param	color is the font color
 * @param	bg is the background color
 * @param	rotate_ccw90 is a boolean flag to rotate the string in counter clockwise 90 degrees
 * @return	return width of the string to draw
 * @note	Example to use with pre-requisite that the binary file "mingliu.bin" contains the ASCII string "Hello World!"
 *			
 *			//Draw a string "Hello World!" with font color black on a white background
 *			ra8876lite.putBfcString(200,100, "mingliu.bin", "Hello World!", color.Black, color.White);
 */

uint16_t Ra8876_Lite::putBfcString(
uint16_t x0,
uint16_t y0, 
const char *pFilename, 
const char *str, 
Color color, 
Color bg, 
bool rotate_ccw90,
uint32_t lnOffset)
{
	int x = x0;
	int y = y0;
	int width = 0;
	char ch = 0;
	
	if( pFilename == 0 || str == 0 )
		return 0;

	while(*str != '\0')
	{
		ch = *str;
		width = putBfcChar(x, y, pFilename, ch, color, bg, rotate_ccw90, lnOffset);
		str++;
		//width = putBfcChar(x, y, pFilename, *str++, color, bg, rotate_ccw90, lnOffset);
		x += width;
	}  
	return (uint16_t)(x-x0);
}

/**
 * @brief	This function draws a string from a *.c font file generated by BitFontCreator. 
 * @param	x0 is the x-coordinate of top left corner
 * @param	y0 is the y-coordinate of top left corner
 * @param	*pFilename is a pointer to the binary file name
 * @param	*str is a pointer to an array 2-byte fonts
 * @param	color is the font color
 * @param	bg is the background color
 * @param	rotate_ccw90 is a boolean flag to rotate the string in counter clockwise 90 degrees
 * @return	width of string to draw
 * @note	Example to use with pre-requisite that the binary file "mingliu.bin" contains the 2-byte characters
 *			declared in chinese_string[] below, and a microSD card is storing the file "mingliu.bin"<br>
 *			//...
 *			
 *			unsigned short chinese_string[]={0x4F60, 0x597D, 0x4E16, 0x754C, 0x676F, '\0'};	//2-byte font is supported
 *			//Draw some Chinese characters with font color black on a white background
 *			ra8876lite.putBfcString(200,100, "mingliu.bin", chinese_string, color.Black, color.White);
 */
uint16_t Ra8876_Lite::putBfcString(
uint16_t x0,uint16_t y0, 
const char *pFilename, 
const uint16_t *str, 
Color color, 
Color bg, 
bool rotate_ccw90,
uint32_t lnOffset)
{
	int x = x0;
	int y = y0;
	int width = 0;
	uint16_t ch = 0;
	
	if( pFilename == 0 || str == 0 )
		return 0;

	while(*str != '\0')
	{
		ch = *str;
		width = putBfcChar(x, y, pFilename, ch, color, bg, rotate_ccw90, lnOffset);
		str++;
		//width = putBfcChar(x, y, pFilename, *str++, color, bg, rotate_ccw90, lnOffset);
		x += width;
	}  	
	return (uint16_t)(x-x0);
}
#endif

/**
 * @brief	This function returns ASCII string width in pixels
 * @param	*pFont is a pointer to BFC_FONT in MCU's Flash
 * @param	*str points to a string of ASCII
 * @return	String width in pixels
 */
uint16_t Ra8876_Lite::getBfcStringWidth(const BFC_FONT *pFont, const char *str)
{
	uint16_t width=0;
	
	const BFC_CHARINFO *pCharInfo;
	char ch=0;
	
	while(*str != '\0')
	{
		ch = *str;
		pCharInfo = GetCharInfo(pFont, ch);
		if(pCharInfo !=0)
			width += pCharInfo->Width;
		
		str++;
	}
	
	return width;	
}

/**
 * @brief	This function returns Unicode string width in pixels
 * @param	*pFont is a pointer to BFC_FONT in MCU's Flash
 * @param	*str points to a string of Unicode
 * @return	String width in pixels
 */
uint16_t Ra8876_Lite::getBfcStringWidth(const BFC_FONT *pFont, const uint16_t *str)
{
	uint16_t width=0;
	
	const BFC_CHARINFO *pCharInfo;
	uint16_t ch=0;
	
	while(*str != '\0')
	{
		ch = *str;
		pCharInfo = GetCharInfo(pFont, ch);
		if(pCharInfo !=0)
			width += pCharInfo->Width;
		
		str++;
	}
	
	return width;
}

/**
 * @brief	This function returns font height in pixels
 * @param	*pFont is a pointer to BFC_FONT in MCU's Flash
 * @return	Font height in pixels
 */
uint16_t Ra8876_Lite::getBfcFontHeight(const BFC_FONT *pFont)
{
	int fontHeight = GetFontHeight(pFont);
	
	return ((uint16_t)fontHeight);
}


#if defined (LOAD_SD_LIBRARY)
/**
 * @brief	This function returns the character width in pixels from a binary file created by BFC.
 * @param	*pFilename points to a null terminated string for the binary file's filename.
 * @param	ch is the character in unicode.
 * @return	Character width in pixels.
 */
uint16_t Ra8876_Lite::getBfcCharWidth(const char *pFilename, const uint16_t ch)
{
	File fontFile = SD.open(pFilename);
	if(!fontFile) return 0;
	
	uint8_t buf[12];
	BFC_BIN_FONT bfcBinFont;
	//Read header of 12 bytes from the binary file for BFC_BIN_FONT
	fontFile.read((uint8_t *)buf, 12);	
	uint16_t chNumRanges = bfcBinFont.NumRanges = (uint16_t)buf[10]|(uint16_t)buf[11]<<8;
	if(!chNumRanges) return 0;	//NumRanges should be >= 1. This is the number of character-range.

	uint32_t addressOffset = (uint32_t)0x0c + (4L*(uint32_t)chNumRanges);	//this is the physical address of "ch", one NumRanges occupies 4 bytes that's why 4*chNumRanges
	uint16_t charInfo_array_index = 0;
	BFC_BIN_CHARRANGE fontxxx_Prop[chNumRanges];
	for(uint16_t i=0; i<chNumRanges; i++)
	{
		fontFile.read((uint8_t *)buf, 4);
		fontxxx_Prop[i].FirstChar = (uint16_t)buf[0]|(uint16_t)buf[1]<<8;
		fontxxx_Prop[i].LastChar  = (uint16_t)buf[2]|(uint16_t)buf[3]<<8;
		
		//scan it from the beginning of BFC_BIN_CHARINFO fontxxx_CharInfo[] array until ch is in range
		if((unsigned short)ch >= fontxxx_Prop[i].FirstChar && (unsigned short)ch <=fontxxx_Prop[i].LastChar)
		{
			//the character "ch" is inside this ranage
			addressOffset += (uint32_t)(charInfo_array_index*8L);
			addressOffset += (uint32_t)(ch-fontxxx_Prop[i].FirstChar)*8L;
			break;	//break the loop once it has been found!
		}
		else
		{			
			charInfo_array_index += (fontxxx_Prop[i].LastChar-fontxxx_Prop[i].FirstChar+1);
		}
	}
	
	//seek physical address of ch's BFC_BIN_CHARINFO
	if(!fontFile.seek(addressOffset)) 
	{
		//printf("Address of "ch" is not valid!\n");
		return 0;
	}	
	fontFile.read((uint8_t*)buf, 8);	//read BFC_BIN_CHARINFO at physical address of "ch"
	BFC_BIN_CHARINFO bfcBinFont_info;
	
	uint16_t width = bfcBinFont_info.Width = (uint16_t)buf[0]|(uint16_t)buf[1]<<8;
	return width;
}

/**
 * @brief	This function returns the length of a ASCII string from a binary file created by BFC.
 * @param	*pFilename points to a null terminated string for the binary file's filename.
 * @param	*str points to an ASCII constant string.
 * @return	width of the string
 */
uint16_t Ra8876_Lite::getBfcStringWidth(const char *pFilename, const char *str)
{
	uint16_t width=0;
	char ch=0;
	
	while(*str!='\0')
	{
		ch=*str;
		width += getBfcCharWidth(pFilename, ch);
		str++;
	}
	
	return width;	
}

/**
 * @brief	This function returns the length of an Unicode string from a binary file created by BFC.
 * @param	*pFilename points to a null terminated string for the binary file's filename.
 * @param	*str points to an Unicode constant string.
 * @return	width of the string
 */
uint16_t Ra8876_Lite::getBfcStringWidth(const char *pFilename, const uint16_t *str)
{
	uint16_t width=0;
	uint16_t ch=0;
	
	while(*str!='\0')
	{
		ch=*str;
		width += getBfcCharWidth(pFilename, ch);
		str++;
	}
	
	return width;
}

/**
 * @brief	This function returns font height in pixels
 *			Pre-requisite : A valid binary file is available from SD card
 * @param	*pFilename is a pointer to the binary file name
 * @return	Font height in pixels
 */
uint16_t Ra8876_Lite::getBfcFontHeight(const char *pFilename)
{
	uint8_t buf[12];
	BFC_BIN_FONT bfcBinFont;
	
	File fontFile = SD.open(pFilename);
	if(!fontFile) return 0;
	
	fontFile.read((uint8_t *)buf, 12);	
	bfcBinFont.FontType = (uint32_t)buf[0]|(uint32_t)buf[1]<<8|(uint32_t)buf[2]<<16|(uint32_t)buf[3]<<24;	
	uint16_t height = bfcBinFont.FontHeight = (uint16_t)buf[4]|(uint16_t)buf[5]<<8;

	fontFile.close();
	
	return height;
}
#endif	//#if defined (LOAD_SD_LIBRARY)
#endif	//#if defined (LOAD_BFC_FONT)

//**************************************************************//
/*These 8 bits determine prescaler value for Timer 0 and 1.*/
/*Time base is 鈥淐ore_Freq / (Prescaler + 1)鈥�*/
//**************************************************************//
void Ra8876_Lite::pwm_Prescaler(uint8_t prescaler)
{
  lcdRegDataWrite(RA8876_PSCLR,prescaler);//84h
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::pwm_ClockMuxReg(uint8_t pwm1_clk_div, uint8_t pwm0_clk_div, uint8_t xpwm1_ctrl, uint8_t xpwm0_ctrl)
{
  lcdRegDataWrite(RA8876_PMUXR,pwm1_clk_div<<6|pwm0_clk_div<<4|xpwm1_ctrl<<2|xpwm0_ctrl);//85h
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::pwm_Configuration(uint8_t pwm1_inverter,uint8_t pwm1_auto_reload,uint8_t pwm1_start,uint8_t 
                       pwm0_dead_zone, uint8_t pwm0_inverter, uint8_t pwm0_auto_reload,uint8_t pwm0_start)
 {
  lcdRegDataWrite(RA8876_PCFGR,pwm1_inverter<<6|pwm1_auto_reload<<5|pwm1_start<<4|pwm0_dead_zone<<3|
                  pwm0_inverter<<2|pwm0_auto_reload<<1|pwm0_start);//86h                
 }   
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::pwm0_Duty(uint16_t duty)
{
  lcdRegDataWrite(RA8876_TCMPB0L,duty);//88h 
  lcdRegDataWrite(RA8876_TCMPB0H,duty>>8);//89h 
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::pwm0_ClocksPerPeriod(uint16_t clocks_per_period)
{
  lcdRegDataWrite(RA8876_TCNTB0L,clocks_per_period);//8ah
  lcdRegDataWrite(RA8876_TCNTB0H,clocks_per_period>>8);//8bh
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::pwm1_Duty(uint16_t duty)
{
  lcdRegDataWrite(RA8876_TCMPB1L,duty);//8ch 
  lcdRegDataWrite(RA8876_TCMPB1H,duty>>8);//8dh
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite::pwm1_ClocksPerPeriod(uint16_t clocks_per_period)
{
  lcdRegDataWrite(RA8876_TCNTB1L,clocks_per_period);//8eh
  lcdRegDataWrite(RA8876_TCNTB1F,clocks_per_period>>8);//8fh
}

/**
 * @brief Portal for SDRAM memory write
 */
void Ra8876_Lite:: ramAccessPrepare(void)
{
  lcdRegWrite(RA8876_MRWDP); //04h
}

/**
 * @brief Set foreground color
 * @param color is an object composed of 4 components: red, green, blue, alpha
 * @note  Example to use:<br>
 *        Ra8876_Lite ra8876lite;<br>
 *        Color color;<br>
 *        ra8876lite.setForegroundColor(color.Red);   //set red as the foreground color<br>
 *        delay(1000);<br>
 *        ra8876lite.setForegroundColor(color.Green); <br>
 *        delay(1000);<br>
 *        ra8876lite.setForegroundColor(color.Blue); <br>
 *        No foreground transparency is supported for RA8876 therefore alpha is always = 255<br>
 *        MSB for each component is used <br>
 *        Example, color.r[7:5] is used for component red in 8BPP<br>
 *        color.g[7:5] used for component green in 16BPP.
 */
void Ra8876_Lite::setForegroundColor(Color color)
{
  uint8_t _mode = getColorMode();

  switch(_mode)
  {
    case COLOR_8BPP_RGB332:   //RGB in 3:3:2
          lcdRegDataWrite(RA8876_FGCR,(color.r&0xE0)); //d2h[7:5]
          lcdRegDataWrite(RA8876_FGCG,(color.g&0xE0)); //d3h[7:5]
          lcdRegDataWrite(RA8876_FGCB,(color.b&0xC0)); //d4h[7:6]
          break;
    case COLOR_24BPP_RGB888:  //RGB in 8:8:8
          lcdRegDataWrite(RA8876_FGCR,color.r); //d2h[7:0]
          lcdRegDataWrite(RA8876_FGCG,color.g); //d3h[7:0]
          lcdRegDataWrite(RA8876_FGCB,color.b); //d4h[7:0]
          break;
    default:  //RGB in 5:6:5
          lcdRegDataWrite(RA8876_FGCR,(color.r&0xF8));//d2h[7:3]
          lcdRegDataWrite(RA8876_FGCG,(color.g&0xFC));//d3h[7:2]
          lcdRegDataWrite(RA8876_FGCB,(color.b&0xF8));//d4h[7:3]
  } 
}

/**
 * @brief Set background color
 * @param color is an object composed of 4 components: red, green, blue, alpha
 * @note  Example to use:<br>
 *        Ra8876_Lite ra8876lite;<br>
 *        Color bg;<br>
 *        ra8876lite.setBackgroundColor(bg.Red);   //set red as the background color<br>
 *        delay(1000);<br>
 *        ra8876lite.setBackgroundColor(bg.Green); <br>
 *        delay(1000);<br>
 *        ra8876lite.setBackgroundColor(bg.Blue); <br>
 *        No background transparency is supported for RA8876 therefore alpha is always = 255<br>
 *        MSB for each component is used <br>
 *        Example, color.r[7:5] is used for component red in 8BPP<br>
 *        color.g[7:5] used for component green in 16BPP.
 */
void Ra8876_Lite::setBackgroundColor(Color color)
{
  uint8_t _mode = getColorMode();

  switch(_mode)
  {
    case COLOR_8BPP_RGB332:   //RGB in 3:3:2
          lcdRegDataWrite(RA8876_BGCR,(color.r&0xE0)); //d5h[7:5]
          lcdRegDataWrite(RA8876_BGCG,(color.g&0xE0)); //d6h[7:5]
          lcdRegDataWrite(RA8876_BGCB,(color.b&0xC0)); //d7h[7:6]
          break;
    case COLOR_24BPP_RGB888:  //RGB in 8:8:8
          lcdRegDataWrite(RA8876_BGCR,color.r); //d5h[7:0]
          lcdRegDataWrite(RA8876_BGCG,color.g); //d6h[7:0]
          lcdRegDataWrite(RA8876_BGCB,color.b); //d7h[7:0]
          break;
    default:  //RGB in 5:6:5
          lcdRegDataWrite(RA8876_BGCR,(color.r&0xF8));//d5h[7:3]
          lcdRegDataWrite(RA8876_BGCG,(color.g&0xFC));//d6h[7:2]
          lcdRegDataWrite(RA8876_BGCB,(color.b&0xF8));//d7h[7:3]
  }    
}

/**
 * @brief 	Set graphic mode ON or OFF
 * @param	on is a boolean flag to set <br>
 *			'true' or '1' for Graphic Mode
 *			'false' or '0' for Text Mode
 */
 void Ra8876_Lite::graphicMode(bool on)
 {
  if(on)
   lcdRegDataWrite(RA8876_ICR,RA8876_GRAPHIC_MODE<<2|RA8876_MEMORY_SELECT_IMAGE);//03h  //switch to graphic mode
  else
   lcdRegDataWrite(RA8876_ICR,RA8876_TEXT_MODE<<2|RA8876_MEMORY_SELECT_IMAGE);//03h  //switch back to text mode
 }


 /**
 * @brief Put pixel at (x,y) with color object of components (r,g,b,a)
 * @param x,y indicate the pixel coordinates
 * @param color in r,g,b components
 * @param lnOffset is the canvas start address represented in line number
 */
void Ra8876_Lite::putPixel(uint16_t x, uint16_t y, Color color, uint32_t lnOffset)
{
	setPixelCursor(x,y,lnOffset);
	ramAccessPrepare(); 
  
  switch(_colorMode)
  {
    case COLOR_8BPP_RGB332:
          lcdDataWrite((color.r&0xE0) | (color.g&0xE0)>>3 | (color.b&0xC0)>>6);
        break;
    case COLOR_24BPP_RGB888:
          lcdDataWrite(color.b);
          lcdDataWrite(color.g);
          lcdDataWrite(color.r);
        break;
	case COLOR_6BPP_ARGB2222:
		  lcdDataWrite((color.a&0x06)<<5 | (color.r&0xC0)>>2 | (color.g&0xC0)>>4 | (color.b&0xC0)>>6);
		break;
    case COLOR_12BPP_ARGB4444:
          lcdDataWrite((color.g&0xF0) | (color.b&0xF0)>>4);
          lcdDataWrite((color.a&0x0F)<<4 | (color.r&0xF0)>>4);
        break;
    default:  //case COLOR_16BPP_RGB565:
          lcdDataWrite((color.g&0x1C)<<3 | (color.b&0xF8)>>3);
          lcdDataWrite((color.r&0xF8) | (color.g&0xE0)>>5);
  }
}

/**
 * @brief Set cursor position and active window before data write to SDRAM (Canvas).
 * @param x,y is the top left corner coordinates.
 * @param width, height indicate the dimension in pixels.
 * @param lnOffset indicates the Canvas Address offset in line number.
 * @note  Coordinates (x,y) are relative to the Canvas Start Address. 
 *		  This function is required before writing pixels to SDRAM.
 */
void Ra8876_Lite:: putPicture_set_frame(uint16_t x,uint16_t y,uint16_t width, uint16_t height, uint32_t lnOffset)
{	
	activeWindowXY(x,y);
	activeWindowWH(width,height);
	setPixelCursor(x,y, lnOffset);
	
	ramAccessPrepare();
}

/**
 * @brief	This function converts line number to physical address for Canvas.
 * @param	lnOffset indicates line offset position.
 * @return	Canvas Address or -1 if lnOffset out-of-range.
 *			
 */
int32_t Ra8876_Lite::canvasAddress_from_lnOffset(uint32_t lnOffset)
{
	int32_t _canvasAddress = -1;
	
	uint8_t bpp = getColorDepth();
	
	if(lnOffset <= (uint32_t)(MEM_SIZE_MAX/(_canvasWidth*bpp)-1))
	{
		_canvasAddress = lnOffset*_canvasWidth*bpp;
	}
	
	return _canvasAddress;
}

/**
 * @brief Draw picture from a static 8-bit data array from Flash.
 * @param x,y is the top left corner coordinates. Max x,y = 8192.
 * @param width, height indicate the dimension in pixels.
 * @param *data is a void pointer to a static array of 8-bit or 16-bit data.
 * @note  Data cast please refer to the note section of function canvasWrite(args).
 * @param rotate_ccw90 is a boolean flag to rotate a single character in counter clockwise 90 degrees.
 * @param lnOffset indicates line offset position.
 * @note  Default value of lnOffset = CANVAS_OFFSET which is the starting address of the Main Display Window.
 *		  One may override this parameter with a non-display line number (e.g. lnOffset = 721) to 
 *		  write an image to some off-screen area. This can avoid a "snail-like" pixel rastering to be 
 *		  visible. This is particularly important for slow MCUs like Arduino or mbed as we are using
 * 		  only a slow SPI bus (4MHz in some case!) with these platforms. After rendering is completed,
 *		  a 2D function bteMemoryCopyWithROP(args) can be used to copy the data from non-display region
 *		  to the Main Display Window with a single instruction. 
 *		  It is also important to notice that coordinates x,y are relative to Canvas Starting Address 
 *		  thus y is relative to lnOffset. For example, setting y=10 with lnOffset = 721 will draw 
 *		  data starting from the line 731.
 *		  
 */
void Ra8876_Lite:: putPicture(	uint16_t x, uint16_t y,
								uint16_t width, uint16_t height, 
								const void* data, 
								bool rotate_ccw90,
								uint32_t lnOffset)
{
	uint16_t _x=x, _y=y, _width=width, _height=height;
	uint8_t MACR;
/*	
	int32_t _canvasAddress = canvasAddress_from_lnOffset(lnOffset);
	if(_canvasAddress<0) return;
	
	canvasImageStartAddress(_canvasAddress);
*/
	///Important::need to set RA8876_MACR prior to putPicture_set_frame() otherwise it doesn't work!
	///Change the host to Memory write direction 
	if(rotate_ccw90){
		_width = height; _height = width;
		_x = y; _y = x;
		MACR = lcdRegDataRead(RA8876_MACR); //read REG[02h]
		lcdRegDataWrite(RA8876_MACR, MACR|0x06);  		
	}	
	//coordinates (x,y) is relative to the canvas address with max x,y=8192
	putPicture_set_frame(_x,_y,_width,_height,lnOffset);
	
	size_t _length = (size_t)width*height*getColorDepth();	//bpp=1 for 8BPP, =2 for 16BPP, and =3 for 24BPP
	hal_spi_write((const uint8_t*)data, _length);			//push data via SPI port with different data length for each BPP

	if(rotate_ccw90)
	{
		lcdRegDataWrite(RA8876_MACR, MACR);  
	}
	
	//Active window restore
	activeWindowXY(ACTIVE_WINDOW_STARTX,ACTIVE_WINDOW_STARTY);
	activeWindowWH(_canvasWidth,_canvasHeight);
	canvasImageStartAddress(CANVAS_OFFSET);
}

#if defined (LOAD_SD_LIBRARY)
/**
 * @brief Draw picture from a RAiO binary file. A file system with SD.h is assumed here.
 * @param x,y is the top left corner coordinates. Max x,y = 8192.
 * @param width, height indicate the dimension in pixels.
 * @param *pFilename is a pointer to the filename.
 * @param rotate_ccw90 is a boolean flag to rotate a single character in counter clockwise 90 degrees.
 * @param lnOffset indicates Canvas Address offset in line number.
 * @note  This is a blocking function as we are reading and writing from an external media (SD card) to SDRAM pixel-by-pixel.
 */
void Ra8876_Lite::putPicture(	uint16_t x, uint16_t y,
								uint16_t width, uint16_t height, 
								const char *pFilename, 
								bool rotate_ccw90,
								uint32_t lnOffset)
{
	uint16_t _x=x, _y=y, _width=width, _height=height;
	uint8_t MACR;
	
	File gfxFile = SD.open(pFilename);
	
	if(!gfxFile) return;
			
	int32_t _canvasAddress = canvasAddress_from_lnOffset(lnOffset);
	if(_canvasAddress<0) return;
	
	canvasImageStartAddress(_canvasAddress);
	///Important::need to set RA8876_MACR prior to putPicture_set_frame() otherwise it doesn't work!
	///Change the host to Memory write direction 
	if(rotate_ccw90){
		_width = height; _height = width;
		_x = y; _y = x;
		MACR = lcdRegDataRead(RA8876_MACR); //read REG[02h]
		lcdRegDataWrite(RA8876_MACR, MACR|0x06);  	///Change to bottom->top, left->right scanning	
	}
	//coordinates (x,y) is relative to the canvas address with max x,y=8192
	putPicture_set_frame(_x,_y,_width,_height,lnOffset);
			
	uint16_t _ln = width*getColorDepth();
	uint8_t lnBuffer[_ln];	//Mind its size for MCU with low SRAM(e.g. Arduino M0). A picture of 1280 pixels (say) width in 24BPP needs
							//1280*3 bytes for this array. Just in case this module is not working, adjust the line buffer size by reducing _ln
	while(gfxFile.available())
	{
		gfxFile.read((uint8_t *)lnBuffer, _ln);
		hal_spi_write(lnBuffer, _ln);
	}
	
	if(rotate_ccw90)
	{
		lcdRegDataWrite(RA8876_MACR, MACR);  	//restore left->right, top->bottom scanning
	}
	/*
	//This is less efficient
	uint8_t bpp = getColorDepth();
	switch(bpp)
	{
		case 1:
			while(gfxFile.available()){
			lcdDataWrite((uint8_t)gfxFile.read());	}
			break;
		case 2:
			while(gfxFile.available()){
			lcdDataWrite((uint8_t)gfxFile.read());lcdDataWrite((uint8_t)gfxFile.read()); }
			break;
		case 3:
			while(gfxFile.available()){
			lcdDataWrite((uint8_t)gfxFile.read());lcdDataWrite((uint8_t)gfxFile.read());lcdDataWrite((uint8_t)gfxFile.read());}
			break;
	}
	*/
	gfxFile.close();
	
	//Active Window restore
	activeWindowXY(ACTIVE_WINDOW_STARTX,ACTIVE_WINDOW_STARTY);
	activeWindowWH(_canvasWidth,_canvasHeight);
	canvasImageStartAddress(CANVAS_OFFSET);
}
#endif
  
 /**
 * @brief 	Set text mode ON or OFF
 * @param	on is a boolean flag to set <br>
 *			'true' or '1' for Text Mode
 *			'false' or '0' for Graphic Mode
 * @note	This function has effect on hardware font engine (Embedded Characters/External Char ROM/CGRAM) ONLY.
 */
void Ra8876_Lite::textMode(bool on)
 { 
  if(on)
   lcdRegDataWrite(RA8876_ICR,RA8876_TEXT_MODE<<2|RA8876_MEMORY_SELECT_IMAGE);//03h  //switch to text mode
  else
   lcdRegDataWrite(RA8876_ICR,RA8876_GRAPHIC_MODE<<2|RA8876_MEMORY_SELECT_IMAGE);//03h  //switch back to graphic mode
 }
 
/**
 * @brief	Screen rotation to transform x,y coordinates by clockwise 90 deg
 */
void Ra8876_Lite::rotateCw90(uint16_t *x, uint16_t *y)
{
	uint16_t _x = *x;
	*x = getCanvasWidth()-*y;
	*y = _x;
}

/**
 * @brief	Screen rotation to transform x,y coordinates by counter-clockwise 90 deg
 */
void Ra8876_Lite::rotateCcw90(uint16_t *x, uint16_t *y)
{
	uint16_t _y = *y;
	*y = getCanvasHeight()-*x;
	*x = _y;	
}

/**
 * @brief 	Set text color for Embedded Characters/External Genitop ROM/CGRAM Font.
 * @param 	text_color  is a color object composed of 4 components: red, green, blue, alpha.
 * @note	Global foreground color is set to the text_color after this function.
 */
 void Ra8876_Lite::setHwTextColor(Color text_color)
 {
  setForegroundColor(text_color);
 }
 
/**
 * @brief Set text color with background and foreground color for Embedded Characters/External Genitop ROM/CGRAM Font
 * @param foreground_color  is a color object composed of 4 components: red, green, blue, alpha
 * @param background_color  is a color object composed of 4 components: red, green, blue, alpha
 */
 void Ra8876_Lite::setHwTextColor(Color foreground_color, Color background_color)
 {
  setForegroundColor(foreground_color);
  setBackgroundColor(background_color);
 }
 
/**
 * @brief	Set Text Write coordinates for Embedded Characters/External Genitop ROM/CGRAM Font
 * @param	(x,y) is the coordinates
 * @param	lnOffset is the canvas start address represented in line number.
 * @note	Example:<br>
 *			ra8876lite.setHwTextCursor(100,50); //set text print starting from x,y=100,50<br>
 *
 *			Example:<br>
 *			ra8876lite.setHwTextCursor(100,50,720); //set text starting from x,y=100,770 to print on an off-screen region (suppose 1280x720 monitor)<br>
 */
void Ra8876_Lite:: setHwTextCursor(uint16_t x, uint16_t y, uint32_t lnOffset)
{
  //uint8_t bpp = getColorDepth();
  //if(lnOffset > (uint32_t)(MEM_SIZE_MAX/(_canvasWidth*bpp)-1)) return;
  //canvasImageStartAddress(lnOffset*_canvasWidth*bpp);
  
  int32_t _canvasAddress = canvasAddress_from_lnOffset(lnOffset);
  if(_canvasAddress<0) return;
  canvasImageStartAddress(_canvasAddress);
  
  ///activeWindowXY(0,0);
  
  lcdRegDataWrite(RA8876_F_CURX0,x); //63h
  lcdRegDataWrite(RA8876_F_CURX1,x>>8);//64h
  lcdRegDataWrite(RA8876_F_CURY0,y);//65h
  lcdRegDataWrite(RA8876_F_CURY1,y>>8);//66h
}

 /**
  * @brief	Set Character Control Register 0 (CCR0) at REG[CCh] for hardware fonts (Embedded Characters/External Genitop ROM/CGRAM Font)
  * @note	RA8876 supports three text sources. They are embedded character set, external font ROM, & user defined character.
  * @param	source_select is enumerated by FONT_SRC in hw_font.h <br>  
				enum FONT_SRC {
					INTERNAL_CGROM =0,
					GENITOP_FONT   =1,
					CUSTOM_CGRAM   =2
				};
  * @param	size_select is enumerated by FONT_HEIGHT in hw_font.h <br>
				enum FONT_HEIGHT {
					 CHAR_HEIGHT_16=0,
					 CHAR_HEIGHT_24=1,
					 CHAR_HEIGHT_32=2
				 };
  * @param	iso_select sets which coding standard to support for internal CGROM only.
  *			iso_select = 0 for ISO/IEC 8859-1<br>
  *			iso_select = 1 for ISO/IEC 8859-2<br>
  *			iso_select = 4 for ISO/IEC 8859-4<br>
  *			iso_select = 5 for ISO/IEC 8859-5<br>
  *			These standards differ only in character codes starting from 0xA0 to 0xFF.
  *			ASCII character map for 0x20(space) to 0x73 (~) is the same.
  */
 void Ra8876_Lite::setHwTextParameter1(FONT_SRC source_select, FONT_HEIGHT size_select, FONT_CODE iso_select)//cch
 {
   lcdRegDataWrite(RA8876_CCR0,source_select<<6|size_select<<4|iso_select);//cch
 }
 
 /**
  * @brief	Set Character Control Register 1 (CCR1) at REG[CDh] for hardware fonts (Embedded Characters/External Genitop ROM/CGRAM Font)
  * @param	align enable (1) or disable (0) full character alignment. What is the difference???
  * @param	chroma_key controls how a character is displayed when<br>
  *			0 : the character's background filled with the background color, 
  *				which is a global parameter set in REG[D5h]-REG[D7h]
  *			1 : the character's background filled with the canvas background.
  *				This allows a string to be drawn on a graphical background.
  *	@param	width_enlarge sets the width enlargement factor from 1~4 for (x1 to x4)
  * @param	height_enlarge sets the height enlargement factor from 1~4 for (x1 to x4)
  */
void Ra8876_Lite::setHwTextParameter2(uint8_t align, bool chroma_key, uint8_t width_enlarge, uint8_t height_enlarge, bool rotate_ccw90)
{
	uint8_t _wFactor = constrain(width_enlarge, 1, 4);	//bound magnification factor within 1~4 inclusive.
	uint8_t _hFactor = constrain(height_enlarge,1, 4);
	_wFactor--;	//it is value 0~3 to be entered into REG[CDh]
	_hFactor--;
	
	if(rotate_ccw90)
		lcdRegDataWrite(RA8876_CCR1,align<<7|chroma_key<<6|RA8876_TEXT_ROTATION<<4|_wFactor<<2|_hFactor);//cdh
	else
		lcdRegDataWrite(RA8876_CCR1,align<<7|chroma_key<<6|_wFactor<<2|_hFactor);//cdh
}

/**
 * @brief	Parameter setup for Genitop external font ROM.
 * @param	coding is an enum stating all possible codings. Refer to enum FONT_CODE in hw_font.h.
 * @param	gt_width control the GT character width with 2-bit value from 0 to 3.
 *			Relationship of character sets and GT character width please refer to a table on page 199 of datasheet.
 * @param	part_no is the Genitop font IC model. It defaults to FONT_ROM_GT21L16T1W matching our hardware.
 * @param	scs_select indicates which pin to wire up for chip select from XnSFCS0 (pin 37) or XnSFCS1 (pin 38). Hardware design fix it to XnSFCS0.
 */
void Ra8876_Lite::genitopCharacterRomParameter(FONT_CODE coding, uint8_t gt_width, GT_FONT_ROM part_no, uint8_t scs_select)
{ 
  lcdRegDataWrite(RA8876_SFL_CTRL,scs_select<<7|RA8876_SERIAL_FLASH_FONT_MODE<<6|RA8876_SERIAL_FLASH_ADDR_24BIT<<5|RA8876_FOLLOW_RA8876_MODE<<4|RA8876_SPI_FAST_READ_8DUMMY);//b7h
  lcdRegDataWrite(RA8876_SPI_DIVSOR,1); //REG[BBh], Fsck = Fcore/((divisor+1)*2), Max Fsck for FONT_ROM_GT21L16T1W is 30MHz, Fcore  =120MHz, setting divisor 2 is safe 
  lcdRegDataWrite(RA8876_GTFNT_SEL,(uint8_t)part_no<<5); //REG[CEh]
  lcdRegDataWrite(RA8876_GTFNT_CR, ((uint8_t)coding<<3)|gt_width); //REG[CFh]
}

/**
 * @brief	This is the API function to set various parameters prior to printing hardware texts.
 * @param	background_color sets the global background color by setting register values in REG[D5h]-REG[D7h].<br>
 *			If background_color==Transparent, the character's background will be filled with the canvas background.<br>
 *			This allows a character to be drawn on a graphical background.
 * @param	width_enlarge sets the width enlargement factor from 1~4 for (x1 to x4)
 * @param	height_enlarge sets the height enlargement factor from 1~4 for (x1 to x4)
 * @param	rotate_ccw90 rotates the whole screen 90 degrees in counter clockwise direction.<br>
 *			Use this option with care because the whole screen is laterally inverted (mirror image) when this option set to true.
 * @note	Example:<br>
 *			sf::Color color;
 *			ra8876lite.setHwTextParam(color.Transparent, 2,2);	///Keep original canvas color with width & height magnified by factor 2<br>
 *			ra8876lite.setHwTextColor(color.Black);
 *			ra8876lite.setHwTextCursor(100,100);
 *			ra8876lite.putHwString(&ICGROM_16, "Hello World");
 */
void Ra8876_Lite::setHwTextParam(Color background_color, uint8_t width_enlarge, uint8_t height_enlarge, bool rotate_ccw90)
{
	bool _chroma_key;
	
	if(background_color==background_color.Transparent)
	{
		_chroma_key=TRANSPARENT;
	}
	else
	{
		_chroma_key=SOLID;
		setBackgroundColor(background_color);
	}		
	
	if(rotate_ccw90)
	{
		setHwTextParameter2(1, _chroma_key, width_enlarge,height_enlarge,true);
		uint8_t DPCR = lcdRegDataRead(RA8876_DPCR);
		lcdRegDataWrite(RA8876_DPCR,DPCR|0x08);	//set VDIR bit 1 to laterally inverted the whole screen
	}
	else
	{
		setHwTextParameter2(1, _chroma_key, width_enlarge,height_enlarge);
	}
}

/**
 * @brief	This function prints a single character at a position predefined with setHwTextCursor().
 * @param	*pFont points to a structure HW_FONT defined in hw_font.h.
 * @param	ch is the constant character to be printed in 8-bit width.
 * @note	Example: <br>
 *
			  ra8876lite.canvasClear(color.Yellow);   	//clear canvas in yellow
			  //Example to display ASCII from Embedded Character Set of 8x16 pixels starting from 0x00 to 0xff
			  //character starts from x=100, y=50; auto line feed when cursor meet Active Window boundary
			  ra8876lite.setHwTextCursor(100,50);  		
			  ra8876lite.setHwTextColor(color.Blue);  
			  ra8876lite.setHwTextParam(color.Transparent, 1,1);
			  for(char ch=0x00; ch<0xff; ch++)
			  { 
				ra8876lite.putHwChar(&ICGROM_16, ch);
			  }  			
 */
void Ra8876_Lite::putHwChar(const HW_FONT *pFont, const char ch)
{
	if((uint16_t)ch < pFont->FirstChar || (uint16_t)ch > pFont->LastChar) return;

	setHwTextParameter1(pFont->FontSource, (FONT_HEIGHT)pFont->FontHeight, pFont->FontCode);

	if(pFont->FontSource == GENITOP_FONT)
	{
		if(pFont->FontWidth!=0)	//Fixed width
			genitopCharacterRomParameter(pFont->FontCode, 0);
		else	//variable width
			genitopCharacterRomParameter(pFont->FontCode, 1);
	}
	
	textMode(true);
	ramAccessPrepare();
	checkWriteFifoNotFull();  
	lcdDataWrite(ch);
	check2dBusy();
	textMode(false);	
}

/**
 * @brief	This function overloads putHwChar(const HW_FONT *pFont, const char ch) with uint16_t as the second parameter for wide characters such as BIG5.
 * @note	Example : <br>
 *
			  //0x0401 means to start from 04区 01点 displaying ぁあぃい...for 300 characters
			  //change wch starting value to 0x0501 will display ァアィイ...
			  //Reference: http://charset.7jp.net/jis0208.html
			  //print wide characters starting from x=100,y=150 with auto line feed when cursor meets Active Window boundary
			  ra8876lite.setHwTextCursor(100,150);
			  ra8876lite.setHwTextColor(color.White);
			  ra8876lite.setHwTextParam(color.Black, 1,1);
			  for(wch=0x0401; wch<(0x0401+301); wch++)
			  {
				ra8876lite.putHwChar(&XCGROM_JIS_16, wch);     
			  } 
 */
void Ra8876_Lite::putHwChar(const HW_FONT *pFont,const uint16_t ch)
{
	if(ch < pFont->FirstChar || ch > pFont->LastChar) return;
	
	setHwTextParameter1(pFont->FontSource, (FONT_HEIGHT)pFont->FontHeight, pFont->FontCode);
	
	if(pFont->FontSource == GENITOP_FONT)
	{
		if(pFont->FontWidth!=0)	//Fixed width
			genitopCharacterRomParameter(pFont->FontCode, 0);
		else	//variable width
			genitopCharacterRomParameter(pFont->FontCode, 1);
	}
	textMode(true);
	ramAccessPrepare();
	checkWriteFifoNotFull(); 
	lcdDataWrite((uint8_t)(ch>>8));
	lcdDataWrite((uint8_t)ch);
	check2dBusy();
	textMode(false);	
}

/**
 * @brief	Print a string from internal CG RAM or external font ROM.
 * @param	*pFont points to a structure HW_FONT defined in hw_font.h.
 * @param	*str pointer to a constant string.
 * @note	Its text color, the background color, magnification factor, and the cursor position must be set prior to running this.<br>
 *			Example:<br>
 *
			long vsyncTime=0, time_s=0, time_e=0;  
			String result;
			
			time_s=time_e=millis();
			ra8876lite.vsyncWait();
			ra8876lite.bteSolidFill(0,random(0,ra8876lite.getCanvasWidth()),random(100,ra8876lite.getCanvasHeight()),random(255),random(255),color);
			time_e= millis();
			result = String(float(time_e-time_s), 2) + "ms";
			ra8876lite.setHwTextColor(color.White);     //set text color white
			ra8876lite.setHwTextCursor(500,40);         //set cursor prior to running ra8876lite.putHwString()
			ra8876lite.setHwTextParam(color.Black,2,3); //set background black with magnified font         
			ra8876lite.putHwString(&ICGROM_16, result);			
 */
void Ra8876_Lite:: putHwString(const HW_FONT *pFont, const char *str)
{ 
  setHwTextParameter1(pFont->FontSource, (FONT_HEIGHT)pFont->FontHeight, pFont->FontCode);
  
	if(pFont->FontSource == GENITOP_FONT)
	{
		if(pFont->FontWidth!=0)	//Fixed width
			genitopCharacterRomParameter(pFont->FontCode, 0);
		else	//variable width
			genitopCharacterRomParameter(pFont->FontCode, 1);
	}
	
  textMode(true);
  ramAccessPrepare();
  while(*str != '\0')
  {
	checkWriteFifoNotFull();  
	lcdDataWrite(*str++);
  } 
  check2dBusy();
  textMode(false);
}

/**
 * @brief	This function overloads putHwString(const HW_FONT *pFont, const char *str) with uint16_t as the second parameter for wide characters such as BIG5.
 */
void Ra8876_Lite:: putHwString(const HW_FONT *pFont, const uint16_t *str)
{ 
	setHwTextParameter1(pFont->FontSource, (FONT_HEIGHT)pFont->FontHeight, pFont->FontCode);
  
	if(pFont->FontSource == GENITOP_FONT)
	{
		if(pFont->FontWidth!=0)	//Fixed width
			genitopCharacterRomParameter(pFont->FontCode, 0);
		else	//variable width
			genitopCharacterRomParameter(pFont->FontCode, 1);
	}
	
  textMode(true);
  ramAccessPrepare();
  while(*str != '\0')
  {
	  checkWriteFifoNotFull();  
	  lcdDataWrite((uint8_t)(*str>>8));
	  lcdDataWrite((uint8_t)*str);
	  str++;
  } 
  check2dBusy();
  textMode(false);
}

/**
 * @brief Draw line function with hardware acceleration  
 * @param x0 is the starting x-coordinate
 * @param y0 is the starting y-coordinate
 * @param x1 is the ending x-coordinate
 * @param y1 is the ending y-coordinate
 * @param color is an object composed of 4 components: red, green, blue, alpha
 */
void Ra8876_Lite::drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DLHSR0,x0);//68h
  lcdRegDataWrite(RA8876_DLHSR1,x0>>8);//69h
  lcdRegDataWrite(RA8876_DLVSR0,y0);//6ah
  lcdRegDataWrite(RA8876_DLVSR1,y0>>8);//6bh
  lcdRegDataWrite(RA8876_DLHER0,x1);//6ch
  lcdRegDataWrite(RA8876_DLHER1,x1>>8);//6dh
  lcdRegDataWrite(RA8876_DLVER0,y1);//6eh
  lcdRegDataWrite(RA8876_DLVER1,y1>>8);//6fh        
  lcdRegDataWrite(RA8876_DCR0,RA8876_DRAW_LINE);//67h,0x80
  check2dBusy();
}

/**
 * @brief Draw rectangle function with hardware acceleration
 * @param x0 is the starting x-coordinate
 * @param y0 is the starting y-coordinate
 * @param x1 is the ending x-coordinate
 * @param y1 is the ending y-coordinate
 * @param color is an object composed of 4 components: red, green, blue, alpha
 */
void Ra8876_Lite::drawSquare(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DLHSR0,x0);//68h
  lcdRegDataWrite(RA8876_DLHSR1,x0>>8);//69h
  lcdRegDataWrite(RA8876_DLVSR0,y0);//6ah
  lcdRegDataWrite(RA8876_DLVSR1,y0>>8);//6bh
  lcdRegDataWrite(RA8876_DLHER0,x1);//6ch
  lcdRegDataWrite(RA8876_DLHER1,x1>>8);//6dh
  lcdRegDataWrite(RA8876_DLVER0,y1);//6eh
  lcdRegDataWrite(RA8876_DLVER1,y1>>8);//6fh        
  lcdRegDataWrite(RA8876_DCR1,RA8876_DRAW_SQUARE);//76h,0xa0
  check2dBusy();
}

/**
 * @brief Draw square with a filled color with hardware acceleration
 * @param x0 is the starting x-coordinate
 * @param y0 is the starting y-coordinate
 * @param x1 is the ending x-coordinate
 * @param y1 is the ending y-coordinate
 * @param color is an object composed of 4 components: red, green, blue, alpha
 */
void Ra8876_Lite::drawSquareFill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DLHSR0,x0);//68h
  lcdRegDataWrite(RA8876_DLHSR1,x0>>8);//69h
  lcdRegDataWrite(RA8876_DLVSR0,y0);//6ah
  lcdRegDataWrite(RA8876_DLVSR1,y0>>8);//6bh
  lcdRegDataWrite(RA8876_DLHER0,x1);//6ch
  lcdRegDataWrite(RA8876_DLHER1,x1>>8);//6dh
  lcdRegDataWrite(RA8876_DLVER0,y1);//6eh
  lcdRegDataWrite(RA8876_DLVER1,y1>>8);//6fh        
  lcdRegDataWrite(RA8876_DCR1,RA8876_DRAW_SQUARE_FILL);//76h,0xa0 fill square with hardware acceleration
  check2dBusy();
}

/**
 * 
 */
void Ra8876_Lite::drawCircleSquare(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t xr, uint16_t yr, Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DLHSR0,x0);//68h
  lcdRegDataWrite(RA8876_DLHSR1,x0>>8);//69h
  lcdRegDataWrite(RA8876_DLVSR0,y0);//6ah
  lcdRegDataWrite(RA8876_DLVSR1,y0>>8);//6bh
  lcdRegDataWrite(RA8876_DLHER0,x1);//6ch
  lcdRegDataWrite(RA8876_DLHER1,x1>>8);//6dh
  lcdRegDataWrite(RA8876_DLVER0,y1);//6eh
  lcdRegDataWrite(RA8876_DLVER1,y1>>8);//6fh    
  lcdRegDataWrite(RA8876_ELL_A0,xr);//77h    
  lcdRegDataWrite(RA8876_ELL_A1,xr>>8);//79h 
  lcdRegDataWrite(RA8876_ELL_B0,yr);//7ah    
  lcdRegDataWrite(RA8876_ELL_B1,yr>>8);//7bh
  lcdRegDataWrite(RA8876_DCR1,RA8876_DRAW_CIRCLE_SQUARE);//76h,0xb0
  check2dBusy();
}

/**
 * 
 */
void Ra8876_Lite::drawCircleSquareFill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t xr, uint16_t yr, Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DLHSR0,x0);//68h
  lcdRegDataWrite(RA8876_DLHSR1,x0>>8);//69h
  lcdRegDataWrite(RA8876_DLVSR0,y0);//6ah
  lcdRegDataWrite(RA8876_DLVSR1,y0>>8);//6bh
  lcdRegDataWrite(RA8876_DLHER0,x1);//6ch
  lcdRegDataWrite(RA8876_DLHER1,x1>>8);//6dh
  lcdRegDataWrite(RA8876_DLVER0,y1);//6eh
  lcdRegDataWrite(RA8876_DLVER1,y1>>8);//6fh    
  lcdRegDataWrite(RA8876_ELL_A0,xr);//77h    
  lcdRegDataWrite(RA8876_ELL_A1,xr>>8);//78h 
  lcdRegDataWrite(RA8876_ELL_B0,yr);//79h    
  lcdRegDataWrite(RA8876_ELL_B1,yr>>8);//7ah
  lcdRegDataWrite(RA8876_DCR1,RA8876_DRAW_CIRCLE_SQUARE_FILL);//76h,0xf0
  check2dBusy();
}

/**
 * 
 */
void Ra8876_Lite::drawTriangle(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DLHSR0,x0);//68h
  lcdRegDataWrite(RA8876_DLHSR1,x0>>8);//69h
  lcdRegDataWrite(RA8876_DLVSR0,y0);//6ah
  lcdRegDataWrite(RA8876_DLVSR1,y0>>8);//6bh
  lcdRegDataWrite(RA8876_DLHER0,x1);//6ch
  lcdRegDataWrite(RA8876_DLHER1,x1>>8);//6dh
  lcdRegDataWrite(RA8876_DLVER0,y1);//6eh
  lcdRegDataWrite(RA8876_DLVER1,y1>>8);//6fh  
  lcdRegDataWrite(RA8876_DTPH0,x2);//70h
  lcdRegDataWrite(RA8876_DTPH1,x2>>8);//71h
  lcdRegDataWrite(RA8876_DTPV0,y2);//72h
  lcdRegDataWrite(RA8876_DTPV1,y2>>8);//73h  
  lcdRegDataWrite(RA8876_DCR0,RA8876_DRAW_TRIANGLE);//67h,0x82
  check2dBusy();
}

/**
 * 
 */
void Ra8876_Lite::drawTriangleFill(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DLHSR0,x0);//68h
  lcdRegDataWrite(RA8876_DLHSR1,x0>>8);//69h
  lcdRegDataWrite(RA8876_DLVSR0,y0);//6ah
  lcdRegDataWrite(RA8876_DLVSR1,y0>>8);//6bh
  lcdRegDataWrite(RA8876_DLHER0,x1);//6ch
  lcdRegDataWrite(RA8876_DLHER1,x1>>8);//6dh
  lcdRegDataWrite(RA8876_DLVER0,y1);//6eh
  lcdRegDataWrite(RA8876_DLVER1,y1>>8);//6fh  
  lcdRegDataWrite(RA8876_DTPH0,x2);//70h
  lcdRegDataWrite(RA8876_DTPH1,x2>>8);//71h
  lcdRegDataWrite(RA8876_DTPV0,y2);//72h
  lcdRegDataWrite(RA8876_DTPV1,y2>>8);//73h  
  lcdRegDataWrite(RA8876_DCR0,RA8876_DRAW_TRIANGLE_FILL);//67h,0xa2
  check2dBusy();
}

/**
 * 
 */
void Ra8876_Lite::drawCircle(uint16_t x0,uint16_t y0,uint16_t r,Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DEHR0,x0);//7bh
  lcdRegDataWrite(RA8876_DEHR1,x0>>8);//7ch
  lcdRegDataWrite(RA8876_DEVR0,y0);//7dh
  lcdRegDataWrite(RA8876_DEVR1,y0>>8);//7eh
  lcdRegDataWrite(RA8876_ELL_A0,r);//77h    
  lcdRegDataWrite(RA8876_ELL_A1,r>>8);//78h 
  lcdRegDataWrite(RA8876_ELL_B0,r);//79h    
  lcdRegDataWrite(RA8876_ELL_B1,r>>8);//7ah
  lcdRegDataWrite(RA8876_DCR1,RA8876_DRAW_CIRCLE);//76h,0x80
  check2dBusy();
}

/**
 * 
 */
void Ra8876_Lite::drawCircleFill(uint16_t x0,uint16_t y0,uint16_t r,Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DEHR0,x0);//7bh
  lcdRegDataWrite(RA8876_DEHR1,x0>>8);//7ch
  lcdRegDataWrite(RA8876_DEVR0,y0);//7dh
  lcdRegDataWrite(RA8876_DEVR1,y0>>8);//7eh
  lcdRegDataWrite(RA8876_ELL_A0,r);//77h    
  lcdRegDataWrite(RA8876_ELL_A1,r>>8);//78h 
  lcdRegDataWrite(RA8876_ELL_B0,r);//79h    
  lcdRegDataWrite(RA8876_ELL_B1,r>>8);//7ah
  lcdRegDataWrite(RA8876_DCR1,RA8876_DRAW_CIRCLE_FILL);//76h,0xc0
  check2dBusy();
}

/**
 * 
 */
void Ra8876_Lite::drawEllipse(uint16_t x0,uint16_t y0,uint16_t xr,uint16_t yr,Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DEHR0,x0);//7bh
  lcdRegDataWrite(RA8876_DEHR1,x0>>8);//7ch
  lcdRegDataWrite(RA8876_DEVR0,y0);//7dh
  lcdRegDataWrite(RA8876_DEVR1,y0>>8);//7eh
  lcdRegDataWrite(RA8876_ELL_A0,xr);//77h    
  lcdRegDataWrite(RA8876_ELL_A1,xr>>8);//78h 
  lcdRegDataWrite(RA8876_ELL_B0,yr);//79h    
  lcdRegDataWrite(RA8876_ELL_B1,yr>>8);//7ah
  lcdRegDataWrite(RA8876_DCR1,RA8876_DRAW_ELLIPSE);//76h,0x80
  check2dBusy();
}

/**
 * 
 */
void Ra8876_Lite::drawEllipseFill(uint16_t x0,uint16_t y0,uint16_t xr,uint16_t yr,Color color)
{
  setForegroundColor(color);
  lcdRegDataWrite(RA8876_DEHR0,x0);//7bh
  lcdRegDataWrite(RA8876_DEHR1,x0>>8);//7ch
  lcdRegDataWrite(RA8876_DEVR0,y0);//7dh
  lcdRegDataWrite(RA8876_DEVR1,y0>>8);//7eh
  lcdRegDataWrite(RA8876_ELL_A0,xr);//77h    
  lcdRegDataWrite(RA8876_ELL_A1,xr>>8);//78h 
  lcdRegDataWrite(RA8876_ELL_B0,yr);//79h    
  lcdRegDataWrite(RA8876_ELL_B1,yr>>8);//7ah
  lcdRegDataWrite(RA8876_DCR1,RA8876_DRAW_ELLIPSE_FILL);//76h,0xc0
  check2dBusy();
}


/**
 * @brief Memory copy with ROP
 * @note  This function will copy two specific areas (s0 and/or s1) of the SDRAM
 * to a destination area of the SDRAM with one of the 16 possible raster operation (ROP).
 * BTE Operation in REG[91h]Bits[3:0] = 2 (RA8876_BTE_MEMORY_COPY_WITH_ROP)
 * Example to use:
 * 
 * @param s0_addr is the physical address of the source image s0 in SDRAM 
 * @param s0_image_width is the source image width
 * @param s0_x is the x-coordinate of the source image
 * @param s0_y is the y-coordinate of the source image
 * @param s1_addr is the physical address of the source image s1 in SDRAM 
 * @param s1_image_width is the source image width
 * @param s1_x is the x-coordinate of the source image
 * @param s1_y is the y-coordinate of the source image
 * @param des_addr is the physical address of the destination image in SDRAM
 * @param des_image_width is the image width of the destination image
 * @param des_x is the x-coordinate of the destination image
 * @param des_y is the y-coordinate of the destination image
 * @param copy_width is the image width to copy starting from des_x
 * @param copy_height is the image height to copy starting from des_y
 * @param rop_code filling REG[91h]Bit[7:4] to set one of the 16 possible operation codes<br>
 *  RA8876_BTE_ROP_CODE_0     0   //0 ( Blackness )<br>
 *  RA8876_BTE_ROP_CODE_1     1   //~S0*~S1 or ~ ( S0+S1 )<br>
 *  RA8876_BTE_ROP_CODE_2     2   //~S0*S1<br>
 *  RA8876_BTE_ROP_CODE_3     3   //~S0<br>
 *  RA8876_BTE_ROP_CODE_4     4   //S0*~S1<br>
 *  RA8876_BTE_ROP_CODE_5     5   //~S1<br>
 *  RA8876_BTE_ROP_CODE_6     6   //S0^S1<br>
 *  RA8876_BTE_ROP_CODE_7     7   //~S0+~S1 or ~ ( S0*S1 )<br>
 *  RA8876_BTE_ROP_CODE_8     8   //S0*S1<br>
 *  RA8876_BTE_ROP_CODE_9     9   //~ ( S0^S1 ) <br>
 *  RA8876_BTE_ROP_CODE_10    10  //S1<br>
 *  RA8876_BTE_ROP_CODE_11    11  //~S0+S1<br>
 *  RA8876_BTE_ROP_CODE_12    12  //S0<br>
 *  RA8876_BTE_ROP_CODE_13    13  //S0+~S1<br>
 *  RA8876_BTE_ROP_CODE_14    14  //S0+S1<br>
 *  RA8876_BTE_ROP_CODE_15    15  //1 ( Whiteness )<br>
 */
void Ra8876_Lite::bteMemoryCopyWithROP( uint32_t s0_addr,
                                        uint16_t s0_image_width,
                                        uint16_t s0_x,uint16_t s0_y,
                                        uint32_t s1_addr,
                                        uint16_t s1_image_width,
                                        uint16_t s1_x,uint16_t s1_y,
                                        uint32_t des_addr,
                                        uint16_t des_image_width, 
                                        uint16_t des_x,uint16_t des_y,
                                        uint16_t copy_width,uint16_t copy_height, uint8_t rop_code)
{
  bte_Source0_MemoryStartAddr(s0_addr);
  bte_Source0_ImageWidth(s0_image_width);
  bte_Source0_WindowStartXY(s0_x,s0_y);
  bte_Source1_MemoryStartAddr(s1_addr);
  bte_Source1_ImageWidth(s1_image_width);
  bte_Source1_WindowStartXY(s1_x,s1_y);
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);
  bte_WindowSize(copy_width,copy_height);
  lcdRegDataWrite(RA8876_BTE_CTRL1,rop_code<<4|RA8876_BTE_MEMORY_COPY_WITH_ROP);//91h
  
/*
  uint8_t _colorDepth = getColorDepth();	//depth_bpp is depth in byte per pixel
  
  if(_colorDepth==1)
  lcdRegDataWrite(RA8876_BTE_COLR,RA8876_S0_COLOR_DEPTH_8BPP<<5|RA8876_S1_COLOR_DEPTH_8BPP<<2|RA8876_DESTINATION_COLOR_DEPTH_8BPP);//92h
  else if (_colorDepth==3)
  lcdRegDataWrite(RA8876_BTE_COLR,RA8876_S0_COLOR_DEPTH_24BPP<<5|RA8876_S1_COLOR_DEPTH_24BPP<<2|RA8876_DESTINATION_COLOR_DEPTH_24BPP);//92h
  else
  lcdRegDataWrite(RA8876_BTE_COLR,RA8876_S0_COLOR_DEPTH_16BPP<<5|RA8876_S1_COLOR_DEPTH_16BPP<<2|RA8876_DESTINATION_COLOR_DEPTH_16BPP);//92h
*/
 
  lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4);//90h
  check2dBusy();
} 

/**
 * @brief Memory copy with Chroma Key
 * @note  The source and destination data belong to different areas of the same SDRAM.
 * When the source0(S0) color is equal to a key color, which is defined in BTE Background Color Register,
 * the destination area is not updated. No raster operation (ROP) is applied for this operation.<br>
 * BTE Operation in REG[91h]Bits[3:0] = 5 (RA8876_BTE_MEMORY_COPY_WITH_CHROMA)
 * Example to use:
 * 
 * @param s0_addr is the physical address of the source image s0 in SDRAM 
 * @param s0_image_width is the source image width
 * @param s0_x is the x-coordinate of the source image
 * @param s0_y is the y-coordinate of the source image
 * @param des_addr is the physical address of the destination image in SDRAM
 * @param des_image_width is the image width of the destination image
 * @param des_x is the x-coordinate of the destination image
 * @param des_y is the y-coordinate of the destination image
 * @param copy_width is the image width to copy starting from des_x
 * @param copy_height is the image height to copy starting from des_y
 * @param chromakey_color is an object with r,g,b,a components.<br>
 * Individual components chromakey_color.r, chromakey_color.g, chromakey_color.b copied to BTE background color registers
 */
void Ra8876_Lite::bteMemoryCopyWithChromaKey( uint32_t s0_addr,
                                              uint16_t s0_image_width,
                                              uint16_t s0_x,uint16_t s0_y,
                                              uint32_t des_addr,
                                              uint16_t des_image_width,
                                              uint16_t des_x,uint16_t des_y,
                                              uint16_t copy_width,uint16_t copy_height,
                                              Color chromakey_color)
{
  bte_Source0_MemoryStartAddr(s0_addr);
  bte_Source0_ImageWidth(s0_image_width);
  bte_Source0_WindowStartXY(s0_x,s0_y);
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);
  bte_WindowSize(copy_width,copy_height);
  
  //backGroundColor16bpp(chromakey_color);  //replaced by setBackgroundColor() for 8BPP & 24BPP support
  setBackgroundColor(chromakey_color);  
  lcdRegDataWrite(RA8876_BTE_CTRL1,RA8876_BTE_MEMORY_COPY_WITH_CHROMA);//91h
/*
  if (_colorDepth==8)
  lcdRegDataWrite(RA8876_BTE_COLR,RA8876_S0_COLOR_DEPTH_8BPP<<5|RA8876_S1_COLOR_DEPTH_8BPP<<2|RA8876_DESTINATION_COLOR_DEPTH_8BPP);//92h
  else if (_colorDepth==24)
  lcdRegDataWrite(RA8876_BTE_COLR,RA8876_S0_COLOR_DEPTH_24BPP<<5|RA8876_S1_COLOR_DEPTH_24BPP<<2|RA8876_DESTINATION_COLOR_DEPTH_24BPP);//92h
  else
  lcdRegDataWrite(RA8876_BTE_COLR,RA8876_S0_COLOR_DEPTH_16BPP<<5|RA8876_S1_COLOR_DEPTH_16BPP<<2|RA8876_DESTINATION_COLOR_DEPTH_16BPP);//92h
*/  
  lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4);//90h
  check2dBusy();
}

/**
 * @brief
 * 
 */
void Ra8876_Lite::bteMpuWriteWithROP( uint32_t s1_addr,
                                      uint16_t s1_image_width,
                                      uint16_t s1_x,uint16_t s1_y,
                                      uint32_t des_addr,
                                      uint16_t des_image_width,
                                      uint16_t des_x,uint16_t des_y,
                                      uint16_t width,uint16_t height,
                                      uint8_t  rop_code,
                                      const uint8_t *data)
{
  uint16_t i,j;
  bte_Source1_MemoryStartAddr(s1_addr);
  bte_Source1_ImageWidth(s1_image_width);
  bte_Source1_WindowStartXY(s1_x,s1_y);
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);
  bte_WindowSize(width,height);
  lcdRegDataWrite(RA8876_BTE_CTRL1,rop_code<<4|RA8876_BTE_MPU_WRITE_WITH_ROP);//91h

  lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4);//90h
  ramAccessPrepare();

  //BLOCK MODE ONLY???
  for(i=0;i<height;i++)
  {	
   for(j=0;j<(width*2);j++)
   {
    //checkWriteFifoNotFull();
    lcdDataWrite(*data);
    data++;
    }
   }
  //checkWriteFifoEmpty();
}

//**************************************************************//
//**************************************************************//
void Ra8876_Lite::bteMpuWriteWithROP( uint32_t s1_addr,
                                      uint16_t s1_image_width,
                                      uint16_t s1_x,uint16_t s1_y,
                                      uint32_t des_addr,
                                      uint16_t des_image_width,
                                      uint16_t des_x,uint16_t des_y,
                                      uint16_t width,uint16_t height,
                                      uint8_t rop_code,
                                      const uint16_t *data)
{
  uint16_t i,j;
  bte_Source1_MemoryStartAddr(s1_addr);
  bte_Source1_ImageWidth(s1_image_width);
  bte_Source1_WindowStartXY(s1_x,s1_y);
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);
  bte_WindowSize(width,height);
  lcdRegDataWrite(RA8876_BTE_CTRL1,rop_code<<4|RA8876_BTE_MPU_WRITE_WITH_ROP);//91h

  lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4);//90h
  ramAccessPrepare();
  
 for(j=0;j<height;j++)
 {
  for(i=0;i<width;i++)
  {
   //checkWriteFifoNotFull();//if high speed mcu and without Xnwait check
   lcdDataWrite16bpp(*data);
   data++;
   //checkWriteFifoEmpty();//if high speed mcu and without Xnwait check
  }
 } 
  checkWriteFifoEmpty();
}

//**************************************************************//
//**************************************************************//
void Ra8876_Lite::bteMpuWriteWithChromaKey( uint32_t des_addr,
                                            uint16_t des_image_width,
                                            uint16_t des_x,uint16_t des_y,
                                            uint16_t width,uint16_t height,
                                            Color chromakey_color,
                                            const uint8_t *data)
{
  uint16_t i,j;
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);
  bte_WindowSize(width,height);

  //add support for 8BPP & 24BPP
  //backGroundColor16bpp(chromakey_color);
  setBackgroundColor(chromakey_color);
  lcdRegDataWrite(RA8876_BTE_CTRL1,RA8876_BTE_MPU_WRITE_WITH_CHROMA);//91h

  lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4);//90h
  ramAccessPrepare();
  
  for(i=0;i< height;i++)
  {	
   for(j=0;j< (width*2);j++)
   {
    //checkWriteFifoNotFull();
    lcdDataWrite(*data);
    data++;
    }
   }
  //checkWriteFifoEmpty();
}

//**************************************************************//
//**************************************************************//
void Ra8876_Lite::bteMpuWriteWithChromaKey( uint32_t des_addr,
                                            uint16_t des_image_width, 
                                            uint16_t des_x,uint16_t des_y,
                                            uint16_t width,uint16_t height,
                                            Color chromakey_color,
                                            const uint16_t *data)
{
  uint16_t i,j;
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);
  bte_WindowSize(width,height);

  //add suppot for 8BPP & 24BPP
  //backGroundColor16bpp(chromakey_color);
  setBackgroundColor(chromakey_color);
  
  lcdRegDataWrite(RA8876_BTE_CTRL1,RA8876_BTE_MPU_WRITE_WITH_CHROMA);//91h

  lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4);//90h
  ramAccessPrepare();
  
 for(j=0;j<height;j++)
 {
  for(i=0;i<width;i++)
  {
   //checkWriteFifoNotFull();//if high speed mcu and without Xnwait check
   lcdDataWrite16bpp(*data);
   data++;
   //checkWriteFifoEmpty();//if high speed mcu and without Xnwait check
  }
 } 
  //checkWriteFifoEmpty();
}

//**************************************************************//
//**************************************************************//
void Ra8876_Lite::bteMpuWriteColorExpansion(uint32_t des_addr,
                                            uint16_t des_image_width,
                                            uint16_t des_x,uint16_t des_y,
                                            uint16_t width,uint16_t height,
                                            Color foreground_color,
                                            Color background_color,
                                            const uint8_t *data)
{
  uint16_t i,j;
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);
  bte_WindowSize(width,height);

  //add support for 8BPP & 24BPP
  //foreGroundColor16bpp(foreground_color);
  //backGroundColor16bpp(background_color);
  setForegroundColor(foreground_color);
  setBackgroundColor(background_color);
  
  lcdRegDataWrite(RA8876_BTE_CTRL1,RA8876_BTE_ROP_BUS_WIDTH8<<4|RA8876_BTE_MPU_WRITE_COLOR_EXPANSION);//91h

  lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4);//90h
  ramAccessPrepare();
  
  for(i=0;i<height;i++)
  {	
   for(j=0;j<(width/8);j++)
   {
    //checkWriteFifoNotFull();
    lcdDataWrite(*data);
    data++;
    }
   }
  //checkWriteFifoEmpty();
  check2dBusy();
}

/**
 * @brief
 */
void Ra8876_Lite::bteMpuWriteColorExpansionWithChromaKey( uint32_t des_addr,
                                                          uint16_t des_image_width, 
                                                          uint16_t des_x,uint16_t des_y,
                                                          uint16_t width,uint16_t height,
                                                          Color foreground_color,
                                                          Color background_color, 
                                                          const uint8_t *data)
{
  /*background_color do not set the same as foreground_color*/
  if(foreground_color==background_color) return;
  
  uint16_t i,j;
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);
  bte_WindowSize(width,height);

  //add support for 8BPP & 24BPP
  //foreGroundColor16bpp(foreground_color);
  //backGroundColor16bpp(background_color);
  setForegroundColor(foreground_color);
  setBackgroundColor(background_color);
  
  lcdRegDataWrite(RA8876_BTE_CTRL1,RA8876_BTE_ROP_BUS_WIDTH8<<4|RA8876_BTE_MPU_WRITE_COLOR_EXPANSION_WITH_CHROMA);//91h
  
  lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4);//90h
  ramAccessPrepare();
  
  for(i=0;i<height;i++)
  {	
   for(j=0;j<(width/8);j++)
   {
    //checkWriteFifoNotFull();
    lcdDataWrite(*data);
    data++;
    }
   }
  //checkWriteFifoEmpty();
  check2dBusy();
}

//**************************************************************//
//**************************************************************//
void Ra8876_Lite:: btePatternFill(uint8_t p8x8or16x16, 
                                  uint32_t s0_addr,
                                  uint16_t s0_image_width,
                                  uint16_t s0_x,uint16_t s0_y,
                                  uint32_t des_addr,
                                  uint16_t des_image_width, 
                                  uint16_t des_x,uint16_t des_y,
                                  uint16_t copy_width,uint16_t copy_height)
{ 
  bte_Source0_MemoryStartAddr(s0_addr);
  bte_Source0_ImageWidth(s0_image_width);
  bte_Source0_WindowStartXY(s0_x,s0_y);
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);
  bte_WindowSize(copy_width,copy_height); 
  lcdRegDataWrite(RA8876_BTE_CTRL1,RA8876_BTE_ROP_CODE_12<<4|RA8876_BTE_PATTERN_FILL_WITH_ROP);//91h

  if(p8x8or16x16 == 0)
   lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4|RA8876_PATTERN_FORMAT8X8);//90h
  else
   lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4|RA8876_PATTERN_FORMAT16X16);//90h
   
  check2dBusy();
}
//**************************************************************//
//**************************************************************//
void Ra8876_Lite:: btePatternFillWithChromaKey( uint8_t p8x8or16x16, 
                                                uint32_t s0_addr,
                                                uint16_t s0_image_width,
                                                uint16_t s0_x,uint16_t s0_y,
                                                uint32_t des_addr,
                                                uint16_t des_image_width,
                                                uint16_t des_x,uint16_t des_y,
                                                uint16_t copy_width,uint16_t copy_height,
                                                Color chromakey_color)
{
  bte_Source0_MemoryStartAddr(s0_addr);
  bte_Source0_ImageWidth(s0_image_width);
  bte_Source0_WindowStartXY(s0_x,s0_y);
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);
  bte_WindowSize(copy_width,copy_height);

  //add support for 8BPP &24BPP
  //backGroundColor16bpp(chromakey_color); 
  setBackgroundColor(chromakey_color);
  
  lcdRegDataWrite(RA8876_BTE_CTRL1,RA8876_BTE_ROP_CODE_12<<4|RA8876_BTE_PATTERN_FILL_WITH_CHROMA);//91h

  if(p8x8or16x16 == 0)
   lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4|RA8876_PATTERN_FORMAT8X8);//90h
  else
   lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4|RA8876_PATTERN_FORMAT16X16);//90h
   
  check2dBusy();
}

/**
 * @brief This function blends s0 & s1 images to a destination in memory with an alpha level
 * @param s0_addr is the physical address of the source image in SDRAM 
 * @param s0_image_width is the source image width
 * @param s0_x is the x-coordinate of the upper left corner of the source image
 * @param s0_y is the y-coordinate of the upper left corner of the source image
 * @param des_addr is the physical address of the destination image in SDRAM
 * @param des_image_width is the image width of the destination image
 * @param des_x is the x-coordinate of the destination image
 * @param des_y is the y-coordinate of the destination image
 * @param copy_width is the image width
 * @param copy_height is the image height
 * @param 	alpha is the blending effect for S0 & S1 from 0 to 32<br>
 *        	Transparency is calculated with the formula alpha/32 in range 1.0 - 0.0<br>
 *        	alpha setting value = REG[B5h]/32 where 1.0 represent fully opaque color.
 * 		  	Output Effect = (S0 image x (1 - alpha setting value)) + (S1 image x alpha setting value)
 * @note	There are two modes supported with RA8876 for opacity, Picture Mode and Pixel Mode:
 *			Picture mode can be operated in 8bpp/16bpp/24bpp mode and only has one opacity value(alpha Level) 
 *			for whole picture.
 *			Pixel mode is only operated in 8bpp/16bpp mode and each pixel have its own opacity value.
 *			Only Picture mode is supported yet.
 */
void Ra8876_Lite::bteMemoryCopyWithOpacity(    
								uint32_t s0_addr,
                                uint16_t s0_image_width,
                                uint16_t s0_x, uint16_t s0_y,
                                uint32_t s1_addr,
                                uint16_t s1_image_width,
                                uint16_t s1_x, uint16_t s1_y,
                                uint32_t des_addr,
                                uint16_t des_image_width,
                                uint16_t des_x, uint16_t des_y,
                                uint16_t copy_width, uint16_t copy_height,
                                uint8_t  alpha)
{
  bte_Source0_MemoryStartAddr(s0_addr);
  bte_Source0_ImageWidth(s0_image_width);
  bte_Source0_WindowStartXY(s0_x,s0_y);   
  bte_Source1_MemoryStartAddr(s1_addr);
  bte_Source1_ImageWidth(s1_image_width);
  bte_Source1_WindowStartXY(s1_x,s1_y);
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(des_image_width);
  bte_DestinationWindowStartXY(des_x,des_y);

  bte_WindowSize(copy_width,copy_height);     
  lcdRegDataWrite(RA8876_BTE_CTRL1,RA8876_BTE_MEMORY_COPY_WITH_OPACITY);//91h 
  
  if(alpha>32) alpha=32;
  
  lcdRegDataWrite(RA8876_APB_CTRL, alpha);	//writing register B5h an alpha level, range 00h to 1Fh
  
  lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4);//90h
  check2dBusy();          
}

/*
void Ra8876_Lite::bteMpuWriteWithOpacity( uint32_t s1_addr,
                                          uint16_t s1_image_width,
                                          uint16_t s1_x, uint16_t s1_y,
                                          uint32_t des_addr,
                                          uint16_t des_image_width,
                                          uint16_t des_x, uint16_t des_y,
                                          uint16_t copy_width, uint16_t copy_height,
                                          uint8_t  alpha,
                                          bool     mode,
                                          const uint8_t* data)
{
  
}
*/

/**
 * @brief This function fills a rectangular area of the SDRAM with a solid color.<br>
 * This operation is used to paint large screen areas or to set areas of the SDRAM to be a given value.<br>
 * The color of Solid Fill is set by “BTE Foreground Color”.
 * @param des_addr is the physical address in SDRAM
 * @param des_x is the starting x-coordinate in block addressing mode
 * @param des_y is the starting y-coordinate in block addressing mode
 * @param bte_width is the width of area to fill 
 * @param bte_height is the height of area to fill
 * @param Color foreground_color object in color(r,g,b)
 */
void Ra8876_Lite::bteSolidFill (  uint32_t des_addr,
                                  uint16_t des_x,uint16_t des_y,
                                  uint16_t bte_width,uint16_t bte_height,
                                  Color foreground_color)
{
  bte_DestinationMemoryStartAddr(des_addr);
  bte_DestinationImageWidth(_canvasWidth);
  bte_DestinationWindowStartXY(des_x,des_y);

  setForegroundColor(foreground_color);
  
  bte_WindowSize(bte_width,bte_height);  

  lcdRegDataWrite(RA8876_BTE_CTRL1,RA8876_BTE_SOLID_FILL);//91h 
  
  lcdRegDataWrite(RA8876_BTE_CTRL0,RA8876_BTE_ENABLE<<4);//90h
  check2dBusy(); 
}
                      

 /*DMA Function*/

 /**
  * @brief	Enable W25Q256FV as the Serial Flash & enable 4-Byte Addressing mode for it.
  * @note	Extract from W25Q256FV datasheet: <br>
  *			W25Q256FV provides two Address Modes (3-Byte & 4-Byte) that can be used to specify any byte of data in the memory array.<br>
  *			4-Byte Address Mode is designed to support Serial Flash Memory devices from 256Mbit to 32G-bit.<br>
  *			To switch between 3-Byte or 4-Byte Address Modes, "Enter 4-Byte Mode (B7h)" instruction mush be used.<br>
  *			It is a W25Q256FV onboard so it is more convenient to use 4-Byte Address Mode for ArduoHDMI shield.<br>
  * @param	scs_select indicates a hardware parameter for chip select.<br>
  *			RA8876_SERIAL_FLASH_SELECT0 if CS pin of W25Q256FV is wired to XnSFCS0 (pin 37).<br>
  *			RA8876_SERIAL_FLASH_SELECT1 if CS pin of W25Q256FV is wired to XnSFCS1 (pin 38) - default.<br>
  *			Just in case there is a change in hardware to wire XnSFCS0 with W25Q256FV's CS,<br>
  *			it is required to replace RA8876_SERIAL_FLASH_SELECT1 by RA8876_SERIAL_FLASH_SELECT0 in dmaDataTransfer().	
  */
void Ra8876_Lite:: setSerialFlash(uint8_t scs_select)
{
	 uint8_t nSS = RA8876_SPIM_NSS_SELECT_1;
	 
	 if(scs_select==RA8876_SERIAL_FLASH_SELECT0) 
		 nSS = RA8876_SPIM_NSS_SELECT_0;
	 
	 lcdRegDataWrite( RA8876_SPIMCR2, nSS<<5|RA8876_SPIM_MODE0);//b9h
	 lcdRegDataWrite( RA8876_SPIMCR2, nSS<<5|RA8876_SPIM_NSS_ACTIVE<<4|RA8876_SPIM_MODE0);//b9h 
	 lcdRegWrite( RA8876_SPIDR);//b8h
	 hal_delayMs(1);
	 lcdDataWrite(0xB7);	//Enter 4-Byte Mode (B7h) command for W25Q256FV Serial Flash
	 hal_delayMs(1);
	 lcdRegDataWrite( RA8876_SPIMCR2, nSS<<5|RA8876_SPIM_NSS_INACTIVE<<4|RA8876_SPIM_MODE0);//b9h 
}
 
/**
 * @brief 	Transfer data from external serial flash (W25Q256FV) to SDRAM in Block Mode by DMA.<br>
 *			Assumed 4-Byte Addressing Mode for W25Q256FV, Block Addressing Mode for RA8876, & XnSFCS1 as CS.<br>
 * @param	x0 defines the upper-left corner x-coordinate on the Canvas Window.
 * @param	y0 defined the upper-left corner y-coordinate on the Canvas Window.
 * @param	copy_width is the DMA block width. It can be smaller than the image width to crop the image. Trim point always start from 0.
 * @param	copy_height is the DMA block height. It can be smaller than the image height to crop the image. Trim point always start from 0.
 * @param	picture_width is the DMA source picture width.
 * @param	src_addr is the Serial flash DMA source address. This is obtained from RAiO Image_AP tool.
 */
 void Ra8876_Lite:: dmaDataBlockTransfer(	uint16_t x0,uint16_t y0,
											uint16_t copy_width,uint16_t copy_height,
											uint16_t picture_width,
											uint32_t src_addr)
 {	 
	lcdRegDataWrite(RA8876_SFL_CTRL,RA8876_SERIAL_FLASH_SELECT1<<7|RA8876_SERIAL_FLASH_DMA_MODE<<6|RA8876_SERIAL_FLASH_ADDR_32BIT<<5|RA8876_FOLLOW_RA8876_MODE<<4|RA8876_SPI_FAST_READ_8DUMMY);//b7h
  
	lcdRegDataWrite(RA8876_SPI_DIVSOR,RA8876_SPI_DIV2);//bbh  
	lcdRegDataWrite(RA8876_DMA_DX0,x0);//c0h
	lcdRegDataWrite(RA8876_DMA_DX1,x0>>8);//c1h
	lcdRegDataWrite(RA8876_DMA_DY0,y0);//c2h
	lcdRegDataWrite(RA8876_DMA_DY1,y0>>8);//c3h 
	lcdRegDataWrite(RA8876_DMAW_WTH0,copy_width);//c6h
	lcdRegDataWrite(RA8876_DMAW_WTH1,copy_width>>8);//c7h
	lcdRegDataWrite(RA8876_DMAW_HIGH0,copy_height);//c8h
	lcdRegDataWrite(RA8876_DMAW_HIGH1,copy_height>>8);//c9h 
	lcdRegDataWrite(RA8876_DMA_SWTH0,picture_width);//cah
	lcdRegDataWrite(RA8876_DMA_SWTH1,picture_width>>8);//cbh 
	lcdRegDataWrite(RA8876_DMA_SSTR0,src_addr);//bch
	lcdRegDataWrite(RA8876_DMA_SSTR1,src_addr>>8);//bdh
	lcdRegDataWrite(RA8876_DMA_SSTR2,src_addr>>16);//beh
	lcdRegDataWrite(RA8876_DMA_SSTR3,src_addr>>24);//bfh  
	lcdRegDataWrite(RA8876_DMA_CTRL,RA8876_DMA_START);//b6h 
	check2dBusy(); 
 }

 /**
 * @brief 	Transfer data from external serial flash (W25Q256FV) to SDRAM in Linear Mode by DMA.<br>
 *			Assumed 4-Byte Addressing Mode for W25Q256FV, Linear Addressing Mode for RA8876, & XnSFCS1 as CS.<br>
 * @param	des_addr defines the address location in Canvas i.e. 32-bit address in SDRAM space.
 * @param	picture_width is the pixel width of the image. Max. 13-bit widht is allowed.
 * @param	picture_height is the image height in line.
 * @param	src_addr is the Serial flash DMA source address. This is obtained from RAiO Image_AP tool.
 * @note	Picture trimming doesn't work in Linear Mode somehow, don't know why...<br>
 *			Only picture width of multiple of 4 pixels allowed. But the reaosn is not known yet.
 */
 void Ra8876_Lite::dmaDataLinearTransfer(	uint32_t des_addr, 
											uint16_t picture_width, uint16_t picture_height, 
											uint32_t src_addr)
{
	lcdRegDataWrite(RA8876_SFL_CTRL,
					RA8876_SERIAL_FLASH_SELECT1<<7|
					RA8876_SERIAL_FLASH_DMA_MODE<<6|
					RA8876_SERIAL_FLASH_ADDR_32BIT<<5|
					RA8876_FOLLOW_RA8876_MODE<<4|
					RA8876_SPI_FAST_READ_8DUMMY);//b7h
  
	lcdRegDataWrite(RA8876_SPI_DIVSOR,RA8876_SPI_DIV2);//bbh
	
	lcdRegDataWrite(RA8876_DMA_DX0,(uint8_t)des_addr);//c0h
	lcdRegDataWrite(RA8876_DMA_DX1,(uint8_t)(des_addr>>8));//c1h
	lcdRegDataWrite(RA8876_DMA_DY0,(uint8_t)(des_addr>>16));//c2h
	lcdRegDataWrite(RA8876_DMA_DY1,(uint8_t)(des_addr>>24));//c3h 
	
	uint32_t byte_count = (uint32_t)picture_width*picture_height*getColorDepth();
	lcdRegDataWrite(RA8876_DMAW_WTH0,(uint8_t)byte_count);//c6h
	lcdRegDataWrite(RA8876_DMAW_WTH1,(uint8_t)(byte_count>>8));//c7h
	lcdRegDataWrite(RA8876_DMAW_HIGH0,(uint8_t)(byte_count>>16));//c8h
	lcdRegDataWrite(RA8876_DMAW_HIGH1,(uint8_t)(byte_count>>24));//c9h 
	
	//seems REG[CAh] & REG[CBh] are not relevant in Linear Mode
	//lcdRegDataWrite(RA8876_DMA_SWTH0,picture_width);//cah	
	//lcdRegDataWrite(RA8876_DMA_SWTH1,picture_width>>8);//cbh 
	lcdRegDataWrite(RA8876_DMA_SSTR0,src_addr);//bch
	lcdRegDataWrite(RA8876_DMA_SSTR1,src_addr>>8);//bdh
	lcdRegDataWrite(RA8876_DMA_SSTR2,src_addr>>16);//beh
	lcdRegDataWrite(RA8876_DMA_SSTR3,src_addr>>24);//bfh  
	lcdRegDataWrite(RA8876_DMA_CTRL,RA8876_DMA_START);//b6h 	
	check2dBusy();
}

 /**
  * 
  */
 void Ra8876_Lite::pwm1BacklightControl(void)
 {
   pwm_Prescaler(3);
   pwm_ClockMuxReg(1, 0, 2, 0); //set 85h for PWM Timer1 /2, enable PWM1 
   pwm1_ClocksPerPeriod(10) ;
   pwm1_Duty(10/2-1);
   pwm_Configuration(0,1,1,0,0,0,0); //pwm1_auto_reload is 1, start pwm1
 }

/**
 * @brief Display a test color bar from RA8876
 * @param on is the ON(1)/OFF(0) control
 */
void Ra8876_Lite::displayColorBar(bool on)
{
  uint8_t DPCR = lcdRegDataRead(RA8876_DPCR); //read REG[12h]

  if(on)
  {
    DPCR|=(RA8876_COLOR_BAR_ENABLE<<5);
  }
  else
  {
    DPCR&=~(RA8876_COLOR_BAR_ENABLE<<5);
  }
  lcdRegDataWrite(RA8876_DPCR, DPCR);  
}


