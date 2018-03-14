/**
    @brief  Example to display a menu for restaurant by WiFi or Serial Monitor.<br>
    
    @note   Major tasks demonstrated in this example are:<br>
            (1) Initialize RA8876 to generate a RGB video source of 800x480 @ 60Hz.<br>
            (2) Initialize CH7035B to convert the video source to a VESA-compatible signal of 1280x768 @ 60Hz.<br>
            (3) Initialize SD library.<br>
            (4) Initialize ArduinoJson library (http://arduinojson.org/).<br>
            (5) Use BitFontCreator Grayscale (v4.5) to create antialiased 4-bpp bitmap fonts in binary format from a truetype font Bradley_Hand installed in PC.<br>
                The binary files created (Brad60.bin, Brad44.bin, & Brad34.bin) are copied to a 8GB microSD card.<br>
            (6) Divide SDRAM into various regions:<br>
                0 - line number 479 as the Main Window (This is the visible display area).<br>
                480 - 959 as frame buffer (2D copy from frame buffer to Main Window only after rendering finished. This avoid snail-like pixel raster).<br>
                960 - 1439 as storage for a background image when the monitor is positioned in landscape.<br>
                1440 - 1919 as storage for a background image when the monitor is positioned in portrait.<br>
            (7) Direct render a picture (in logo.bin) to screen centre together with a message "Loading..." from embedded character set of RA8876 to <br>
                the Main Window as a simple user interface on system startup.<br>
                If the host mcu is ESP8266, ESP8266WiFi.h library will be utilized to setup a web server to accept JSON messages over WiFi.<br>
                The ip address of web server will be printed on screen top.<br>
                A simple Android App developed by MIT App Inventor 2 is provided to update the menu in landscape or Portrait.<br>
                Courses can be edited and updated in wireless manner, triggered by a simple <update> button in the App.<br>
                If the host mcu is not ESP8266 (Arduino Due/Teensy/M0), command in JSON format is supported over Serial Monitor. <br>

            Following hardware is assumed :<br>
            (1) HDMI Shield Shield of DTE 20171228 or later. <br>
            (2) A Due Zipper Board of DTE 20171024 or later. Jumpers set to P13 if Arduino Due is used as the host. Jumpers set to P12 to use ESP8266 as host + Web server.<br>
            (3) A monitor with DVI or HDMI interface compliant with VESA timing standard 1280*768 or higher
            (4) Compatible hosts tested so far are ESP8266, Arduino DUE, Genuino 101 (direct stack), Teensy 3.2 & 3.5.<br>
            (5) Android smart phone and a 8GB microSD card.<br>
          
            Debug via Serial Monitor is available for all hosts including ESP8266. Therefore it is possible to update the restaurant menu by WiFi or Serial Monitor for ESP8266.<br>
            (1) Open Serial Monitor, set to Newline with baud rate set to 115200baud. <br>
            (2) Send a JSON command by typing<br>
                {"Orientation":"Landscape","Appetizer":"Butternut Squash Ravioli","Salad":"Baby Kale Greens","Main Course":"Garlic Roast Chicken","Dessert":"Chocolate Cake"}<br>
                Change orientation to Portrait with this command<br>
                {"Orientation":"Portrait","Appetizer":"Butternut Squash Ravioli","Salad":"Baby Kale Greens","Main Course":"Garlic Roast Chicken","Dessert":"Chocolate Cake"}<br>
                If you want to display more than one dish for Salad say, input this:
                {"Orientation":"Portrait","Appetizer":"Butternut Squash Ravioli","Salad":"Baby Kale Greens \n Thai Salad","Main Course":"Garlic Roast Chicken","Dessert":"Chocolate Cake"}<br>

            Trying to update the menu with a smart phone.<br>
            (1) Install the apk provided with this demo.<br>
            (2) Make sure you are using  Due Zipper Board of DTE 20171024 or later with jumpers set to P12 to use ESP8266.<br>
            (3) From Arduino IDE set board to Generic ESP8266 Module. Flash size 4M (1M SPIFFS). IMPORTANT: type in the ssid and password in this file matching your router.<br>
                Rebuild and download this sketch to ESP8266. Don't forget to bring it to download mode with PROG key press-n-hold then RESET.
            (4) Restart Due Zipper Board with power up or reset key press. From top right corner the web server IP will be displayed.<br>
            (5) Launch hello_world Android App, input the IP address in the text box. Click <update>. At the same time you may open Serial Monitor to observe some debug messages.<br>
             
   @file     RestaurantMenu.ino
   @author   John Leung @ TechToys (www.TechToys.com.hk)
   @section  HISTORY
   Date      6th Dec 2017 first release
*/

