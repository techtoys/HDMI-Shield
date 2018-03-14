/**
 * @brief	RA8876 have three text sources; they are Embedded Characters built-in RA8876, 
 *			External Character ROM, and User-defined Characters. 
 *			(1) Embedded Characters: ROM-based 8x16, 12x24, 16x32 ASCII characters that users 
 *				need only input characters by ASCII code. Coding standards ISO/IEC 8859-1/2/4/5
 *				are all supported. Because this character set is a ROM-based data, no Flash space is required
 *				from the host or MCU which is rather expensive to store static data like ASCII code. 
 *				Details can be found from Section 14.1 of RA8876 datasheet. 
 *				This option is enumerated by FONT_SRC as INTERNAL_CGROM below.
 *			(2) External Character ROM: RA8876 is compatible with External serial ROMs from Genitop Inc.
 *				Supporting product numbers are GT21L16T1W, GT30L16U2W, GT30L24T3Y, GT30L24M1Z, GT30L32S4W, GT20L24F6Y, and GT21L24S1W.
 *				According to different products, there are different character’s size including 16x16, 24x24, 32x32, 
 *				and variable width character size in them. Detail functionality description please refers section: 
 *				“External Serial Flash/ROM Interface” in RA8876 datasheet. 
 *				Important : Only footprints for Genitop Font ROM is available. No ROM chip has been soldered.
 *				Users may choose the appropriate product from Genitop Inc and solder it by himself at designator U33/U34.
 *				This option is enumerated by FONT_SRC as GENITOP_FONT below.
 *			(3) User-defined Characters: User can create characters or symbols and then get them stored in SDRAM. 
 *				This mode is not supported by current version. Details can be found under section 14.3 on RA8876 datasheet.
 *				This option is enumerated by FONT_SRC as CUSTOM_CGRAM below.
 */

#ifndef _HW_FONT_H
#define _HW_FONT_H

/**
 *@note 	Enum for three hardware font sources; they are Embedded Characters builtin RA8876, 
 *			External Character ROM, and User-defined Characters.
 */
enum FONT_SRC {
	INTERNAL_CGROM =0,	//RA8876 embedded Char set
	GENITOP_FONT   =1,	//External CGROM
	CUSTOM_CGRAM   =2	//User-defined CGRAM
};

/**
 *@note		Enum for character height for hardware fonts, variation subject to source of characters also.
 *			Details refer RA8876's REG[CCh]
 */
enum FONT_HEIGHT {
	 CHAR_HEIGHT_16=16,
	 CHAR_HEIGHT_24=24,
	 CHAR_HEIGHT_32=32
 };
 
 
///@note External character ROM, parameter to be set in REG[CEh] bit7:5
enum GT_FONT_ROM {  
  FONT_ROM_GT21L16T1W=0,    	
  FONT_ROM_GT30L16U2W=1,  
  FONT_ROM_GT30L24T3Y=2,  
  FONT_ROM_GT30L24M1Z=3,
  FONT_ROM_GT30L32S4W=4,
  FONT_ROM_GT20L24F6Y=5,
  FONT_ROM_GT21L24S1W=6
  };
  
///@note Character encoding, parameter to be set in REG[CC] / REG[CF] for Internal Char ROM or External CGROM (Genitop)
enum FONT_CODE {
	ICGROM_ISO_8859_1 = 0,	//Internal CGROM ISO char code
	XCGROM_GB2312 = 0,		//Genitop ROM double byte character code
	ICGROM_ISO_8859_2 = 1,
	XCGROM_GB12345 = 1,
	ICGROM_ISO_8859_4 = 2,
	XCGROM_BIG5 = 2,
	ICGROM_ISO_8859_5 = 3,
	XCGROM_UNICODE = 3,
	XCGROM_ASCII = 4,		//ASCII only from 00h-1Fh, 80-FFh will send "blank space"
	XCGROM_UNI_JAPANESE = 5,
	XCGROM_JIS0208 = 6,		//2-byte character set specified as a Japanese Industrial Standard	
	XCGROM_LATIN = 7, 		//Latin, Greek, Cyrillic, Arabic, Thai, Hebrew
	XCGROM_ISO_8859_1 = 17,
	XCGROM_ISO_8859_2 = 18,
	XCGROM_ISO_8859_3 = 19,
	XCGROM_ISO_8859_4 = 20,
	XCGROM_ISO_8859_5 = 21,
	XCGROM_ISO_8859_7 = 22,
	XCGROM_ISO_8859_8 = 23,
	XCGROM_ISO_8859_9 = 24,
	XCGROM_ISO_8859_10= 25,
	XCGROM_ISO_8859_11= 26,
	XCGROM_ISO_8859_13= 27,
	XCGROM_ISO_8859_14= 28,
	XCGROM_ISO_8859_15= 29,
	XCGROM_ISO_8859_16= 30
};
 
 /**
  *@note	Font background with either SOLID (character's background filled with the background color) or 
  *			TRANSPARENT(the character's background filled with the canvas background)
  */	
const bool SOLID = 0;
const bool TRANSPARENT = 1;

/**
 * @note	Structure to describe hardware fonts (Embedded Characters and Genitop) 
 */
typedef struct HW_FONT{
	const char	*name;		/* name of font */
	wchar_t		FirstChar;	/* first character available in ROM (Embedded/Genitop) */
	wchar_t		LastChar;	/* last character available in ROM (Embedded/Genitop) */
	FONT_SRC 	FontSource;	/* Enum FONT_SRC */
	uint16_t	FontWidth;	/* Fixed width or zero if it is a variable width */
	uint16_t	FontHeight; /* Font height */
	FONT_CODE	FontCode;	/* Encoding method with enum listing in FONT_CODE above */
}HW_FONT;

