#include "bfcFontMgr.h"

/**
 * @brief	Return font's bits-per-pixel 
 * @param	FontType is defined in bfcfont.h
 * @return	bits-per-pixel in values 1,2,4, or 8. If FontType is not valid, -1 is returned
 */
int GetFontBpp(unsigned long FontType)
{
	// bits per pixel
	int bpp = -1;

	switch(FontType & 0xFF)
	{
	case FONTTYPE_MONO:
	case FONTTYPE_PROP:
		bpp = 1;
		break;
	case FONTTYPE_MONO_AA2:
	case FONTTYPE_PROP_AA2:
		bpp = 2;
		break;
	case FONTTYPE_MONO_AA4:
	case FONTTYPE_PROP_AA4:
		bpp = 4;
		break;
	case FONTTYPE_MONO_AA8:
	case FONTTYPE_PROP_AA8:
		bpp = 8;
		break;
	}

	return bpp;		
}

/**
 * @brief	Return font's endianness
 * @param	FontType is defined in bfcfont.h
 * @return	0: Big Endian (default), 1: Little Endian
 */
int GetFontEndian(unsigned long FontType)
{
	// 0: Big Endian (default)
	// 1: Little Endian
	int endian = (FontType & BFC_LITTLE_ENDIAN) > 0 ? 1 : 0;

	return endian;
}

/**
 * @brief	Return font's byte arrangement either in Row or Column
 * @param	FontType is defined in bfcfont.h
 * @return	0: Row (default), 1: Column
 * @note	This function is not used yet. <br>
 * 			Please fix font generation data format to Big Endian, Row based, Row preferred, Unpacked in BitFontCreator 
 */
int GetFontScanBase(unsigned long FontType)
{
	// 0: Row (default)
	// 1: Column
	int scanBase = (FontType & COLUMN_BASED) > 0 ? 1 : 0;

	return scanBase;
}


int GetFontScanPrefer(unsigned long FontType)
{
	// 0: Row (default)
	// 1: Column
	int scanPrefer = (FontType & COLUMN_PREFERRED) > 0 ? 1 : 0;

	return scanPrefer;
}

int GetFontDataPack(unsigned long FontType)
{
	// 0: No (default)
	// 1: Yes
	int dataPack = (FontType & DATA_PACKED) > 0 ? 1 : 0;

	return dataPack;
}

int GetFontHeight(const BFC_FONT *pFont)
{
	int height = -1;

	if(pFont != 0)
		height = pFont->FontHeight;

	return height;
}

const BFC_CHARINFO* GetCharInfo(const BFC_FONT *pFont, unsigned short ch)
{
	const BFC_CHARINFO	*pCharInfo = 0;
	const BFC_FONT_PROP *pProp = pFont->p.pProp;
	unsigned short first_char, last_char;

	if(pFont == 0 || pFont->p.pProp == 0)
		return 0;

	while(pProp != 0)
	{
		first_char = pProp->FirstChar;
		last_char = pProp->LastChar;
		pCharInfo = pProp->pFirstCharInfo;

		if( ch >= first_char && ch <= last_char )
		{
			// the character "ch" is inside this range,
			// return this char info, and not search anymore.
			pCharInfo = pCharInfo + (ch - first_char);
			return pCharInfo;
		}
		else 
		{
			// the character "ch" is not in this range
			// so search it in the next range
			pProp = pProp->pNextProp;
		}
	}

	// if the character "ch" is not rendered in this font,
	// we use the first character in this font as the default one.
	if( pCharInfo == 0 )
	{
		pProp = pFont->p.pProp;
		pCharInfo = pProp->pFirstCharInfo;
	}

	return pCharInfo;
}

