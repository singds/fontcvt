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

//____________________________________________________________INCLUDES - DEFINES
#include "fontCvt.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

// FreeType 2 library headers
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "builderForC.h"


#define L_PRINT_GEN_ERR                                fprintf (stderr, "ERROR ON %s:%d\n", __FILE__, __LINE__)
#define L_NELEMENTS(array)                             (sizeof (array) / sizeof (array[0]))

typedef struct
{
	wchar_t first; /* first unicode character's code (included) */
	wchar_t last; /* last unicode character's code (included) */
} UnicodeRange_t;


//____________________________________________________________PRIVATE PROTOTYPES
static void PrintHelp (void);
static void Export (void);
static void DoExportFont (fontCvt_Builder_t *builder, FT_Face face, UnicodeRange_t *ranges, uint8_t ranges_sz);
static void DoExportKerinig (fontCvt_Builder_t *builder, FT_Face face, wchar_t left_char, UnicodeRange_t *ranges, uint8_t ranges_sz);
void ConvertBitmap (FT_Bitmap *ft_bmp, char *pxlmap);

//___________________________________________________________________PRIVATE VAR
/* ArgIn_xxx variable substitutes what should be parsed from the command line */
static UnicodeRange_t *ArgIn_UnicodeRanges = NULL;
static uint16_t ArgIn_UnicodeRangesNum = 0;
/* font file path */
static char *ArgIn_FnameFont = NULL;
/* output file path */
static char *ArgIn_FnameOut = NULL;
/* font EM square scaled pixel height */
static uint8_t ArgIn_Size = 30;
/* export font bpp */
static uint8_t ArgIn_Bpp = 4;


//____________________________________________________________________GLOBAL VAR

//______________________________________________________________GLOBAL FUNCTIONS

/* Executable entry point.
    Args:
- <argc>[in] command line argument's number.
- <argv>[in] command line argument's list.
    Ret:
0 on success.
*/
int main (int argc, char *argv[])
{
	int c; /* option identifier character */
	/* flag meaning all provided arguments are ok */
	bool argsOk = true;

	/* parse command line options */
	while ((c = getopt (argc, argv, ":b:s:r:o:h")) != -1)
	{
		switch (c)
		{
			/* export glyph bitmap bpp option */
			case 'b':
			{
				ArgIn_Bpp = atoi (optarg);
				if (ArgIn_Bpp != 1
				 && ArgIn_Bpp != 2
				 && ArgIn_Bpp != 4
				 && ArgIn_Bpp != 8)
				{
					argsOk = false;
					fprintf (stderr, "%d is not a valid -b option's argument\n", ArgIn_Bpp);
				}
				break;
			}
			
			/* export glyph pixel size option */
			case 's':
			{
				ArgIn_Size = atoi (optarg);
				break;
			}

			/* comma separated list of character ranges */
			case 'r':
			{
				for (char *save, *range = strtok_r (optarg, ",", &save);
				     range;
				     range = strtok_r (NULL, ",", &save))
				{
					char *first; /* first character of the range */
					char *last; /* last character of the range */
					char *next, *save;
					
					first = last = strtok_r (range, "-", &save);
					while ((next = strtok_r (NULL, "-", &save)) != NULL)
						last = next;

					ArgIn_UnicodeRanges = realloc (ArgIn_UnicodeRanges, sizeof (UnicodeRange_t) * (ArgIn_UnicodeRangesNum + 1));
					if (ArgIn_UnicodeRanges == NULL)
					{
						argsOk = false;
						fprintf (stderr, "ragnes allocation fail\n");
						break;
					}
					ArgIn_UnicodeRanges[ArgIn_UnicodeRangesNum].first = atol (first);
					ArgIn_UnicodeRanges[ArgIn_UnicodeRangesNum].last = atol (last);

					if (ArgIn_UnicodeRanges[ArgIn_UnicodeRangesNum].first == 0
					 || ArgIn_UnicodeRanges[ArgIn_UnicodeRangesNum].last == 0
					 || ArgIn_UnicodeRanges[ArgIn_UnicodeRangesNum].first > ArgIn_UnicodeRanges[ArgIn_UnicodeRangesNum].last)
					{
						argsOk = false;
						fprintf (stderr, "invalid range\n");
						break;
					}
					/* range correctly acquired */
					ArgIn_UnicodeRangesNum++;
				}
				break;
			}

			/* output destination */
			case 'o':
			{
				ArgIn_FnameOut = optarg;
				break;
			}

			/* print the help */
			case 'h':
			{
				PrintHelp ( );
				return 0;
			}

			/* missing option argument */
			case ':':
			{
				argsOk = false;
				fprintf (stderr, "missing option argument for -%c option\n", optopt);
				break;
			}
			
			/* unknown option */
			default: /* '?' */
			{
				argsOk = false;
				fprintf (stderr, "-%c is not a valid option\n", optopt);
				break;
			}
		}
	}

	/* the user most provide the input font file */
	if (optind == argc -1)
	{	/* we get the input font file name */
		ArgIn_FnameFont = argv[optind];
	}
	else
	{	/* the user provided not even one or more than one input file name */
		argsOk = false;
		fprintf (stderr, "you must porvide at least one and only one input font file\n");
	}

	if (ArgIn_FnameOut == NULL)
	{
		argsOk = false;
		fprintf (stderr, "-o with specified output destination is mandatory\n");
	}

	if (argsOk)
		Export ( );

	return 0;
}

