/**
    @brief  A slide show demo with screen transition in and out, plus font display in multi-languages with bitmap fonts created<br>
            by a commercial font package (BitFontCreator).
    
   @file     Ra8876_slideshow.ino
   @author   John Leung @ TechToys (www.TechToys.com.hk)
   @section  HISTORY
   Date      11th Dec 2017 first release
*/
 
#include "Ra8876_Lite.h"
#include "HDMI/Ch703x.h"
#include <SD.h>

#if defined (TEENSYDUINO)
#include "Audio.h"
AudioPlaySdWav           playSdWav1;
AudioOutputI2S           i2s1;
AudioConnection          patchCord1(playSdWav1, 0, i2s1, 0);
AudioConnection          patchCord2(playSdWav1, 1, i2s1, 1);
#endif

/**
 * RA8876 GFX controller class constructor
 */
Ra8876_Lite ra8876lite(RA8876_XNSCS, RA8876_XNRESET, RA8876_MOSI, RA8876_MISO, RA8876_SCK);
Color color; 
      
/**
 * @note  Local functions
 */
String  messageParser(void) ;
void    dmaPutImages(void);
//frame buffer related functions prefix with FB_
void    FB_refresh(void);             //update visible area with frame buffer
void    FB_fadeIn(uint16_t msec);     //fade in what ever in frame buffer
void    FB_fadeOut(uint16_t msec);    //fade out main window for black screen
void    FB_clear(Color color);        //clear frame buffer with a pure color
void    FB_fill(const String image);  //fill frame buffer with an image of type xImages below
const   uint16_t FB_StartX = 0;       //Canvas size initialized to 1280*720 in setup() below
const   uint16_t FB_StartY = 720;     //All drawing performed in frame buffer starting from y=720 (off-screen area)

///@note  A structure to describe images converted by RAiO Image Tool. 
typedef struct xImages{
  const String imgName;
  uint16_t imgWidth;
  uint16_t imgHeight;
  uint32_t imgSize;
  uint32_t imgAddress;  //starting address with data from RAiO Image Tool
}xImages;

///@note  Source of images and their assocated binary file located in ..\images.
const xImages gfx_assets[]=
{
  {"bruce1",  1464, 1096,   3209088,  0},
  {"bruce2",  4800, 820,    7872000,  3209088},
  {"bruce3",  3968, 426,    3380736,  11081088},
  {"leopard", 1280, 720,    1843200,  14461824},
  {"martial", 1280, 720,    1843200,  16305024},
  {"bride",   850,  1280,   2170880,  18148224},
  {"burger",  640,  472,    604160,   20319104},
  {"dance",   1280, 709,    1815040,  20923264},
  {"wind",    1280, 960,    2457600,  22738304},
  {"tiger",   1280, 720,    1843200,  25195904},
  {"fb_ico",  256,  256,    131072,   27039104},
  {"in_ico",  256,  256,    131072,   27170176},
  {"inst_ico",256,  256,    131072,   27301248},
  {"pharmacy",640,  426,    545280,   27432320},
  {"pharmacy_rccw90", 424,  640,  542720, 27977600},
  {"wifi",    256,  223,    114176,   28520320},
  {"finger",  848,  480,    814080,   28634496},
  {"rains",   1280, 853,    2183680,  29448576}
};

///@note  unicode 16 for extended character sets in Japanese, Korean, and Chinese.
///       Further details in folder .\fonts\BFC_projects
///@note  Font: GN_Kin_iro_SansSerif48hAA4 こんにちは in unicode 16
const uint16_t hello_japanese[]={0x3053, 0x3093, 0x306B, 0x3061, 0x306F, '\0'};
///@note  Font: BM_YEONSUNG77hAA4 여보세요 in unicode 16
const uint16_t hello_korean[]  ={0xC5EC, 0xBCF4, 0xC138, 0xC694, '\0'};
///@note Font: SimHei 你好 in unicode 16
const uint16_t hello_chinese[] ={0x4F60, 0x597D, '\0'};

/**
   @brief isr for Vsync
*/
void isr(void)
{
  ra8876lite.irqEventHandler();
}
     
