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
// cl_util.h
//

#include "cvardef.h"

#define EXPORT	_declspec( dllexport )
#define DLLEXPORT	__declspec( dllexport )

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern int developer_level;
extern int r_currentMessageNum;

extern int g_iXashEngineBuildNumber;
extern BOOL g_fRenderInitialized;
extern BOOL g_fRenderInterfaceValid;
extern BOOL g_fXashEngine;

enum
{
	DEV_NONE = 0,
	DEV_NORMAL,
	DEV_EXTENDED
};

typedef HMODULE dllhandle_t;

typedef struct dllfunc_s
{
	const char *name;
	void	**func;
} dllfunc_t;

// misc cvars
extern cvar_t	*r_test;	// just cvar for testify new effects
extern cvar_t	*r_stencilbits;
extern cvar_t	*r_drawentities;
extern cvar_t	*gl_extensions;
extern cvar_t	*cv_dynamiclight;
extern cvar_t	*r_detailtextures;
extern cvar_t	*r_lighting_ambient;
extern cvar_t	*r_lighting_modulate;
extern cvar_t	*r_lightstyle_lerping;
extern cvar_t	*r_lighting_extended;
extern cvar_t	*r_occlusion_culling;
extern cvar_t	*r_show_lightprobes;
extern cvar_t	*r_show_cubemaps;
extern cvar_t	*r_show_viewleaf;
extern cvar_t	*cv_crosshair;
extern cvar_t	*r_shadows;
extern cvar_t	*r_fullbright;
extern cvar_t	*r_draw_beams;
extern cvar_t	*r_overview;
extern cvar_t	*r_novis;
extern cvar_t	*r_nocull;
extern cvar_t	*r_lockpvs;
extern cvar_t	*r_dof;
extern cvar_t	*r_dof_hold_time;
extern cvar_t	*r_dof_change_time;
extern cvar_t	*r_dof_focal_length;
extern cvar_t	*r_dof_fstop;
extern cvar_t	*r_dof_debug;
extern cvar_t	*r_allow_mirrors;
extern cvar_t	*cv_renderer;
extern cvar_t	*cv_brdf;
extern cvar_t	*cv_bump;
extern cvar_t	*cv_specular;
extern cvar_t	*cv_parallax;
extern cvar_t	*cv_decals;
extern cvar_t	*cv_realtime_puddles;
extern cvar_t	*cv_shadow_offset;
extern cvar_t	*cv_cubemaps;
extern cvar_t	*cv_deferred;
extern cvar_t	*cv_deferred_full;
extern cvar_t	*cv_deferred_maxlights;
extern cvar_t	*cv_deferred_tracebmodels;
extern cvar_t	*cv_cube_lod_bias;
extern cvar_t	*cv_gamma;
extern cvar_t	*cv_brightness;
extern cvar_t	*cv_water;
extern cvar_t	*cv_decalsdebug;
extern cvar_t	*cv_show_tbn;
extern cvar_t	*cv_nosort;
extern cvar_t	*r_lightmap;
extern cvar_t	*r_speeds;
extern cvar_t	*r_decals;
extern cvar_t	*r_studio_decals;
extern cvar_t	*r_hand;
extern cvar_t	*r_sunshadows;
extern cvar_t	*r_sun_allowed;
extern cvar_t	*r_shadow_split_weight;
extern cvar_t	*r_wireframe;
extern cvar_t	*r_lightstyles;
extern cvar_t	*r_polyoffset;
extern cvar_t	*r_dynamic;
extern cvar_t	*r_finish;
extern cvar_t	*r_clear;
extern cvar_t	*r_grass;
extern cvar_t	*r_grass_alpha;
extern cvar_t	*r_grass_lighting;
extern cvar_t	*r_grass_shadows;
extern cvar_t	*r_grass_fade_start;
extern cvar_t	*r_grass_fade_dist;
extern cvar_t	*r_scissor_glass_debug;
extern cvar_t	*r_scissor_light_debug;
extern cvar_t	*r_showlightmaps;
extern cvar_t	*r_recursion_depth;
extern cvar_t	*r_shadowmap_size;
extern cvar_t	*r_pssm_show_split;
extern cvar_t	*v_sunshafts;
extern cvar_t	*v_glows;

extern "C" void DLLEXPORT HUD_StudioEvent( const struct mstudioevent_s *event, const struct cl_entity_s *entity );

// Macros to hook function calls into the HUD object
#define HOOK_MESSAGE(x) gEngfuncs.pfnHookUserMsg(#x, __MsgFunc_##x );

#define DECLARE_MESSAGE(y, x) int __MsgFunc_##x(const char *pszName, int iSize, void *pbuf) \
							{ \
							return gHUD.##y.MsgFunc_##x(pszName, iSize, pbuf ); \
							}


