//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// view/refresh setup functions

#include "hud.h"
#include "cl_util.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "stringlib.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "ref_params.h"
#include "in_defs.h" // PITCH YAW ROLL
#include "pm_shared.h"
#include "pm_defs.h"
#include "event_api.h"
#include "pmtrace.h"
#include "screenfade.h"
#include "shake.h"
#include "hltv.h"
#include "gl_local.h" // buz

extern int g_flashlight;
extern int g_iGunMode; // buz

void SetupFlashlight( struct ref_params_s *pparams );

cvar_t	*r_test;
cvar_t	*gl_extensions;
cvar_t	*r_detailtextures;
cvar_t	*r_lighting_ambient;
cvar_t	*r_lighting_modulate;
cvar_t	*r_lighting_extended;
cvar_t	*r_lightstyle_lerping;
cvar_t	*r_occlusion_culling;
cvar_t	*r_show_lightprobes;
cvar_t	*r_show_cubemaps;
cvar_t	*r_show_viewleaf;
cvar_t	*r_allow_mirrors;
cvar_t	*r_drawentities;
cvar_t	*r_draw_beams;
cvar_t	*cv_crosshair;
cvar_t	*r_fullbright;
cvar_t	*r_overview;
cvar_t	*r_dynamic;
cvar_t	*r_shadows;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lockpvs;
cvar_t	*r_dof;
cvar_t	*r_dof_hold_time;
cvar_t	*r_dof_change_time;
cvar_t	*r_dof_focal_length;
cvar_t	*r_dof_fstop;
cvar_t	*r_dof_debug;
cvar_t	*cv_renderer;
cvar_t	*cv_brdf;
cvar_t	*cv_bump;
cvar_t	*cv_bumpvecs;
cvar_t	*cv_specular;
cvar_t	*cv_cubemaps;
cvar_t	*cv_deferred;
cvar_t	*cv_deferred_full;
cvar_t	*cv_deferred_maxlights;
cvar_t	*cv_deferred_tracebmodels;
cvar_t	*cv_cube_lod_bias;
cvar_t	*cv_dynamiclight;
cvar_t	*cv_realtime_puddles;
cvar_t	*cv_shadow_offset;
cvar_t	*cv_parallax;
cvar_t	*cv_decals;
cvar_t	*cv_gamma;
cvar_t	*cv_brightness;
cvar_t	*cv_water; 
cvar_t	*cv_decalsdebug;
cvar_t	*cv_show_tbn;
cvar_t	*cv_nosort;
cvar_t	*r_wireframe;
cvar_t	*r_lightmap;
cvar_t	*r_speeds;
cvar_t	*r_decals;
cvar_t	*r_studio_decals;
cvar_t	*r_clear;
cvar_t	*r_finish;
cvar_t	*r_sunshadows;
cvar_t	*r_sun_allowed;
cvar_t	*r_shadow_split_weight;
cvar_t	*r_lightstyles;
cvar_t	*r_polyoffset;
cvar_t	*r_grass;
cvar_t	*r_grass_alpha;
cvar_t	*r_grass_lighting;
cvar_t	*r_grass_shadows;
cvar_t	*r_grass_fade_start;
cvar_t	*r_grass_fade_dist;
cvar_t	*r_shadowmap_size;
cvar_t	*r_scissor_glass_debug;
cvar_t	*r_scissor_light_debug;
cvar_t	*r_showlightmaps;
cvar_t	*r_recursion_depth;
cvar_t	*r_pssm_show_split;
cvar_t	*v_glows;

// Spectator Mode
float	vecNewViewAngles[3];
int	iHasNewViewAngles;
float	vecNewViewOrigin[3];
int	iHasNewViewOrigin;
int	iIsSpectator;
float	m_flOfs;

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

extern "C" 
{
	int CL_IsThirdPerson( void );
	void CL_CameraOffset( float *ofs );

	void DLLEXPORT V_CalcRefdef( struct ref_params_s *pparams );
}

void		PM_ParticleLine( float *start, float *end, int pcolor, float life, float vert);
int		PM_GetVisEntInfo( int ent );
int		PM_GetPhysEntInfo( int ent );
void		InterpolateAngles(  float * start, float * end, float * output, float frac );
float		AngleBetweenVectors(  const float * v1,  const float * v2 );

extern float	vJumpOrigin[3];
extern float	vJumpAngles[3];
ref_params_t	*g_pViewParams = NULL; // pointer to ref_params for use outside V_CalcRefdef

void V_DropPunchAngle ( float frametime, float *ev_punchangle );
void VectorAngles( const float *forward, float *angles );

#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;

/*
The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.
*/

extern cvar_t	*cl_forwardspeed;
extern cvar_t	*chase_active;
extern cvar_t	*scr_ofsx, *scr_ofsy, *scr_ofsz;
extern cvar_t	*cl_vsmoothing;
extern cvar_t	*cl_rollspeed;
extern cvar_t	*cl_rollangle;

#define CAM_MODE_RELAX		1
#define CAM_MODE_FOCUS		2

#define clamp( val, min, max ) ( ((val) > (max)) ? (max) : ( ((val) < (min)) ? (min) : (val) ) ) // thx BUzer

#define HL2_BOB_CYCLE_MIN	0.1f
#define HL2_BOB_CYCLE_MAX	0.3f
#define HL2_BOB		0.1f
#define HL2_BOB_UP		0.5f

float    g_lateralBob;
float    g_verticalBob;

Vector	v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles;
float	v_frametime, v_lastDistance;	
float	v_cameraRelaxAngle	= 5.0f;
float	v_cameraFocusAngle	= 35.0f;
int	v_cameraMode = CAM_MODE_FOCUS;
qboolean	v_resetCamera = 1;

Vector	g_CrosshairAngle; // buz

Vector	ev_punchangle;

cvar_t	*scr_ofsx;
cvar_t	*scr_ofsy;
cvar_t	*scr_ofsz;

cvar_t	*v_centermove;
cvar_t	*v_centerspeed;

cvar_t	*cl_bobcycle;
cvar_t	*cl_bob;
cvar_t	*cl_bobup;
cvar_t	*cl_waterdist;
cvar_t	*cl_chasedist;

// These cvars are not registered (so users can't cheat), so set the ->value field directly
// Register these cvars in V_Init() if needed for easy tweaking
cvar_t	v_iyaw_cycle = {"v_iyaw_cycle", "2", 0, 2};
cvar_t	v_iroll_cycle = {"v_iroll_cycle", "0.5", 0, 0.5};
cvar_t	v_ipitch_cycle = {"v_ipitch_cycle", "1", 0, 1};
cvar_t	v_iyaw_level = {"v_iyaw_level", "0.3", 0, 0.3};
cvar_t	v_iroll_level = {"v_iroll_level", "0.1", 0, 0.1};
cvar_t	v_ipitch_level = {"v_ipitch_level", "0.3", 0, 0.3};

float	v_idlescale;  // used by TFC for concussion grenade effect

//=============================================================================
void V_NormalizeAngles( float *angles )
{
	// Normalize angles
	for( int i = 0; i < 3; i++ )
	{
		if ( angles[i] > 180.0 )
		{
			angles[i] -= 360.0;
		}
		else if ( angles[i] < -180.0 )
		{
			angles[i] += 360.0;
		}
	}
}

float Distance( const Vector &v1, const Vector &v2 )
{
	return (v2 - v1).Length();
}

/*
===================
V_InterpolateAngles

Interpolate Euler angles.
Frac is 0.0 to 1.0 ( i.e., should probably be clamped, but doesn't have to be )
===================
*/
void V_InterpolateAngles( float *start, float *end, float *output, float frac )
{
	int i;
	float ang1, ang2;
	float d;
	
	V_NormalizeAngles( start );
	V_NormalizeAngles( end );

	for ( i = 0 ; i < 3 ; i++ )
	{
		ang1 = start[i];
		ang2 = end[i];

		d = ang2 - ang1;
		if ( d > 180 )
		{
			d -= 360;
		}
		else if ( d < -180 )
		{	
			d += 360;
		}

		output[i] = ang1 + d * frac;
	}

	V_NormalizeAngles( output );
}

