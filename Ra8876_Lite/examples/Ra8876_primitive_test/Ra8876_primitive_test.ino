/**
    @brief Primitive test for the following functions<br>
    (1) SPI initialization for read/write RA8876 in register level; together with GPIO setup for SPI-CS, reset, and interrupt.<br>
    (2) Initialize RA8876 to generate RGB video with LCD set to Cyan color. Resolution is 1280x720@60Hz (720p).<br>
    (3) Initialize I2C interface for CH7035B.<br>
        No parameter has been subsituted yet because CH7035B is able to run with firmware fetched from the onboard BootROM (9904)<br>
        with an internal frame buffer to boost 720p to 1080p@60Hz with HDMI output.<br>
    (4) Program flows to an infinite main loop to wait for commands from Serial Monitor (115200bps).
    
        Following hardware is assumed :<br>
    (1) HDMI Shield of date code DTE 20171228 or later. <br>
    (2) A Due Zipper Board of DTE 20171024 or later. Jumpers set to P12 to use ESP8266 as the host. <br>
    (3) Compatible hosts tested so far are Arduino DUE, Arduino M0 (direct stack), Genuino 101 (direct stack), Teensy 3.2 stacking on a bridge PCB <br>
        to make Teensy compatible with legacy Arduino Uno form factor (PCB version is DTE20171030), and Teensy 3.5 stacking on the same PCB.<br>
        Successfully tested with ESP8266 (Due Zipper Board DTE 20171024), and ESP32-PICO-KIT with jumper wires<br>
        When an Arduino M0 is used, we need to replace all occurences of Serial with SerialUSB in this file and printf.h file in the source code folder.<br>
        If it is not an Arduino M0, no change is required.<br>
        
        How to use this demo with ESP8266: <br>
    (1) On HDMI Shield of date code DTE 20171228 or later, make sure dip switch S1 set to 1(ON), 2(OFF), 3(ON), 4(OFF). This setting uses SPI access for RA8876 (SPI_dipSw_setting.jpg).<br>
    (2) Stack HDMI Shield on top of Due Zipper Board. Make sure all jumpers set to P12 onboard to use ESP8266 WiFi module as the host (ESP8266_use_P12_jumpers.jpg). <br>
    (3) Because it is a CH340C USB<->UART converter onboard of Due Zipper, you will have to download its driver<br>
        from https://sparks.gogo.co.nz/ch340.html, or http://www.wch.cn/download/CH341SER_EXE.html. <br>
    (4) Install ESP8266 Arduino library with Additional Boards Manager URLs set to http://arduino.esp8266.com/stable/package_esp8266com_index.json.<br>
        Detailed instruction can be found at http://esp8266.github.io/Arduino/versions/2.0.0/doc/installing.html<br>
        At time of writing the ESP8266 Arduino library version is 2.4.1.<br>
    (5) From Arduino IDE (version 1.8.1 or later), set Generic ESP8226 Module from Tools -> Board -> Generic ESP8226 Module.<br>
        Configurations in detail please refer to Tools_ESP8266_Config.jpg.<br>
    (6) Connect HDMI Shield to your HDTV/LCD monitor of 1080p with a HDMI cable. No DVI yet. Only use HDMI interface. Switch on the TV.<br>
    (7) Now connect Due Zipper to your PC with a microUSB cable. A new COM Port will be enumerated available from Tools->Port. It is COM55 in my case (New_ESP8266_comm_port.jpg).<br>
    (8) Select the new COM port created, on Due Zipper close to its left edge, there are two buttons, RESET and PROG (ESP8266_prog_reset_keys.jpg).<br>
        Hold the PROG key with a single click on RESET key. This will bring ESP8266 to bootloader mode. You may release PROG key after releasing RESET key.<br>
    (9) Click Upload from Arduino IDE (Sketch->Upload). Observe a Done Uploading message with 100% from Arduino IDE.<br>
    (10)If it is a whole screen of Cyan color displayed on the TV, congratulation; everything seems fine so far. 

        Debug via Serial Monitor: <br>
    (1) Open Serial Monitor, set to Newline with 115200baud. <br>
    (2) Type dma<Send> or <Enter>. Type tiger <Send>. Can you see a tiger? Try other nice pictures by repeating dma<Send> bride<Send> etc.<br>
        Name of images preloaded in gfx_assets[] array in this file.<br>
    (3) Type hwFontTest<Send>. Now you see hardware fonts in various languages.<br>
    (4) Type drawSquareFill<Send>. Follow the instructions to input x0,y0,x1,y1, with color in rgb; see if a solid square is drawn.<br>
        Follow the instruction from note on drawPrimitiveTest() for more shapes.<br>
    (5) Type vsyncTest<Send>. This demo displays random primitive shapes for around 17s with hardware acceleration.<br>
        This is equivalent to 300 vsync intervals in a frame rate of 60Hz.<br>
    (6) Finally, type spi,w,0x12,0x60 <Send>. Low level SPI read/write is available to debug it in Register level.<br>
        This command is to write(w) to Register 0x12 a value of 0x60 to switch on the color bar test feature of RA8876.<br>
        Now type spi,r,0x12 <Send>. Can you read val=60h from Serial Monitor? It is the value you wrote to Register 0x12.<br>
        Restore normal operation by typing spi,w,0x12,0x40 <Send>
        Now, type in i2c,edid<Send>. Switching to CH7035B HDMI encoder we can read and write in Register level with I2C interface.<br>
        This is an encapsulated command to read the EDID information of the monitor.
        
    
   @file     Ra8876_primitive_test.ino
   @author   John Leung @ TechToys (www.TechToys.com.hk)
   @section  HISTORY
   Date       3rd Nov 2017 first release
              16th April 2018 instruction revised with screen shots in jpg uploaded.
*/

