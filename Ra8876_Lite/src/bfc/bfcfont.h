/****************************************************************************
*                                                                           *
* bfcfont.h -- Basic Type Definitions                                       *
*                                                                           *
* Copyright (c) 2012, Iseatech Software. All rights reserved.				*
* website: http://www.iseasoft.com/bfc.htm									*
* email:   support@iseatech.com												*
*                                                                           *
*****************************************************************************
*																			*
* @file      bfcfont.h												        *
* @version   0.3.1.0														*
* @date      Mar-28-2016													*
* @brief     BitFontCreator (Pro & Grayscale) font header include file		*
*																			*
*****************************************************************************/

/**
 * LITTLE_ENDIAN -> BFC_LITTLE_ENDIAN for Arduino
 * Please fix font generation data format to Big Endian, Row based, Row preferred, Unpacked in BitFontCreator 
 * Useful tool online:
 * //(1) is an online language translator
 * (1) https://www.google.com.hk/search?q=online+chinese+to+english+dictionary&oq=online+chinese+to+english+dictionary&gs_l=psy-ab.12...0.0.0.625904.0.0.0.0.0.0.0.0..0.0....0...1..64.psy-ab..0.0.0....0.TZ9ummyxy0g
 * //(2) is a unicode generator tool
 * (2) http://mylanguages.org/converter.php
 */
 
#ifndef _BFC_FONT_H_
#define _BFC_FONT_H_

#define UCHAR	unsigned char 
#define USHORT	unsigned short
#define ULONG	unsigned long

#define FONTTYPE_MONO           (1<<0)      /* Is monospaced font */
#define FONTTYPE_PROP           (1<<1)      /* Is proportional font */
#define FONTTYPE_MONO_AA2       (1<<2)      /* Is an antialiased mono font, 2bpp */
#define FONTTYPE_MONO_AA4       (1<<3)      /* Is an antialiased mono font, 4bpp */
#define FONTTYPE_MONO_AA8       (1<<4)      /* Is an antialiased mono font, 8bpp */
#define FONTTYPE_PROP_AA2       (1<<5)      /* Is an antialiased prop font, 2bpp */
#define FONTTYPE_PROP_AA4       (1<<6)      /* Is an antialiased prop font, 4bpp */
#define FONTTYPE_PROP_AA8       (1<<7)      /* Is an antialiased prop font, 8bpp */

/************************************************************************************
 * the following 4 flags is added since Pro v3.7 & Grayscale v4.5 on 2016-03-28     *
*************************************************************************************/
#define BFC_LITTLE_ENDIAN       (1<<8)      /* Is Little Endian if set, or is Big Endian (default) */
#define COLUMN_BASED            (1<<9)      /* Is Column if set, or is Row (default) */
#define COLUMN_PREFERRED        (1<<10)     /* Is Column if set, or is Row (default) */
#define DATA_PACKED             (1<<11)     /* Is Packed if set, or is Unpacked (default) */

#define ENCODING_ASCII          (1<<16)     /* Character encoding: Ascii + ISO8859 */
#define ENCODING_UNICODE        (1<<17)     /* Character encoding: Unicode */
#define ENCODING_SHIFTJIS       (1<<18)     /* Character encoding: Shift_JIS */
#define ENCODING_BIG5           (1<<19)     /* Character encoding: Big5 */
#define ENCODING_GBK            (1<<20)     /* Character encoding: GBK */
#define ENCODING_JOHAB          (1<<21)     /* Character encoding: Johab */

#define DATALENGTH_8            (1<<24)     /* Data length:  8 bits per unit (1 byte) */
#define DATALENGTH_16           (1<<25)     /* Data length: 16 bits per unit (2 bytes) */
#define DATALENGTH_32           (1<<26)     /* Data length: 32 bits per unit (4 bytes) */

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*       C font structures
**********************************************************************/
typedef struct BFC_CHARINFO
{
    USHORT		Width;         /* character width in pixels */
    USHORT		DataSize;      /* # bytes/words/dwords of pixel data */
    union 
    {
        const void      *pData;
        const UCHAR     *pData8;    /* pixel data in bytes */   
        const USHORT    *pData16;   /* pixel data in words */
        const ULONG     *pData32;   /* pixel data in dwords */ 
    } p;
} BFC_CHARINFO;

typedef struct BFC_FONT_PROP
{
    USHORT		FirstChar;     /* index of first character */
    USHORT		LastChar;      /* index of last character */
    const BFC_CHARINFO	*pFirstCharInfo;    /* address of first character */
    const struct BFC_FONT_PROP	*pNextProp;	/* pointer to next BFC_FONT_PROP */
} BFC_FONT_PROP;

typedef struct BFC_FONT_MONO
{
    USHORT		FirstChar;     /* index of first character */
    USHORT		LastChar;      /* index of last character */
    USHORT		FontWidth;     /* font width in pixels */
    USHORT		DataSize;      /* # bytes/words/dwords data of single character */
    union 
    {
        const void      *pData;
        const UCHAR     *pData8;     /* pixel data in bytes */   
        const USHORT    *pData16;    /* pixel data in words */
        const ULONG     *pData32;    /* pixel data in dwords */ 
    } p;
} BFC_FONT_MONO;

typedef struct
{
    ULONG		FontType;      /* font type */
    USHORT		FontHeight;    /* font height in pixels */		
    USHORT		Baseline;      /* font ascent (baseline) in pixels */
    ULONG		Reversed;      /* reversed, =0 */
    union 
    {
        const void          * pData;
        const BFC_FONT_MONO	* pMono; /* point to Monospaced font */ 
        const BFC_FONT_PROP	* pProp; /* point to proportional font */
    } p;
} BFC_FONT;


/*********************************************************************
*       Binary font structures (BIN)
**********************************************************************/
typedef struct
{
    ULONG       FontType;      /* font type */
    USHORT		FontHeight;    /* font height in pixels */		
    USHORT		Baseline;      /* font ascent (baseline) in pixels */
    USHORT		Reversed;      /* reversed, =0 */
    USHORT		NumRanges;     /* number of character ranges */
} BFC_BIN_FONT;

typedef struct 
{
    USHORT		FirstChar;     /* index of first character */
    USHORT		LastChar;      /* index of last charcter */
} BFC_BIN_CHARRANGE;

typedef struct 
{
    USHORT		Width;         /* character width in pixels */
    USHORT		DataSize;      /* # bytes/words/dwords of pixel data */	
    ULONG		OffData;       /* Offset of pixel data */
} BFC_BIN_CHARINFO;

#ifdef __cplusplus
}
#endif

#endif //#ifndef _BFC_FONT_H_