/**
 * Known bug :  When Ra8876_Lite::bfc_DrawChar_RowRowUnpacked() is repeatly called, SD.open(pFilename) inside sometimes fail even if the file exists.
 *              It is advised to copy all font data to SDRAM prior to main loop or move binary data to external serial Flash.
 */

#include "Ra8876_Lite.h"
#include "HDMI/Ch703x.h"
#include <SD.h>
// ArduinoJson - arduinojson.org
#include <ArduinoJson.h>

#if defined (ESP8266)
#include <ESP8266WiFi.h>
#elif defined (ESP32)
#include <WiFi.h>
#endif

/**
   RA8876 GFX controller class constructor
*/
Ra8876_Lite ra8876lite(RA8876_XNSCS, RA8876_XNRESET, RA8876_MOSI, RA8876_MISO, RA8876_SCK);
Color   color;

/**
 * The max. JSON buffer length (512) was calculated by an online tool ArduinoJson Assistant at http://arduinojson.org/assistant/ <br>
 * with input set to an arbitrary menu like the Json message below. Multiline courses is separated by terminator '\n'. <br>
{"Appetizer":"Butternut Squash Ravioli \n werertrerrettret \n wertre",
  "Salad":"Caesar Salad \n rertertrrtdsfdsf \n rereretert \nrrttrt",
  "Main Course":"Garlic Roast Chicken \n werttet \n trtrrtrtyt",
  "Dessert":"Vanilla ice-cream \n reyyuioitret \n erxtyxertrtxx",
  "Orientation":"Portrait"}
 */
const uint16_t MAX_JSON_LEN = 512;  
const uint16_t MAX_ARR_LEN  = 256;  //It is the max. incoming character length by Serial or Wifi

///@note  Handle JSON messages sent over Serial Monitor
void    jsonOverSerial(void);
///@note  All GUI components handled in screenUpdateTask()
void    screenUpdateTask(void);
///@note  The major function in this example to decode incoming message and get it displayed
bool    decodeJsonMsg(String msg);
///@note  Split a string and separate it based on a given character (cDelim) and return the item between separators in sParams[]
int     StringSplit(String sInput, char cDelim, String sParams[], int iMaxParams);

///@note frame buffer related functions prefix with FB_
//@note Function to update visible area with frame buffer
void    FB_refresh(void);     
//@note Function to clear the frame buffer with a color        
void    FB_clear(Color color);        //clear frame buffer with a pure color
//@note Function to fill the frame buffer with a preloaded background starting from lnOffset
void    FB_fill(uint32_t lnOffset); 

const   uint32_t FB_StartX = 0;               //Frame buffer start x-coordinate
const   uint32_t FB_StartY = 480;             //Frame buffer start y-coordinate, in this case it is 480means outside of the main window
const   uint32_t FB_StartY_menuL = 2*480l;    //starting position for menu in landscape (800x480), filename menuL.bin
const   uint32_t FB_StartY_menuP = 3*480l;    //starting position for menu in portrait (800x480), filename menuP.bin

///@note The array - courses[] define the strings that are going to appear on the restaurant menu for each course
const   char* courses[] = {"Appetizer", "Salad", "Main Course", "Dessert"};
//starting position in y-direction for the word "Menu" when the monitor configured in Landscape (width=800, height=480)
const   uint16_t MENU_StartY_menuL  = 18;
//starting position in y-direction for "Menu" when it is in Portrait.
//It is not the same as MENU_StartY_menuL because (x,y) will be swapped when the monitor is configured in Portrait (i.e. width=480, height=800 in Portrait)
const   uint16_t MENU_StartY_menuP  = 180;    

