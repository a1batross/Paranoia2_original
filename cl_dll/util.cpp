/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// util.cpp
//
// implementation of class-less helper functions
//

#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"

#include "hud.h"
#include "cl_util.h"
#include <stringlib.h>
#include "com_model.h"
#include "triangleapi.h"
#include <mathlib.h>
#include "gl_local.h"
#include "event_api.h"
#include "r_efx.h"
#include "pmtrace.h"
#include "gl_studio.h"
#include "gl_world.h"

static byte	decompressed[MAX_MAP_LEAFS/8];
	
/*
=============
pfnAlertMessage

=============
*/
void ALERT( ALERT_TYPE level, char *szFmt, ... )
{
	char	buffer[2048];	// must support > 1k messages
	va_list	args;

	if( developer_level <= DEV_NONE )
		return;

	if( level == at_aiconsole && developer_level < DEV_EXTENDED )
		return;

	va_start( args, szFmt );
	Q_vsnprintf( buffer, 2048, szFmt, args );
	va_end( args );

	if( level == at_warning )
	{
		gEngfuncs.Con_Printf( va( "^3Warning:^7 %s", buffer ));
	}
	else if( level == at_error  )
	{
		gEngfuncs.Con_Printf( va( "^1Error:^7 %s", buffer ));
	} 
	else
	{
		gEngfuncs.Con_Printf( buffer );
	}
}

/*
====================
Sys LoadGameDLL

====================
*/
bool Sys_LoadLibrary( const char* dllname, dllhandle_t* handle, const dllfunc_t *fcts )
{
	if( !handle ) return false;

	const dllfunc_t *gamefunc;

	// Initializations
	for( gamefunc = fcts; gamefunc && gamefunc->name != NULL; gamefunc++ )
		*gamefunc->func = NULL;

	char dllpath[128];

	// is direct path used ?
	if( dllname[0] == '*' ) Q_strncpy( dllpath, dllname + 1, sizeof( dllpath ));
	else Q_snprintf( dllpath, sizeof( dllpath ), "%s/cl_dlls/%s", gEngfuncs.pfnGetGameDirectory(), dllname );

	dllhandle_t dllhandle = LoadLibrary( dllpath );
        
	// No DLL found
	if( !dllhandle ) return false;

	// Get the function adresses
	for( gamefunc = fcts; gamefunc && gamefunc->name != NULL; gamefunc++ )
	{
		if( !( *gamefunc->func = (void *)Sys_GetProcAddress( dllhandle, gamefunc->name )))
		{
			Sys_FreeLibrary( &dllhandle );
			return false;
		}
	}          

	ALERT( at_aiconsole, "%s loaded succesfully!\n", (dllname[0] == '*') ? (dllname+1) : (dllname));
	*handle = dllhandle;

	return true;
}

void *Sys_GetProcAddress( dllhandle_t handle, const char *name )
{
	return (void *)GetProcAddress( handle, name );
}

void Sys_FreeLibrary( dllhandle_t *handle )
{
	if( !handle || !*handle )
		return;

	FreeLibrary( *handle );
	*handle = NULL;
}

bool Sys_RemoveFile( const char *path )
{
	char	real_path[1024];
	int	iRet = false;

	if( !path || !*path )
		return false;

	Q_snprintf( real_path, sizeof( real_path ), "%s/%s", gEngfuncs.pfnGetGameDirectory(), path );
	COM_FixSlashes( real_path );
	iRet = remove( real_path );

	return (iRet == 0) ? true : false;
}

/*
=============
WorldToScreen

convert world coordinates (x,y,z) into screen (x, y)
=============
*/
int WorldToScreen( const Vector &world, Vector &screen )
{
	int retval = R_WorldToScreen( world, screen );

	screen[0] =  0.5f * screen[0] * (float)RI->view.port[2];
	screen[1] = -0.5f * screen[1] * (float)RI->view.port[3];
	screen[0] += 0.5f * (float)RI->view.port[2];
	screen[1] += 0.5f * (float)RI->view.port[3];

	return retval;
}

//=================
//   UTIL_IsPlayer
//=================
bool UTIL_IsPlayer( int idx )
{
	if ( idx >= 1 && idx <= gEngfuncs.GetMaxClients() )
		return true;
	return false;
}


//=================
//     UTIL_IsLocal
//=================
bool UTIL_IsLocal( int idx )
{
	return gEngfuncs.pEventAPI->EV_IsLocal( idx - 1 ) ? true : false;
}

