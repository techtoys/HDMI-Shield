#include "allegro.h"

/**
 * @note	Allegro creates a global screen pointer *screen allegro_init is called.
 *			For RA8876, this pointer is equivalent to the Canvas with starting address
 *			defined by CANVAS_OFFSET and its width/height defined by CANVAS_WIDTH/CANVAS_HEIGHT
 *			in Ra8876_Lite.h. 
 */
BITMAP *screen = NULL;
Memory *mmu = new Memory();
static int color_depth = 16;

 /****************************************************************************/
 /* C functions (inspired by Allegro 4.4.2 API) */
 /****************************************************************************/

/**
 * @brief	Function to initialize the Allegro library and create a global BITMAP *screen.
 * @return	'0' on success
 *			'-1' on failure
 */
int allegro_init(void)
{							
	screen = create_bitmap(VIRTUAL_W, VIRTUAL_H);
	//screen = create_bitmap(VIRTUAL_W, 240);
	//printf("Virtual w & h = %d, %d\n", VIRTUAL_W, VIRTUAL_H);
	
	if(screen==NULL) 
		return -1;
	else
		return 0;
}

/**
 * @brief	Function to closes down the Allegro system and free all memory
 * @note	It is your job to delete all created BITMAPs in your program.
 *			This function only delete and free memory for the global *screen
 *			and dynamic array memory_tbl[]
 */
void allegro_exit(void)
{
	delete screen;
	delete mmu;
}

/**
 * @brief	Sets the pixel format to be used by subsequent calls to set_gfx_mode() and create_bitmap().
 * @param	depth is the color depth in bit-per-pixel. Valid depths are 8, 16, & 24 for RA8876.
 * @note	Default color depth is 16bpp.
 */
void set_color_depth(int depth)
{
	if(depth==8 || depth==16 || depth==24)
		color_depth = depth;
}

/**
 * @brief	This function initializes RA8876 and CH7035B and set RA8876 in graphics mode.
 * @param	card is a parameter selected from the enum GFX_VIDEO_MODE defined in allegro.h.
 * @param	v_w is the virtual screen width equivalent to Canvas width of RA8876.
 * @param	v_h is the virtual screen height equivalent to Canvas height of RA8876.
 * @return	'0' on success
 *			'-1' on failure
 * @note	The color depth of the graphic mode has to be specified before calling this function 
 *			with set_color_depth(); otherwise, the default 16 bit-per-pixel color will be applied.
 *			The input resolution of the RGB signal generated by RA8876 is specified by the prefix 
 *			GFX_ZZZ with _ZZZ as the video format e.g. VGA(640x480), WVGA(800x480), etc. 
 *			Video frames will be multiplied by a scaling engine of CH7035B HDMI encoder for higher
 *			resolution that fits a HDTV/LCD monitor. Output HDMI resolution is specified by the suffixes 
 *			_YYY in GFX_ZZZ_YYY.<br>
 *
 *			Example to use:<br>
 *			set_color_depth(8);									//set a color depth 8bpp
 *			set_gfx_mode(GFX_FWVGA_HD720p_HDMI, 848, 480);		//RA8876 generates 848x480 FWVGA @60Hz
 *																//CH7035B upscale it to 1280x720p @60Hz HDMI interface
 *																//set canvas size 848x480 for the input (not output)
 
 *			set_color_depth(16);									//set a color depth 16bpp
 *			set_gfx_mode(GFX_HD720p_HD1080i_HDMI, 1280*2, 720*2);	//RA8876 generates 1280x720 @60Hz (720p) HDMI interface
 *																	//CH7035B upscale it to 1080i interlaced HDMI 
 *																	//v_w & v_h set to 1280*2 & 720*2 for screen scrolling & panning
 *
 */