#define HOOK_COMMAND(x, y) gEngfuncs.pfnAddCommand( x, __CmdFunc_##y );
#define DECLARE_COMMAND(y, x) void __CmdFunc_##x( void ) \
							{ \
								gHUD.##y.UserCmd_##x( ); \
							}

#define SPR_Set (*gEngfuncs.pfnSPR_Set)
#define SPR_Frames (*gEngfuncs.pfnSPR_Frames)
client_sprite_t *SPR2_GetList( char *psz, int *piCount );	// new version

// SPR_Draw  draws a the current sprite as solid
#define SPR_Draw (*gEngfuncs.pfnSPR_Draw)
// SPR_DrawHoles  draws the current sprites,  with color index255 not drawn (transparent)
#define SPR_DrawHoles (*gEngfuncs.pfnSPR_DrawHoles)
// SPR_DrawAdditive  adds the sprites RGB values to the background  (additive transulency)
#define SPR_DrawAdditive (*gEngfuncs.pfnSPR_DrawAdditive)

// SPR_EnableScissor  sets a clipping rect for HUD sprites.  (0,0) is the top-left hand corner of the screen.
#define SPR_EnableScissor (*gEngfuncs.pfnSPR_EnableScissor)
// SPR_DisableScissor  disables the clipping rect
#define SPR_DisableScissor (*gEngfuncs.pfnSPR_DisableScissor)
//
#define FillRGBA (*gEngfuncs.pfnFillRGBA)


// ScreenHeight returns the height of the screen, in pixels
#define ScreenHeight (gHUD.m_scrinfo.iHeight)
// ScreenWidth returns the width of the screen, in pixels
#define ScreenWidth (gHUD.m_scrinfo.iWidth)

// Use this to set any co-ords in 640x480 space
#define XRES(x)		((int)(float(x)  * ((float)ScreenWidth / 640.0f) + 0.5f))
#define YRES(y)		((int)(float(y)  * ((float)ScreenHeight / 480.0f) + 0.5f))

// use this to project world coordinates to screen coordinates
#define XPROJECT(x)		( (1.0f+(x))*ScreenWidth*0.5f )
#define YPROJECT(y)		( (1.0f-(y))*ScreenHeight*0.5f )

#define GetScreenInfo (*gEngfuncs.pfnGetScreenInfo)
#define ServerCmd (*gEngfuncs.pfnServerCmd)
#define ClientCmd (*gEngfuncs.pfnClientCmd)
#define SetCrosshair (*gEngfuncs.pfnSetCrosshair)
#define AngleVectors (*gEngfuncs.pfnAngleVectors)

inline int ConsoleStringLen( const char *string )
{
	int _width, _height;
	GetConsoleStringSize( string, &_width, &_height );
	return _width;
}

// returns the players name of entity no.
#define GetPlayerInfo (*gEngfuncs.pfnGetPlayerInfo)

#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#define fabs(x)	   ((x) > 0 ? (x) : 0 - (x))

void ScaleColors( int &r, int &g, int &b, int a );

#define DotProduct(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorSubtract(a,b,c) {(c)[0]=(a)[0]-(b)[0];(c)[1]=(a)[1]-(b)[1];(c)[2]=(a)[2]-(b)[2];}
#define VectorAdd(a,b,c) {(c)[0]=(a)[0]+(b)[0];(c)[1]=(a)[1]+(b)[1];(c)[2]=(a)[2]+(b)[2];}
#define VectorCopy(a,b)	((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
inline void VectorClear(float *a) { a[0]=0.0;a[1]=0.0;a[2]=0.0;}
float Length(const float *v);
void VectorMA (const float *veca, float scale, const float *vecb, float *vecc);
void VectorScale (const float *in, float scale, float *out);
float VectorNormalize (float *v);

extern vec3_t vec3_origin;
extern struct ref_params_s	*g_pViewParams;

// disable 'possible loss of data converting float to int' warning message
#pragma warning( disable: 4244 )
// disable 'truncation from 'const double' to 'float' warning message
#pragma warning( disable: 4305 )

inline void UnpackRGB(int &r, int &g, int &b, unsigned long ulRGB)\
{\
	r = (ulRGB & 0xFF0000) >>16;\
	g = (ulRGB & 0xFF00) >> 8;\
	b = ulRGB & 0xFF;\
}

inline unsigned int PackRGBA( int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );

	return ((a)<<24|(r)<<16|(g)<<8|(b));
}

inline void UnpackRGBA( int &r, int &g, int &b, int &a, unsigned int ulRGBA )
{
	a = (ulRGBA & 0xFF000000) >> 24;
	r = (ulRGBA & 0xFF0000) >> 16;
	g = (ulRGBA & 0xFF00) >> 8;
	b = (ulRGBA & 0xFF) >> 0;
}