void setup() {
  Serial.begin(115200);
  pinMode(RA8876_XNINTR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RA8876_XNINTR), isr, FALLING); //pending for Vsync hardware is not ready yet
  
  ///@note  Initialize RA8876 starts from here
  if (!ra8876lite.begin(&CEA_1280x720p_60Hz))  //init RA8876 with a video resolution of 1280*720 @ 60Hz with map defined in .\HDMI\LcdParam.h
  {
#ifdef DEBUG_LLD_RA8876
    printf("RA8876 or RA8877 Fail\n");
#endif
  }
  else
  {
    ra8876lite.canvasImageBuffer(1280,720);      //Canvas set to the same size of 1280*720 in default 16 bit-per-pixel color depth
    ra8876lite.displayMainWindow();             //align display window to the same canvas starting point
    ra8876lite.canvasClear(color.Black);        //set whole screen to simple white on startup
    ra8876lite.graphicMode(true);
    ra8876lite.displayOn(true);
    
    HDMI_Tx.begin();                  //Init I2C
    HDMI_Tx.init(VIDEO_in_1280x720_out_HDMI_1080p_60Hz); //map defined in .\HDMI\videoInOutMap.h
    HDMI_Tx.setI2SAudio(0, 0, 0);     //disable audio 
    delay(1000);
  }

  printf("Init SD card.");
  uint8_t iteration = 0;
#ifdef ESP32
  SPIClass _HSPI;
  _HSPI.begin(SDCARD_SCK_PIN, SDCARD_MISO_PIN, SDCARD_MOSI_PIN, SDCARD_CS_PIN);  //pins defined in UserConfig.h
  while (!SD.begin(15, _HSPI, 50000000UL, "/sd")) //Can be 50MHz for ESP32 HSPI interface! Good!
#else
  while (!(SD.begin(SDCARD_CS_PIN)))
#endif
  {
    delay(500);
    if(iteration++ == 3)
      break;
    else
      printf(".");    
  }
  printf("\n");
  if (iteration > 3)
    { 
      printf("SD card init failed. Have you forgot to insert a sdcard?\n");
    }
  else
    {
      printf("SD card OK!\n");
    }

    ra8876lite.dmaDataBlockTransfer(
    0,720l*5,
    gfx_assets[2].imgWidth,gfx_assets[2].imgHeight,
    gfx_assets[2].imgWidth,
    gfx_assets[2].imgAddress);      
    
}

void loop() {
        FB_fill("tiger");
        FB_fadeIn(1000);
        FB_fadeOut(500);
        FB_fill("burger");
        FB_fadeIn(1000);
        FB_fadeOut(500);
        FB_fill("finger");
        FB_fadeIn(1000);
        //print some text in unicode and numbers
#ifdef ESP32
        ra8876lite.blitBfcString(100, 600, "/SimHei.bin", hello_chinese, color.Yellow, color.Black,0);      
        ra8876lite.blitBfcString(600, 620, "/GN_Kin.bin", hello_japanese, color.Red, color.Black,0);
        ra8876lite.blitBfcString(1000, 600, "/YEONSUNG.bin", hello_korean, color.Cyan, color.Black,0);
#else
        ra8876lite.blitBfcString(100, 600, "SimHei.bin", hello_chinese, color.Yellow, color.Black,0);      
        ra8876lite.blitBfcString(600, 620, "GN_Kin.bin", hello_japanese, color.Red, color.Black,0);
        ra8876lite.blitBfcString(1000, 600, "YEONSUNG.bin", hello_korean, color.Cyan, color.Black,0);
#endif
        int ticket=0;
        while(ticket++<10){
#ifdef ESP32
        //Known bug for ESP32 with blitBfcString(... "/Arial72.bin", ...); //seems file too big? Some problem with heap allocation...
        /**
         * assertion "heap != NULL && "free() target pointer is outside heap areas"" 
         * failed: file "/Users/ficeto/Desktop/ESP32/ESP32/esp-idf-public/components/heap/./heap_caps.c", line 274, function: heap_caps_free
            abort() was called at PC 0x400dd123 on core 1
         */
        //ra8876lite.blitBfcString(1050, 480, "/Arial72.bin", String(ticket), color.Yellow, color.Black, 0);  
#else
        ra8876lite.blitBfcString(1050, 480, "Arial72.bin", String(ticket), color.Yellow, color.Black, 0);  
#endif
        #ifdef ESP8266
        yield();  //this is to avoid wdt reset in ESP8266
        #endif
        }
        delay(1000);
        FB_fadeOut(500);
}
/**
 * ***********************************************************************************************************************************************
 * End of loop()
 * *********************************************************************************************************************************************** 
 */

