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
#include "builderForC.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define L_TYPE_FONT           "fontC_Font_t"
#define L_TYPE_RANGE          "fontC_Range_t"
#define L_TYPE_CHARACTER      "fontC_Character_t"

#define L_MAX(a, b)           (((a) >= (b)) ? (a) : (b))
#define L_MIN(a, b)           (((a) <= (b)) ? (a) : (b))

//____________________________________________________________PRIVATE PROTOTYPES
static void StartFont (fontCvt_Font_t *font, const char *output);
static void StartRange (fontCvt_Range_t *range);
static void StartCharacter (fontCvt_Character_t *character);
static void PutKerning (fontCvt_Kerning_t *kerning);
static void EndCharacter (void);
static void EndRange (void);
static void EndFont (void);

static void CloseAllFile (void);
static void AllFileWrite (FILE *f_dst, FILE *f_src);

//___________________________________________________________________PRIVATE VAR
static FILE *FSource; /* exported c source file */
static FILE *TmpfFont; /* temporary file to store the font structure */
static FILE *TmpfRange; /* temporary file to store the character ranges array */
static FILE *TmpfCharacter; /* temporary file to store characters descriptors */
static FILE *TmpfBitmap; /* temporary file to store characters bitmaps */

static uint8_t Bpp; /* bit per pixel for character bitmaps */
static uint16_t RangeIndex; /* exported character rage index */
static uint32_t BmpArrayOffset;

//____________________________________________________________________GLOBAL VAR
fontCvt_Builder_t builderForC_Builder;

//______________________________________________________________GLOBAL FUNCTIONS

/* Initialize the builer before it can be used.
    Args:
    Ret:
*/
void builderForC_Init (void)
{
	memset (&builderForC_Builder, 0, sizeof (builderForC_Builder));

	builderForC_Builder.startFont = StartFont;
	builderForC_Builder.startRange = StartRange;
	builderForC_Builder.startCharacter = StartCharacter;
	builderForC_Builder.putKerning = PutKerning;
	builderForC_Builder.endCharacter = EndCharacter;
	builderForC_Builder.endRange = EndRange;
	builderForC_Builder.endFont = EndFont;
}

//_____________________________________________________________PRIVATE FUNCTIONS
/* Function description.
    Args:
    Ret:
*/
static void StartFont (fontCvt_Font_t *font, const char *output)
{
	FILE *fHeader;
	char sourceFname[256], headerFname[256];

	snprintf (sourceFname, sizeof (sourceFname), "%s.c", output);
	snprintf (headerFname, sizeof (headerFname), "%s.h", output);

	fHeader = fopen (headerFname, "wb");
	if (fHeader)
	{	/* i can immediatly create the header file */
		fprintf (fHeader, "header file\n");
		fclose (fHeader);
	}

	/* open temporary files
	   those file are used to build different sections wich will be merged in a
	   single source file during the last build step.
	*/
	if ((TmpfFont = tmpfile ( )) == NULL)
		goto __errexit;
	if ((TmpfRange = tmpfile ( )) == NULL)
		goto __errexit;
	if ((TmpfCharacter = tmpfile ( )) == NULL)
		goto __errexit;
	if ((TmpfBitmap = tmpfile ( )) == NULL)
		goto __errexit;
	if ((FSource = fopen (sourceFname, "wb")) == NULL)
		goto __errexit;

	Bpp = font->bpp; /* save bpp for later use */
	RangeIndex = 0;
	BmpArrayOffset = 0;

	fprintf (FSource, "#include \"fontC.h\"\n\n");
	
	fprintf (TmpfFont, "const " L_TYPE_FONT " fontName =\n");
	fprintf (TmpfFont, "{\n");
	fprintf (TmpfFont, "\t.bpp = %d,\n", font->bpp);
	fprintf (TmpfFont, "\t.pxl_baseline_to_baseline = %d,\n", font->pxl_baseline_to_baseline);
	fprintf (TmpfFont, "\t.pxl_max_glyph_height = %d,\n", font->pxl_max_glyph_height);
	fprintf (TmpfFont, "\t.ranges = &FontRanges,\n");

	fprintf (TmpfRange, "const " L_TYPE_RANGE " FontRanges[] =\n");
	fprintf (TmpfRange, "{\n");

	fprintf (TmpfBitmap, "const char FontBitmaps[] =\n");
	fprintf (TmpfBitmap, "{\n");
	return;

__errexit:
	CloseAllFile ( );
}

/* Function description.
    Args:
    Ret:
*/
static void StartRange (fontCvt_Range_t *range)
{
	uint16_t char_num; /* number of characters in this range */

	char_num = range->last - range->first + 1;
	fprintf (TmpfRange, "\t{");
	fprintf (TmpfRange, " .first = %d,", range->first);
	fprintf (TmpfRange, " .num_characters = %d,", char_num);
	fprintf (TmpfRange, " .characters = &FontCharacters%d", RangeIndex);
	fprintf (TmpfRange, " },\n");

	fprintf (TmpfCharacter, "const " L_TYPE_CHARACTER " FontCharacters%d[] =\n", RangeIndex);
	fprintf (TmpfCharacter, "{\t// Unicode character range [0x%04X-0x%04X] (%d characters)\n", range->first, range->last, char_num);
}

