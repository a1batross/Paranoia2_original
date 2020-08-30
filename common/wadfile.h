/*
wadfile.h - WAD3 file format
Copyright (C) 2015 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef WADFILE_H
#define WADFILE_H

/*
========================================================================
.WAD archive format	(WhereAllData - WAD)

List of compressed files, that can be identify only by TYP_*

<format>
header:	dwadinfo_t[dwadinfo_t]
file_1:	byte[dwadinfo_t[num]->disksize]
file_2:	byte[dwadinfo_t[num]->disksize]
file_3:	byte[dwadinfo_t[num]->disksize]
...
file_n:	byte[dwadinfo_t[num]->disksize]
infotable	dlumpinfo_t[dwadinfo_t->numlumps]
========================================================================
*/

#define IDWAD2HEADER	(('2'<<24)+('D'<<16)+('A'<<8)+'W')	// little-endian "WAD2" quake wads
#define IDWAD3HEADER	(('3'<<24)+('D'<<16)+('A'<<8)+'W')	// little-endian "WAD3" half-life wads

// dlumpinfo_t->attribs
#define ATTR_NONE		0	// allow to read-write
#define ATTR_READONLY	BIT( 0 )	// don't overwrite this lump in anyway
#define ATTR_COMPRESSED	BIT( 1 )	// not used for now, just reserved
#define ATTR_HIDDEN		BIT( 2 )	// not used for now, just reserved
#define ATTR_SYSTEM		BIT( 3 )	// not used for now, just reserved

// dlumpinfo_t->type
#define TYP_ANY		-1	// any type can be accepted
#define TYP_NONE		0	// unknown lump type
#define TYP_LABEL		1	// legacy from Doom1. Empty lump - label (like P_START, P_END etc)
#define TYP_PALETTE		64	// quake or half-life palette (768 bytes)
#define TYP_DDSTEX		65	// contain DDS texture
#define TYP_GFXPIC		66	// menu or hud image (not contain mip-levels)
#define TYP_MIPTEX		67	// quake1 and half-life in-game textures with four miplevels
#define TYP_SCRIPT		68	// contain script files
#define TYP_COLORMAP2	69	// old stuff. build palette from LBM file (not used)
#define TYP_QFONT		70	// half-life font (qfont_t)

// dlumpinfo_t->img_type
#define IMG_DIFFUSE		0	// same as default pad1 always equal 0
#define IMG_ALPHAMASK	1	// alpha-channel that stored separate as luminance texture
#define IMG_NORMALMAP	2	// indexed normalmap
#define IMG_GLOSSMAP	3	// luminance or color specularity map
#define IMG_GLOSSPOWER	4	// gloss power map (each value is a specular pow)
#define IMG_HEIGHTMAP	5	// heightmap (for parallax occlusion mapping or source of normalmap)
#define IMG_LUMA		6	// luma or glow texture with self-illuminated parts
#define IMG_DECAL_ALPHA	7	// it's a decal texture (last color in palette is base color, and other colors his graduations)
#define IMG_DECAL_COLOR	8	// decal without alpha-channel uses base, like 127 127 127 as transparent color

#define WAD3_NAMELEN	16
#define HINT_NAMELEN	5	// e.g. _mask, _norm
#define MAX_FILES_IN_WAD	65535	// real limit as above <2Gb size not a lumpcount

typedef struct
{
	int		ident;		// should be WAD3
	int		numlumps;		// num files
	int		infotableofs;	// LUT offset
} dwadinfo_t;

typedef struct
{
	int		filepos;		// file offset in WAD
	int		disksize;		// compressed or uncompressed
	int		size;		// uncompressed
	char		type;		// TYP_*
	char		attribs;		// file attribs
	char		img_type;		// IMG_*
	char		pad;
	char		name[WAD3_NAMELEN];	// must be null terminated
} dlumpinfo_t;

/*
========================================================================

.LMP image format	(Half-Life gfx.wad lumps)

========================================================================
*/
typedef struct lmp_s
{
	unsigned int	width;
	unsigned int	height;
} lmp_t;

/*
========================================================================

.MIP image format	(half-Life textures)

========================================================================
*/
typedef struct mip_s
{
	char		name[WAD3_NAMELEN];
	unsigned int	width;
	unsigned int	height;
	unsigned int	offsets[4];	// four mip maps stored
} mip_t;

/*
========================================================================

.FNT image format	(half-Life fonts)

========================================================================
*/
#define NUM_GLYPHS	256

typedef struct
{
	short	startoffset;
	short	charwidth;
} charinfo;

typedef struct qfont_s
{
	int	width, height;
	int	rowcount;
	int	rowheight;
	charinfo	fontinfo[NUM_GLYPHS];
} qfont_t;

#endif//WADFILE_H