//=================
//     UTIL_WeaponAnimation
//=================
void UTIL_WeaponAnimation( int iAnim, float framerate )
{
	cl_entity_t *view = GET_VIEWMODEL();

	gEngfuncs.pEventAPI->EV_WeaponAnimation( iAnim, view->curstate.body );

	view->curstate.framerate = framerate;
	view->curstate.scale = 1.0f;
#if 0
	// just for test Glow Shell effect
	view->curstate.renderfx = kRenderFxGlowShell;
	view->curstate.rendercolor.r = 255;
	view->curstate.rendercolor.g = 255;
	view->curstate.rendercolor.b = 255;
	view->curstate.renderamt = 100;
#endif	
}

void UTIL_StudioDecal( const char *pDecalName, pmtrace_t *pTrace, const Vector &vecSrc )
{
	g_StudioRenderer.StudioDecalShoot( pTrace, pDecalName, false );
}

/*
====================
SPRITE_GetList

====================
*/
char *ParseHudSprite( char *pfile, char *psz, client_sprite_t *result )
{
	client_sprite_t tempSprite;
	int x = 0, y = 0, width = 0, height = 0;
	int section = 0;
	char token[256];
	
	memset( &tempSprite, 0, sizeof( tempSprite ));
	memset( result, 0, sizeof( *result ));
          
	while( pfile )
	{
		pfile = COM_ParseFile( pfile, token );	

		if( !Q_stricmp( token, psz ))
		{
			pfile = COM_ParseFile( pfile, token );	
			if( !Q_stricmp( token, "{" )) section = 1;
		}

		if( section )
		{
			if( !Q_stricmp( token, "}" ))
				break; // end section
			
			if( !Q_stricmp( token, "file" )) 
			{                                          
				pfile = COM_ParseFile( pfile, token );
				Q_strcpy( tempSprite.szSprite, token );
				void *testSprite;

				if(( testSprite = gEngfuncs.COM_LoadFile( tempSprite.szSprite, 5, NULL )) != NULL )
				{
					gEngfuncs.COM_FreeFile( testSprite );

					// fill structure at default
					HSPRITE m_hSprite = SPR_Load( tempSprite.szSprite );

					width = SPR_Width( m_hSprite, 0 );
					height = SPR_Height( m_hSprite, 0 );
					x = y = 0;
				}
				else
				{
					return pfile;
				}
			}
			else if( !Q_stricmp( token, "name" )) 
			{                                          
				pfile = COM_ParseFile( pfile, token );
				Q_strcpy( tempSprite.szName, token );
			}
			else if( !Q_stricmp( token, "x" )) 
			{                                          
				pfile = COM_ParseFile( pfile, token );
				x = Q_atoi( token );
			}
			else if( !Q_stricmp( token, "y" )) 
			{                                          
				pfile = COM_ParseFile( pfile, token );
				y = Q_atoi( token );
			}
			else if( !Q_stricmp( token, "width" )) 
			{                                          
				pfile = COM_ParseFile( pfile, token );
				width = Q_atoi( token );
			}
			else if( !Q_stricmp( token, "height" )) 
			{                                          
				pfile = COM_ParseFile( pfile, token );
				height = Q_atoi( token );
			}
		}
	}

	if( !section )
		return pfile; // data not found

	// calculate sprite position
	tempSprite.rc.left = x;
	tempSprite.rc.right = x + width; 
	tempSprite.rc.top = y;
	tempSprite.rc.bottom = y + height;

	// write resolution for backward compatibility
	if( ScreenWidth < 640 )
		tempSprite.iRes = 320;
	else tempSprite.iRes = 640;

	*result = tempSprite;

	return pfile;
}

client_sprite_t *SPR2_GetList( char *psz, int *piCount )
{
	int iSprCount = 0;
	char *afile = (char *)gEngfuncs.COM_LoadFile( psz, 5, NULL );

	if( !afile )
	{
		*piCount = iSprCount;
		return NULL;
	}
	
	char token[256];
	char *pfile = afile;

	while( pfile )
	{
		// calculate count of sprites
		pfile = COM_ParseFile( pfile, token );

		if( !Q_stricmp( token, "hudsprite" ))
			iSprCount++;
	}

          client_sprite_t *phud = new client_sprite_t[iSprCount];
	pfile = afile;
          
	for( int i = 0; i < iSprCount; i++ )
	{
		pfile = ParseHudSprite( pfile, "hudsprite", &phud[i] );
	}
          
          gEngfuncs.COM_FreeFile( afile );
          
          *piCount = iSprCount;

	return phud;
}