/**
 * Changes :
 * (1) ra8876lite.begin() -> ra8876lite.begin(&CEA_1280x720p_60Hz) for less heat from HDMI encoder.
 * Instead of using onboard boot ROM with ra8876lite.begin(), an argument &CEA_1280x720p_60Hz is now used for initialization.
 * Resolution is the same at 1280*720 60Hz output from RA8876 for both methods. With boot ROM, it is possible to fetch EDID information
 * from a TV/monitor. With explicit argument &CEA_1280x720p_60Hz no EDID is available.
 * This change is to avoid overheat from CH7035 HDMI encoder leading to overloading of 5V supply from USB cable. 
 * (2) New function HDMI_Tx.init(VIDEO_in_1280x720_out_HDMI_1080p_60Hz) after HDMI_Tx.begin() to initialize CH7035B for
 * 1280*720 video input -> 1080p 60Hz HDMI output. 
 * Date: 29th Jan 2019
 */
#include "Ra8876_Lite.h"
#include "HDMI/Ch703x.h"

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
  {"bride",   848,  1280,   2170880,  18148224},
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

/**
 * RA8876 GFX controller class constructor
 */
Ra8876_Lite ra8876lite(RA8876_XNSCS, RA8876_XNRESET, RA8876_MOSI, RA8876_MISO, RA8876_SCK);
Color color; 
      
/**
 * @note  Local functions
 */
int16_t string2int(void);
String messageParser(void) ;
void drawPrimitiveTest(void);
void vsyncTest(void);
void hwFontTest(void);
void dmaPutImages(void);

/**
 * @note Enum to be used in drawPrimitiveTest()
 */
enum {
  DRAW_INVALID = -1,
  DRAW_LINE = 0,
  DRAW_SQUARE = 1,
  DRAW_SQUARE_FILL = 2,
  DRAW_CIRCLE_SQUARE,
  DRAW_CIRCLE_SQUARE_FILL,
  DRAW_TRIANGLE,
  DRAW_TRIANGLE_FILL,
  DRAW_CIRCLE,
  DRAW_CIRCLE_FILL,
  DRAW_ELLIPSE,
  DRAW_ELLIPSE_FILL
};

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
    ra8876lite.canvasClear(color.Cyan);         //set whole screen to Cyan on startup
    ra8876lite.graphicMode(true);
    ra8876lite.displayOn(true);
    
    HDMI_Tx.begin();                  //Init I2C
    HDMI_Tx.init(VIDEO_in_1280x720_out_HDMI_1080p_60Hz); //map defined in .\HDMI\videoInOutMap.h
    HDMI_Tx.setI2SAudio(0, 0, 0);     //disable audio 
    delay(1000);
  }
}

