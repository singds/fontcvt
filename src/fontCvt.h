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

#ifndef FONTCVT_H_INCLUDED
#define FONTCVT_H_INCLUDED

//____________________________________________________________INCLUDES - DEFINES
#include <stdint.h>
#include <wchar.h>

typedef struct
{	/* general font caracteristics */
	const char *family_name;
	const char *style_name;
	uint16_t bpp; /* glyph bit per pixel */
	uint16_t pxl_baseline_to_baseline;
	uint16_t pxl_max_glyph_height;
	uint16_t pxl_em_square;
} fontCvt_Font_t;

typedef struct
{
	wchar_t first;
	wchar_t last;
} fontCvt_Range_t;

typedef struct
{	/* character caracteristics */
	wchar_t unicode; /* character unicode value */
	uint16_t bmp_pxl_width;
	uint16_t bmp_pxl_height;
	const char *bmp;
	int16_t pxl_left; /* bitmap's left edge position relative to the pen position */
	int16_t pxl_top; /* bitmap's top edge position relative to the pen position */
	uint16_t pxl_advance; /* advance the pen position this amount for the next character */
} fontCvt_Character_t;

typedef struct
{	/* character caracteristics */
	wchar_t left_char; /* left character unicode */
	wchar_t right_char; /* right character unicode */
	int16_t x_pxl_adjust; /* kerning x adjustment betweet left and right character */
} fontCvt_Kerning_t;

typedef struct
{
	void (*startFont) (fontCvt_Font_t *font, const char *output);
	void (*startRange) (fontCvt_Range_t *range);
	void (*startCharacter) (fontCvt_Character_t *character);
	void (*putKerning) (fontCvt_Kerning_t *kerning);
	void (*endCharacter) (void);
	void (*endRange) (void);
	void (*endFont) (void);
} fontCvt_Builder_t;

//____________________________________________________________________GLOBAL VAR

//______________________________________________________________GLOBAL FUNCTIONS

#endif /* FONTCVT_H_INCLUDED */