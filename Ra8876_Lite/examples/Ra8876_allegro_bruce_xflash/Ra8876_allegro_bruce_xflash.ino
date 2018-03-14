/**
 * @brief This example plays three GIF animations on a leopard background. Some preparation is required with GIF animations converted to sprite sheets in bmp/jpg format.
 *        The bmp/jpg files are converted to binary files with a free conversion tool provided by RAiO.
 *        Afterwards the binary files can be preloaded to W25Q256xxx Winbond Serial Flash for DMA transfer.
 *        Binary files being played are bruce1.bin, bruce2.bin, bruce3.bin, and leopard.bin. 
 *        It also possible to use microSD card to store the binary files and playback from there.
 *        The switch to control program flow is LOAD_BITMAP_XFLASH. Comment this line to use microSD or uncomment to use Serial Flash.
 *
 *        If Teensy is used we can leverage the resourceful Teensy Audio Library for background music.
 *		    A WAV file (lee_voc.WAV) is played from SDCard for Teensy. Digital video and music are carried by a HDMI cable to your HDTV.
 *        
 *		    Various Arduino boards have beed tested at time of writing including ESP3266, ESP32, Teensy 3.2/3.5, and Arduino DUE.
 *
 *        Programmer: John Leung @ TechToys www.TechToys.com.hk
 *        
 * @file    Ra8876_allegro_bruce_xflash.ino
 * @History 9th Dec 2017 first release
 */
 
//comment this line to play from a microSD card. Don't forget to copy bruce1.bin, bruce2.bin, bruce3.bin, leopard.bin and lee_voc.WAV to the SDcard.
#define LOAD_BITMAP_XFLASH  

#include "Ra8876_Lite.h"
#include "HDMI/Ch703x.h"
#include "Allegro/allegro.h"
#include <SD.h>

#if defined (TEENSYDUINO)
#include "Audio.h"
AudioPlaySdWav           playSdWav1;
AudioOutputI2S           i2s1;
AudioConnection          patchCord1(playSdWav1, 0, i2s1, 0);
AudioConnection          patchCord2(playSdWav1, 1, i2s1, 1);  
#endif

/**
   RA8876 GFX controller class constructor
*/
Ra8876_Lite ra8876lite(RA8876_XNSCS, RA8876_XNRESET, RA8876_MOSI, RA8876_MISO, RA8876_SCK);
Color   color;

///@note  A structure to describe images converted by RAiO Image Tool. 
typedef struct xImages{
  const String imgName;
  uint16_t imgWidth;
  uint16_t imgHeight;
  uint32_t imgSize;
  uint32_t imgAddress;  //starting address with data from RAiO Image Tool
}xImages;

///@note  Source of images and their assocated binary file located in ..\images.
///All images restricted dimension to 8192 for w/h, and the width should be a multiple of 4
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

/**
   @brief isr for Vsync
*/
void isr(void)
{
  ra8876lite.irqEventHandler(); //not used here, just for reference
}

//Background BITMAP
BITMAP *bg;
//short animated gif converted to sprite sheets
BITMAP *bruce1,*bruce2,*bruce3;
SPRITE *sprite_bruce1, *sprite_bruce2, *sprite_bruce3;
uint16_t frame=0;
const char quote[] = {"There are only plateaus, and you must not stay there, you must go beyond them : by Bruce Lee."};
long time_s=0, time_e=0;
bool play_bruce2 = false, play_bruce3 = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(RA8876_XNINTR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RA8876_XNINTR), isr, FALLING); //Vsync pin, not used here (for reference only)
  
  set_color_depth(16);                              	//color set to 16bit-per-pixel
  set_gfx_mode(GFX_HD720p_HD1080p_HDMI, 1280, 720);  	//set gfx mode with ra8876 generates 720p, scale up to 1080p HDMI
														//canvas area set to 1280*720
  allegro_init(); 

#if defined (TEENSYDUINO)
    HDMI_Tx.setI2SAudio(0, 0, 1); //enable audio for Teensy 3.2/3.5
    AudioMemory(8);
#else
    HDMI_Tx.setI2SAudio(0, 0, 0); //disable audio for other platforms
#endif

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
        
    //create background, can be some image as the background too
#ifdef LOAD_BITMAP_XFLASH
    bg  = load_binary_xflash(gfx_assets[3].imgWidth, gfx_assets[3].imgHeight, gfx_assets[3].imgAddress);
#else
    bg  = load_binary_sd(1280,720, "leopard.bin");
#endif
    blit(bg, screen, 0,0,0,0,1280,720);

    //print text on background from RA8876's embedded character set
    ra8876lite.setHwTextCursor(100, 640);
    ra8876lite.setHwTextColor(color.Green);
    ra8876lite.setHwTextParam(color.Black, 1, 1);
    
    const char *ch = quote;
    while(*ch!='\0')
    {
      ra8876lite.putHwChar(&ICGROM_16, *ch++); //use internal font to print this quote  
      delay(50);
    } 
#ifdef LOAD_BITMAP_XFLASH
    bruce1  = load_binary_xflash(gfx_assets[0].imgWidth, gfx_assets[0].imgHeight, gfx_assets[0].imgAddress);
#else
    bruce1  = load_binary_sd(1464,1096, "bruce1.bin");        //12 frames
#endif
    ra8876lite.putHwString(&ICGROM_16,".");

#ifdef LOAD_BITMAP_XFLASH
    bruce2  = load_binary_xflash(gfx_assets[1].imgWidth, gfx_assets[1].imgHeight, gfx_assets[1].imgAddress);
#else
    bruce2 = load_binary_sd(4800,820,"bruce2.bin");           //40 frames
#endif
    ra8876lite.putHwString(&ICGROM_16,".");

#ifdef LOAD_BITMAP_XFLASH
    bruce3  = load_binary_xflash(gfx_assets[2].imgWidth, gfx_assets[2].imgHeight, gfx_assets[2].imgAddress);
#else
    bruce3 = load_binary_sd(3968,426,"bruce3.bin");           //16 frames
#endif

    ra8876lite.putHwString(&ICGROM_16,".");

    sprite_bruce1 = create_sprite(bruce1, 488, 274);
    sprite_bruce2 = create_sprite(bruce2, 480, 205);
    sprite_bruce3 = create_sprite(bruce3, 496, 213);

    time_s=time_e=millis();
}

void loop() {
      if((time_e-time_s) > 3000)
      {
        play_bruce2 = true;
        if((time_e-time_s) > 6000)
          play_bruce3 = true;
      }
      //time it takes to update three sprites + background is around 100msec, i.e. the max frame rate is 10fps
      set_sprite_frame(sprite_bruce1, frame%12);
      draw_sprite(bg, sprite_bruce1,700,250);   
      if(play_bruce2)
      {
        set_sprite_frame(sprite_bruce2, frame%40);
        draw_sprite(bg, sprite_bruce2,80,420);   
      }
      if(play_bruce3)
      {
        set_sprite_frame(sprite_bruce3, frame%16);
        draw_sprite(bg, sprite_bruce3,100,80); 
      }
      blit(bg, screen, 0,0,0,0,1280,720);
      
      if(play_bruce3)
        erase_sprite(bg, sprite_bruce3);
      
      frame++;
      time_e=millis();
    //background music available for Teensy 3.2/3.5
    #if defined (TEENSYDUINO)
      if (playSdWav1.isPlaying() == false) {
        Serial.println("Start playing");
        playSdWav1.play("lee_voc.WAV");
        //delay(10); // wait for library to parse WAV & i2s enable to work
      }
    #endif
}