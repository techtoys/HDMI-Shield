/**
 * @brief
 * @note	Reference : https://www.allegro.cc/manual/4/api/blitting-and-sprites/
 * 
 */

#ifndef _BLIT_H
#define _BLIT_H

#include "Bitmap.h"

class BITMAP;

extern Color mask;
///@note	MASK_COLOR (sf::Color object) is used to mask transparent sprite pixels
#define MASK_COLOR		mask.Magenta

/* Starts C function definitions when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

void blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void masked_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void alpha_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height, char alpha);
#ifdef __cplusplus
}
#endif	
/* Ends C function definitions when using C++ */

#endif	//#define _BLIT_H