#ifdef ESP32
const char* logoFilename        = "/logo.bin";
const char* landscapeFilename   = "/menuL.bin";
const char* portraitFilename    = "/menuP.bin";
const char* fontMenuFilename    = "/Brad60.bin"; ///font for "Menu". Possible to change it with another font here.
const char* fontCoursesFilename = "/Brad44.bin"; ///font for {"Appetizer", "Salad", "Main Course", "Dessert"}
const char* fontDishFilename    = "/Brad34.bin"; ///font for each dish
#else
const char* logoFilename        = "logo.bin";
const char* landscapeFilename   = "menuL.bin";
const char* portraitFilename    = "menuP.bin";
const char* fontMenuFilename    = "Brad60.bin"; ///font for "Menu". Possible to change it with another font here.
const char* fontCoursesFilename = "Brad44.bin"; ///font for {"Appetizer", "Salad", "Main Course", "Dessert"}
const char* fontDishFilename    = "Brad34.bin"; ///font for each dish
#endif

#if defined (ESP8266) || defined (ESP32)
const char* ssid = "your-ssid";
const char* password = "your-pw";
WiFiServer server(80);
///@note Handle JSON messages sent over WiFi
void  jsonOverWifi(void);
#endif

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
  attachInterrupt(digitalPinToInterrupt(RA8876_XNINTR), isr, FALLING); //Vsync pin

  ///@note  Initialize RA8876 starts from here
  if (!ra8876lite.begin(&WVGA_800x480_60Hz))  //init RA8876 with a video resolution of 800x480 @ 60Hz with map defined in .\HDMI\LcdParam.h
  {
#ifdef DEBUG_LLD_RA8876
    printf("RA8876 or RA8877 Fail\n");
#endif
  }
  else
  {
    ra8876lite.canvasImageBuffer(800, 480);     //Canvas set to the same size of 800*480 in default 16 bit-per-pixel color depth
    ra8876lite.displayMainWindow();             //align display window to the same canvas starting point
    ra8876lite.canvasClear(color.White);        //clear the Main window in black
    ra8876lite.graphicMode(true);
    ra8876lite.displayOn(true);

    HDMI_Tx.begin();                  //Init I2C
    HDMI_Tx.init(VIDEO_in_800x480_out_DVI_1280x768_60Hz); //map defined in .\HDMI\videoInOutMap.h
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
    printf("SD card init failed. Program halted.\n"); 
    ra8876lite.setHwTextCursor(100, 100);
    ra8876lite.setHwTextColor(color.Black);
    ra8876lite.setHwTextParam(color.Transparent, 1, 1);
    //alert the user here
    ra8876lite.putHwString(&ICGROM_16, "SD card init failed. Program halted.");
  
    while(1);   //program halted up to this line if there is no SD card.
   }
  else
    printf("SD card OK!\n");
   
    //Direct render the restaurant logo to Main Window
    ra8876lite.putPicture((800-300)/2,(480-300)/2,300,300,logoFilename,0,0); 

#if defined (ESP8266) || defined (ESP32)

  // Print the IP address on TV
  ra8876lite.setHwTextCursor(10, 10);
  ra8876lite.setHwTextColor(color.Black);
  ra8876lite.setHwTextParam(color.Transparent, 1, 1);
  ra8876lite.putHwString(&ICGROM_16, "Connecting to AP..."); //use internal font to print IP address  

  Serial.print("Connecting to ");Serial.println(ssid);
  WiFi.begin(ssid, password);
  iteration = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (iteration++ == 30) break;
    Serial.print(".");
    ra8876lite.putHwString(&ICGROM_16, ".");
  }
 
  if (iteration > 30)
  {
    Serial.println("WiFi AP not found. Please make sure ssid and password are correct.");
    ra8876lite.putHwString(&ICGROM_16, "WiFi AP not found!");
  }
  else
  {
    ///start http server
    server.begin();
    ra8876lite.putHwString(&ICGROM_16, "Web Server IP " + WiFi.localIP().toString()); //use internal font to print IP address
    Serial.print("Server IP = ");
    Serial.println(WiFi.localIP()); //eq. IPAddress ip = WiFi.localIP(); printf("IP Add = %s\n", ip.toString().c_str());
  }