//_____________________________________________________________PRIVATE FUNCTIONS
/* Print help using the command line arguments.
    Args:
    Ret:
*/
static void PrintHelp (void)
{
	printf ("\n");
	printf ("\
fontcvt use:\n\
    fontcvt [OPTIONS] ... FONT_FILE -o OUTPUT_NAME\n");
	printf ("\n");
	printf ("\
-b) Set exported glyph bpp. Valid argument are 1,2,4,8. (default 4)\n");
	printf ("\
-s) Set exported glyph pixel size. This is the size in pixel of the scaled EM\n\
    square. (default 30)\n");
	printf ("\
-r) Comma separated list of unicode characters to export. Valid range are ex.\n\
    32-128,1020 to export characters between 32 and 128 included and the lonely\n\
    1020 character.\n");
	printf ("\
-o) Specify the output filename. (mandatory)\n");
	printf ("\
-h) Print this help and exit.\n");
}

/* Main program function, called after all input oprions are parsed.
    Args:
    Ret:
*/
static void Export (void)
{
	FT_Library library; /* handle to library */
	FT_Face face; /* handle to face object */
	FT_Error error;

	/* initialize builders */
	builderForC_Init ( );

	error = FT_Init_FreeType (&library);
	if (!error)
	{
		/* open the font face specified for the given font file.
		   Font faces example are Regular, Italic, Bold ...
		   Most fonts provvide one font face per file.
		*/
		error = FT_New_Face (library, ArgIn_FnameFont, 0, &face );
		if (!error)
		{
			/* scale the font to the given pixel size.
			   scaleing the font means scaling the font's EM square, witch is
			   the reference grid for the glyph's outlines.
			*/
			error = FT_Set_Pixel_Sizes (face, /* handle to face object */
				0, /* pixel_width (0 means same as pxel_height) */
				ArgIn_Size); /* pixel_height */
			if (!error)
			{
				DoExportFont (&builderForC_Builder, /* target bulder */
					face, /* font face pointer */
					ArgIn_UnicodeRanges,
					ArgIn_UnicodeRangesNum);
			}
			else
				L_PRINT_GEN_ERR;
		}
		else
			L_PRINT_GEN_ERR;
	}
	else
		L_PRINT_GEN_ERR;
}

/* Function description.
    Args:
    Ret:
*/
static void DoExportFont (fontCvt_Builder_t *builder, FT_Face face, UnicodeRange_t *ranges, uint8_t ranges_sz)
{
	{
		fontCvt_Font_t itfc_font;

		itfc_font.bpp = ArgIn_Bpp;
		itfc_font.pxl_baseline_to_baseline = face->size->metrics.height >> 6;
		itfc_font.pxl_em_square = face->size->metrics.y_ppem;
		/* ??? */
		itfc_font.pxl_max_glyph_height = (face->size->metrics.ascender - face->size->metrics.descender) >> 6;

		/* this identifies the builder's export procedure start */
		builder->startFont (&itfc_font, ArgIn_FnameOut);
	}

	/* for all the specified ranges */
	for (uint8_t range_idx = 0; range_idx < ranges_sz; range_idx++)
	{
		UnicodeRange_t *range; /* current uncode range */

		range = &ranges[range_idx];
		{
			fontCvt_Range_t itfc_range;

			itfc_range.first = range->first;
			itfc_range.last = range->last;
			/* we say the builder we are going to export this character range */
			builder->startRange (&itfc_range);
		}

		/* for all the characters inside the current range */
		for (wchar_t letter = range->first; letter <= range->last; letter++)
		{
			FT_Error error;
			FT_UInt glyph_idx; /* glyph index of this letter */

			glyph_idx = FT_Get_Char_Index (face, letter);
			if (!glyph_idx)
			{	/* undefined character code */
				/* there is no glyph for this character code */
				continue;
			}

			error = FT_Load_Glyph (face, /* handle to face object */
				glyph_idx, /* glyph index */
				FT_LOAD_DEFAULT); /* load flags */
			if (!error)
			{
				FT_Render_Mode renderMode = FT_RENDER_MODE_NORMAL;

				/* convert glyph to bitmap */
				if (ArgIn_Bpp == 1)
					renderMode = FT_RENDER_MODE_MONO;
				error = FT_Render_Glyph (face->glyph, /* glyph slot  */
					renderMode); /* render mode */
				if (!error)
				{
					FT_Bitmap *bitmap;
					char *pxlmap;

					bitmap = &face->glyph->bitmap;
					pxlmap = calloc (bitmap->width, bitmap->rows);
					if (pxlmap)
					{
						fontCvt_Character_t itfc_character;

						itfc_character.unicode = letter;
						itfc_character.pxl_left = face->glyph->bitmap_left;
						itfc_character.pxl_top = face->glyph->bitmap_top;
						itfc_character.bmp_pxl_width = bitmap->width;
						itfc_character.bmp_pxl_height = bitmap->rows;
						itfc_character.pxl_advance = face->glyph->advance.x >> 6;
						itfc_character.bmp = pxlmap;

						ConvertBitmap (bitmap, pxlmap);
						/* give this character to the builder for export */
						builder->startCharacter (&itfc_character);
						DoExportKerinig (builder, face, letter, ranges, ranges_sz);
						builder->endCharacter ( );
					}
					else
						L_PRINT_GEN_ERR;
				}
				else
					L_PRINT_GEN_ERR;
			}
			else
				L_PRINT_GEN_ERR;
		}

		/* we say the builder this range it's over */
		builder->endRange ( );
	}

	/* finalize the export procedure. This call should delate any garbage and
	   put the peces together to conclude the export.
	*/
	builder->endFont ( );
}