int set_gfx_mode(int card, int v_w, int v_h)
{
	LCDParam video_in;
	COLOR_MODE _mode;
	sf::Color color;
	
	switch (card)
	{
		case GFX_AUTO_DETECT:	//use 9904 bootROM for 1280x720 @60Hz input
			break;
		case GFX_VGA_SVGA_DVI:
			video_in = CEA_640x480p_60Hz;	
			break;
		case GFX_WVGA_WXGA_DVI:
			video_in = WVGA_800x480_60Hz;
			break;	
		case GFX_SVGA_XGA_DVI:
		case GFX_SVGA_SXGA_DVI:
		case GFX_SVGA_UXGA_DVI:
			video_in = SVGA_800x600_60Hz;
			break;		
		case GFX_FWVGA_HD720p_HDMI:
			video_in = FWVGA_848x480_60Hz;
			break;
		case GFX_HD480p_HD720p_HDMI:
		case GFX_HD480p_HD720p_DVI:
			video_in = CEA_720x480p_60Hz;
			break;
		case GFX_HD576p_SXGA_HDMI:
			video_in = CEA_720x576p_50Hz;
			break;
		case GFX_HD720p_HD1080i_HDMI:
		case GFX_HD720p_HD1080p_HDMI:
		case GFX_HD720p_HD1080p_DVI:
			video_in = CEA_1280x720p_60Hz;
			break;
		case GFX_WQVGA_HD480p_HDMI:
		case GFX_WQVGA_HD720p_HDMI:
		case GFX_WQVGA_HD1080p_HDMI:
			video_in = WQVGA_480x272_60Hz;
			break;
		default:
			printf("Incorrect 'card' argument in set_gfx_mode() call.\n");
			return -1;
	}
	
	bool _success = 0;
	
	if(card!=GFX_AUTO_DETECT)
		_success = ra8876lite.begin(&video_in);
	else
		_success = ra8876lite.begin();	//use 9904 bootROM for 1280x720 @60Hz input, output 1080p 60Hz
	
	if(!_success) return -1;
	
	if(color_depth==24)
		_mode = COLOR_24BPP_RGB888;
	else if(color_depth==8)
		_mode = COLOR_8BPP_RGB332;
	else
		_mode = COLOR_16BPP_RGB565;
		
	ra8876lite.canvasImageBuffer(v_w, v_h, ACTIVE_WINDOW_STARTX, ACTIVE_WINDOW_STARTY, _mode, CANVAS_OFFSET);
	ra8876lite.displayMainWindow();
	ra8876lite.canvasClear(color.Black);
	ra8876lite.graphicMode(true);
	ra8876lite.displayOn(true);	
	
	HDMI_Tx.begin();
	switch(card)
	{
		case GFX_VGA_SVGA_DVI:
		HDMI_Tx.init(VIDEO_in_640x480_out_DVI_800x600_60Hz);
		break;
		case GFX_WVGA_WXGA_DVI:
		HDMI_Tx.init(VIDEO_in_800x480_out_DVI_1280x768_60Hz);
		break;
		case GFX_SVGA_XGA_DVI:
		HDMI_Tx.init(VIDEO_in_800x600_out_DVI_1024x768_60Hz);
		break;
		case GFX_SVGA_SXGA_DVI:
		HDMI_Tx.init(VIDEO_in_800x600_out_DVI_1280x960_60Hz);
		break;
		case GFX_SVGA_UXGA_DVI:
		HDMI_Tx.init(VIDEO_in_800x600_out_DVI_1600x1200_60Hz);
		break;
		case GFX_HD480p_HD720p_DVI:
		HDMI_Tx.init(VIDEO_in_720x480_out_DVI_720p_60Hz);
		break;
		case GFX_HD720p_HD1080p_DVI:
		HDMI_Tx.init(VIDEO_in_1280x720_out_DVI_1080p_60Hz);
		break;
		
		case GFX_FWVGA_HD720p_HDMI:
		HDMI_Tx.init(VIDEO_in_848x480_out_HDMI_720p_60Hz);
		break;
		case GFX_HD480p_HD720p_HDMI:
		HDMI_Tx.init(VIDEO_in_720x480_out_HDMI_720p_60Hz);
		break;
		case GFX_HD576p_SXGA_HDMI:
		HDMI_Tx.init(VIDEO_in_720x576_out_DVI_1280x1024_60Hz);
		break;
		case GFX_HD720p_HD1080i_HDMI:
		HDMI_Tx.init(VIDEO_in_1280x720_out_HDMI_1080i_60Hz);
		break;
		case GFX_HD720p_HD1080p_HDMI:
		HDMI_Tx.init(VIDEO_in_1280x720_out_HDMI_1080p_60Hz);
		break;
		case GFX_WQVGA_HD480p_HDMI:
		HDMI_Tx.init(VIDEO_in_480x272_out_HDMI_480p_60Hz);
		break;
		case GFX_WQVGA_HD720p_HDMI:
		HDMI_Tx.init(VIDEO_in_480x272_out_HDMI_720p_60Hz);
		break;
		case GFX_WQVGA_HD1080p_HDMI:
		HDMI_Tx.init(VIDEO_in_480x272_out_HDMI_1080p_60Hz);
		break;
	}
		return 0;
}