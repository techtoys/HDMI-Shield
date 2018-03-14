/**
 * @brief
 * @note	Reference : https://www.allegro.cc/manual/4/api/blitting-and-sprites/
 * 
 */

#ifndef _SPRITE_H
#define _SPRITE_H

#include "Bitmap.h"
class BITMAP;

/**
 * @brief	SPRITE Class declaration
 * @note 	Reference: Game Programming All in One by <br>
 *			J. Harbour. Chapter 9: Advanced Sprite Programming
 *			Class modified by John Leung @ TechToys
 */
class SPRITE {
	public:
		SPRITE(BITMAP *graphics, uint16_t width, uint16_t height);
		~SPRITE();
		BITMAP		*bgsave=NULL;	//pointer to background the sprite covering
		BITMAP		*frames=NULL;	//pointer to BITMAP of matrix n*m frames
		uint16_t 	getWidth(){return w;}
		uint16_t	getHeight(){return h;}
		uint16_t	getCurFrame(){return curframe;}
		void		setCurFrame(uint16_t frame) {curframe = frame;}
		void		updatePosition(int16_t x, int16_t y){pos_x = x; pos_y = y;}
		uint16_t	getX(void){return pos_x;}
		uint16_t	getY(void){return pos_y;}
	private:
		uint16_t	w,h;
		int16_t		pos_x=0,pos_y=0;	//sprite position
		int16_t 	xspeed=0, yspeed=0;//velocity elements in pixel
		uint16_t	curframe=0;
};

/* Starts C function definitions when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

static 	BITMAP *grabframe(BITMAP *source,int width,int height,int startx,int starty,int columns,int frame);

SPRITE 	*create_sprite(BITMAP *graphics, int width, int height);
void 	destroy_sprite(SPRITE *sprite);
void 	set_sprite_speed(SPRITE *sprite, int16_t xspeed, int16_t yspeed);
void 	set_sprite_position(SPRITE *sprite, int16_t x, int16_t y);
void 	set_sprite_frame(SPRITE *sprite, uint16_t frame);
//void 	set_sprite_animation(SPRITE *sprite, uint16_t framedelay, int16_t dir);
void	draw_sprite(BITMAP *bg, SPRITE *sprite, int x, int y);
void	draw_trans_sprite(BITMAP *bg, SPRITE *sprite, int x, int y, char alpha);
void 	erase_sprite(BITMAP *bg, SPRITE *sprite);

#ifdef __cplusplus
}
#endif	
/* Ends C function definitions when using C++ */

#endif	//#define _SPRITE_H