#endif  //#ifdef ESP8266
    ///@note  For some unknown reason, placing this section above WiFi.begin(ssid,password) will paralyze connection of 
    ///       ESP8266 to an access point (AP). 
    ra8876lite.setHwTextCursor(320, 440);
    ra8876lite.setHwTextColor(color.Black);
    ra8876lite.setHwTextParam(color.Transparent, 1, 1);
    ra8876lite.putHwString(&ICGROM_16, "Loading...");   
    ///@note start loading menuL and menuP pictures to SDRAM
    //rotate_ccw90=0 for not rotate, lnOffset=FB_StartY_menuL
    ra8876lite.putPicture(0,0,800,480,landscapeFilename,0,FB_StartY_menuL); 
    //rotate_ccw90=0 for not rotate (graphics rotated by PS already), lnOffset=FB_StartY_menuP
    ra8876lite.putPicture(0,0,800,480,portraitFilename,0,FB_StartY_menuP); 
    ///auto position increment from ra8876lite.putHwString(&ICGROM_16, "Loading...") above
    ra8876lite.putHwString(&ICGROM_16, "finished.");
    
    Serial.println("Setup done!");
}

void loop() {
#if defined (ESP8266) || defined (ESP32)
  jsonOverWifi();
#endif
  jsonOverSerial();
}
/**
 * ***********************************************************************************************************************************************
   End of loop()
 * ***********************************************************************************************************************************************
*/

/**
* @brief  Update the Main Window (visible area) with frame buffer.<br>
*         All rendering tasks performed on the frame buffer is not seen until this function is called to copy the frame buffer to Main Window.<br>
*         This can avoid snail-like pixel-by-pixel plot for slow microcontrollers because rendering is done in the background.
*/
void    FB_refresh(void)
{
  ra8876lite.vsyncWait();
  //2D BTE copy from frame buffer to Main Window
  ra8876lite.bteMemoryCopyWithROP(
    0,
    ra8876lite.getCanvasWidth(),
    FB_StartX, FB_StartY, //s0_x, s0_y
    0, 0, 0, 0,
    0,
    ra8876lite.getCanvasWidth(),
    0, 0,                 //des_x, des_y
    ra8876lite.getCanvasWidth(), ra8876lite.getCanvasHeight(), //copy_width,copy_height
    RA8876_BTE_ROP_CODE_12);
}

/**
   @brief clear frame buffer with a pure color
*/
void    FB_clear(Color color)
{
  ra8876lite.canvasClear(color, FB_StartX, FB_StartY);
}

/**
   @brief JSON message handler for strings sending from Serial Monitor
   @note  Open Serial Monitor, set to Carriage return, 115200bps. Send
   {"Appetizer":"Butternut Squash Ravioli","Salad":"Caesar Salad","Main Course":"Garlic Roast Chicken","Dessert":"Vanilla ice-cream","Orientation":"Portrait"}
*/
void    jsonOverSerial(void)
{
  if (Serial.available()) {   
    //local array to store incoming characters from Serial Monitor
    char inChar[MAX_ARR_LEN]={0};
    //read until \r received from serial Monitor
    Serial.readBytesUntil('\r', inChar, MAX_ARR_LEN);
    //print the original message
    Serial.print("Original Serial message: ");          
    Serial.println(inChar);

    if(decodeJsonMsg(String(inChar))==false)
    {
      Serial.println("decodeJsonMsg() in  jsonOverSerial() failed!!!");
      return;
    }
  } //if (Serial.available()) {}
}