/*
======================================================================

LEAF LISTING

NOTE: this code ripped out from Xash3D
======================================================================
*/
static void Mod_BoxLeafnums_r( leaflist_t *ll, mnode_t *node, model_t *worldmodel )
{
	int	s;

	while( 1 )
	{
		if( node->contents == CONTENTS_SOLID )
			return;

		if( node->contents < 0 )
		{
			mleaf_t	*leaf = (mleaf_t *)node;

			// it's a leaf!
			if( ll->count >= ll->maxcount )
			{
				ll->overflowed = true;
				return;
			}

			ll->list[ll->count++] = leaf->cluster;
			return;
		}
	
		s = BOX_ON_PLANE_SIDE( ll->mins, ll->maxs, node->plane );

		if( s == 1 )
		{
			node = node->children[0];
		}
		else if( s == 2 )
		{
			node = node->children[1];
		}
		else
		{
			// go down both
			if( ll->headnode == NULL )
				ll->headnode = node;
			Mod_BoxLeafnums_r( ll, node->children[0], worldmodel );
			node = node->children[1];
		}
	}
}

/*
==================
Mod_BoxLeafnums
==================
*/
int Mod_BoxLeafnums( const Vector &mins, const Vector &maxs, short *list, int listsize, mnode_t **headnode )
{
	leaflist_t ll;
	model_t	*worldmodel;

	worldmodel = gEngfuncs.GetEntityByIndex( 0 )->model;
	if( headnode ) *headnode = NULL;

	if( !worldmodel )
		return 0;

	ll.mins = mins;
	ll.maxs = maxs;
	ll.count = 0;
	ll.maxcount = listsize;
	ll.list = list;
	ll.headnode = NULL;
	ll.overflowed = false;

	Mod_BoxLeafnums_r( &ll, worldmodel->nodes, worldmodel );

	if( headnode )
		*headnode = ll.headnode;
	return ll.count;
}

/*
=============
Mod_HeadnodeVisible
=============
*/
bool Mod_HeadnodeVisible( mnode_t *node, const byte *visbits )
{
	if( !node || node->contents == CONTENTS_SOLID )
		return false;

	if( node->contents < 0 )
	{
		if( CHECKVISBIT( visbits, ((mleaf_t *)node)->cluster ))
			return true;
		return false;
	}

	if( Mod_HeadnodeVisible( node->children[0], visbits ))
		return true;

	return Mod_HeadnodeVisible( node->children[1], visbits );
}

/*
=============
Mod_BoxVisible

Returns true if any leaf in boxspace
is potentially visible
=============
*/
bool Mod_BoxVisible( const Vector &mins, const Vector &maxs, const byte *visbits )
{
	short leafList[48];
	leaflist_t ll;

	if( !visbits ) return true;

	ll.maxcount = ARRAYSIZE( leafList );
	ll.list = leafList;
	ll.headnode = NULL;
	ll.overflowed = false;
	ll.mins = mins;
	ll.maxs = maxs;
	ll.count = 0;

	Mod_BoxLeafnums_r( &ll, worldmodel->nodes, worldmodel );

	for( int i = 0; i < ll.count; i++ )
	{
		if( CHECKVISBIT( visbits, leafList[i] ))
			return true;
	}

	if( !ll.overflowed )
		return false; // invisible

	// overflowed? go by headnode
	if( Mod_HeadnodeVisible( ll.headnode, visbits ))
		return true;

	return false;
}

/*
===================
Mod_DecompressVis
===================
*/
void Mod_DecompressVis( const byte *in, byte *pvs )
{
	int	c;
	byte	*out;
	int	row;

	row = (worldmodel->numleafs+7)>>3;	
	out = pvs;

	if( !in )
	{	// no vis info, so make all visible
		while( row )
		{
			*out++ = 0xff;
			row--;
		}
		return;
	}

	do
	{
		if( *in )
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while( c )
		{
			*out++ = 0;
			c--;
		}
	} while( out - pvs < row );
}

byte *Mod_LeafPVS( mleaf_t *leaf, model_t *model )
{
	if( !model || !leaf || leaf == model->leafs || !model->visdata )
		Mod_DecompressVis( NULL, decompressed );
	else Mod_DecompressVis( leaf->compressed_vis, decompressed );

	return decompressed;
}

/*
==================
Mod_PointInLeaf

==================
*/
mleaf_t *Mod_PointInLeaf( const Vector &p, mnode_t *node )
{
	while( 1 )
	{
		if( node->contents < 0 )
			return (mleaf_t *)node;
		node = node->children[PlaneDiff( p, node->plane ) < 0];
	}

	// never reached
	return NULL;
}

bool Mod_PointInSolid( const Vector &p )
{
	return (Mod_PointInLeaf( p, worldmodel->nodes )->contents == CONTENTS_SOLID) ? true : false;
}

byte *Mod_GetCurrentVis( void )
{
	return RI->view.pvsarray;
}

bool Mod_CheckBoxVisible( const Vector &absmin, const Vector &absmax )
{
	return Mod_BoxVisible( absmin, absmax, Mod_GetCurrentVis( ));
}