// Quakeworld bob code, this fixes jitters in the mutliplayer since the clock (pparams->time) isn't quite linear
float V_CalcBob ( struct ref_params_s *pparams )
{
	static	double	bobtime;
	static float	bob;
	float	cycle;
	static float	lasttime;
	Vector	vel;
	

	if ( pparams->onground == -1 ||
		 pparams->time == lasttime )
	{
		// just use old value
		return bob;	
	}

	lasttime = pparams->time;

	bobtime += pparams->frametime;
	cycle = bobtime - (int)( bobtime / cl_bobcycle->value ) * cl_bobcycle->value;
	cycle /= cl_bobcycle->value;
	
	if ( cycle < cl_bobup->value )
	{
		cycle = M_PI * cycle / cl_bobup->value;
	}
	else
	{
		cycle = M_PI + M_PI * ( cycle - cl_bobup->value )/( 1.0 - cl_bobup->value );
	}

	// bob is proportional to simulated velocity in the xy plane
	// (don't count Z, or jumping messes it up)
	VectorCopy( pparams->simvel, vel );
	vel[2] = 0;

	bob = sqrt( vel[0] * vel[0] + vel[1] * vel[1] ) * cl_bob->value;
	bob = bob * 0.3 + bob * 0.7 * sin(cycle);
	bob = min( bob, 4 );
	bob = max( bob, -7 );
	return bob;
	
}

float V_CalcNewBob ( struct ref_params_s *pparams )
{
	static    float bobtime;
	static    float lastbobtime;
	float    cycle;
    
	Vector    vel;
	VectorCopy( pparams->simvel, vel );
	vel[2] = 0;
    
	if ( pparams->onground == -1 || pparams->time == lastbobtime )
	{
		return 0.0f;
	}
    
	float speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
    
	speed = clamp( speed, -320, 320 );
    
	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );
    
	bobtime += ( pparams->time - lastbobtime ) * bob_offset;
	lastbobtime = pparams->time;
    
	//Calculate the vertical bob
	cycle = bobtime - (int)(bobtime/HL2_BOB_CYCLE_MAX)*HL2_BOB_CYCLE_MAX;
	cycle /= HL2_BOB_CYCLE_MAX;
    
	if ( cycle < HL2_BOB_UP )
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-HL2_BOB_UP)/(1.0 - HL2_BOB_UP);
	}
    
	g_verticalBob = speed*0.015f;
	g_verticalBob = g_verticalBob*0.3 + g_verticalBob*0.7*sin(cycle);
    
	g_verticalBob = clamp( g_verticalBob, -15.0f, 4.0f );
    
	//Calculate the lateral bob
	cycle = bobtime - (int)(bobtime/HL2_BOB_CYCLE_MAX*2)*HL2_BOB_CYCLE_MAX*2;
	cycle /= HL2_BOB_CYCLE_MAX*2;
    
	if ( cycle < HL2_BOB_UP )
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-HL2_BOB_UP)/(1.0 - HL2_BOB_UP);
	}
    
	g_lateralBob = speed*0.004f;
	g_lateralBob = g_lateralBob*0.3 + g_lateralBob*0.7*sin(cycle);
    
	g_lateralBob = clamp( g_lateralBob, -7.0f, 4.0f );
    
	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

/*
===============
V_CalcRoll
Used by view and sv_user
===============
*/
float V_CalcRoll (Vector angles, Vector velocity, float rollangle, float rollspeed )
{
	float   sign;
	float   side;
	float   value;
	Vector  forward, right, up;
    
	AngleVectors ( angles, forward, right, up );
    
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs( side );
    
	value = rollangle;
	if (side < rollspeed)
	{
		side = side * value / rollspeed;
	}
	else
	{
		side = value;
	}
	return side * sign;
}

typedef struct pitchdrift_s
{
	float	pitchvel;
	int	nodrift;
	float	driftmove;
	double	laststop;
} pitchdrift_t;

static pitchdrift_t pd;

void V_StartPitchDrift( void )
{
	if ( pd.laststop == gEngfuncs.GetClientTime() )
	{
		return;		// something else is keeping it from drifting
	}

	if ( pd.nodrift || !pd.pitchvel )
	{
		pd.pitchvel = v_centerspeed->value;
		pd.nodrift = 0;
		pd.driftmove = 0;
	}
}

void V_StopPitchDrift ( void )
{
	pd.laststop = gEngfuncs.GetClientTime();
	pd.nodrift = 1;
	pd.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
===============
*/
void V_DriftPitch ( struct ref_params_s *pparams )
{
	float		delta, move;

	if ( gEngfuncs.IsNoClipping() || !pparams->onground || pparams->demoplayback || pparams->spectator )
	{
		pd.driftmove = 0;
		pd.pitchvel = 0;
		return;
	}

	// don't count small mouse motion
	if (pd.nodrift)
	{
		if ( fabs( pparams->cmd->forwardmove ) < cl_forwardspeed->value )
			pd.driftmove = 0;
		else
			pd.driftmove += pparams->frametime;
	
		if ( pd.driftmove > v_centermove->value)
		{
			V_StartPitchDrift ();
		}
		return;
	}
	
	delta = pparams->idealpitch - pparams->cl_viewangles[PITCH];

	if (!delta)
	{
		pd.pitchvel = 0;
		return;
	}

	move = pparams->frametime * pd.pitchvel;
	pd.pitchvel += pparams->frametime * v_centerspeed->value;
	
	if (delta > 0)
	{
		if (move > delta)
		{
			pd.pitchvel = 0;
			move = delta;
		}
		pparams->cl_viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			pd.pitchvel = 0;
			move = -delta;
		}
		pparams->cl_viewangles[PITCH] -= move;
	}
}