#if defined (ESP8266) || defined (ESP32)
/**
   @brief Launch Android App or debug from source code in MIT App Inventor 2
   @note  Open Serial Monitor for monitoring, and click Update button from Android app supplied.<br>
          Android app is sending over strings in JSON format like this<br>
          GET /{"Appetizer":"Butternut Squash Ravioli","Salad":"Caesar Salad","Main Course":"Garlic Roast Chicken","Dessert":"Vanilla ice-cream","Orientation":"Landscape"} HTTP/1.1 <br>
          This function is to pool for any client connected and wait for an incoming HTTP connection. If it complies with the JSON definition the restaurant menu is updated.
*/
void  jsonOverWifi(void)
{
  WiFiClient client = server.available();

  if (client) {    
    Serial.println("new client connected.");
    //Reserve memory space for JSON object, inside the brackets MAX_JSON_LEN is the size of the pool in bytes estimated
    //by an online tool ArduinoJson Assistant at http://arduinojson.org/assistant/
    StaticJsonBuffer<MAX_JSON_LEN> jsonBuffer;

    char inChar[MAX_ARR_LEN] = {0};
    // Read the first line of the HTTP message over Wifi into inChar array until \r or hit MAX_ARR_LEN
    client.readBytesUntil('\r',inChar,MAX_ARR_LEN);

    // Prepare the response
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n Incoming message received OK ";
    s += "</html>\n";

    // Send the response to the client
    client.print(s);
    
    Serial.print("Original JSON message over Wifi: ");
    Serial.println(inChar);    
    // First line of HTTP request looks like "GET /{"Appetizer":"Butternut Squash Ravioli","xxx":"yy",...} HTTP/1.1"
    // Retrieve the "{...}" part by skipping GET / and HTTP/1.1
    // Wanted to use HTTP POST but don't know how to use HTTP POST in MIT App Inventor 2!!!

    String menuString(inChar);                  ///convert to a String to take advantage of indexOf() and substring() functions below
    int addr_start = menuString.indexOf('{');   ///look for opening bracket '{'
    int addr_end = menuString.indexOf('}');     ///closing bracket '}'

    if (addr_start == -1 || addr_end == -1) {
      Serial.print("It is not a valid JSON message: ");
      return;
    }
 
    menuString = menuString.substring(addr_start, addr_end + 1); ///extract the part {"Apettizer":"xx",....,"Orientation":"Portrait"}
    //bug here for ESP32
    if(decodeJsonMsg(menuString)==false)
    {
      Serial.println("decodeJsonMsg() in  jsonOverWifi() failed!!!");
      client.stop();
      Serial.println("Client disconnected");
      return;
    }
    
  }
}
#endif

/**
 * @brief Fill the frame buffer with an image of starting postion at lnOffset. Also print the text "Menu" on top
 * @param lnOffset is the line offset in y
 */
void FB_fill(uint32_t lnOffset)
{
      ra8876lite.bteMemoryCopyWithROP(
      0,
      ra8876lite.getCanvasWidth(),
      FB_StartX, lnOffset, //s0_x, s0_y
      0, 0, 0, 0,
      0,
      ra8876lite.getCanvasWidth(),
      FB_StartX, FB_StartY,       //des_x, des_y
      ra8876lite.getCanvasWidth(), ra8876lite.getCanvasHeight(), //copy_width,copy_height
      RA8876_BTE_ROP_CODE_12); 

      uint16_t cursorX, cursorY = MENU_StartY_menuL;
      uint16_t _width = 800;
      bool _rotateCcw90 = false;
      
      if(lnOffset==FB_StartY_menuP)
      {
        _width = 480;
        _rotateCcw90 = true;
        cursorY = MENU_StartY_menuP;
      }
      cursorX = (_width-ra8876lite.getBfcStringWidth(fontMenuFilename, "Menu"))/2; 
      ra8876lite.putBfcString(cursorX, cursorY, fontMenuFilename, "Menu", color.Black, color.Transparent,_rotateCcw90,FB_StartY);
}

/**
 * @brief Decode incoming JSON message and get it displayed on the monitor.
 * @param msg is a string fetched from either Serial Monitor or Wifi. Format in JSON as follows:<br>
 *        {"Appetizer":"Butternut Squash Ravioli","Salad":"Caesar Salad","Main Course":"Garlic Roast Chicken","Dessert":"Vanilla ice-cream","Orientation":"Landscape"}.<br>
 *        or <br>
 *        {"Appetizer":"Butternut Squash Ravioli \n Apple-Blue Cheese Chutney","Salad":"Caesar Salad","Main Course":"Garlic Roast Chicken","Dessert":"Vanilla ice-cream","Orientation":"Landscape"}.<br>
 *        '\n' is the delimiter for multiple dishes<br>
 *        The keys are 4 constant strings defined in const char* courses = {"Appetizer", "Salad", "Main Course", "Dessert"}.<br>
 *        If either of the keys doesn't match, content for that key will be skipped.
 */