bool Mod_CheckEntityPVS( cl_entity_t *ent )
{
	if( !ent || !ent->index )
		return false;	// not exist on the client

	if( ent->curstate.messagenum != r_currentMessageNum )
		return false;	// already culled by server

	Vector mins = ent->curstate.origin + ent->curstate.mins;
	Vector maxs = ent->curstate.origin + ent->curstate.maxs;

	return Mod_BoxVisible( mins, maxs, Mod_GetCurrentVis( ));
}

bool Mod_CheckEntityLeafPVS( const Vector &absmin, const Vector &absmax, mleaf_t *leaf )
{
	return Mod_BoxVisible( absmin, absmax, Mod_GetCurrentVis( ));
}

bool Mod_CheckTempEntityPVS( TEMPENTITY *pTemp )
{
	if( !pTemp ) return false; // not exist on the client

	Vector mins = pTemp->entity.curstate.origin + pTemp->entity.curstate.mins;
	Vector maxs = pTemp->entity.curstate.origin + pTemp->entity.curstate.maxs;

	return Mod_BoxVisible( mins, maxs, Mod_GetCurrentVis( ));
}

model_t *Mod_Handle( int modelIndex )
{
	return IEngineStudio.GetModelByIndex( modelIndex );
}

int Mod_GetType( int modelIndex )
{
	model_t 	*m_pModel;

	m_pModel = IEngineStudio.GetModelByIndex( modelIndex );
	if( m_pModel == NULL )
		return mod_bad;

	return m_pModel->type;
}

/*
===================
Mod_GetFrames
===================
*/
void Mod_GetFrames( int modelIndex, int &numFrames )
{
	model_t 	*m_pModel;

	m_pModel = IEngineStudio.GetModelByIndex( modelIndex );

	if( !m_pModel )
	{
		numFrames = 1;
		return;
	}

	numFrames = m_pModel->numframes;
	if( numFrames < 1 ) numFrames = 1;
}

/*
==============
GetVisCache

decompress visibility string
==============
*/
mleaf_t *GetVisCache( mleaf_t *lastleaf, mleaf_t *leaf, byte *pvs )
{
	// get the PVS for the pos to limit the number of checks
	if( !worldmodel->visdata )
	{
		if( leaf != lastleaf )
		{
			memset( pvs, 255, (worldmodel->numleafs + 7) / 8 );
			leaf = worldmodel->leafs;
		}
	}
	else if( leaf != lastleaf )
	{
		if( leaf == worldmodel->leafs )
			memset( pvs, 0, (worldmodel->numleafs + 7) / 8 );
		else Mod_DecompressVis( leaf->compressed_vis, pvs );
		lastleaf = leaf;
	}

	return lastleaf;
}

/*
==============
SetDLightVis

Init dlight visibility
==============
*/
void SetDLightVis( mworldlight_t *wl, int leafnum )
{
	if( !wl->pvs )
		wl->pvs = (byte *)Mem_Alloc( (worldmodel->numleafs + 7) / 8 );
	GetVisCache( NULL, worldmodel->leafs + leafnum, wl->pvs );
}

/*
==============
MergeDLightVis

Merge dlight vis with another leaf
==============
*/
void MergeDLightVis( mworldlight_t *wl, int leafnum )
{
	if( !wl->pvs )
	{
		SetDLightVis( wl, leafnum );
	}
	else
	{
		byte	pvs[(MAX_MAP_LEAFS + 7) / 8];
	
		GetVisCache( NULL, worldmodel->leafs + leafnum, pvs );

		// merge both vis graphs
		for( int i = 0; i < (worldmodel->numleafs + 7) / 8; i++ )
			wl->pvs[i] |= pvs[i];
	}
}

#define MAX_POLYGON_POINTS	64
#define PLANESIDE_FRONT	1
#define PLANESIDE_BACK	2
#define PLANESIDE_ON	3