/* 
============================================================================== 
						VIEW RENDERING 
============================================================================== 
*/ 
/*
==================
V_CalcGunAngle
==================
*/
void V_CalcGunAngle ( struct ref_params_s *pparams )
{	
	cl_entity_t *viewent;
	
	viewent = gEngfuncs.GetViewModel();
	if ( !viewent )
		return;

	viewent->angles[YAW]   =  pparams->viewangles[YAW]   + pparams->crosshairangle[YAW];
	viewent->angles[PITCH] = -pparams->viewangles[PITCH] + pparams->crosshairangle[PITCH] * 0.25;
	viewent->angles[ROLL]  -= v_idlescale * sin(pparams->time*v_iroll_cycle.value) * v_iroll_level.value;

	// don't apply all of the v_ipitch to prevent normally unseen parts of viewmodel from coming into view.
	viewent->angles[PITCH] -= v_idlescale * sin(pparams->time*v_ipitch_cycle.value) * (v_ipitch_level.value * 0.5);
	viewent->angles[YAW]   -= v_idlescale * sin(pparams->time*v_iyaw_cycle.value) * v_iyaw_level.value;

	// buz: add half of the punchangle to viewmodel
	viewent->angles[0] -= pparams->punchangle[0]/2;
	viewent->angles[1] += pparams->punchangle[1]/2;
	viewent->angles[2] += pparams->punchangle[2]/2;

	VectorCopy( viewent->angles, viewent->curstate.angles );
	VectorCopy( viewent->angles, viewent->latched.prevangles );	
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle( struct ref_params_s *pparams )
{
	pparams->viewangles[ROLL] += v_idlescale * sin(pparams->time*v_iroll_cycle.value) * v_iroll_level.value;
	pparams->viewangles[PITCH] += v_idlescale * sin(pparams->time*v_ipitch_cycle.value) * v_ipitch_level.value;
	pparams->viewangles[YAW] += v_idlescale * sin(pparams->time*v_iyaw_cycle.value) * v_iyaw_level.value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll ( struct ref_params_s *pparams )
{
	cl_entity_t *viewentity;
	
	viewentity = gEngfuncs.GetEntityByIndex( pparams->viewentity );
	if( !viewentity ) return;

	pparams->viewangles[ROLL] = V_CalcRoll (pparams->viewangles, pparams->simvel, cl_rollangle->value, cl_rollspeed->value ) * 4;

	if ( pparams->health <= 0 && ( pparams->viewheight[2] != 0 ) )
	{
		// only roll the view if the player is dead and the viewheight[2] is nonzero 
		// this is so deadcam in multiplayer will work.
		pparams->viewangles[ROLL] = 80;	// dead view angle
		return;
	}
}


/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef ( struct ref_params_s *pparams )
{
	cl_entity_t	*ent, *view;
	float		old;

	// ent is the player model ( visible when out of body )
	ent = gEngfuncs.GetLocalPlayer();
	
	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	VectorCopy ( pparams->simorg, pparams->vieworg );
	VectorCopy ( pparams->cl_viewangles, pparams->viewangles );

	view->model = NULL;

	// allways idle in intermission
	old = v_idlescale;
	v_idlescale = 1;

	V_AddIdle ( pparams );

	if ( gEngfuncs.IsSpectateOnly() )
	{
		// in HLTV we must go to 'intermission' position by ourself
		VectorCopy( gHUD.m_Spectator.m_cameraOrigin, pparams->vieworg );
		VectorCopy( gHUD.m_Spectator.m_cameraAngles, pparams->viewangles );
	}

	v_idlescale = old;

	v_cl_angles = pparams->cl_viewangles;
	v_origin = pparams->vieworg;
	v_angles = pparams->viewangles;
}

#define ORIGIN_BACKUP 64
#define ORIGIN_MASK ( ORIGIN_BACKUP - 1 )

typedef struct 
{
	Vector	Origins[ORIGIN_BACKUP];
	float	OriginTime[ORIGIN_BACKUP];

	Vector	Angles[ORIGIN_BACKUP];
	float	AngleTime[ORIGIN_BACKUP];

	int	CurrentOrigin;
	int	CurrentAngle;
} viewinterp_t;

float m_flWeaponLag = 0.5f;

void V_CalcViewModelLag( ref_params_t *pparams, Vector &origin, Vector &angles, Vector original_angles )
{
	static Vector m_vecLastFacing;
	Vector vOriginalOrigin = origin;
	Vector vOriginalAngles = angles;
    
	// Calculate our drift
	Vector    forward, right, up;
	AngleVectors( angles, forward, right, up );

	if( pparams->paused ) // not in paused
		return;
    
	if ( pparams->frametime != 0.0f )
	{
		Vector vDifference;
        
		vDifference = forward - m_vecLastFacing;
        
		float flSpeed = 4.0f;
        
		// If we start to lag too far behind, we'll increase the "catch up" speed.
		// Solves the problem with fast cl_yawspeed, m_yaw or joysticks rotating quickly.
		// The old code would slam lastfacing with origin causing the viewmodel to pop to a new position
		float flDiff = vDifference.Length();

		if (( flDiff > m_flWeaponLag ) && ( m_flWeaponLag > 0.0f ))
		{
			float flScale = flDiff / m_flWeaponLag;
			flSpeed *= flScale;
		}
        
		m_vecLastFacing = m_vecLastFacing + vDifference * ( flSpeed * pparams->frametime );
		// Make sure it doesn't grow out of control!!!
		m_vecLastFacing = m_vecLastFacing.Normalize();
		origin = origin + (vDifference * -1.0f) * 1.0f;        
	}
    
	AngleVectors( original_angles, forward, right, up );
    
	float pitch = original_angles[PITCH];
    
	if ( pitch > 180.0f )
	{
		pitch -= 360.0f;
	}
	else if ( pitch < -180.0f )
	{
		pitch += 360.0f;
	}
    
	if ( m_flWeaponLag <= 0.0f )
	{
		origin = vOriginalOrigin;
		angles = vOriginalAngles;
	}
	else
	{
		origin = origin + forward * ( -pitch * 0.00f );
		origin = origin + right * ( -pitch * 0.00f );
		origin = origin + up * ( -pitch * 0.00f );
	}    
}

//==========================
// V_CalcWaterLevel
//==========================
float V_CalcWaterLevel( struct ref_params_s *pparams )
{
	float waterOffset = 0.0f;
	
	if( pparams->waterlevel >= 2 )
	{
		int waterEntity = WATER_ENTITY( pparams->simorg );
		float waterDist = cl_waterdist->value;

		if( waterEntity >= 0 && waterEntity < pparams->max_entities )
		{
			cl_entity_t *pwater = GET_ENTITY( waterEntity );
			if( pwater && ( pwater->model != NULL ))
				waterDist += ( pwater->curstate.scale * 16.0f );
		}

		Vector point = pparams->vieworg;

		// eyes are above water, make sure we're above the waves
		if( pparams->waterlevel == 2 )	
		{
			point.z -= waterDist;

			for( int i = 0; i < waterDist; i++ )
			{
				int contents = POINT_CONTENTS( point );
				if( contents > CONTENTS_WATER )
					break;
				point.z += 1;
			}
			waterOffset = (point.z + waterDist) - pparams->vieworg[2];
		}
		else
		{
			// eyes are under water. Make sure we're far enough under
			point[2] += waterDist;

			for( int i = 0; i < waterDist; i++ )
			{
				int contents = POINT_CONTENTS( point );
				if( contents <= CONTENTS_WATER )
					break;

				point.z -= 1;
			}
			waterOffset = (point.z - waterDist) - pparams->vieworg[2];
		}
	}

	return waterOffset;
}

/*
==================
V_CalcRefdef

==================
*/
void V_CalcNormalRefdef ( struct ref_params_s *pparams )
{
	cl_entity_t		*ent, *view;
	Vector			angles;
	float			bob;
	static viewinterp_t		ViewInterp;	
	int			i;

	static float oldz = 0;
	static float lasttime;

	Vector camAngles, camForward, camRight, camUp;

	static struct model_s *savedviewmodel;

	VectorCopy(pparams->crosshairangle, g_CrosshairAngle); // save it for crosshair rendering

	V_DriftPitch ( pparams );

	if ( gEngfuncs.IsSpectateOnly() )
	{
		ent = gEngfuncs.GetEntityByIndex( g_iUser2 );
	}
	else
	{
		// ent is the player model ( visible when out of body )
		ent = gEngfuncs.GetLocalPlayer();
	}
	
	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	// save old position for light lerping
	view->prevstate.origin = view->origin;

	// transform the view offset by the model's matrix to get the offset from
	// model origin for the view
	bob = V_CalcBob ( pparams );

	// refresh position
	VectorCopy ( pparams->simorg, pparams->vieworg );
	//pparams->vieworg[2] += ( bob );
	VectorAdd( pparams->vieworg, pparams->viewheight, pparams->vieworg );

	VectorCopy ( pparams->cl_viewangles, pparams->viewangles );

	gEngfuncs.V_CalcShake();
	gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0 );

	// never let view origin sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	
	pparams->vieworg[0] += 1.0/32;
	pparams->vieworg[1] += 1.0/32;
	pparams->vieworg[2] += 1.0/32;

	float waterOffset = V_CalcWaterLevel( pparams );
	pparams->vieworg[2] += waterOffset;

	V_CalcViewRoll ( pparams );

	if( gHUD.m_flBlurAmount <= 0.0f )
	{
		if( g_iGunMode == 3 )
			v_idlescale = 0.15f;
		else v_idlescale = 0.0f;
	}

	V_AddIdle ( pparams );

	// offsets
	VectorCopy( pparams->cl_viewangles, angles );

	AngleVectors ( angles, pparams->forward, pparams->right, pparams->up );   	

	// buz: spec tank view
	if (gHUD.m_SpecTank_on)
	{
		VectorCopy(gHUD.m_SpecTank_point, pparams->vieworg);
		VectorMA(pparams->vieworg, -gHUD.m_SpecTank_distFwd, pparams->forward, pparams->vieworg);
		VectorMA(pparams->vieworg, gHUD.m_SpecTank_distUp, pparams->up, pparams->vieworg);
	}


	// don't allow cheats in multiplayer
	if ( pparams->maxclients <= 1 )
	{
		pparams->vieworg += scr_ofsx->value*pparams->forward + scr_ofsy->value*pparams->right + scr_ofsz->value*pparams->up;
	}

	Vector pl_vel;
	VectorCopy(pparams->simvel, pl_vel);

	float m_flSpeed = pl_vel.Length2D();

	if (!(gHUD.m_iKeyBits & IN_DUCK) && !(gHUD.m_iKeyBits & IN_JUMP)) 
	{		
		if ((gHUD.m_iKeyBits & IN_RUN) && (gHUD.m_iKeyBits & IN_FORWARD) && (m_flSpeed > 210.0))
		{
			if ( m_flOfs <= 1 )
				m_flOfs += 0.05;
    
			if(m_flOfs >= 1)
				m_flOfs = 1;
		}
		else
		{
			if ( m_flOfs >=0 )
				m_flOfs -=0.05;
    
			if(m_flOfs <= 0)
				m_flOfs = 0;
		}
	}

	pparams->vieworg[2] += m_flOfs;
	
	// Treating cam_ofs[2] as the distance
	if( CL_IsThirdPerson() )
	{
		Vector ofs;

		ofs[0] = ofs[1] = ofs[2] = 0.0;

		CL_CameraOffset( (float *)&ofs );

		VectorCopy( ofs, camAngles );
		camAngles[ ROLL ]	= 0;

		AngleVectors( camAngles, camForward, camRight, camUp );

		for ( i = 0; i < 3; i++ )
		{
			pparams->vieworg[ i ] += -ofs[2] * camForward[ i ];
		}
	}
	
	// Give gun our viewangles
	VectorCopy ( pparams->cl_viewangles, view->angles );
	Vector lastAngles = view->angles;// save oldangles
	
	// set up gun position
	V_CalcGunAngle ( pparams );  

	// Use predicted origin as view origin.
	VectorCopy ( pparams->vieworg, view->origin );      
//	VectorAdd( view->origin, pparams->viewheight, view->origin );

	// Let the viewmodel shake at about 10% of the amplitude
	gEngfuncs.V_ApplyShake( view->origin, view->angles, 0.9 );

	Vector    forward, right;

	AngleVectors( view->angles, forward, right, NULL );

	V_CalcNewBob ( pparams );
    
	// Apply bob, but scaled down to 40%
	VectorMA( view->origin, g_verticalBob * 0.1f, forward, view->origin );
 
	// Z bob a bit more
	view->origin[2] += g_verticalBob * 0.1f;

	// bob the angles
	angles[ ROLL ]+= g_verticalBob * 0.3f;
	angles[ PITCH ]-= g_verticalBob * 0.8f;

	angles[ YAW ]-= g_lateralBob  * 0.5f;

	VectorMA( view->origin, g_lateralBob * 0.8f, right, view->origin );

	for ( i = 0; i < 2; i++ )
	{
		pparams->viewangles[ROLL] += bob * 0.3;
		pparams->viewangles[YAW] += bob * 0.2;
	}

	pparams->vieworg[ROLL] -= sqrt(bob * bob) * 2 * pparams->up[ ROLL ];
	view->origin[ROLL] -= sqrt(bob * bob) * 2 * pparams->up[ ROLL ];

	// pushing the view origin down off of the same X/Z plane as the ent's origin will give the
	// gun a very nice 'shifting' effect when the player looks up/down. If there is a problem
	// with view model distortion, this may be a cause. (SJB). 
	// g-cont. disabled to avoid IronSight issues
//	view->origin[2] -= 1;

	V_CalcViewModelLag( pparams, view->origin, view->angles, lastAngles );

	// fudge position around to keep amount of weapon visible
	// roughly equal with different FOV
	if (pparams->viewsize == 110)
	{
		view->origin[2] += 1;
	}
	else if (pparams->viewsize == 100)
	{
		view->origin[2] += 2;
	}
	else if (pparams->viewsize == 90)
	{
		view->origin[2] += 1;
	}
	else if (pparams->viewsize == 80)
	{
		view->origin[2] += 0.5;
	}

	// Add in the punchangle, if any
	VectorAdd ( pparams->viewangles, pparams->punchangle, pparams->viewangles );

	// Include client side punch, too
	VectorAdd ( pparams->viewangles, (float *)&ev_punchangle, pparams->viewangles);

	V_DropPunchAngle ( pparams->frametime, (float *)&ev_punchangle );

	// smooth out stair step ups
#if 1
	if ( !pparams->smoothing && pparams->onground && pparams->simorg[2] - oldz > 0)
	{
		float steptime;
		
		steptime = pparams->time - lasttime;
		if (steptime < 0)
			steptime = 0;

		oldz += steptime * 120;
		if (oldz > pparams->simorg[2])
			oldz = pparams->simorg[2];
		if (pparams->simorg[2] - oldz > 18)
			oldz = pparams->simorg[2]- 18;
		pparams->vieworg[2] += oldz - pparams->simorg[2];
		view->origin[2] += oldz - pparams->simorg[2];
	}
	else
	{
		oldz = pparams->simorg[2];
	}
#endif
	SetupFlashlight( pparams ); // buz

	{
		static float lastorg[3];
		Vector delta;

		VectorSubtract( pparams->simorg, lastorg, delta );

		if ( Length( delta ) != 0.0 )
		{
			VectorCopy( pparams->simorg, ViewInterp.Origins[ ViewInterp.CurrentOrigin & ORIGIN_MASK ] );
			ViewInterp.OriginTime[ ViewInterp.CurrentOrigin & ORIGIN_MASK ] = pparams->time;
			ViewInterp.CurrentOrigin++;

			VectorCopy( pparams->simorg, lastorg );
		}
	}

	// Smooth out whole view in multiplayer when on trains, lifts
	if ( cl_vsmoothing && cl_vsmoothing->value && ( pparams->smoothing && ( pparams->maxclients > 1 ) ) )
	{
		int foundidx;
		int i;
		float t;

		if ( cl_vsmoothing->value < 0.0 )
		{
			gEngfuncs.Cvar_SetValue( "cl_vsmoothing", 0.0 );
		}

		t = pparams->time - cl_vsmoothing->value;

		for ( i = 1; i < ORIGIN_MASK; i++ )
		{
			foundidx = ViewInterp.CurrentOrigin - 1 - i;
			if ( ViewInterp.OriginTime[ foundidx & ORIGIN_MASK ] <= t )
				break;
		}

		if ( i < ORIGIN_MASK &&  ViewInterp.OriginTime[ foundidx & ORIGIN_MASK ] != 0.0 )
		{
			// Interpolate
			Vector delta;
			double frac;
			double dt;
			Vector neworg;

			dt = ViewInterp.OriginTime[ (foundidx + 1) & ORIGIN_MASK ] - ViewInterp.OriginTime[ foundidx & ORIGIN_MASK ];
			if ( dt > 0.0 )
			{
				frac = ( t - ViewInterp.OriginTime[ foundidx & ORIGIN_MASK] ) / dt;
				frac = min( 1.0, frac );
				delta = ViewInterp.Origins[(foundidx + 1) & ORIGIN_MASK] - ViewInterp.Origins[foundidx & ORIGIN_MASK];
				VectorMA( ViewInterp.Origins[ foundidx & ORIGIN_MASK ], frac, delta, neworg );

				// Dont interpolate large changes
				if ( Length( delta ) < 64 )
				{
					VectorSubtract( neworg, pparams->simorg, delta );

					VectorAdd( pparams->simorg, delta, pparams->simorg );
					VectorAdd( pparams->vieworg, delta, pparams->vieworg );
					VectorAdd( view->origin, delta, view->origin );

				}
			}
		}
	}

	// Store off v_angles before munging for third person
	v_angles = pparams->viewangles;
	v_lastAngles = pparams->viewangles;
//	v_cl_angles = pparams->cl_viewangles;	// keep old user mouse angles !

	if ( CL_IsThirdPerson() )
	{
		VectorCopy( camAngles, pparams->viewangles);
		float pitch = camAngles[ 0 ];

		// Normalize angles
		if ( pitch > 180 ) 
			pitch -= 360.0;
		else if ( pitch < -180 )
			pitch += 360;

		// Player pitch is inverted
		pitch /= -3.0;

		// Slam local player's pitch value
		ent->angles[ 0 ] = pitch;
		ent->curstate.angles[ 0 ] = pitch;
		ent->prevstate.angles[ 0 ] = pitch;
		ent->latched.prevangles[ 0 ] = pitch;
	}

	// override all previous settings if the viewent isn't the client
	if ( pparams->viewentity > pparams->maxclients )
	{
		cl_entity_t *viewentity;
		viewentity = gEngfuncs.GetEntityByIndex( pparams->viewentity );
		if ( viewentity )
		{
			VectorCopy( viewentity->origin, pparams->vieworg );
			VectorCopy( viewentity->angles, pparams->viewangles );

			// Store off overridden viewangles
			v_angles = pparams->viewangles;
		}
	}

	lasttime = pparams->time;

	v_origin = pparams->vieworg;
}

void V_SmoothInterpolateAngles( float * startAngle, float * endAngle, float * finalAngle, float degreesPerSec )
{
	float absd,frac,d,threshhold;
	
	V_NormalizeAngles( startAngle );
	V_NormalizeAngles( endAngle );

	for ( int i = 0 ; i < 3 ; i++ )
	{
		d = endAngle[i] - startAngle[i];

		if ( d > 180.0f )
		{
			d -= 360.0f;
		}
		else if ( d < -180.0f )
		{	
			d += 360.0f;
		}

		absd = fabs(d);

		if ( absd > 0.01f )
		{
			frac = degreesPerSec * v_frametime;

			threshhold= degreesPerSec / 4;

			if ( absd < threshhold )
			{
				float h = absd / threshhold;
				h *= h;
				frac*= h;  // slow down last degrees
			}

			if ( frac >  absd )
			{
				finalAngle[i] = endAngle[i];
			}
			else
			{
				if ( d>0)
					finalAngle[i] = startAngle[i] + frac;
				else
					finalAngle[i] = startAngle[i] - frac;
			}
		}
		else
		{
			finalAngle[i] = endAngle[i];
		}

	}

	V_NormalizeAngles( finalAngle );
}

// Get the origin of the Observer based around the target's position and angles
void V_GetChaseOrigin( float * angles, float * origin, float distance, float * returnvec )
{
	Vector	vecEnd;
	Vector	forward;
	Vector	vecStart;
	pmtrace_t * trace;
	int maxLoops = 8;

	int ignoreent = -1;	// first, ignore no entity
	
	cl_entity_t	 *	ent = NULL;
	
	// Trace back from the target using the player's view angles
	AngleVectors(angles, forward, NULL, NULL);
	
	VectorScale(forward,-1,forward);

	VectorCopy( origin, vecStart );

	VectorMA(vecStart, distance , forward, vecEnd);

	while ( maxLoops > 0)
	{
		trace = gEngfuncs.PM_TraceLine( vecStart, vecEnd, PM_TRACELINE_PHYSENTSONLY, 2, ignoreent );

		// WARNING! trace->ent is is the number in physent list not the normal entity number

		if ( trace->ent <= 0)
			break;	// we hit the world or nothing, stop trace

		ent = gEngfuncs.GetEntityByIndex( PM_GetPhysEntInfo( trace->ent ) );

		if ( ent == NULL )
			break;

		// hit non-player solid BSP , stop here
		if ( ent->curstate.solid == SOLID_BSP && !ent->player ) 
			break;

		// if close enought to end pos, stop, otherwise continue trace
		if( Distance(trace->endpos, vecEnd ) < 1.0f )
		{
			break;
		}
		else
		{
			ignoreent = trace->ent;	// ignore last hit entity
			VectorCopy( trace->endpos, vecStart);
		}

		maxLoops--;
	}  

	VectorMA( trace->endpos, 4, trace->plane.normal, returnvec );

	v_lastDistance = Distance(trace->endpos, origin);	// real distance without offset
}

void V_GetSingleTargetCam(cl_entity_t * ent1, float * angle, float * origin)
{
	float newAngle[3]; float newOrigin[3]; 
	
	int flags 	   = gHUD.m_Spectator.m_iObserverFlags;

	// see is target is a dead player
	qboolean deadPlayer = ent1->player && (ent1->curstate.solid == SOLID_NOT);
	
	float dfactor   = ( flags & DRC_FLAG_DRAMATIC )? -1.0f : 1.0f;

	float distance = 112.0f + ( 16.0f * dfactor ); // get close if dramatic;
	
	// go away in final scenes or if player just died
	if ( flags & DRC_FLAG_FINAL )
		distance*=2.0f;	
	else if ( deadPlayer )
		distance*=1.5f;	

	// let v_lastDistance float smoothly away
	v_lastDistance+= v_frametime * 32.0f;	// move unit per seconds back

	if ( distance > v_lastDistance )
		distance = v_lastDistance;
	
	VectorCopy(ent1->origin, newOrigin);

	if ( ent1->player )
	{
		if ( deadPlayer )  
			newOrigin[2]+= 2;	//laying on ground
		else
			newOrigin[2]+= 17; // head level of living player
			
	}
	else
		newOrigin[2]+= 8;	// object, tricky, must be above bomb in CS

	// we have no second target, choose view direction based on
	// show front of primary target
	VectorCopy(ent1->angles, newAngle);

	// show dead players from front, normal players back
	if ( flags & DRC_FLAG_FACEPLAYER )
		newAngle[1]+= 180.0f;


	newAngle[0]+= 12.5f * dfactor; // lower angle if dramatic

	// if final scene (bomb), show from real high pos
	if ( flags & DRC_FLAG_FINAL )
		newAngle[0] = 22.5f; 

	// choose side of object/player			
	if ( flags & DRC_FLAG_SIDE )
		newAngle[1]+=22.5f;
	else
		newAngle[1]-=22.5f;

	V_SmoothInterpolateAngles( v_lastAngles, newAngle, angle, 120.0f );

	// HACK, if player is dead don't clip against his dead body, can't check this
	V_GetChaseOrigin( angle, newOrigin, distance, origin );
}

float MaxAngleBetweenAngles(  float * a1, float * a2 )
{
	float d, maxd = 0.0f;

	V_NormalizeAngles( a1 );
	V_NormalizeAngles( a2 );

	for ( int i = 0 ; i < 3 ; i++ )
	{
		d = a2[i] - a1[i];
		if ( d > 180 )
		{
			d -= 360;
		}
		else if ( d < -180 )
		{	
			d += 360;
		}

		d = fabs(d);

		if ( d > maxd )
			maxd=d;
	}

	return maxd;
}

void V_GetDoubleTargetsCam(cl_entity_t	 * ent1, cl_entity_t * ent2,float * angle, float * origin)
{
	float newAngle[3]; float newOrigin[3]; float tempVec[3];

	int flags 	   = gHUD.m_Spectator.m_iObserverFlags;

	float dfactor   = ( flags & DRC_FLAG_DRAMATIC )? -1.0f : 1.0f;

	float distance = 112.0f + ( 16.0f * dfactor ); // get close if dramatic;
	
	// go away in final scenes or if player just died
	if ( flags & DRC_FLAG_FINAL )
		distance*=2.0f;	
	
	// let v_lastDistance float smoothly away
	v_lastDistance+= v_frametime * 32.0f;	// move unit per seconds back

	if ( distance > v_lastDistance )
		distance = v_lastDistance;

	VectorCopy(ent1->origin, newOrigin);

	if ( ent1->player )
		newOrigin[2]+= 17; // head level of living player
	else
		newOrigin[2]+= 8;	// object, tricky, must be above bomb in CS

	// get new angle towards second target
	VectorSubtract( ent2->origin, ent1->origin, newAngle );

	VectorAngles( newAngle, newAngle );
	newAngle[0] = -newAngle[0];

	// set angle diffrent in Dramtaic scenes
	newAngle[0]+= 15.5f * dfactor; // lower angle if dramatic
			
	if ( flags & DRC_FLAG_SIDE )
		newAngle[1]+=22.5f;
	else
		newAngle[1]-=22.5f;

	float d = MaxAngleBetweenAngles( v_lastAngles, newAngle );

	if ( ( d < v_cameraFocusAngle) && ( v_cameraMode == CAM_MODE_RELAX ) )
	{
		// difference is to small and we are in relax camera mode, keep viewangles
		VectorCopy(v_lastAngles, newAngle );
	}
	else if ( (d < v_cameraRelaxAngle) && (v_cameraMode == CAM_MODE_FOCUS) )
	{
		// we catched up with our target, relax again
		v_cameraMode = CAM_MODE_RELAX;
	}
	else
	{
		// target move too far away, focus camera again
		v_cameraMode = CAM_MODE_FOCUS;
	}

	// and smooth view, if not a scene cut
	if ( v_resetCamera || (v_cameraMode == CAM_MODE_RELAX) )
	{
		VectorCopy( newAngle, angle );
	}
	else
	{
		V_SmoothInterpolateAngles( v_lastAngles, newAngle, angle, 180.0f );
	}

	V_GetChaseOrigin( newAngle, newOrigin, distance, origin );

	// move position up, if very close at target
	if ( v_lastDistance < 64.0f )
		origin[2]+= 16.0f*( 1.0f - (v_lastDistance / 64.0f ) );

	// calculate angle to second target
	VectorSubtract( ent2->origin, origin, tempVec );
	VectorAngles( tempVec, tempVec );
	tempVec[0] = -tempVec[0];

	/* take middle between two viewangles
	InterpolateAngles( newAngle, tempVec, newAngle, 0.5f); */
}

void V_GetDirectedChasePosition(cl_entity_t* ent1, cl_entity_t * ent2,float * angle, float * origin)
{
	if ( v_resetCamera )
	{
		v_lastDistance = 4096.0f;
		// v_cameraMode = CAM_MODE_FOCUS;
	}

	if ( ( ent2 == (cl_entity_t*)0xFFFFFFFF ) || ( ent1->player && (ent1->curstate.solid == SOLID_NOT) ) )
	{
		// we have no second target or player just died
		V_GetSingleTargetCam(ent1, angle, origin);
	}
	else if ( ent2 )
	{
		// keep both target in view
		V_GetDoubleTargetsCam( ent1, ent2, angle, origin );
	}
	else
	{
		// second target disappeard somehow (dead)

		// keep last good viewangle
		float newOrigin[3];

		int flags 	   = gHUD.m_Spectator.m_iObserverFlags;

		float dfactor   = ( flags & DRC_FLAG_DRAMATIC )? -1.0f : 1.0f;

		float distance = 112.0f + ( 16.0f * dfactor ); // get close if dramatic;
	
		// go away in final scenes or if player just died
		if ( flags & DRC_FLAG_FINAL )
			distance*=2.0f;	
	
		// let v_lastDistance float smoothly away
		v_lastDistance+= v_frametime * 32.0f;	// move unit per seconds back

		if ( distance > v_lastDistance )
			distance = v_lastDistance;
		
		VectorCopy(ent1->origin, newOrigin);

		if ( ent1->player )
			newOrigin[2]+= 17; // head level of living player
		else
			newOrigin[2]+= 8;	// object, tricky, must be above bomb in CS

		V_GetChaseOrigin( angle, newOrigin, distance, origin );
	}

	VectorCopy(angle, v_lastAngles);
}

void V_GetChasePos(int target, float * cl_angles, float * origin, float * angles)
{
	cl_entity_t	 *	ent = NULL;
	
	if ( target ) 
	{
		ent = gEngfuncs.GetEntityByIndex( target );
	};
	
	if (!ent)
	{
		// just copy a save in-map position
		VectorCopy ( vJumpAngles, angles );
		VectorCopy ( vJumpOrigin, origin );
		return;
	}
	
	
	
	if ( gHUD.m_Spectator.m_autoDirector->value )
	{
		if ( g_iUser3 )
			V_GetDirectedChasePosition( ent, gEngfuncs.GetEntityByIndex( g_iUser3 ),
				angles, origin );
		else
			V_GetDirectedChasePosition( ent, ( cl_entity_t*)0xFFFFFFFF,
				angles, origin );
	}
	else
	{
		if ( cl_angles == NULL )	// no mouse angles given, use entity angles ( locked mode )
		{
			VectorCopy ( ent->angles, angles);
			angles[0]*=-1;
		}
		else
			VectorCopy ( cl_angles, angles);


		VectorCopy ( ent->origin, origin);
		
		origin[2]+= 28; // DEFAULT_VIEWHEIGHT - some offset

		V_GetChaseOrigin( angles, origin, cl_chasedist->value, origin );
	}

	v_resetCamera = false;	
}

void V_ResetChaseCam()
{
	v_resetCamera = true;
}

void V_GetInEyePos(int target, float * origin, float * angles )
{
	if ( !target)
	{
		// just copy a save in-map position
		VectorCopy ( vJumpAngles, angles );
		VectorCopy ( vJumpOrigin, origin );
		return;
	};


	cl_entity_t	 * ent = gEngfuncs.GetEntityByIndex( target );

	if ( !ent )
		return;

	VectorCopy ( ent->origin, origin );
	VectorCopy ( ent->angles, angles );

	angles[PITCH]*=-3.0f;	// see CL_ProcessEntityUpdate()

	if ( ent->curstate.solid == SOLID_NOT )
	{
		angles[ROLL] = 80;	// dead view angle
		origin[2]+= -8 ; // PM_DEAD_VIEWHEIGHT
	}
	else if (ent->curstate.usehull == 1 )
		origin[2]+= 12; // VEC_DUCK_VIEW;
	else
		// exacty eye position can't be caluculated since it depends on
		// client values like cl_bobcycle, this offset matches the default values
		origin[2]+= 28; // DEFAULT_VIEWHEIGHT
}

void V_GetMapFreePosition( float * cl_angles, float * origin, float * angles )
{
	Vector forward;
	Vector zScaledTarget;

	VectorCopy(cl_angles, angles);

	// modify angles since we don't wanna see map's bottom
	angles[0] = 51.25f + 38.75f*(angles[0]/90.0f);

	zScaledTarget[0] = gHUD.m_Spectator.m_mapOrigin[0];
	zScaledTarget[1] = gHUD.m_Spectator.m_mapOrigin[1];
	zScaledTarget[2] = gHUD.m_Spectator.m_mapOrigin[2] * (( 90.0f - angles[0] ) / 90.0f );
	

	AngleVectors(angles, forward, NULL, NULL);

	VectorNormalize(forward);

	VectorMA(zScaledTarget, -( 4096.0f / gHUD.m_Spectator.m_mapZoom ), forward , origin);
}

void V_GetMapChasePosition(int target, float * cl_angles, float * origin, float * angles)
{
	Vector forward;

	if ( target )
	{
		cl_entity_t	 *	ent = gEngfuncs.GetEntityByIndex( target );

		if ( gHUD.m_Spectator.m_autoDirector->value )
		{
			// this is done to get the angles made by director mode
			V_GetChasePos(target, cl_angles, origin, angles);
			VectorCopy(ent->origin, origin);
			
			// keep fix chase angle horizontal
			angles[0] = 45.0f;
		}
		else
		{
			VectorCopy(cl_angles, angles);
			VectorCopy(ent->origin, origin);

			// modify angles since we don't wanna see map's bottom
			angles[0] = 51.25f + 38.75f*(angles[0]/90.0f);
		}
	}
	else
	{
		// keep out roaming position, but modify angles
		VectorCopy(cl_angles, angles);
		angles[0] = 51.25f + 38.75f*(angles[0]/90.0f);
	}

	origin[2] *= (( 90.0f - angles[0] ) / 90.0f );
	angles[2] = 0.0f;	// don't roll angle (if chased player is dead)

	AngleVectors(angles, forward, NULL, NULL);

	VectorNormalize(forward);

	VectorMA(origin, -1536, forward, origin); 
}

int V_FindViewModelByWeaponModel(int weaponindex)
{

	static char * modelmap[][2] =	{
		{ "models/p_crowbar.mdl",		"models/v_crowbar.mdl"	},
		{ "models/p_9mmhandgun.mdl",		"models/v_9mmhandgun.mdl"	},
		{ "models/p_grenade.mdl",		"models/v_grenade.mdl"	},
		{ "models/p_9mmAR.mdl",		"models/v_9mmAR.mdl"	},
		{ "models/p_rpg.mdl",		"models/v_rpg.mdl"		},
		{ "models/p_shotgun.mdl",		"models/v_shotgun.mdl"	},
		{ NULL, NULL } };

	struct model_s * weaponModel = IEngineStudio.GetModelByIndex( weaponindex );

	if ( weaponModel )
	{
		int len = strlen( weaponModel->name );
		int i = 0;

		while ( modelmap[i] != NULL )
		{
			if ( !strnicmp( weaponModel->name, modelmap[i][0], len ) )
			{
				return gEngfuncs.pEventAPI->EV_FindModelIndex( modelmap[i][1] );
			}
			i++;
		}
	}

	return 0;
}


/*
==================
V_CalcSpectatorRefdef

==================
*/
void V_CalcSpectatorRefdef ( struct ref_params_s * pparams )
{
	static Vector			velocity ( 0.0f, 0.0f, 0.0f);

	static int lastWeaponModelIndex = 0;
	static int lastViewModelIndex = 0;
		
	cl_entity_t	 * ent = gEngfuncs.GetEntityByIndex( g_iUser2 );
	
	pparams->onlyClientDraw = false;

	// refresh position
	VectorCopy ( pparams->simorg, v_sim_org );

	// get old values
	VectorCopy ( pparams->cl_viewangles, v_cl_angles );
	VectorCopy ( pparams->viewangles, v_angles );
	VectorCopy ( pparams->vieworg, v_origin );

	if (  ( g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_pip->value == INSET_IN_EYE ) && ent )
	{
		// calculate player velocity
		float timeDiff = ent->curstate.msg_time - ent->prevstate.msg_time;

		if ( timeDiff > 0 )
		{
			Vector distance;
			VectorSubtract(ent->prevstate.origin, ent->curstate.origin, distance);
			VectorScale(distance, 1/timeDiff, distance );

			velocity[0] = velocity[0]*0.9f + distance[0]*0.1f;
			velocity[1] = velocity[1]*0.9f + distance[1]*0.1f;
			velocity[2] = velocity[2]*0.9f + distance[2]*0.1f;
			
			VectorCopy(velocity, pparams->simvel);
		}

		// predict missing client data and set weapon model ( in HLTV mode or inset in eye mode )
		if ( gEngfuncs.IsSpectateOnly() )
		{
			V_GetInEyePos( g_iUser2, pparams->simorg, pparams->cl_viewangles );

			pparams->health = 1;

			cl_entity_t	 * gunModel = gEngfuncs.GetViewModel();

			if ( lastWeaponModelIndex != ent->curstate.weaponmodel )
			{
				// weapon model changed

				lastWeaponModelIndex = ent->curstate.weaponmodel;
				lastViewModelIndex = V_FindViewModelByWeaponModel( lastWeaponModelIndex );
				if ( lastViewModelIndex )
				{
					gEngfuncs.pfnWeaponAnim(0,0);	// reset weapon animation
				}
				else
				{
					// model not found
					gunModel->model = NULL;	// disable weapon model
					lastWeaponModelIndex = lastViewModelIndex = 0;
				}
			}

			if ( lastViewModelIndex )
			{
				gunModel->model = IEngineStudio.GetModelByIndex( lastViewModelIndex );
				gunModel->curstate.modelindex = lastViewModelIndex;
				gunModel->curstate.frame = 0;
				gunModel->curstate.colormap = 0; 
				gunModel->index = g_iUser2;
			}
			else
			{
				gunModel->model = NULL;	// disable weaopn model
			}
		}
		else
		{
			// only get viewangles from entity
			VectorCopy ( ent->angles, pparams->cl_viewangles );
			pparams->cl_viewangles[PITCH]*=-3.0f;	// see CL_ProcessEntityUpdate()
		}
	}

	v_frametime = pparams->frametime;

	if ( pparams->nextView == 0 )
	{
		// first renderer cycle, full screen

		switch( g_iUser1 )
		{
		case OBS_CHASE_LOCKED:
			V_GetChasePos( g_iUser2, NULL, v_origin, v_angles );
			break;
		case OBS_CHASE_FREE:
			V_GetChasePos( g_iUser2, v_cl_angles, v_origin, v_angles );
			break;
		case OBS_ROAMING:
			VectorCopy (v_cl_angles, v_angles);
			VectorCopy (v_sim_org, v_origin);
			break;
		case OBS_IN_EYE:
			V_CalcNormalRefdef ( pparams );
			break;
		case OBS_MAP_FREE:
			pparams->onlyClientDraw = true;
			V_GetMapFreePosition( v_cl_angles, v_origin, v_angles );
			break;
		case OBS_MAP_CHASE:
			pparams->onlyClientDraw = true;
			V_GetMapChasePosition( g_iUser2, v_cl_angles, v_origin, v_angles );
			break;
		}

		if ( gHUD.m_Spectator.m_pip->value )
			pparams->nextView = 1;	// force a second renderer view

		gHUD.m_Spectator.m_iDrawCycle = 0;

	}
	else
	{
		// second renderer cycle, inset window

		// set inset parameters
		pparams->viewport[0] = XRES(gHUD.m_Spectator.m_OverviewData.insetWindowX);	// change viewport to inset window
		pparams->viewport[1] = YRES(gHUD.m_Spectator.m_OverviewData.insetWindowY);
		pparams->viewport[2] = XRES(gHUD.m_Spectator.m_OverviewData.insetWindowWidth);
		pparams->viewport[3] = YRES(gHUD.m_Spectator.m_OverviewData.insetWindowHeight);
		pparams->nextView	 = 0;	// on further view

		// override some settings in certain modes
		switch( (int)gHUD.m_Spectator.m_pip->value )
		{
		case INSET_CHASE_FREE:
			V_GetChasePos( g_iUser2, v_cl_angles, v_origin, v_angles );
			break;	
		case INSET_IN_EYE:
			V_CalcNormalRefdef ( pparams );
			break;
		case INSET_MAP_FREE:
			pparams->onlyClientDraw = true;
			V_GetMapFreePosition( v_cl_angles, v_origin, v_angles );
			break;
		case INSET_MAP_CHASE:
			pparams->onlyClientDraw = true;

			if( g_iUser1 == OBS_ROAMING )
				V_GetMapChasePosition( 0, v_cl_angles, v_origin, v_angles );
			else
				V_GetMapChasePosition( g_iUser2, v_cl_angles, v_origin, v_angles );
			break;
		}

		gHUD.m_Spectator.m_iDrawCycle = 1;
	}

	// write back new values into pparams
	VectorCopy ( v_cl_angles, pparams->cl_viewangles );
	VectorCopy ( v_angles, pparams->viewangles );
	VectorCopy ( v_origin, pparams->vieworg );

}

void DLLEXPORT V_CalcRefdef( struct ref_params_s *pparams )
{
	g_pViewParams = pparams;

	// intermission / finale rendering
	if ( pparams->intermission )
	{	
		V_CalcIntermissionRefdef ( pparams );	
	}
	else if ( pparams->spectator || g_iUser1 )	// g_iUser true if in spectator mode
	{
		V_CalcSpectatorRefdef ( pparams );	
	}
	else if ( !pparams->paused )
	{
		V_CalcNormalRefdef ( pparams );
	}
}

/*
=============
V_DropPunchAngle

=============
*/
void V_DropPunchAngle ( float frametime, float *ev_punchangle )
{
	float	len;
	
	len = VectorNormalize ( ev_punchangle );
	len -= (10.0 + len * 0.5) * frametime;
	len = max( len, 0.0 );
	VectorScale ( ev_punchangle, len, ev_punchangle );
}

/*
=============
V_PunchAxis

Client side punch effect
=============
*/
void V_PunchAxis( int axis, float punch )
{
//	ev_punchangle[ axis ] = punch; // buz - disable
}

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	gEngfuncs.pfnAddCommand ("centerview", V_StartPitchDrift );
	gEngfuncs.pfnAddCommand ("buildcubemaps", CL_BuildCubemaps_f );
	ADD_COMMAND( "gpu_mem_usage", GL_GpuMemUsage_f );

	scr_ofsx			= CVAR_REGISTER( "scr_ofsx","0", 0 );
	scr_ofsy			= CVAR_REGISTER( "scr_ofsy","0", 0 );
	scr_ofsz			= CVAR_REGISTER( "scr_ofsz","0", 0 );

	v_centermove		= CVAR_REGISTER( "v_centermove", "0.15", 0 );
	v_centerspeed		= CVAR_REGISTER( "v_centerspeed","500", 0 );

	cl_bobcycle		= CVAR_REGISTER( "cl_bobcycle","0.8", 0 );// best default for my experimental gun wag (sjb)
	cl_bob			= CVAR_REGISTER( "cl_bob","0.01", 0 );// best default for my experimental gun wag (sjb)
	cl_bobup			= CVAR_REGISTER( "cl_bobup","0.5", 0 );
	cl_waterdist		= CVAR_REGISTER( "cl_waterdist","4", FCVAR_ARCHIVE );
	cl_chasedist		= CVAR_REGISTER( "cl_chasedist","112", FCVAR_ARCHIVE );
	r_shadows			= CVAR_REGISTER( "r_shadows", "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE ); 
	r_shadow_split_weight	= CVAR_REGISTER( "r_pssm_split_weight", "0.9", FCVAR_ARCHIVE );
	r_pssm_show_split		= CVAR_REGISTER( "r_pssm_show_split", "0", 0 ); // debug feature
	r_scissor_glass_debug	= CVAR_REGISTER( "r_scissor_glass_debug", "0", FCVAR_ARCHIVE );
	r_scissor_light_debug	= CVAR_REGISTER( "r_scissor_light_debug", "0", FCVAR_ARCHIVE );
	r_recursion_depth		= CVAR_REGISTER( "gl_recursion_depth", "1", FCVAR_ARCHIVE );
	r_showlightmaps		= CVAR_REGISTER( "r_showlightmaps", "0", 0 );

	// setup some engine cvars for custom rendering
	r_test		= CVAR_GET_POINTER( "gl_test" );
	cv_nosort		= CVAR_GET_POINTER( "gl_nosort" );
	r_overview	= CVAR_GET_POINTER( "dev_overview" );
	r_fullbright	= CVAR_GET_POINTER( "r_fullbright" );
	r_drawentities	= CVAR_GET_POINTER( "r_drawentities" );
	gl_extensions	= CVAR_GET_POINTER( "gl_allow_extensions" );
	r_finish		= CVAR_GET_POINTER( "gl_finish" );
	cv_crosshair	= CVAR_GET_POINTER( "crosshair" );
	r_lighting_ambient	= CVAR_GET_POINTER( "r_lighting_ambient" );
	r_lighting_modulate	= CVAR_GET_POINTER( "r_lighting_modulate" );
	r_lightstyle_lerping= CVAR_GET_POINTER( "cl_lightstyle_lerping" );
	r_lighting_extended	= CVAR_GET_POINTER( "r_lighting_extended" );
	r_draw_beams	= CVAR_GET_POINTER( "cl_draw_beams" );
	r_detailtextures	= CVAR_GET_POINTER( "r_detailtextures" );
	r_speeds		= CVAR_GET_POINTER( "r_speeds" );
	r_novis		= CVAR_GET_POINTER( "r_novis" );
	r_nocull		= CVAR_GET_POINTER( "r_nocull" );
	r_lockpvs		= CVAR_GET_POINTER( "r_lockpvs" );
	r_wireframe	= CVAR_GET_POINTER( "gl_wireframe" );
	r_lightmap	= CVAR_GET_POINTER( "r_lightmap" );
	r_decals		= CVAR_GET_POINTER( "r_decals" );
	r_clear		= CVAR_GET_POINTER( "gl_clear" );
	r_dynamic		= CVAR_GET_POINTER( "r_dynamic" );
	cv_gamma		= CVAR_GET_POINTER( "gamma" );
	cv_brightness	= CVAR_GET_POINTER( "brightness" );
	r_polyoffset	= CVAR_GET_POINTER( "gl_polyoffset" );

	v_glows		= CVAR_REGISTER( "gl_glows", "1", FCVAR_ARCHIVE );

	r_dof = CVAR_REGISTER( "r_dof", "1", FCVAR_ARCHIVE );
	r_dof_hold_time = CVAR_REGISTER( "r_dof_hold_time", "0.2", FCVAR_ARCHIVE );
	r_dof_change_time = CVAR_REGISTER( "r_dof_change_time", "0.8", FCVAR_ARCHIVE );
	r_dof_focal_length = CVAR_REGISTER( "r_dof_focal_length", "600.0", FCVAR_ARCHIVE );
	r_dof_fstop = CVAR_REGISTER( "r_dof_fstop", "8", FCVAR_ARCHIVE );
	r_dof_debug = CVAR_REGISTER( "r_dof_debug", "0", FCVAR_ARCHIVE );

	cv_renderer	= CVAR_REGISTER( "gl_renderer", "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE );
	cv_bump		= CVAR_REGISTER( "gl_bump", "1", FCVAR_ARCHIVE );
	cv_bumpvecs	= CVAR_REGISTER( "bump_vecs", "0", 0 );
	cv_specular	= CVAR_REGISTER( "gl_specular", "1", FCVAR_ARCHIVE );
	cv_dynamiclight	= CVAR_REGISTER( "gl_dynlight", "1", FCVAR_ARCHIVE );
	cv_parallax	= CVAR_REGISTER( "gl_parallax", "1", FCVAR_ARCHIVE );
	cv_shadow_offset	= CVAR_REGISTER( "gl_shadow_offset", "0.02", FCVAR_ARCHIVE );
	cv_deferred	= CVAR_REGISTER( "gl_deferred", "0", FCVAR_ARCHIVE );
	cv_deferred_full	= CVAR_REGISTER( "gl_deferred_fullres_shadows", "0", FCVAR_ARCHIVE );
	cv_deferred_maxlights = CVAR_REGISTER( "gl_deferred_maxlights", "8", FCVAR_ARCHIVE|FCVAR_LATCH );
	cv_deferred_tracebmodels = CVAR_REGISTER( "gl_deferred_tracebmodels", "0", FCVAR_ARCHIVE );
	cv_cubemaps	= CVAR_REGISTER( "gl_cubemap_reflection", "1", FCVAR_ARCHIVE );
	cv_cube_lod_bias	= CVAR_REGISTER( "gl_cube_lod_bias", "3", FCVAR_ARCHIVE );
	cv_water		= CVAR_REGISTER( "gl_waterblur", "1", FCVAR_ARCHIVE );
	cv_decalsdebug	= CVAR_REGISTER( "gl_decals_debug", "0", FCVAR_ARCHIVE );
	cv_realtime_puddles	= CVAR_REGISTER( "gl_realtime_puddles", "0", FCVAR_ARCHIVE );
	r_sunshadows	= CVAR_REGISTER( "gl_sun_shadows", "0", FCVAR_ARCHIVE );
	r_sun_allowed	= CVAR_REGISTER( "r_sun_allowed", "1", FCVAR_ARCHIVE );
	r_shadowmap_size	= CVAR_REGISTER( "gl_shadowmap_size", "512", FCVAR_ARCHIVE );
	r_occlusion_culling	= CVAR_REGISTER( "r_occlusion_culling", "0", FCVAR_ARCHIVE );
	r_show_lightprobes	= CVAR_REGISTER( "r_show_lightprobes", "0", FCVAR_ARCHIVE );
	r_show_cubemaps	= CVAR_REGISTER( "r_show_cubemaps", "0", FCVAR_ARCHIVE );
	r_show_viewleaf	= CVAR_REGISTER( "r_show_viewleaf", "0", FCVAR_ARCHIVE );
	cv_decals		= CVAR_REGISTER( "gl_decals", "1", FCVAR_ARCHIVE );
	r_lightstyles	= CVAR_REGISTER( "gl_lightstyles", "1", FCVAR_ARCHIVE );
	r_allow_mirrors	= CVAR_REGISTER( "gl_allow_mirrors", "1", FCVAR_ARCHIVE );
	r_studio_decals	= CVAR_REGISTER( "r_studio_decals", "32", FCVAR_ARCHIVE );
	cv_show_tbn	= CVAR_REGISTER( "gl_show_basis", "0", FCVAR_ARCHIVE );
	cv_brdf		= CVAR_REGISTER( "r_lighting_brdf", "1", FCVAR_ARCHIVE );

	r_grass		= CVAR_REGISTER( "r_grass", "1", FCVAR_ARCHIVE );
	r_grass_alpha	= CVAR_REGISTER( "r_grass_alpha", "0.5", FCVAR_ARCHIVE );
	r_grass_lighting	= CVAR_REGISTER( "r_grass_lighting", "1", FCVAR_ARCHIVE );
	r_grass_shadows	= CVAR_REGISTER( "r_grass_shadows", "1", FCVAR_ARCHIVE );
	r_grass_fade_start	= CVAR_REGISTER( "r_grass_fade_start", "1024", FCVAR_ARCHIVE );
	r_grass_fade_dist	= CVAR_REGISTER( "r_grass_fade_dist", "2048", FCVAR_ARCHIVE );
}

//===============================
// buz: flashlight managenemt
//===============================
void SetupFlashlight( struct ref_params_s *pparams )
{
	if( !g_flashlight )
		return;

	static float add = 0.0f;
	float addideal = 0.0f;
	pmtrace_t ptr;

	Vector origin = pparams->vieworg + Vector( 0.0f, 0.0f, 6.0f ) + (pparams->right * 5.0f) + (pparams->forward * 2.0f);
	Vector vecEnd = origin + (pparams->forward * 700.0f);

	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, vecEnd, PM_NORMAL, -1, &ptr );
	const char *texName = gEngfuncs.pEventAPI->EV_TraceTexture( ptr.ent, origin, vecEnd );

	if( ptr.fraction < 1.0f )
		addideal = (1.0f - ptr.fraction) * 30.0f;
	float speed = (add - addideal) * 10.0f;
	if( speed < 0 ) speed *= -1.0f;

	if( add < addideal )
	{
		add += pparams->frametime * speed;
		if( add > addideal ) add = addideal;
	}
	else if( add > addideal )
	{
		add -= pparams->frametime * speed;
		if( add < addideal ) add = addideal;
	}

	CDynLight *flashlight = CL_AllocDlight( FLASHLIGHT_KEY );

	R_SetupLightParams( flashlight, origin, pparams->viewangles, 700.0f, 35.0f + add, LIGHT_SPOT );
	R_SetupLightTexture( flashlight, tr.flashlightTexture );

	flashlight->color = Vector( 1.4f, 1.4f, 1.4f ); // make model dymanic lighting happy
	flashlight->die = pparams->time + 0.05f;

	if( texName && ( !Q_strnicmp( texName, "reflect", 7 ) || !Q_strnicmp( texName, "mirror", 6 )) && r_allow_mirrors->value )
	{
		CDynLight *flashlight2 = CL_AllocDlight( FLASHLIGHT_KEY + 1 );
		float d = 2.0f * DotProduct( pparams->forward, ptr.plane.normal );
		Vector angles, dir = (pparams->forward - ptr.plane.normal) * d;
		VectorAngles( dir, angles );

		R_SetupLightParams( flashlight2, ptr.endpos, angles, 700.0f, 35.0f + add, LIGHT_SPOT );
		R_SetupLightTexture( flashlight2, tr.flashlightTexture );
		flashlight2->color = Vector( 1.4f, 1.4f, 1.4f ); // make model dymanic lighting happy
		flashlight2->die = pparams->time + 0.01f;
	}
}