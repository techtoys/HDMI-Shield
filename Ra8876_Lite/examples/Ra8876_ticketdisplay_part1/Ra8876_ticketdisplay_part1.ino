/**
   @brief    Ticket display application with large digits on HDTV (1080p) - Part I.
             Images in binary format is preloaded to Serial Flash with RA8876/77 AP Software.
             A simple application to parse Serial Monitor message with digits and get them displayed, e.g. 0123<cr>, 23<cr>, etc.
             Open Serial Monitor, set baud rate to 115200 with New Line or Carriage return as end char. 
             From input box type in any 3 digit number e.g. 123<Send>. The monitor/TV will show the number you have entered.
             If you want to change the background image, please change from setup() a function: 
             putSerialFlashImage(0, 0, "background_1") -> putSerialFlashImage(0, 0, "background_2"), or vice versa.

             Important: If you are using HDMI Shield version 1 (with 256Mbit Serial Flash), please open UserConfig.h
             from library folder, and comment #define BOARD_VERSION_2 for this demo to work.
             
   @file     Ra8876_ticketdisplay_part1.ino
   @author   John Leung @ TechToys (www.TechToys.com.hk)
   @section  HISTORY
   Date      10th Aug 2019 first release
*/

#include "Ra8876_Lite.h"
#include "HDMI/Ch703x.h"

///@note  A structure to describe images converted by RAiO Image Tool. 
typedef struct xImages{
  const String imgName;
  uint16_t imgWidth;
  uint16_t imgHeight;
  uint32_t imgAddress;  //starting address with data from RAiO Image Tool
}xImages;

///@note  Source of images and their assocated binary file located in ..\assets. containing digits from '0' to '9' with two background images
const xImages gfx_assets[]=
{
  //digits with color background
  {"char0020",  108, 216, 0},
  {"char0030",  108, 216, 46656},
  {"char0031",  108, 216, 93312},
  {"char0032",  108, 216, 139968},
  {"char0033",  108, 216, 186624},
  {"char0034",  108, 216, 233280},
  {"char0035",  108, 216, 279936},
  {"char0036",  108, 216, 326592},
  {"char0037",  108, 216, 373248},
  {"char0038",  108, 216, 419904},
  {"char0039",  108, 216, 466560},
  //digits with color mask
  {"char0020_mask",  108, 216, 513216},
  {"char0030_mask",  108, 216, 559872},
  {"char0031_mask",  108, 216, 606528},
  {"char0032_mask",  108, 216, 653184},
  {"char0033_mask",  108, 216, 699840},
  {"char0034_mask",  108, 216, 746496},
  {"char0035_mask",  108, 216, 793152},
  {"char0036_mask",  108, 216, 839808},
  {"char0037_mask",  108, 216, 886464},
  {"char0038_mask",  108, 216, 933120},
  {"char0039_mask",  108, 216, 979776},
  //background images
  {"background_1",  1280,  720, 1026432}, //pharmacy bg
  {"background_2",  1280,  720, 2869632}  //broccoli bg
};

/**
 * RA8876 GFX controller class constructor
 */
Ra8876_Lite ra8876lite(RA8876_XNSCS, RA8876_XNRESET, RA8876_MOSI, RA8876_MISO, RA8876_SCK);
Color color; 
int16_t bgIndex;

/**
 * @brief   Interpret incoming message in ASCII string from Serial Monitor. This is a blocking function until carriage return is received from Serial Monitor.
 * @return  The original message with \n included.
 */
String messageParser(void)
{
  String inString="";
  
  while(!Serial.available());
  inString = Serial.readStringUntil('\n');
  return inString;
}

/**
 * @brief Return the array index of a certain image in Serial Flash
 * @return  -1 if no such image
 *          id>=0 if an image exists in Serial Flash
 */