bool    decodeJsonMsg(String msg)
{
  const char *dish;
  uint16_t cursorX, cursorY;
  //Reserve memory space for JSON object, inside the brackets MAX_JSON_LEN is the size of the pool in bytes,
  StaticJsonBuffer<MAX_JSON_LEN> jsonBuffer;
  //Deserialize incoming character array inChar[] with inChar modified by inserting string endings \0 & translate escaped characters (e.g. \n or \t)
  JsonObject& menuJson = jsonBuffer.parseObject(msg);
  
    if (menuJson.success()) {
      bool _rotateCcw90 = false;
      uint16_t _width = 800;
      dish = (const char*) menuJson["Orientation"];
      
      if(!strcmp(dish,"Landscape"))
      {
        FB_fill(FB_StartY_menuL);
        cursorY = MENU_StartY_menuL + ra8876lite.getBfcFontHeight(fontMenuFilename);  //start line to print "Appetizer"
      }
      else if (!strcmp(dish,"Portrait"))
      {
        FB_fill(FB_StartY_menuP);
        cursorY = MENU_StartY_menuP + (2*ra8876lite.getBfcFontHeight(fontMenuFilename));
        _rotateCcw90 = true;
        _width = 480;
      }
      else
      {
        Serial.println("It is not a valid JSON for our menu.");
        Serial.println("Correct format is {\"Orientation\":\"Landscape\",\"Appetizer\":\"...\",...}");
        Serial.println("-or-");
        Serial.println("Correct format is {\"Orientation\":\"Portrait\",\"Appetizer\":\"...\",...}");
        return false;
      }

        ///update the background first to get a view on text rendering. 
        ///An alternative is to draw on frame buffer then call FB_refresh() after the for-loop below (need to change the last param 0->FB_StartY in putBfcString())
        FB_refresh();
        
        for(uint8_t i=0; i<4; i++)
        {
              cursorX = (_width-ra8876lite.getBfcStringWidth(fontCoursesFilename, courses[i]))/2;
              ///print "Appetizer"|"Salad"|"Main Course"|"Dessert"
              ra8876lite.putBfcString(cursorX, cursorY, fontCoursesFilename, courses[i], color.Black, color.Transparent,_rotateCcw90,0);  
              //Serial.print("CursorY is "); Serial.print(cursorY); Serial.print(" ");Serial.println(courses[i]);
              cursorY += ra8876lite.getBfcFontHeight(fontCoursesFilename);
              
              dish = (const char*) menuJson[courses[i]];

              if(dish){//parse and display only for valid key(s)
                  String sParams[5]; //limit it to max 5 selections for each course
                  int iCount=StringSplit(String(dish), '\n', sParams, 5);
                  for(uint8_t j=0; j<iCount; j++)
                  {
                    cursorX = (_width-ra8876lite.getBfcStringWidth(fontDishFilename, '(' + String(j+1) + ") " + sParams[j]))/2; 
                    ra8876lite.putBfcString(cursorX, cursorY, fontDishFilename, '(' + String(j+1) + ") " + sParams[j], color.Black, color.Transparent,_rotateCcw90,0);                    
                    //Serial.print("CursorY is "); Serial.print(cursorY); Serial.print(" ");Serial.println(sParams[j]);
                    (j!=iCount-1)? cursorY += ra8876lite.getBfcFontHeight(fontDishFilename):cursorY += ra8876lite.getBfcFontHeight(fontCoursesFilename);  
                  }
              }//if(dish)
         }//for(uint8_t i=0; i<4; i++)
      return true;
    } //if (menuJson.success())
    else
    {
      return false;
    }
} //decodeJsonMsg(String msg)

/**
 * @brief   This function splits a String into substrings
 * @param   String sInput: the input lines to be parsed
 * @param   char cDelim: the delimiter character between parameters
 * @param   String sParams[]: the output array of parameters
 * @param   int iMaxParams: the maximum number of parameters
 * @return  the number of parsed parameters
 * @note    https://arduino.stackexchange.com/questions/1013/how-do-i-split-an-incoming-string
 */
int StringSplit(String sInput, char cDelim, String sParams[], int iMaxParams)
{
    int iParamCount = 0;
    int iPosDelim, iPosStart = 0;

    do {
        // Searching the delimiter using indexOf()
        iPosDelim = sInput.indexOf(cDelim,iPosStart);
        if (iPosDelim > (iPosStart+1)) {
            // Adding a new parameter using substring() 
            sParams[iParamCount] = sInput.substring(iPosStart,iPosDelim); //sInput.substring(iPosStart,iPosDelim-1);
            iParamCount++;
            // Checking the number of parameters
            if (iParamCount >= iMaxParams) {
                return (iParamCount);
            }
            iPosStart = iPosDelim + 1;
        }
    } while (iPosDelim >= 0);
    if (iParamCount < iMaxParams) {
        // Adding the last parameter as the end of the line
        sParams[iParamCount] = sInput.substring(iPosStart);
        iParamCount++;
    }

    return (iParamCount);
}