/************************************************************************************/
/************************************************************************************/
/**
 * @brief	Internal CG ROM character set of 8x16 pixels
 */
const HW_FONT	ICGROM_16 =
{
	"Internal CG ROM",
	0x00,
	0xff,
	FONT_SRC::INTERNAL_CGROM,
	8,
	16,
	FONT_CODE::ICGROM_ISO_8859_1
};

/**
 * @note	BIG5 character set in Genitop GT21L16T1W ROM. <br>
 *			BIG5 code reference : http://ash.jp/code/cn/big5tbl.htm <br>
 *			Characters of 15x16 dots in the range 0xA140 - 0xC67E.<br>
 *			Example to display 100 characters from '一' to '世'. <br>
 *
 *			wchar_t wch;
			//set printing position starts from (100,100); auto line feed when cursor meets Active Window boundary
			ra8876lite.setHwTextCursor(100,100);
			ra8876lite.setHwTextColor(color.Black);			//set font color black
			ra8876lite.setHwTextParam(color.White, 1,1);	//set background white, magnification factor 1:1
			
			for(wch=0xA440; wch<(0xA440+101); wch++)
			{
				ra8876lite.putHwChar(&XCGROM_BIG5_16, wch);      
			}
 */
const HW_FONT	XCGROM_BIG5_16 = 
{
	"Genitop BIG5 16",
	0xA140,
	0xC67E,
	FONT_SRC::GENITOP_FONT,
	15,
	16,
	FONT_CODE::XCGROM_BIG5
};

/**
 * @note	Janpanese JIS0208 character set in Genitop GT21L16T1W ROM.<br>
 *			JIS0208 Reference: http://charset.7jp.net/jis0208.html.<br>
 *			JIS code of ぁ is 2421. Its UTF-16 representation is 0x3041.<br>
 *			Somehow we need to use 0x0401 for 'ぁ'.<br>
 *			Hex value 0x0401 means to start from 04区 01点 displaying ぁあぃい...for 300 characters.<br>
 *
 *			Example to use to display 300 characters starting from ぁ.<br>
 
 *			wchar_t wch;
			//0x0401 means to start from 04区 01点 displaying ぁあぃい...for 300 characters
			//print wide characters starting from x=100,y=150 with auto line feed when cursor meets x=canvas width=1280 in this demo
			ra8876lite.setHwTextCursor(100,150);
			ra8876lite.setHwTextColor(color.White);
			ra8876lite.setHwTextParam(color.Black, 1,1);
			for(wch=0x0401; wch<(0x0401+301); wch++)
			{
				ra8876lite.putHwChar(&XCGROM_JIS_16, wch);     
			} 
 */
const HW_FONT XCGROM_JIS_16 = 
{
	"Genitop JIS0208 16",
	0x0101,
	0x8794,
	FONT_SRC::GENITOP_FONT,
	15,
	16,
	FONT_CODE::XCGROM_JIS0208
};

/**
 * @note	Cyrillic character set in Genitop GT21L16T1W ROM.<br>
 *			Reference: http://www.unicode.org/charts/PDF/U0400.pdf<br>
 *			Example to display 'Ё' to 'ӹ'<br>
 *
			  wchar_t wch;
			  ra8876lite.setHwTextCursor(100,250);
			  ra8876lite.setHwTextColor(color.Black);
			  ra8876lite.setHwTextParam(color.Cyan, 2,2);
			  for(wch=XCGROM_CYRIL_16.FirstChar; wch<XCGROM_CYRIL_16.LastChar+1; wch++)
			  {
				ra8876lite.putHwChar(&XCGROM_CYRIL_16, wch);      
			  }  		
 */
const HW_FONT XCGROM_CYRIL_16 =
{
	"Genitop Cyril 16",
	0x0401,
	0x04F9,
	FONT_SRC::GENITOP_FONT,
	8,
	16,
	FONT_CODE::XCGROM_LATIN
};

/**
 * @note	Traditional Chinese GB12345 character set in Genitop GT21L16T1W ROM.<br>
 *			Reference: https://zh.wikipedia.org/wiki/GB_12345<br>
 *			GB12345 is compatible with GB2312<br>
 *			Reference: http://www.khngai.com/chinese/charmap/tblgb.php?page=1<br>
 *
 *			Example to display '啊' to '剥' <br>
 
 *			wchar_t wch;
			ra8876lite.setHwTextCursor(100,500);
			ra8876lite.setHwTextParam(color.Magenta, 2,2);
			for(wch=0xB0A1; wch<0xB0FF; wch++)
			{
				ra8876lite.putHwChar(&XCGROM_GB12345_16, wch);      
			} 			
 */
const HW_FONT XCGROM_GB12345_16 =
{
	"Genitop GB12345 16",
	0xA1A1,
	0xF9A9,
	FONT_SRC::GENITOP_FONT,
	15,
	16,
	FONT_CODE::XCGROM_GB12345
};

/**
 * @note	Arabian 16-dots font(250 characters) <br>
 *			Reference: http://jrgraphix.net/r/Unicode/0600-06FF <br>
 *			Example to display '؟' to '۹' <br>
 *
			wchar_t wch;
      
			ra8876lite.setHwTextCursor(100,400);
			ra8876lite.setHwTextParam(color.Yellow, 1,1);
			for(wch=0x061F; wch<XCGROM_ARABIA_16.LastChar+1; wch++)
			{
				ra8876lite.putHwChar(&XCGROM_ARABIA_16, wch);      
			} 
 */
const HW_FONT XCGROM_ARABIA_16 = 
{
	"Genitop Arabia 16",
	0x0600,
	0x06F9,
	FONT_SRC::GENITOP_FONT,
	0,	/* It is a variable width */
	16,
	FONT_CODE::XCGROM_LATIN
};

#endif //_HW_FONT_H