void loop() {
#ifdef ESP8266
  yield();  //this is to avoid wdt reset in ESP8266
#endif
     String inString = messageParser(); //blocking function here until \n received
     
      /**
       * @note  Low level debug RA8876 in register level by SPI read/write or debug CH7025B with I2C register read/write.<br>
       *        Example : Open Serial Monitor, set baud rate to 115200bps<br>
       *        spi,w,0x12,0x60 <Send>  //write a value 0x60 to register 0x12 to turn on color bar test for RA8876<br>
       *        spi,r,0x12 <Send>       //read the value from 0x12<br>
       *        spi,w,0x12,0x40 <Send>  //write 0x40 to 0x12 to cancel color bar test<br>
       *        i2c,w,1,0x7f,0x14 <Send>//write a value 0x14 to register page 1 at 0x7f of CH7035B to turn on color bar from CH7035B<br>
       *        i2c,w,1,0x7f,0x04 <Send>//restore normal operation from CH7035B color bar<br>
       *        i2c,r,1,0x7f,1 <Send>   //read 1 byte starting from register 0x7F on page 1<br>
       *        i2c,r,2,0x10,1 <Send>   //read 1 byte starting from register 0x10 on page 2<br>
       *        i2c,edid <Send>         //special for CH7035B, this message reads EDID information from a monitor<br>
       */
      if (inString.indexOf("spi")!=-1 || inString.indexOf("i2c")!=-1)
      {
        char *debugMsg = new char[inString.length()+1];
        inString.toCharArray(debugMsg, inString.length()+1);
        (inString.substring(0,3)=="spi")?ra8876lite.parser(debugMsg):HDMI_Tx.parser(debugMsg);
        delete[] debugMsg;
      }
      ///@note  Primitive demo to draw geometrical shape. Details described in drawPrimitiveTest() function.
      else if (inString.indexOf("draw")!=-1)
      {
        drawPrimitiveTest();
      }
      ///@note  Display various hardware fonts (Embedded and Genitop fonts)
      //else if (inString.substring(0,10)=="hwFontTest")
      else if (inString.indexOf("hwFontTest")!=-1)
      {
        hwFontTest();
      }
      ///@note  This demo displays random rectangles with BTE Solid Fill for 1000 iterations with average vsync interval displayed on LCD.<br>
      else if (inString.indexOf("vsyncTest")!=-1)
      {
        vsyncTest();
      }
      ///@note  Display preloaded pictures from Serial Flash by DMA transfer
      else if (inString.indexOf("dma")!=-1)
      {
        dmaPutImages();
      }   
}
/**
 * ***********************************************************************************************************************************************
 * End of loop()
 * *********************************************************************************************************************************************** 
 */

/**
 * @brief   Convert a string to integer via Serial Monitor with Newline or CR set as the terminator.
 * @note    This is a blocking function until some characters received from Serial Monitor. Use it with care.
 * @return  integer converted from characters input via Serial Monitor
 */
int16_t string2int(void)
{
    String inString="";
    int16_t x=0;
      
    while(!Serial.available());
    for(;;){
    int inChar = Serial.read();
    if(isDigit(inChar)){inString+=(char)inChar;}
    if(inChar=='\n' || inChar=='\r')
    {
      x = inString.toInt();
      return x;
    }
    }
}

/**
 * @brief   Interpret incoming message in ASCII string from Serial Monitor. This is a blocking function until a character received from Serial.
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
 * @brief   Draw primitive shape by using various hardware accelerated functions (11 functions overall) with commands in String from Serial Monitor.
 */
