/*  Copyright 2019 Giacomo Dal Sasso

    This file is part of fontcvt.

    fontcvt is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    fontcvt is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with fontcvt.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FONTBUILDERFORC_H_INCLUDED
#define FONTBUILDERFORC_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

#define FONTBUILDERFORC_TYPE_FONT           fontBuilderForC_Font_t
#define FONTBUILDERFORC_TYPE_RANGE          fontBuilderForC_Range_t
#define FONTBUILDERFORC_TYPE_CHARACTER      fontBuilderForC_Character_t
#define FONTBUILDERFORC_TYPE_KERNING        fontBuilderForC_Kerning_t

typedef struct
{
	uint16_t bmp_offset; // bitmap offset inside the 
	uint8_t bmp_pxl_width; // width of the bitmap (in pixel)
	uint8_t bmp_pxl_height; // height of the bitmap (in pixel)
	/* after rendering the glyph you have to advance the cursor x postion about
	this quantity */
	uint8_t pxl_advance;
	/* you have to position the glyph bitmap with the top left corner on
	coordinates (cursorX + pxl_left, cursorY - pxl_top) */
	int8_t pxl_left;
	int8_t pxl_top;
	uint16_t kerning_index;
} FONTBUILDERFORC_TYPE_CHARACTER;

typedef struct
{
	uint32_t first; // unicode value of the first glyph of this range
	uint32_t num_characters;
	const FONTBUILDERFORC_TYPE_CHARACTER *characters; // glyphs haracteristics table
} FONTBUILDERFORC_TYPE_RANGE;

typedef struct
{	/* kerning is used to adjust the spacing between two specific character to
	get an optimal layout */
	uint32_t left_ch;
	uint32_t right_ch;
	/* if you are writing glyph 'right_ch' and the glyph right before this, is
	'legt_ch', you should move the cursor position of 'pxl_adjust' pixels before
	rendering 'right_ch' */
	int8_t pxl_adjust;
} FONTBUILDERFORC_TYPE_KERNING;

typedef struct
{
	uint8_t bpp;
	/* you have to move the cursor y position of this quantity when you proceed
	with rendering a new line */
	uint8_t pxl_baseline_to_baseline;
	uint8_t pxl_max_glyph_height;
	const char *bitmaps_table; // byte array containing all glyphs bitmap
	const FONTBUILDERFORC_TYPE_RANGE *ranges;
	const FONTBUILDERFORC_TYPE_KERNING *kerning; // null if no kerning info available
	uint16_t num_kerning; // kerning array size
	uint16_t num_ranges; // ranges array size
} FONTBUILDERFORC_TYPE_FONT;

#endif // FONTBUILDERFORC_H_INCLUDED
