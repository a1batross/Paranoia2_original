/*
bspfile.h - BSP format included q1, hl1 support
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef BSPEXTRAFILE_H
#define BSPEXTRAFILE_H

/*
==============================================================================

EXTRA BSP INFO

==============================================================================
*/

#define IDEXTRAHEADER		(('H'<<24)+('S'<<16)+('A'<<8)+'X') // little-endian "XASH"
#define EXTRA_VERSION		3	// ver. 1 was occupied by old versions of XashXT, ver. 2 was occupied by old vesrions of P2:savior

#define MAX_MAP_CUBEMAPS		1024
#define MAX_MAP_LANDSCAPES		8192	// can be increased but not needs
#define MAX_MAP_LEAFLIGHTS		0x40000	// can be increased but not needs
#define MAX_MAP_WORLDLIGHTS		65535	// including a light surfaces too

#define LUMP_VERTNORMALS		0	// phong shaded vertex normals
#define LUMP_LIGHTVECS		1	// deluxemap data
#define LUMP_CUBEMAPS		2	// cubemap description
#define LUMP_LANDSCAPES		3	// landscape and lightmap resolution info
#define LUMP_LEAF_LIGHTING		4	// contain compressed light cubes per empty leafs
#define LUMP_WORLDLIGHTS		5	// list of all the virtual and real lights (used to relight models in-game)
#define LUMP_COLLISION		6	// physics engine collision hull dump
#define LUMP_AINODEGRAPH		7	// node graph that stored into the bsp
#define EXTRA_LUMPS			8	// count of the extra lumps

#define LM_ENVIRONMENT_STYLE		20	// light_environment always handle into separate style, so we can ignore it

typedef struct
{
	int	id;	// must be little endian XASH
	int	version;
	dlump_t	lumps[EXTRA_LUMPS];	
} dextrahdr_t;

// MODIFIED: int flags -> short flags, short landscape (total struct size not changed!)
typedef struct
{
	float	vecs[2][4];		// texmatrix [s/t][xyz offset]
	int	miptex;
	short	flags;
	short	landscape;
} dtexinfo_t;

//============================================================================
typedef struct
{
	float	normal[3];
} dnormal_t;

typedef struct
{
	short	origin[3];	// position of light snapped to the nearest integer
	short	size;		// cubemap side size
} dcubemap_t;

typedef struct
{
	byte	color[6][3];	// 6 sides 1x1 (single pixel per side)
} dsample_t;

typedef struct
{
	dsample_t	cube;
	short	origin[3];
	short	leafnum;		// leaf that contain this sample
} dleafambient_t;

typedef struct
{
	char	landname[16];	// name of decsription in mapname_land.txt
	word	texture_step;	// default is 16, pixels\luxels ratio
	word	max_extent;	// default is 16, subdivision step ((texture_step * max_extent) - texture_step)
	short	groupid;		// to determine equal landscapes from various groups
} dlandscape_t;

typedef enum
{
	emit_surface,
	emit_point,
	emit_spotlight,
	emit_skylight
} emittype_t;

#define VERTEXNORMAL_CONE_INNER_ANGLE	DEG2RAD( 7.275 )
#define DWL_FLAGS_INAMBIENTCUBE	0x0001	// This says that the light was put into the per-leaf ambient cubes.

typedef struct
{
	byte	emittype;
	byte	style;
	byte	flags;		// will be set in ComputeLeafAmbientLighting
	short	origin[3];	// light abs origin
	float	intensity[3];	// RGB
	float	normal[3];	// for surfaces and spotlights
	float	stopdot;		// for spotlights
	float	stopdot2;		// for spotlights
	float	fade;		// falloff scaling for linear and inverse square falloff 1.0 = normal, 0.5 = farther, 2.0 = shorter etc
	float	radius;		// light radius
	short	leafnum;		// light linked into this leaf
	byte	falloff;		// falloff style 0 = default (inverse square), 1 = inverse falloff, 2 = inverse square (arghrad compat)
	word	facenum;		// face number for emit_surface
	short	modelnumber;	// g-cont. we can't link lights with entities by entity number so we link it by bmodel number
} dworldlight_t;

#endif//BSPFILE_H