void drawPrimitiveTest(void)
{
  Color color;
  uint16_t x0=0, y0=0, x1=0, y1=0, x2=0, y2=0, r=0, xr=0, yr=0;
  int8_t index = DRAW_INVALID;

  printf("Please input any one of these commands to start drawing :\n");
  printf("(1) drawLine\n");
  printf("(2) drawSquare or drawSquareFill\n");
  printf("(3) drawCircleSquare or drawCircleSquareFill\n");
  printf("(4) drawTriangle or drawTriangleFill\n");
  printf("(5) drawCircle or drawCircleFill\n");
  printf("(6) drawEllipse or drawEllipseFill\n");
  
  ra8876lite.canvasClear(color.Black);
    
  String message = messageParser();
  
  if(message=="drawLine")
    index = DRAW_LINE;
  else if (message=="drawSquare")
    index = DRAW_SQUARE;   
  else if (message=="drawSquareFill")
    index = DRAW_SQUARE_FILL;
  else if (message=="drawCircleSquare")
    index = DRAW_CIRCLE_SQUARE;
  else if (message=="drawCircleSquareFill")
    index = DRAW_CIRCLE_SQUARE_FILL;
  else if (message=="drawTriangle")
    index = DRAW_TRIANGLE;
  else if (message=="drawTriangleFill")
    index = DRAW_TRIANGLE_FILL;
  else if (message=="drawCircle")
    index = DRAW_CIRCLE;
  else if (message=="drawCircleFill")
    index = DRAW_CIRCLE_FILL;
  else if (message=="drawEllipse")
    index = DRAW_ELLIPSE;
  else if (message=="drawEllipseFill")
    index = DRAW_ELLIPSE_FILL;
  else{
      printf("Is not a valid Draw Operation.\n");
      return;
  }          
  switch(index)
  {
    case DRAW_LINE:printf("Draw line with hardware acceleration.\n");
      break;
    case DRAW_SQUARE:printf("Draw square with hardware acceleration.\n");
      break;
    case DRAW_SQUARE_FILL:printf("Draw square fill with hardware acceleration.\n");
      break;
    case DRAW_CIRCLE_SQUARE:printf("Draw circle square with hardware acceleration.\n");
      break;
    case DRAW_CIRCLE_SQUARE_FILL:printf("Draw circle square fill with hardware acceleration.\n");
      break;
    case DRAW_TRIANGLE:printf("Draw triangle with hardware acceleration.\n");
      break;
    case DRAW_TRIANGLE_FILL:printf("Draw triangle fill with hardware acceleration.\n");
      break;
    case DRAW_CIRCLE:printf("Draw circle with hardware acceleration.\n");
      break;
    case DRAW_CIRCLE_FILL:printf("Draw circle fill with hardware acceleration.\n");
      break;
    case DRAW_ELLIPSE:printf("Draw ellipse with hardware acceleration.\n");
      break;
    case DRAW_ELLIPSE_FILL:printf("Draw ellipse fill with hardware acceleration.\n");
      break;
  }
  
  printf("x0 = ?"); x0 = string2int(); printf("%d\n", x0);
  printf("y0 = ?"); y0 = string2int(); printf("%d\n", y0);
     
  switch(index)
  {
    case DRAW_CIRCLE:
    case DRAW_CIRCLE_FILL:
        printf("r = ?"); r = string2int(); printf("%d\n", r);
        break;
    case DRAW_ELLIPSE:
    case DRAW_ELLIPSE_FILL:
        printf("xr = ?"); xr = string2int(); printf("%d\n", xr);
        printf("yr = ?"); yr = string2int(); printf("%d\n", yr);
        break;
    case DRAW_LINE:
    case DRAW_SQUARE:
    case DRAW_SQUARE_FILL:
    case DRAW_CIRCLE_SQUARE:
    case DRAW_CIRCLE_SQUARE_FILL:
      printf("x1 = ?"); x1 = string2int(); printf("%d\n", x1);
      printf("y1 = ?"); y1 = string2int(); printf("%d\n", y1);
      if(index==DRAW_CIRCLE_SQUARE || index==DRAW_CIRCLE_SQUARE_FILL)
      {
        printf("xr = ?"); xr = string2int(); printf("%d\n", xr);
        printf("yr = ?"); yr = string2int(); printf("%d\n", yr);        
      }
      break;
    case DRAW_TRIANGLE:
    case DRAW_TRIANGLE_FILL:
      printf("x1 = ?"); x1 = string2int(); printf("%d\n", x1);
      printf("y1 = ?"); y1 = string2int(); printf("%d\n", y1);
      printf("x2 = ?"); x2 = string2int(); printf("%d\n", x2);
      printf("y2 = ?"); y2 = string2int(); printf("%d\n", y2);            
      break;
    default:
      printf("Is not a valid input.\n");
      return;
  }
      //finally get color
      printf("Color red component (0-255) = ?"); color.r=string2int(); printf("%d\n", color.r);
      printf("Color green component (0-255) = ?"); color.g=string2int(); printf("%d\n", color.g);
      printf("Color blue component (0-255) = ?"); color.b=string2int(); printf("%d\n", color.b);
      
  switch(index)
  {
    case DRAW_LINE:ra8876lite.drawLine(x0,y0,x1,y1,color);
      break;
    case DRAW_SQUARE:ra8876lite.drawSquare(x0,y0,x1,y1,color);
      break;
    case DRAW_SQUARE_FILL:ra8876lite.drawSquareFill(x0,y0,x1,y1,color);
      break;
    case DRAW_CIRCLE_SQUARE:ra8876lite.drawCircleSquare(x0,y0,x1,y1,xr,yr,color);
      break;
    case DRAW_CIRCLE_SQUARE_FILL:ra8876lite.drawCircleSquareFill(x0,y0,x1,y1,xr,yr,color);
      break;
    case DRAW_TRIANGLE:ra8876lite.drawTriangle(x0,y0,x1,y1,x2,y2,color);
      break;
    case DRAW_TRIANGLE_FILL:ra8876lite.drawTriangleFill(x0,y0,x1,y1,x2,y2,color);
      break;
    case DRAW_CIRCLE:ra8876lite.drawCircle(x0,y0,r,color);
      break;
    case DRAW_CIRCLE_FILL:ra8876lite.drawCircleFill(x0,y0,r,color);
      break;
    case DRAW_ELLIPSE:ra8876lite.drawEllipse(x0,y0,xr,yr,color);
      break;
    case DRAW_ELLIPSE_FILL:ra8876lite.drawEllipseFill(x0,y0,xr,yr,color);
      break;
  }   
}

