#ifndef _BITMAP_H
#define _BITMAP_H

#include "Ra8876_Lite.h"
#include "memory/memory.h"
#include "Allegro/allegro.h"

/**
 * @brief	BITMAP Class declaration
 */
class BITMAP {
	public:
		BITMAP(uint16_t width, uint16_t height);
		~BITMAP();
		
		uint16_t getWidth(){return w;}
		uint16_t getHeight(){return h;}
		uint32_t getAddress() {return thisBitmapAddress;}
		bool	 getClipState(){return clipping;}
		void	 setClipState(bool state);
		void	 setClipRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
		void	 getClipRect(uint16_t *x1, uint16_t *y1, uint16_t *x2, uint16_t *y2);
	private:
		uint16_t w;	//width of this BITMAP
		uint16_t h;	//height of this BITMAP
		uint32_t thisBitmapAddress;	//address offset allocated in SDRAM of Ra8876
		bool	 clipping;	//clipping to be turned on when true
		//clip rectangle left, right, top, and bottom (inclusive), 
		//nothing will be drawn on this BITMAP outside the clip rectangle
		uint16_t cl,cr,ct,cb;
};
extern BITMAP *screen;	//global BITMAP of dimension CANVAS_WIDTH & CANVAS_HEIGHT for the visible screen


/* Starts C function definitions when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

BITMAP*	create_bitmap(int width, int height);
BITMAP*	load_flash(int width, int height, const void *flash);
#if defined (LOAD_SD_LIBRARY) || defined (LOAD_SDFAT_LIBRARY)
BITMAP* load_binary_sd(int width, int height, const char *pFilename);
#endif
BITMAP* load_binary_xflash(int picture_width, int picture_height, long src_addr);

void 	destroy_bitmap(BITMAP *bitmap);
void 	set_clip_state(BITMAP *bitmap, int state);
int 	get_clip_state(BITMAP *bitmap);
void	set_clip_rect(BITMAP *bitmap, int x1, int y1, int x2, int y2);
void	get_clip_rect(BITMAP *bitmap, int *x1, int *y1, int *x2, int *y2);

void 	rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
void	rect(BITMAP *bmp, int x1, int y1, int x2, int y2, int color); 
void	clear_bitmap(BITMAP *bitmap);
void	clear_to_color(BITMAP *bitmap, int color);

void	textout_ex(BITMAP *bmp, const BFC_FONT *f, const char *s, int x, int y, int color, int bg);
#ifdef __cplusplus
}
#endif	
/* Ends C function definitions when using C++ */

#endif
