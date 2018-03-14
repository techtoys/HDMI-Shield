#include "Sprite.h"

/**
 * @note	Default constructor with BITMAP bgsave created to store backgroung this sprite is going to cover.
 */
SPRITE::SPRITE(BITMAP *graphics, uint16_t width, uint16_t height)
{
	if(graphics!=NULL) {
	frames = graphics;
	bgsave = new BITMAP(width, height);
	w = width;
	h = height;
	}
}

SPRITE::~SPRITE()
{
	if(frames!=NULL) delete frames;
	if(bgsave!=NULL) delete bgsave;
	//printf("Sprite deleted!\n");
}

static BITMAP *grabframe(BITMAP *source,int width,int height,int startx,int starty,int columns,int frame)
{
	if(source==NULL) return NULL;
	
	BITMAP *temp = create_bitmap(width,height);

	if(temp==NULL) return NULL;
	
	int x = startx + (frame % columns) * width;
	int y = starty + (frame / columns) * height;
	blit(source,temp,x,y,0,0,width,height);
	return temp;
}

SPRITE 	*create_sprite(BITMAP *graphics, int width, int height)
{	
	SPRITE *pSprite = new SPRITE(graphics, width, height);
	
	if(pSprite==NULL) return NULL;
	//pSprite->bgsave = create_bitmap(width, height);
	//if(pSprite->bgsave==NULL) return NULL;
	
	return pSprite;
}

void set_sprite_position(SPRITE *sprite, int16_t x, int16_t y)
{
	sprite->updatePosition(x,y);
}

void set_sprite_frame(SPRITE *sprite, uint16_t frame)
{
	sprite->setCurFrame(frame);
}

/**
 * @brief	Draws a copy of the sprite (*sprite) onto the destination bitmap (*bg) at a specified position (x,y).
 * @param	*bg is a pointer to the background BITMAP this sprite covers.
 * @param	*sprite is a pointer to the sprite to be drawn.
 * @param	x,y indicate the position.
 * @note	This is a modified version of masked_blit(sprite, bg, 0, 0, x, y, sprite->getWidth(), sprite->getHeight()) such that
 *			the area of the background BITMAP this sprite covering is saved to a member BITMAP *bgsave of the sprite. 
 *			Function masked_blit() is used here to skip transparent pixels MASK_COLOR defined in Blit.h.
 */
void draw_sprite(BITMAP *bg, SPRITE *sprite, int x, int y)
{
	if(bg==NULL || sprite==NULL) return;
	
	//save the background first
	blit(bg, sprite->bgsave, x, y, 0, 0, sprite->getWidth(), sprite->getHeight());

	int column = (sprite->frames -> getWidth())/sprite->getWidth();
	BITMAP *frame = grabframe(sprite->frames, sprite->getWidth(),sprite->getHeight(),0,0,column,sprite->getCurFrame());

	if(frame!=NULL)
	{
		masked_blit(frame, bg,0,0,x,y,sprite->getWidth(),sprite->getHeight());
		destroy_bitmap(frame);
		sprite->updatePosition(x,y);
	}
}

/**
 * @brief	This function works like draw_sprite() except an extra argument alpha is used to 
 *			indicate the opacity level of the sprite, and the function alpha_blit() is used to 
 *			copy the sprite to a background instead of masked_blit(). MASK_COLOR is also supported.
 * @note	If vsync() is not used it is advised to create a background BIMTAP with all graphics copied
 *			to it. After rendering is finished, a single instruction to move the whole background to
 *			screen to avoid flickering.
 */
void draw_trans_sprite(BITMAP *bg, SPRITE *sprite, int x, int y, char alpha)
{
	if(bg==NULL || sprite==NULL) return;
	
	//save the background first
	blit(bg, sprite->bgsave, x, y, 0, 0, sprite->getWidth(), sprite->getHeight());

	int column = (sprite->frames -> getWidth())/sprite->getWidth();
	BITMAP *frame = grabframe(sprite->frames, sprite->getWidth(),sprite->getHeight(),0,0,column,sprite->getCurFrame());

	if(frame!=NULL)
	{
		masked_blit(frame, bg,0,0,x,y,sprite->getWidth(),sprite->getHeight());
		blit(bg, frame, x,y,0,0,sprite->getWidth(),sprite->getHeight());			//copy a masked version of the frame
		blit(sprite->bgsave, bg, 0,0,x,y,sprite->getWidth(),sprite->getHeight());	//restore the background
		alpha_blit(frame, bg,0,0,x,y,sprite->getWidth(),sprite->getHeight(), alpha);
		destroy_bitmap(frame);
		sprite->updatePosition(x,y);
	}
}

/**
 * @brief	This function erases the sprite with the background redrawn.
 * @param	*bg is a pointer to the background BITMAP where this sprite is covering.
 * @param	*sprite is a pointer to the sprite to be erased.
 */
void erase_sprite(BITMAP *bg, SPRITE *sprite)
{
	if((bg==NULL)||(sprite==NULL)) return;
	
	blit(sprite->bgsave, bg, 0,0,
	sprite->getX(), sprite->getY(), 
	sprite->getWidth(), sprite->getHeight());
}