/*
==================
R_ClipPolygon
==================
*/
bool R_ClipPolygon( int numPoints, Vector *points, const mplane_t *plane, int *numClipped, Vector *clipped )
{
	float	dists[MAX_POLYGON_POINTS];
	int	sides[MAX_POLYGON_POINTS];
	bool	frontSide, backSide;
	float	frac;
	int	i;

	if( numPoints >= MAX_POLYGON_POINTS - 2 )
		HOST_ERROR( "R_ClipPolygon: MAX_POLYGON_POINTS hit\n" );

	*numClipped = 0;

	// Determine sides for each point
	frontSide = false;
	backSide = false;

	for( i = 0; i < numPoints; i++ )
	{
		dists[i] = PlaneDiff( points[i], plane );

		if( dists[i] > ON_EPSILON )
		{
			sides[i] = PLANESIDE_FRONT;
			frontSide = true;
			continue;
		}

		if( dists[i] < -ON_EPSILON )
		{
			sides[i] = PLANESIDE_BACK;
			backSide = true;
			continue;
		}

		sides[i] = PLANESIDE_ON;
	}

	if( !frontSide )
		return false;	// Not clipped

	if( !backSide )
	{
		*numClipped = numPoints;
		memcpy( clipped, points, numPoints * sizeof( Vector ));

		return true;
	}

	// xlip it
	points[i] = points[0];
	dists[i] = dists[0];
	sides[i] = sides[0];

	for( i = 0; i < numPoints; i++ )
	{
		if( sides[i] == PLANESIDE_ON )
		{
			clipped[(*numClipped)++] = points[i];
			continue;
		}

		if( sides[i] == PLANESIDE_FRONT )
			clipped[(*numClipped)++] = points[i];

		if( sides[i+1] == PLANESIDE_ON || sides[i+1] == sides[i] )
			continue;

		if( dists[i] == dists[i+1] )
		{
			clipped[(*numClipped)++] = points[i];
		}
		else
		{
			frac = dists[i] / (dists[i] - dists[i+1]);
			clipped[(*numClipped)++] = points[i] + (points[i+1] - points[i]) * frac;
		}
	}

	return true;
}

/*
==================
R_SplitPolygon
==================
*/
void R_SplitPolygon( int numPoints, Vector *points, const mplane_t *plane, int *numFront, Vector *front, int *numBack, Vector *back )
{
	float	dists[MAX_POLYGON_POINTS];
	int	sides[MAX_POLYGON_POINTS];
	bool	frontSide, backSide;
	Vector	mid;
	float	frac;
	int	i;

	if( numPoints >= MAX_POLYGON_POINTS - 2 )
		HOST_ERROR( "R_SplitPolygon: MAX_POLYGON_POINTS hit\n" );

	*numFront = 0;
	*numBack = 0;

	// Determine sides for each point
	frontSide = false;
	backSide = false;

	for( i = 0; i < numPoints; i++ )
	{
		dists[i] = PlaneDiff( points[i], plane );

		if( dists[i] > ON_EPSILON )
		{
			sides[i] = PLANESIDE_FRONT;
			frontSide = true;
			continue;
		}

		if( dists[i] < -ON_EPSILON )
		{
			sides[i] = PLANESIDE_BACK;
			backSide = true;
			continue;
		}

		sides[i] = PLANESIDE_ON;
	}

	if( !frontSide )
	{
		*numBack = numPoints;
		memcpy( back, points, numPoints * sizeof( Vector ));
		return;
	}

	if( !backSide )
	{
		*numFront = numPoints;
		memcpy( front, points, numPoints * sizeof( Vector ));
		return;
	}

	// split it
	points[i] = points[0];

	dists[i] = dists[0];
	sides[i] = sides[0];

	for( i = 0; i < numPoints; i++ )
	{
		if( sides[i] == PLANESIDE_ON )
		{
			front[(*numFront)++] = points[i];
			back[(*numBack)++] = points[i];
			continue;
		}

		if( sides[i] == PLANESIDE_FRONT )
			front[(*numFront)++] = points[i];

		if( sides[i] == PLANESIDE_BACK )
			back[(*numBack)++] = points[i];

		if( sides[i+1] == PLANESIDE_ON || sides[i+1] == sides[i] )
			continue;

		if( dists[i] == dists[i+1] )
		{
			front[(*numFront)++] = points[i];
			back[(*numBack)++] = points[i];
		}
		else
		{
			frac = dists[i] / (dists[i] - dists[i+1]);
			mid = points[i] + (points[i+1] - points[i]) * frac;
			front[(*numFront)++] = mid;
			back[(*numBack)++] = mid;
		}
	}
}

/*
==================
R_TransformWorldToDevice
==================
*/
void R_TransformWorldToDevice( const Vector &world, Vector &ndc )
{
	Vector4D	clip;
	float	scale;

	clip[0] = world[0] * RI->view.worldProjectionMatrix[0][0] + world[1] * RI->view.worldProjectionMatrix[1][0] + world[2] * RI->view.worldProjectionMatrix[2][0] + RI->view.worldProjectionMatrix[3][0];
	clip[1] = world[0] * RI->view.worldProjectionMatrix[0][1] + world[1] * RI->view.worldProjectionMatrix[1][1] + world[2] * RI->view.worldProjectionMatrix[2][1] + RI->view.worldProjectionMatrix[3][1];
	clip[2] = world[0] * RI->view.worldProjectionMatrix[0][2] + world[1] * RI->view.worldProjectionMatrix[1][2] + world[2] * RI->view.worldProjectionMatrix[2][2] + RI->view.worldProjectionMatrix[3][2];
	clip[3] = world[0] * RI->view.worldProjectionMatrix[0][3] + world[1] * RI->view.worldProjectionMatrix[1][3] + world[2] * RI->view.worldProjectionMatrix[2][3] + RI->view.worldProjectionMatrix[3][3];

	if( clip[3] == 0.0f )
	{
		ndc[0] = clip[0];
		ndc[1] = clip[1];
		ndc[2] = -1.0f;
	}
	else
	{
		scale = 1.0f / clip[3];
		ndc[0] = clip[0] * scale;
		ndc[1] = clip[1] * scale;
		ndc[2] = clip[2] * scale;
	}
}