/**
 * @brief   Interpret incoming message in ASCII string from Serial Monitor. This is a blocking function with an infinite loop. Use with care.
 * @return  String received with \r or \n removed.
 */
String messageParser(void)
{
  String inString="";
  if(Serial.available())
  {
    inString = Serial.readStringUntil('\n');
  }
  return inString;
}

/**
 * @brief update visible area with frame buffer
 */
void    FB_refresh(void)
{
  //ra8876lite.vsyncWait();
  ra8876lite.bteMemoryCopyWithROP(
    0,
    ra8876lite.getCanvasWidth(),
    FB_StartX, FB_StartY, //s0_x, s0_y
    0,0,0,0,
    0,
    ra8876lite.getCanvasWidth(),
    0,0,                  //des_x, des_y
    ra8876lite.getCanvasWidth(),ra8876lite.getCanvasHeight(), //copy_width,copy_height
    RA8876_BTE_ROP_CODE_12);
}

/**
 * @brief   Fade in whatever in frame buffer to the Main Window.
 */
void  FB_fadeIn(uint16_t msec)
{
  uint16_t transition_time = msec/32;
  
  for(uint8_t alpha=0; alpha<33; alpha++){
  ra8876lite.vsyncWait();
  ra8876lite.bteMemoryCopyWithOpacity(
    0,
    ra8876lite.getCanvasWidth(),
    FB_StartX, FB_StartY, //s0_x, s0_y
    0,0,0,0,
    0,
    ra8876lite.getCanvasWidth(),
    0,0,                  //des_x, des_y
    ra8876lite.getCanvasWidth(),ra8876lite.getCanvasHeight(), //copy_width,copy_height
    32-alpha);
    delay (transition_time);
    #ifdef ESP8266
    yield();  //this is to avoid wdt reset in ESP8266
    #endif 
  }
}

void  FB_fadeOut(uint16_t msec)
{
  uint16_t transition_time = msec/32;
  
  for(uint8_t alpha=0; alpha<33; alpha++){
  ra8876lite.vsyncWait();
  ra8876lite.bteMemoryCopyWithOpacity(
    0,
    ra8876lite.getCanvasWidth(),
    FB_StartX, FB_StartY, //s0_x, s0_y
    0,0,0,0,
    0,
    ra8876lite.getCanvasWidth(),
    0,0,                  //des_x, des_y
    ra8876lite.getCanvasWidth(),ra8876lite.getCanvasHeight(), //copy_width,copy_height
    alpha);
    delay (transition_time); 
    #ifdef ESP8266
    yield();  //this is to avoid wdt reset in ESP8266
    #endif
  }
}

/**
 * @brief clear frame buffer with a pure color
 */
void    FB_clear(Color color)
{
  ra8876lite.canvasClear(color,FB_StartX,FB_StartY);
}

/**
 * @brief fill frame buffer with an image of type xImages below
 */
void    FB_fill(const String image)
{
  uint16_t arraySize = sizeof(gfx_assets) / sizeof(xImages);
  uint16_t x=0, y=0;;
  uint16_t copy_width=0, copy_height=0;
  
  FB_clear(color.Black);
  for(uint8_t i=0; i<arraySize; i++)
  {
    if(image==gfx_assets[i].imgName)
    {
      if(gfx_assets[i].imgWidth <= ra8876lite.getCanvasWidth())
      {
        x = (ra8876lite.getCanvasWidth() - gfx_assets[i].imgWidth )/2;
        copy_width = gfx_assets[i].imgWidth;
      }
      else
      {
        x = 0;
        copy_width = ra8876lite.getCanvasWidth(); //bound to canvas width for large images
      }
      
      if(gfx_assets[i].imgHeight <= ra8876lite.getCanvasHeight())
      {
        y = (ra8876lite.getCanvasHeight() - gfx_assets[i].imgHeight )/2;
        copy_height = gfx_assets[i].imgHeight;
      }
      else
      {
        y = 0;
        copy_height = ra8876lite.getCanvasHeight();
      }
      
    ra8876lite.dmaDataBlockTransfer(
    x,FB_StartY+y,
    copy_width, copy_height,
    gfx_assets[i].imgWidth,
    gfx_assets[i].imgAddress);      
    }
  }
}