/**
 * @brief   This demo displays random rectangles with BTE Solid Fill for 300 iterations.<br>
 *          Synchronization with Vsync is enabled with irqEvenSet(). Total time for 300 iterations printed via SerialMonitor & displayed on LCD too.
 */
void vsyncTest(void)
{
      long vsyncTime=0, time_s=0, time_e=0, looping=300;  
      String result;
      
      ra8876lite.canvasClear(color.Black);        //clear the background to start with...
      ra8876lite.setHwTextColor(color.Black);     //set text color black
      ra8876lite.setHwTextCursor(100,40);         //text cursor set
      ra8876lite.setHwTextParam(color.Cyan,1,1);  //set background Cyan for text
      ra8876lite.putHwString(&ICGROM_16, "VSYNC Test *******************");
          
      ra8876lite.irqEventSet(RA8876_VSYNC_IRQ_ENABLE, 1); //enable Vsync IRQ
      while(looping--)
      {
        #ifdef ESP8266
        yield();  //this is to avoid wdt reset in ESP8266
        #endif
        color.r = random(255);
        color.g = random(255);
        color.b = random(255);
        time_s=time_e=millis();
        ra8876lite.vsyncWait();
        ra8876lite.bteSolidFill(0,random(0,ra8876lite.getCanvasWidth()),random(100,ra8876lite.getCanvasHeight()),random(255),random(255),color);
        time_e= millis();
        result = String(float(time_e-time_s), 2) + "ms";
        ra8876lite.setHwTextColor(color.White);     //set text color white
        ra8876lite.setHwTextCursor(500,40);         //set cursor prior to running ra8876lite.putHwString()
        ra8876lite.setHwTextParam(color.Black,2,3); //set background black with magnified font         
        ra8876lite.putHwString(&ICGROM_16, result);
        vsyncTime += (time_e-time_s); //total loop time to iterate 1000 cycles
      }
      ra8876lite.irqEventSet(RA8876_VSYNC_IRQ_ENABLE, 0);   //disable Vsync IRQ
      printf("Total loop time = %d msec\n",vsyncTime);
      result= String((float)vsyncTime/300,2) + "ms";
      ra8876lite.setHwTextColor(color.White);     //set text color white
      ra8876lite.setHwTextCursor(500,40);         //set cursor prior to running ra8876lite.putHwString()
      ra8876lite.setHwTextParam(color.Black,2,3); //set background black with magnified font  
      ra8876lite.putHwString(&ICGROM_16, result);
}

/**
 * @brief This is a demonstration to display Embedded Characters built-in RA8876 and External Character ROM Genitop GT21L16T1W.<br>
 * Simply input hwFontTest <Send> in Serial Monitor to view this demo.
 */