int16_t getSerialFlashImageId(const String& name)
{
  int16_t i;
  
  uint16_t arraySize = sizeof(gfx_assets) / sizeof(xImages);
  for(i=0; i<arraySize; i++)
  {
    if(name.indexOf(gfx_assets[i].imgName)!=-1) break;
  }

  if(i < arraySize)
    return i;
  else
    return -1;
}

/**
 * @brief Display an image from Serial Flash with DMA
 * @param x0 is the start position in x at top left
 * @param y0 is the start position in y at top left
 * @param name is a reference to image in Serial Flash
 * @return  array index as the handler id(>=0) of the image in Serial Flash
 *          -1 if there is no such image in Flash
 */
int16_t putSerialFlashImage(uint16_t x0, uint16_t y0, const String& name)
{
  int16_t id = getSerialFlashImageId(name);
  if (id > -1)
  {
    ra8876lite.dmaDataBlockTransfer(
    x0,y0,
    gfx_assets[id].imgWidth,gfx_assets[id].imgHeight,
    gfx_assets[id].imgWidth,
    gfx_assets[id].imgAddress);    
  }

  return id;
}

/**
 * @brief Print digits from a constant character array (null terminated) to HDTV
 */
void     putJumboScore(uint16_t x0, uint16_t y0, const char *pStr)
{
  uint16_t cursor_x = x0;
  int16_t id;
  
  while(*pStr!='\0')
  {
    switch(*pStr)
    {
      case ' ':
                id = putSerialFlashImage(cursor_x, y0, "char0020");
          break;
      case '0': id = putSerialFlashImage(cursor_x, y0, "char0030");
          break;
      case '1': id = putSerialFlashImage(cursor_x, y0, "char0031");
          break;
      case '2': id = putSerialFlashImage(cursor_x, y0, "char0032");
          break;
      case '3': id = putSerialFlashImage(cursor_x, y0, "char0033");
          break;
      case '4': id = putSerialFlashImage(cursor_x, y0, "char0034");
          break;
      case '5': id = putSerialFlashImage(cursor_x, y0, "char0035");
          break; 
      case '6': id = putSerialFlashImage(cursor_x, y0, "char0036");
          break;
      case '7': id = putSerialFlashImage(cursor_x, y0, "char0037");
          break;
      case '8': id = putSerialFlashImage(cursor_x, y0, "char0038");
          break;       
      case '9': id = putSerialFlashImage(cursor_x, y0, "char0039");
          break;
    }
    cursor_x += gfx_assets[id].imgWidth;
    pStr++;
  }
}

/**
   @brief isr for Vsync, not used in this example
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
  if (!ra8876lite.begin(&CEA_1280x720p_60Hz))  //init RA8876 with a video resolution of 1280*720@60Hz (720p)
  {
#ifdef DEBUG_LLD_RA8876
    printf("RA8876 or RA8877 Fail\n");
#endif
  }
  else
  {
    ra8876lite.canvasImageBuffer(1280,720);     //Canvas set to the same size of 1280*720 in 16 bit-per-pixel color depth
    ra8876lite.displayMainWindow();             //align display window to the same canvas starting point
    ra8876lite.canvasClear(color.White);        //set whole screen to white on startup
    ra8876lite.graphicMode(true);
    ra8876lite.displayOn(true);
    
    HDMI_Tx.begin();                  //Init I2C
    HDMI_Tx.init(VIDEO_in_1280x720_out_DVI_1080p_60Hz); //map defined in .\HDMI\videoInOutMap.h
    HDMI_Tx.setI2SAudio(0, 0, 0);     //disable audio 
    delay(1000);
  }

  bgIndex = putSerialFlashImage(0, 0, "background_2");
}

void loop() {
     char char_buffer[4];
#ifdef ESP8266
  yield();  //this is to avoid wdt reset in ESP8266
#endif
     String inString = messageParser(); //blocking function here until \n received    
     sprintf(char_buffer, "%3d", inString.toInt()); //format to 3 digits
     char_buffer[3]='\0'; //null terminate
     putJumboScore(930,128, char_buffer); //print digits on screen
}