/*
==================
R_TransformDeviceToScreen
==================
*/
void R_TransformDeviceToScreen( const Vector &ndc, Vector &screen )
{
	screen[0] = (ndc[0] * 0.5f + 0.5f) * (RI->view.port[2] - RI->view.port[0]) + RI->view.port[0];
	screen[1] = (ndc[1] * 0.5f + 0.5f) * (RI->view.port[3] - RI->view.port[1]) + RI->view.port[1];
	screen[2] = (ndc[2] * 0.5f + 0.5f);
}

/*
==================
R_ScissorForBounds
==================
*/
static bool R_ScissorForBounds( const Vector bbox[8], float *x, float *y, float *w, float *h )
{
	static int	cornerIndices[6][4] = {{3, 2, 6, 7}, {0, 1, 5, 4}, {2, 3, 1, 0}, {4, 5, 7, 6}, {1, 3, 7, 5}, {2, 0, 4, 6}};
	Vector		points[2][MAX_POLYGON_POINTS];
	float		ix1, iy1, ix2, iy2;
	float		x1, y1, x2, y2;
	int		numPoints;
	Vector		bounds[2];
	Vector		ndc;
	int		pingPong = 0;
	int		i, j;

	// Clip the light volume to the view frustum
	ClearBounds( bounds[0], bounds[1] );

	for( i = 0; i < 6; i++ )
	{
		numPoints = 4;

		points[pingPong][0] = bbox[cornerIndices[i][0]];
		points[pingPong][1] = bbox[cornerIndices[i][1]];
		points[pingPong][2] = bbox[cornerIndices[i][2]];
		points[pingPong][3] = bbox[cornerIndices[i][3]];

		for( j = 0; j < FRUSTUM_PLANES; j++ )
		{
			if( !FBitSet( RI->view.frustum.GetClipFlags(), BIT( j )))
				continue;

			if( !R_ClipPolygon( numPoints, points[pingPong], RI->view.frustum.GetPlane( j ), &numPoints, points[!pingPong] ))
				break;

			pingPong ^= 1;
		}

		if( j != FRUSTUM_PLANES )
			continue;

		// Add the clipped points
		for( j = 0; j < numPoints; j++ )
		{
			// Transform into normalized device coordinates
			R_TransformWorldToDevice( points[pingPong][j], ndc );

			// Add it
			AddPointToBounds( ndc, bounds[0], bounds[1] );
		}
	}

	// If completely clipped away, clear the scissor
	if( BoundsIsCleared( bounds[0], bounds[1] ))
		return false;

	// Transform into screen space
	R_TransformDeviceToScreen( bounds[0], bounds[0] );
	R_TransformDeviceToScreen( bounds[1], bounds[1] );

	x1 = bounds[0].x - 1.0f;
	y1 = bounds[0].y - 1.0f;
	x2 = bounds[1].x + 1.0f;
	y2 = bounds[1].y + 1.0f;

	ix1 = Q_max( x1, RI->view.port[0] );
	ix2 = Q_min( x2, RI->view.port[2] );
	iy1 = Q_max( y1, RI->view.port[1] );
	iy2 = Q_min( y2, RI->view.port[3] );

	*x = ix1 + 1.0f;
	*y = RI->view.port[3] - iy2; // stupid OpenGL bug
	*w = ix2 - ix1 - 1.0f;
	*h = iy2 - iy1 - 1.0f;

	// headshield issues
	if( ix1 >= ix2 || iy1 >= iy2 )
		return false;
	return true;
}

void R_BoundsForAABB( const Vector &absmin, const Vector &absmax, Vector bbox[8] )
{
	ASSERT( bbox != NULL );

	// compute a full bounding box
	for( int i = 0; i < 8; i++ )
	{
  		bbox[i][0] = ( i & 1 ) ? absmin[0] : absmax[0];
  		bbox[i][1] = ( i & 2 ) ? absmin[1] : absmax[1];
  		bbox[i][2] = ( i & 4 ) ? absmin[2] : absmax[2];
	}
}

