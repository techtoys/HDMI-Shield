/**
 * @brief This program demonnstrates how Allegro library is used to display an animation with three layers. Transparency is shown too.
 *        Various Arduino boards have beed tested at time of writing including ESP3266, ESP32, Teensy 3.2/3.5, and Arduino DUE.
 *        Initialization of RA8876 and CH7035 encapsulated in two functions:
 *        //color set to 16 bit-per-pixel
 *        set_color_depth(16);
 *        //set gfx mode with ra8876 generates 720p, scale up to 1080p DVI canvas area set to 1280*720.
 *        set_gfx_mode(GFX_HD720p_HD1080p_DVI, 1280, 720);
 *        Global BITMAP* screen required by Allegro created in the function allegro_init().
 *        Screen cleared in red color by clear_to_color(screen, 255<<24).
 *        Next SD Card is initialized. The same SPI bus with different CS lines is used for all systems except ESP32.
 *        There are HSPI and VSPI available from ESP32. It is more efficient to use dual SPI ports to move data from SD card
 *        to frame buffer. Directive #ifdef ESP32...#else...#endif is available to customerize ESP32 for this.
 *        
 *        User need to copy three files fish.bin, sound.bin, and wp23.bin from the \asset folder to a microSD card for this program to work.
 *        After SDcard initialization the binary files will be copied from SD to SDRAM allocated in off-screen area.
 *        Then you will see a loud speaker icon, fished moving across a graphical background with transparency supported, 3 layers.
 *        
 *        Programmer: John Leung @ TechToys www.TechToys.com.hk
 *        
 * @file    Ra8876_allegro_sprite.ino
 * @History 13-03-2018 first release
 */

#include "Ra8876_Lite.h"
#include "HDMI/Ch703x.h"
#include "Allegro/allegro.h"
#include <SD.h>

/**
   RA8876 GFX controller class constructor
*/
Ra8876_Lite ra8876lite(RA8876_XNSCS, RA8876_XNRESET, RA8876_MOSI, RA8876_MISO, RA8876_SCK);
Color   color;
BITMAP *fish;
BITMAP *sound;
BITMAP *flower_bg;

/**
   @brief isr for Vsync, no need here
*/
/*
void isr(void)
{
  ra8876lite.irqEventHandler();
}
*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //pinMode(RA8876_XNINTR, INPUT_PULLUP);                                 //no need here
  //attachInterrupt(digitalPinToInterrupt(RA8876_XNINTR), isr, FALLING);  //no need here
  set_color_depth(16);                              //color set to 16bit-per-pixel
  set_gfx_mode(GFX_HD720p_HD1080p_DVI, 1280, 720);  //set gfx mode with ra8876 generates 720p, scale up to 1080p DVI
                                                    //canvas area set to 1280*720  
  allegro_init();                                   
  clear_to_color(screen, 255<<24);                  //clear screen to red color on startup

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
    { printf("SD card init failed. Have you forgot to insert a sdcard? Program not working with .bin file not present.\n");
      while(1);//infinite loop here
    }
  else
    {
      printf("SD card OK! Now we load binary file from SD Card to SDRAM for buffering.\n");
#ifdef ESP32
    fish = load_binary_sd(480,300, "/fish.bin");
    sound = load_binary_sd(128,128, "/sound.bin");
    flower_bg = load_binary_sd(800,480, "/wp23.bin");  
    //also possible to load from Serial Flash like this if the binary file preloaded
    //BITMAP *flower_bg = load_binary_xflash(1280,720,30780208); 
#else
    fish = load_binary_sd(480,300, "fish.bin");
    sound = load_binary_sd(128,128, "sound.bin");
    flower_bg = load_binary_sd(800,480, "wp23.bin");
#endif
    }
}

void loop() {
    blit(flower_bg, screen,0,0,0,0,848,480);
    SPRITE *sprite_fish = create_sprite(fish, 240, 300);
    SPRITE *sprite_sound = create_sprite(sound, 128, 128);
    
    for(;;){
      uint16_t x;
      // put your main code here, to run repeatedly:
      for(x=0; x<500; x++){
        draw_trans_sprite(flower_bg, sprite_sound, 0, x, 20);
        draw_trans_sprite(flower_bg, sprite_fish, x, 100, 25);
        blit(flower_bg, screen,0,0,0,0,848,480);
        #ifdef ESP8266
        yield();  //this is to avoid wdt reset in ESP8266
        #endif        
        erase_sprite(flower_bg, sprite_fish); //draw in the order 1,2,3, erase in 3,2,1 and so on.
        erase_sprite(flower_bg, sprite_sound);
      }
      for(x=500; x>0; x--){
        draw_trans_sprite(flower_bg, sprite_sound, 0, x, 8);
        draw_trans_sprite(flower_bg, sprite_fish, x, 100, 20);
        blit(flower_bg, screen,0,0,0,0,848,480);
        #ifdef ESP8266
        yield();  //this is to avoid wdt reset in ESP8266
        #endif
        erase_sprite(flower_bg, sprite_fish);
        erase_sprite(flower_bg, sprite_sound); 
      }  
    }
}