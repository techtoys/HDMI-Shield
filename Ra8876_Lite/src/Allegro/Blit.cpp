/**
 * @brief
 * @note	Reference : https://www.allegro.cc/manual/4/api/blitting-and-sprites/
 * 
 */

#include "Blit.h"

/**
 * @brief	Copies a rectangular area of the source BITMAP to the destination BITMAP.
 * @param	*source points to the source BITMAP.
 * @param	*dest points to the destination BITMAP to be copied to.
 * @param	source_x & source_y are the top left corner of the area to copy from the source BITMAP.
 * @param	dest_x and dest_y are the corresponding position in the destination BITMAP.
 * @param	width & height indicate the dimension of the source BITMAP to be copied.
 * @note	This fuction makes use of the hardware feature (BTE engine) of RA8876 to copy graphical content
 *			from one area of the SDRAM to another. 
 *			Comply with legacy Allegro 4.4.x
 */
void blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{	
	if(source_x > source->getWidth() || source_y > source->getHeight()) return;
	if(dest_x > dest->getWidth() || dest_y > dest->getHeight()) return;

	if((dest_x+width) > dest->getWidth())
		width = dest->getWidth()-dest_x;
	
	if((dest_y+height) > dest->getHeight())
		height = dest->getHeight()-dest_y;
	
	ra8876lite.bteMemoryCopyWithROP( 
	source->getAddress(),	//src image physical address
	source->getWidth(),
	source_x,source_y,		
	0,0,0,0,				//set s1_addr, s1_image_width, s1_x, s1_y all '0'
	dest->getAddress(),
	dest->getWidth(),
	dest_x, dest_y,
	width, height,
	RA8876_BTE_ROP_CODE_12
	);	
}

/**
 * @brief	This function works like blit() but skipping transparent pixels defined by MASK_COLOR in blit.h
 * @param	*source points to the source BITMAP.
 * @param	*dest points to the destination BITMAP to be copied to. 
 * @param	source_x & source_y are the top left corner of the area to copy from the source BITMAP.
 * @param	dest_x and dest_y are the corresponding position in the destination BITMAP.
 * @param	width & height indicate the dimension of the source BITMAP to be copied.
 * @note	This fuction makes use of the hardware feature (BTE engine) of RA8876 to copy graphical content
 *			from one area of the SDRAM to another with chroma key. 
 *			Comply with legacy Allegro 4.4.x
 */
void masked_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{	
	if(source_x > source->getWidth() || source_y > source->getHeight()) return;
	if(dest_x > dest->getWidth() || dest_y > dest->getHeight()) return;

	if((dest_x+width) > dest->getWidth())
		width = dest->getWidth()-dest_x;

	if((dest_y+height) > dest->getHeight())
		height = dest->getHeight()-dest_y;
	
	ra8876lite.bteMemoryCopyWithChromaKey( 
	source->getAddress(),	//src image physical address
	source->getWidth(),
	source_x,source_y,		//src image width & height
	dest->getAddress(),
	dest->getWidth(),
	dest_x, dest_y,
	width, height,
	MASK_COLOR
	);	
}

/**
 * @brief	This function works like blit() but now with opacity level.
 * @param	*source points to the source BITMAP.
 * @param	*dest points to the destination BITMAP to be copied to. 
 * @param	source_x & source_y are the top left corner of the area to copy from the source BITMAP.
 * @param	dest_x and dest_y are the corresponding position in the destination BITMAP.
 * @param	width & height indicate the dimension of the source BITMAP to be copied.
 * @param	alpha is the opacity level in 33 steps from 0 - 32 with opaque color level = 0
 */
void alpha_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height, char alpha)
{
	if(source_x > source->getWidth() || source_y > source->getHeight()) return;
	if(dest_x > dest->getWidth() || dest_y > dest->getHeight()) return;
	
	if((dest_x+width) > dest->getWidth())
		width = dest->getWidth()-dest_x;

	if((dest_y+height) > dest->getHeight())
		height = dest->getHeight()-dest_y;
	
	//Output Effect = (S0 image x (1 - alpha setting value)) + (S1 image x alpha setting value)
	//Don't ignore the background because transparency always refer to opacity against a background
	ra8876lite.bteMemoryCopyWithOpacity(
	dest->getAddress(),
	dest->getWidth(),
	dest_x, dest_y,	
	source->getAddress(),
	source->getWidth(),
	source_x,source_y,	
	dest->getAddress(),
	dest->getWidth(),
	dest_x, dest_y,
	width, height,
	alpha
	);	
}