bool R_ScissorForAABB( const Vector &absmin, const Vector &absmax, float *x, float *y, float *w, float *h )
{
	Vector bbox[8];

	R_BoundsForAABB( absmin, absmax, bbox );

	return R_ScissorForBounds( bbox, x, y, w, h );
}

bool R_ScissorForFrustum( CFrustum *frustum, float *x, float *y, float *w, float *h )
{
	Vector bbox[8];

	frustum->ComputeFrustumCorners( bbox );

	// NOTE: disable farplane because dynamic zFar
	RI->view.frustum.DisablePlane( FRUSTUM_FAR );
	bool result = R_ScissorForBounds( bbox, x, y, w, h );
	RI->view.frustum.EnablePlane( FRUSTUM_FAR );

	return result;
}

bool R_AABBToScreen( const Vector &absmin, const Vector &absmax, Vector2D &scrmin, Vector2D &scrmax, wrect_t *rect )
{
	float x, y, w, h;
	Vector bbox[8];

	R_BoundsForAABB( absmin, absmax, bbox );
	ClearBounds( scrmin, scrmax );

	if( !R_ScissorForBounds( bbox, &x, &y, &w, &h ))
	{
		if( rect ) memset( rect, 0, sizeof( *rect ));
		return false;
          }

	// copy rectangle
	if( rect )
	{
		rect->left = x;
		rect->right = RI->view.port[3] - h - y; // left bottom corner
		rect->top = w;
		rect->bottom = h;
	}

	// compute a bounding box
	AddPointToBounds( Vector2D( x, y ), scrmin, scrmax );
	AddPointToBounds( Vector2D( x + w, y ), scrmin, scrmax );
	AddPointToBounds( Vector2D( x, y + h ), scrmin, scrmax );
	AddPointToBounds( Vector2D( x + w, y + h ), scrmin, scrmax );

	return true;
}

// debug thing
void R_DrawScissorRectangle( float x, float y, float w, float h )
{
	GL_BindTexture( GL_TEXTURE0, tr.whiteTexture );
	pglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	GL_Setup2D();

	pglColor3f( 1, 0.5, 0 );

	pglBegin( GL_LINES );
		pglVertex2f( x, y );
		pglVertex2f( x + w, y );
		pglVertex2f( x, y );
		pglVertex2f( x, y + h );
		pglVertex2f( x + w, y );
		pglVertex2f( x + w, y + h );
		pglVertex2f( x, y + h );
		pglVertex2f( x + w, y + h );
	pglEnd();

	GL_Setup3D();
}

//-----------------------------------------------------------------------------
// This returns the diameter of the sphere in pixels based on 
// the current model, view, + projection matrices and viewport.
//-----------------------------------------------------------------------------
static float ComputePixelDiameterOfSphere( const Vector& vecAbsOrigin, float flRadius )
{
	// This is sort of faked, but it's faster that way
	// FIXME: Also, there's a much faster way to do this with similar triangles
	// but I want to make sure it exactly matches the current matrices, so
	// for now, I do it this conservative way
	Vector4D testPoint1 = vecAbsOrigin + GetVUp() * flRadius;
	Vector4D testPoint2 = vecAbsOrigin + GetVUp() * -flRadius;
	Vector4D clipPos1 = RI->view.worldProjectionMatrix.VectorTransform( testPoint1 );
	Vector4D clipPos2 = RI->view.worldProjectionMatrix.VectorTransform( testPoint2 );

	if( clipPos1.w >= 0.001f )
	{
		clipPos1.y /= clipPos1.w;
	}
	else
	{
		clipPos1.y *= 1000.0f;
	}

	if( clipPos2.w >= 0.001f )
	{
		clipPos2.y /= clipPos2.w;
	}
	else
	{
		clipPos2.y *= 1000.0f;
	}

	// The divide-by-two here is because y goes from -1 to 1 in projection space
	return RI->view.port[3] * fabs( clipPos2.y - clipPos1.y ) / 2.0f;
}

float ComputePixelWidthOfSphere( const Vector& vecOrigin, float flRadius )
{
	return ComputePixelDiameterOfSphere( vecOrigin, flRadius ) * 2.0f;
}

HSPRITE LoadSprite(const char *pszName)
{
	int i;
	char sz[256]; 

	if (ScreenWidth < 640)
		i = 320;
	else
		i = 640;

	sprintf(sz, pszName, i);

	return SPR_Load(sz);
}