void hwFontTest(void)
{
  ra8876lite.canvasClear(color.Yellow);   //clear canvas in yellow
  //Example to display ASCII from Embedded Character Set of 8x16 pixels starting from 0x00 to 0xff
  //character starts from x=100, y=50; auto line feed when cursor meets Active Window boundary i.e. when x=1280 in this demo.
  ra8876lite.setHwTextCursor(100,50);  
  ra8876lite.setHwTextColor(color.Blue);  
  ra8876lite.setHwTextParam(color.Transparent, 1,1);
  for(char ch=0x00; ch<0xff; ch++)
  { 
    ra8876lite.putHwChar(&ICGROM_16, ch);
  }  

  uint16_t wch;
  //Example to display 100 characters from '一' to '世'
  ra8876lite.setHwTextCursor(100,100);        //printing position starts from (100,100)
  ra8876lite.setHwTextColor(color.Black);     //set font color black
  ra8876lite.setHwTextParam(color.White, 1,1);//set background white, magnification factor 1:1
  for(wch=0xA440; wch<(0xA440+101); wch++)
  {
    ra8876lite.putHwChar(&XCGROM_BIG5_16, wch);      
  }
  
  //0x0401 means to start from 04区 01点 displaying ぁあぃい...for 300 characters
  //change wch starting value to 0x0501 will display ァアィイ...
  //Reference: http://charset.7jp.net/jis0208.html
  //print wide characters starting from x=100,y=150 with auto line feed when cursor meets x=canvas width=1280 in this demo
  ra8876lite.setHwTextCursor(100,150);
  ra8876lite.setHwTextColor(color.White);
  ra8876lite.setHwTextParam(color.Black, 1,1);
  for(wch=0x0401; wch<(0x0401+301); wch++)
  {
    ra8876lite.putHwChar(&XCGROM_JIS_16, wch);     
  } 
  
  //Example to display 'Ё' to 'ӹ'<br>
  ra8876lite.setHwTextCursor(100,250);
  ra8876lite.setHwTextColor(color.Black);
  ra8876lite.setHwTextParam(color.Cyan, 2,2);
  for(wch=XCGROM_CYRIL_16.FirstChar; wch<XCGROM_CYRIL_16.LastChar+1; wch++)
  {
    ra8876lite.putHwChar(&XCGROM_CYRIL_16, wch);      
  }   

  //Example to display '؟' to '۹' <br>
  ra8876lite.setHwTextCursor(100,400);
  ra8876lite.setHwTextParam(color.Yellow, 1,1);
  for(wch=0x061F; wch<XCGROM_ARABIA_16.LastChar+1; wch++)
  {
    ra8876lite.putHwChar(&XCGROM_ARABIA_16, wch);      
  }    
  
  //Example to display '啊' to '剥' <br>
  ra8876lite.setHwTextCursor(100,500);
  ra8876lite.setHwTextParam(color.Magenta, 2,2);
  for(wch=0xB0A1; wch<0xB0FF; wch++)
  {
    ra8876lite.putHwChar(&XCGROM_GB12345_16, wch);      
  }
}

/**
 * This demo transfer images from Serial Flash W25Q256FV by DMA. 
 * Open Serial Monitor, type dma<Send>. Console will ask for the name of image to show.
 * From gfx_assets[] input one of the image name e.g. kiss<Send> to show it.
 */
void dmaPutImages(void)
{
  uint8_t i; 
  String image = "";
  uint16_t arraySize = sizeof(gfx_assets) / sizeof(xImages);
  
  printf("Input the image name to display...\n");
  image = messageParser();
  
  for(i=0; i<arraySize; i++)
  {
   // if(image==gfx_assets[i].imgName) break;
      if(image.indexOf(gfx_assets[i].imgName)!=-1) break;
  }
  
  if(i<arraySize){
    ra8876lite.dmaDataBlockTransfer(
    0,0,
    gfx_assets[i].imgWidth,gfx_assets[i].imgHeight,
    gfx_assets[i].imgWidth,
    gfx_assets[i].imgAddress);
  }
  else{
    printf("No such image, DMA demo exit. Type dma again to start over.\n");
  }  
}