/* Export all the kerning information for this character in respect to all the
other exported characters.
    Args:
    Ret:
*/
static void DoExportKerinig (fontCvt_Builder_t *builder, FT_Face face, wchar_t left_char, UnicodeRange_t *ranges, uint8_t ranges_sz)
{
	FT_UInt l_glyph_idx; /* left glyph index */

	l_glyph_idx = FT_Get_Char_Index (face, left_char);
	if (l_glyph_idx)
	{
		for (uint8_t j = 0; j < ranges_sz; j++)
		{
			UnicodeRange_t *range = &ranges[j];

			for (wchar_t right_char = range->first; right_char <= range->last; right_char++)
			{
				FT_UInt r_glyph_idx; /* right glyph index */
				FT_Vector delta; /* kerning adjustment */
				
				r_glyph_idx = FT_Get_Char_Index (face, right_char);
				if (r_glyph_idx)
				{
					uint16_t x_pxl_adj; /* x pixel adjustment */

					FT_Get_Kerning (face, l_glyph_idx, r_glyph_idx, FT_KERNING_DEFAULT, &delta);
					x_pxl_adj = delta.x >> 6;
					if (x_pxl_adj)
					{
						fontCvt_Kerning_t kerning;

						kerning.left_char = left_char;
						kerning.right_char = right_char;
						kerning.x_pxl_adjust = x_pxl_adj;
						builder->putKerning (&kerning);
					}
				}
			}
		}
	}
}


/* Convert the bitmap provided by FreeType in a simpler format, if we can say so.
Always 8bit per pixel (also in monotone) and no padding bytes. This should
simplify the work inside the builder.
    Args:
<ft_bmp>[in] FreeType bitmap.
<pxlmap>[out] destination pxlmap. You must provide an array with enough space.
    Ret:

*/
void ConvertBitmap (FT_Bitmap *ft_bmp, char *pxlmap)
{
	if (ft_bmp->pixel_mode == FT_PIXEL_MODE_MONO
	 || ft_bmp->pixel_mode == FT_PIXEL_MODE_GRAY)
	{
		char *destPxl; /* destination pixel pointer */
		/* source byte pointer
		   based on the source bitmap representation mode this could contain
		   more than one pixel */
		const uint8_t *srcByte;
		uint8_t bpp; /* source bitmap bit per pixel */

		/* 1 - 8 bit per pixel format supported only */
		if (ft_bmp->pixel_mode == FT_PIXEL_MODE_MONO)
			bpp = 1;
		else if (ft_bmp->pixel_mode == FT_PIXEL_MODE_GRAY)
			bpp = 8;

		destPxl = pxlmap;
		for (uint8_t y = 0; y < ft_bmp->rows; y++)
		{
			/* bit offset of the pixel rappresentation inside the source byte */
			int8_t bit_pos = 8 - bpp;

			/* set the source to the start of the next line */
			srcByte = ft_bmp->buffer;
			srcByte += y * ft_bmp->pitch;
			for (uint8_t x = 0; x < ft_bmp->width; x++)
			{
				uint8_t gray_val; /* destination pixel gray value, always from 0 to 255 */
				/* bitmask of the source pixel rappresentation inside the source
				   byte */
				uint8_t bitMask;

				bitMask = (0x01 << bpp) - 1; /* [0b00000001 with bpp = 1] [0b00001111 with bpp = 4] etc */
				bitMask <<= bit_pos;
				gray_val = (*srcByte & bitMask) >> bit_pos;
				gray_val <<= (8 - bpp);

				*destPxl = gray_val;

				bit_pos -= bpp;
				if (bit_pos < 0)
				{	/* no more pixels inside this byte, move to the next */
					bit_pos = 8 - bpp;
					srcByte++;
				}
				destPxl++;
			}
		}
	}
	else
	{
		/* other formats not supported */
	}
}