float PackColor( const color24 &color );
color24 UnpackColor( float pack );

HSPRITE LoadSprite(const char *pszName);

#define CHECKVISBIT( vis, b )		((b) >= 0 ? (byte)((vis)[(b) >> 3] & (1 << ((b) & 7))) : (byte)false )
#define SETVISBIT( vis, b )( void )	((b) >= 0 ? (byte)((vis)[(b) >> 3] |= (1 << ((b) & 7))) : (byte)false )
#define CLEARVISBIT( vis, b )( void )	((b) >= 0 ? (byte)((vis)[(b) >> 3] &= ~(1 << ((b) & 7))) : (byte)false )

typedef struct leaflist_s
{
	int		count;
	int		maxcount;
	bool		overflowed;
	short		*list;
	Vector		mins, maxs;
	struct mnode_s	*headnode;	// for overflows where each leaf can't be stored individually
} leaflist_t;

struct mleaf_s *Mod_PointInLeaf( const Vector &p, struct mnode_s *node );
byte *Mod_LeafPVS( struct mleaf_s *leaf, struct model_s *model );
byte *Mod_GetCurrentVis( void );
bool Mod_BoxVisible( const Vector &mins, const Vector &maxs, const byte *visbits );
bool Mod_CheckEntityPVS( cl_entity_t *ent );
bool Mod_CheckTempEntityPVS( struct tempent_s *pTemp );
bool Mod_CheckEntityLeafPVS( const Vector &absmin, const Vector &absmax, struct mleaf_s *leaf );
bool Mod_CheckBoxVisible( const Vector &absmin, const Vector &absmax );
void Mod_GetFrames( int modelIndex, int &numFrames );
struct model_s *Mod_Handle( int modelIndex );
bool Mod_PointInSolid( const Vector &p );
int Mod_GetType( int modelIndex );
extern void ParseRain( void );
extern int CL_IsDead( void );
void SetDLightVis( struct mworldlight_s *wl, int leafnum );
void MergeDLightVis( struct mworldlight_s *wl, int leafnum );

bool UTIL_IsPlayer( int idx );
bool UTIL_IsLocal( int idx );
void UTIL_WeaponAnimation( int iAnim, float framerate );
void UTIL_StudioDecal( const char *pDecalName, struct pmtrace_s *pTrace, const Vector &vecSrc );
bool R_ScissorForAABB( const Vector &absmin, const Vector &absmax, float *x, float *y, float *w, float *h );
bool R_AABBToScreen( const Vector &absmin, const Vector &absmax, Vector2D &scrmin, Vector2D &scrmax, wrect_t *rect = NULL );
bool R_ScissorForFrustum( class CFrustum *frustum, float *x, float *y, float *w, float *h );
void R_DrawScissorRectangle( float x, float y, float w, float h );
int WorldToScreen( const Vector &world, Vector &screen );
void R_TransformWorldToDevice( const Vector &world, Vector &ndc );
void R_TransformDeviceToScreen( const Vector &ndc, Vector &screen );
float ComputePixelWidthOfSphere( const Vector& vecOrigin, float flRadius );

bool R_SkyIsVisible( void );

bool R_ClipPolygon( int numPoints, Vector *points, const struct mplane_s *plane, int *numClipped, Vector *clipped );
void R_SplitPolygon( int numPoints, Vector *points, const struct mplane_s *plane, int *numFront, Vector *front, int *numBack, Vector *back );

// dll managment
bool Sys_LoadLibrary( const char *dllname, dllhandle_t *handle, const dllfunc_t *fcts = NULL );
void *Sys_GetProcAddress( dllhandle_t handle, const char *name );
void Sys_FreeLibrary( dllhandle_t *handle );
bool Sys_RemoveFile( const char *path );

void UTIL_CreateAurora( cl_entity_t *ent, const char *file, int attachment, float lifetime = 0.0f );
void UTIL_RemoveAurora( cl_entity_t *ent );

extern void AngleMatrix (const float *angles, float (*matrix)[4] );
extern void VectorTransform (const float *in1, float in2[3][4], float *out);
extern void SetPoint( float x, float y, float z, float (*matrix)[4]);
extern void CreateDecal( const Vector &p, const Vector &n, float ang, const char *sz, int flags = 0, int eIdx = 0, int mIdx = 0, bool source = true );
extern void CreateDecal( struct pmtrace_s *tr, const char *name, float angle, bool visent = false );

extern void Physic_SweepTest( struct cl_entity_s *pTouch, const Vector &start, const Vector &mins, const Vector &maxs, const Vector &end, trace_t *tr );

extern void BuildGammaTable( void );
extern float TextureToLinear( int c );
extern int LinearToTexture( float f );
extern void GL_GpuMemUsage_f( void );