/* Function description.
    Args:
    Ret:
*/
static void StartCharacter (fontCvt_Character_t *character)
{
	/* write the character information structure */
	fprintf (TmpfCharacter, "\t{");
	fprintf (TmpfCharacter, " .bmp_offset = % 7d,", BmpArrayOffset);
	fprintf (TmpfCharacter, " .bmp_pxl_width = % 3d,", character->bmp_pxl_width);
	fprintf (TmpfCharacter, " .bmp_pxl_height = % 3d,", character->bmp_pxl_height);
	fprintf (TmpfCharacter, " .pxl_advance = % 3d,", character->pxl_advance);
	/* those following two could be negative, meaning their string
	   rappresentation could be 4 character long (es. "-123"). Thus % 4d */
	fprintf (TmpfCharacter, " .pxl_left = % 4d,", character->pxl_left);
	fprintf (TmpfCharacter, " .pxl_top = % 4d", character->pxl_top);
	fprintf (TmpfCharacter, " },");
	fprintf (TmpfCharacter, " // Unicode 0x%04X\n", character->unicode);


	/* add this character bitmap to the array */
	fprintf (TmpfBitmap, "\t// Unicode 0x%04X\n", character->unicode);
	for (uint8_t y = 0; y < character->bmp_pxl_height; y++)
	{
		/* destination bitmap byte wiating to be filled before write */
		uint8_t wr_byte;
		/* the next pixel value offset position inside wr_byte */
		int8_t bit_pos;
		/* just to make it esier to recognize a glyph we add a 4-level-only
		   picture next it's byte rappresentation.
		*/
		char lview[256]; /* glyph picture line */
		uint16_t lview_sz;

		fprintf (TmpfBitmap, "\t");

		lview_sz = 0;
		wr_byte = 0;
		bit_pos = 8 - Bpp;
		for (uint8_t x = 0; x < character->bmp_pxl_width; x++)
		{
			const uint8_t *src_pxl; /* pointer to the source bitamp pixel */
			uint8_t gray_val; /* gray value for this pixel */ 

			src_pxl = &character->bmp[x + y * character->bmp_pxl_width];

			gray_val = *src_pxl >> (8 - Bpp);
			{	/* add this pixel to the picture line */
				uint8_t view_val;
				char view_char = '.';

				/* we only use max 4-level (.-1-2-3) always in 1-2-4 and 8 bpp */
				/* translate the original pixel gray value to a max 4-level gray
				   value. thus a 2 bit rappresentation.
				*/
				view_val = gray_val >> L_MAX (0, Bpp - 2);
				if (view_val)
					view_char = '0' + view_val;
				
				lview_sz += snprintf (lview + lview_sz, sizeof (lview) - lview_sz, "%c", view_char);
			}

			wr_byte |= gray_val << bit_pos;
			bit_pos -= Bpp; /* advance the position for the nex pixel */
			if (bit_pos < 0)
			{	/* wr_byte is fill of pixels */
				fprintf (TmpfBitmap, "0x%02X, ", wr_byte);
				BmpArrayOffset ++;
				/* refresh wr_byte and bit_pos */
				wr_byte = 0;
				bit_pos = 8 - Bpp;
			}
		}

		if (bit_pos != (8 - Bpp))
		{	/* some pixel are inside wr_byte waiting to be write */
			fprintf (TmpfBitmap, "0x%02X, ", wr_byte);
			BmpArrayOffset ++;
		}
		/* add the picture line next to the byte line */
		fprintf (TmpfBitmap, "// %s", lview);
		fprintf (TmpfBitmap, "\n");
	}
	fprintf (TmpfBitmap, "\n");
}

/* Function description.
    Args:
    Ret:
*/
static void PutKerning (fontCvt_Kerning_t *kerning)
{
//	printf ("kerning info: left = %04x | right = %04x     advance = %d\n", kerning->left_char, kerning->right_char, kerning->x_pxl_adjust);
}

/* Function description.
    Args:
    Ret:
*/
static void EndCharacter (void)
{

}

/* Function description.
    Args:
    Ret:
*/
static void EndRange (void)
{
	fprintf (TmpfCharacter, "};\n\n");
	RangeIndex++;
}

/* Function description.
    Args:
    Ret:
*/
static void EndFont (void)
{
	fprintf (TmpfFont, "\t.num_ranges = %d,\n", RangeIndex);
	fprintf (TmpfFont, "};\n");
	fprintf (TmpfRange, "};\n");
	fprintf (TmpfBitmap, "};\n");

	AllFileWrite (FSource, TmpfBitmap);
	fprintf (FSource, "\n\n");
	AllFileWrite (FSource, TmpfCharacter);
	fprintf (FSource, "\n\n");
	AllFileWrite (FSource, TmpfRange);
	fprintf (FSource, "\n\n");
	AllFileWrite (FSource, TmpfFont);

	CloseAllFile ( );
}



/* Close all the open files.
    Args:
    Ret:
*/
static void CloseAllFile (void)
{
	if (FSource)
		fclose (FSource);
	if (TmpfFont)
		fclose (TmpfFont);
	if (TmpfRange)
		fclose (TmpfRange);
	if (TmpfCharacter)
		fclose (TmpfCharacter);
	if (TmpfBitmap)
		fclose (TmpfBitmap);
}

/* Write all the source file content at the end of the destination file.
    Args:
<f_dst>[in] destination file.
<f_src>[in] source file.
    Ret:
*/
static void AllFileWrite (FILE *f_dst, FILE *f_src)
{
	char ch;

	fseek (f_dst, 0, SEEK_END);
	fseek (f_src, 0, SEEK_SET);

	while ((ch = fgetc (f_src) ) != EOF)
    	fputc (ch, f_dst);
}