void Physic_SweepTest( cl_entity_t *pTouch, const Vector &start, const Vector &mins, const Vector &maxs, const Vector &end, trace_t *tr )
{
	if( !pTouch->model || !pTouch->model->cache.data )
	{
		// bad model?
		tr->allsolid = false;
		return;
	}

	Vector trace_mins, trace_maxs, bounds[2];
	UTIL_MoveBounds( start, mins, maxs, end, trace_mins, trace_maxs );

	if( !R_StudioGetBounds( pTouch, bounds ))
	{
		tr->allsolid = false;
		return;
	}

	// NOTE: pmove code completely ignore a bounds checking. So we need to do it here
	if( !BoundsIntersect( trace_mins, trace_maxs, bounds[0], bounds[1] ))
	{
		tr->allsolid = false;
		return;
	}

	CMeshDesc	*bodyMesh = UTIL_GetCollisionMesh( pTouch->curstate.modelindex );

	if( !bodyMesh )
	{
		tr->allsolid = false;
		return;
	}

	mmesh_t *pMesh = bodyMesh->GetMesh();
	areanode_t *pHeadNode = bodyMesh->GetHeadNode();
	entity_state_t *pev = &pTouch->curstate;

	if( !pMesh )
	{
		// failed to build mesh for some reasons, so skip them
		tr->allsolid = false;
		return;
	}

	TraceMesh	trm;	// a name like Doom3 :-)

	trm.SetTraceMesh( pMesh, pHeadNode, pTouch->curstate.modelindex );
	trm.SetMeshOrientation( pev->origin, pev->angles, pev->startpos );
	trm.SetupTrace( start, mins, maxs, end, tr );

	if( trm.DoTrace( ))
	{
		if( tr->fraction < 1.0f || tr->startsolid )
			tr->ent = (edict_t *)pTouch;	// g-cont. no need, really. i'm leave this just for fun
		tr->surf = trm.GetLastHitSurface();
	}
}

#define GAMMA		( 2.2f )		// Valve Software gamma
#define INVGAMMA		( 1.0f / 2.2f )	// back to 1.0

static float	texturetolinear[256];	// texture (0..255) to linear (0..1)
static int	lineartotexture[1024];	// linear (0..1) to texture (0..255)

void BuildGammaTable( void )
{
	int	i;
	float	g1, g3;
	float	g = bound( 0.5f, GAMMA, 3.0f );

	g = 1.0 / g;
	g1 = GAMMA * g; 
	g3 = 0.125f;

	for( i = 0; i < 256; i++ )
	{
		// convert from nonlinear texture space (0..255) to linear space (0..1)
		texturetolinear[i] =  pow( i / 255.0f, GAMMA );
	}

	for( i = 0; i < 1024; i++ )
	{
		// convert from linear space (0..1) to nonlinear texture space (0..255)
		lineartotexture[i] =  pow( i / 1023.0, INVGAMMA ) * 255;
	}
}

// convert texture to linear 0..1 value
float TextureToLinear( int c ) { return texturetolinear[bound( 0, c, 255 )]; }

// convert texture to linear 0..1 value
int LinearToTexture( float f ) { return lineartotexture[bound( 0, (int)(f * 1023), 1023 )]; }

/*
===============================================================================

	LEGACY STUFF

  moved here from studio_util.cpp
===============================================================================
*/
/*
====================
AngleMatrix

====================
*/
void AngleMatrix (const float *angles, float (*matrix)[4] )
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	
	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp*cy;
	matrix[1][0] = cp*sy;
	matrix[2][0] = -sp;
	matrix[0][1] = sr*sp*cy+cr*-sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[2][1] = sr*cp;
	matrix[0][2] = (cr*sp*cy+-sr*-sy);
	matrix[1][2] = (cr*sp*sy+-sr*cy);
	matrix[2][2] = cr*cp;
	matrix[0][3] = 0.0;
	matrix[1][3] = 0.0;
	matrix[2][3] = 0.0;
}

/*
====================
VectorTransform

====================
*/
void VectorTransform (const float *in1, float in2[3][4], float *out)
{
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
}

vec3_t vec3_origin( 0, 0, 0 );

float Length(const float *v)
{
	int		i;
	float	length;
	
	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}

void VectorAngles( const float *forward, float *angles )
{
	float	tmp, yaw, pitch;
	
	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		tmp = sqrt (forward[0]*forward[0] + forward[1]*forward[1]);
		pitch = (atan2(forward[2], tmp) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

float VectorNormalize (float *v)
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);		// FIXME

	if (length)
	{
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;

}

void VectorScale (const float *in, float scale, float *out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void VectorMA (const float *veca, float scale, const float *vecb, float *vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}

float PackColor( const color24 &color )
{
	return (float)((double)((color.r << 16) | (color.g << 8) | color.b) / (double)(1 << 24));
}

color24 UnpackColor( float pack )
{
	Vector color = Vector( 1.0f, 256.0f, 65536.0f ) * pack;

	// same as fract( color )
	color.x = color.x - floor( color.x );
	color.y = color.y - floor( color.y );
	color.z = color.z - floor( color.z );

	return color24( color.x * 256, color.y * 256, color.z * 256 );
}