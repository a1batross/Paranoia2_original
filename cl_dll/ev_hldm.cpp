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
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"
#include "gl_rpart.h"
#include <string.h>
#include "material.h"
#include "r_studioint.h"
#include "com_model.h"
#include "studio.h"

extern engine_studio_api_t IEngineStudio;

static int tracerCount[ 32 ];

void V_PunchAxis( int axis, float punch );
void VectorAngles( const float *forward, float *angles );

extern cvar_t *cl_lw;

extern "C"
{
void EV_FireGeneric( struct event_args_s *args );
void EV_TrainPitchAdjust( struct event_args_s *args );
void EV_SmokePuff( struct event_args_s *args );
}

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
void EV_HLDM_PlayTextureSound( pmtrace_t *ptr, const Vector &vecSrc, const Vector &vecEnd )
{
	// hit the world, try to play sound based on texture material type
	char *rgsz[MAX_MAT_SOUNDS];
	float fattn = ATTN_NORM;
	int cnt = 0;

	physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( ptr->ent );

	matdef_t *pMat = NULL;

	if( pe )
	{
		if( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP )
			pMat = COM_MatDefFromSurface( gEngfuncs.pEventAPI->EV_TraceSurface( ptr->ent, (float *)&vecSrc, (float *)&vecEnd ), ptr->endpos );
		else if( pe->solid == SOLID_CUSTOM && ptr->surf )
			pMat = ptr->surf->effects;

		if( !pMat ) return;

		// fire effects
		for( cnt = 0; pMat->impact_parts[cnt] != NULL; cnt++ )
			g_pParticles.CreateEffect( pMat->impact_parts[cnt], ptr->endpos, ptr->plane.normal );

		// count sounds
		for( cnt = 0; pMat->impact_sounds[cnt] != NULL; cnt++ )
			rgsz[cnt] = (char *)pMat->impact_sounds[cnt];
	}

	if( !cnt ) return;

	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[RANDOM_LONG( 0, cnt - 1 )], 0.9, fattn, 0, 96 + RANDOM_LONG( 0, 0xf ));
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, const Vector &vecSrc, const Vector &vecEnd )
{
	if( pTrace->inwater ) return;

	int iRand = RANDOM_LONG( 0, 0x7FFF );

	if( iRand < ( 0x7fff / 2 )) // not every bullet makes a sound.
	{
		switch( iRand % 5)
		{
		case 0: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 1: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 2: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 3: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 4: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		}
	}

	physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	matdef_t *pMat = NULL;

	if( pe )
	{	
		if( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP )
		{
			pMat = COM_MatDefFromSurface( gEngfuncs.pEventAPI->EV_TraceSurface( pTrace->ent, (float *)&vecSrc, (float *)&vecEnd ), pTrace->endpos );
			if ( pMat ) CreateDecal( pTrace, pMat->impact_decal, RANDOM_FLOAT( 0.0f, 360.0f ));
		}
		else if( pe->solid == SOLID_CUSTOM && pTrace->surf )
		{
			pMat = pTrace->surf->effects;
			if ( pMat ) UTIL_StudioDecal( pMat->impact_decal, pTrace, vecSrc );
		}
	}
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY )
{
	int i;
	pmtrace_t tr;
	int iShot;

	for ( iShot = 1; iShot <= cShots; iShot++ )	
	{
		Vector vecDir, vecEnd;
			
		float x, y, z;

		// We randomize for the Shotgun.
		if ( iBulletType == BULLET_BUCKSHOT )
		{
			do {
				x = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
				y = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
				z = x * x + y * y;
			} while( z > 1.0f );

			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + x * flSpreadX * right[i] + y * flSpreadY * up[i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		}
		else
		{
			// but other guns already have their spread randomized in the synched spread.
			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + flSpreadX * right[i] + flSpreadY * up[i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, 0, -1, &tr );

		// do damage, paint decals
		if ( tr.fraction != 1.0 && !tr.inwater ) // buz: dont smoke and particle if shoot in sky
		{
			EV_HLDM_PlayTextureSound( &tr, vecSrc, vecEnd );
			EV_HLDM_GunshotDecalTrace( &tr, vecSrc, vecEnd );			
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

//======================
//	WEAPON GENERIC START
//======================
void EV_FireGeneric( struct event_args_s *args )
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	int shell = args->bparam1; // brass shell
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );
	
	if( EV_IsLocal( idx ))
	{
		// add muzzle flash to current weapon model
		EV_MuzzleFlash();

		UTIL_WeaponAnimation(( args->iparam1 & 0xFFF ), 1.0f ); // FIXME: how to send framerate?

		V_PunchAxis( 0, RANDOM_FLOAT( ((float)args->iparam2 / 255.0f ) * -3.0f, 0.0f ));

		cl_entity_t *ent = GetViewEntity();
		if( ent ) ShellOrigin = ent->attachment[1];
	}

	int numShots = ((args->iparam1 >> 12) & 0xF);
	int soundType = TE_BOUNCE_SHELL;
	int bulletType = BULLET_NORMAL;
	float flDist = 8192.0f;

	// shotgun case
	if( numShots > 1 )
	{
		bulletType = BULLET_BUCKSHOT;
		soundType = TE_BOUNCE_SHOTSHELL;
		flDist = 2048.0f;
	}

	EV_EjectBrass( ShellOrigin, ShellVelocity , RANDOM_FLOAT( -angles[YAW], angles[YAW] ), shell, soundType ); 

	if( args->bparam2 )
	{
		const char *fireSound = gEngfuncs.pEventAPI->EV_SoundForIndex( args->bparam2 );

		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, fireSound, 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 0xf ));
	}	

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, numShots, vecSrc, vecAiming, flDist, bulletType, 2, &tracerCount[idx-1], args->fparam1, args->fparam2 );
}
//======================
//	WEAPON GENERIC END
//======================
#define SND_CHANGE_PITCH	(1<<7)		// duplicated in protocol.h change sound pitch

void EV_TrainPitchAdjust( event_args_t *args )
{
	int idx;
	Vector origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;
	
	char sz[ 256 ];

	idx = args->entindex;
	
	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop	  = args->bparam1;

	m_flVolume	= (float)(us_params & 0x003f)/40.0;
	noise		= (int)(((us_params) >> 12 ) & 0x0007);
	pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch ( noise )
	{
	case 1: strcpy( sz, "plats/ttrain1.wav"); break;
	case 2: strcpy( sz, "plats/ttrain2.wav"); break;
	case 3: strcpy( sz, "plats/ttrain3.wav"); break; 
	case 4: strcpy( sz, "plats/ttrain4.wav"); break;
	case 5: strcpy( sz, "plats/ttrain6.wav"); break;
	case 6: strcpy( sz, "plats/ttrain7.wav"); break;
	default:
		// no sound
		strcpy( sz, "" );
		return;
	}

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

int EV_TFC_IsAllyTeam( int iTeam1, int iTeam2 )
{
	return 0;
}

// buz: smokepuff event - for invoke puffs from server, when monsters firing, etc..
void EV_SmokePuff( struct event_args_s *args )
{
	VectorNormalize(args->angles); // there is surface normal actually
	g_pParticles.BulletParticles( args->origin, args->angles );
}

void EV_GunSmoke( const Vector &pos )
{
	g_pParticles.GunSmoke( pos, 2 );    
}

void EV_ExplodeSmoke( const Vector &pos )
{
	g_pParticles.SmokeParticles( pos, 10 );    
}

void EV_HLDM_WaterSplash( float x, float y, float z, float ScaleSplash1, float ScaleSplash2 )
{
    int  iWaterSplash = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/splash1.spr");
    TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->R_TempSprite( Vector( x, y, z + 15 ),
    Vector( 0, 0, 0 ),
    /*ScaleSplash1*/0.2, iWaterSplash, kRenderTransAdd, kRenderFxNone, 1.0, 0.5, FTENT_SPRANIMATE | FTENT_FADEOUT | FTENT_COLLIDEKILL );
    
    if(pTemp)
    {
        pTemp->fadeSpeed = 90.0;
        pTemp->entity.curstate.framerate = 70.0;
        pTemp->entity.curstate.renderamt = 155;
        pTemp->entity.curstate.rendercolor.r = 255;
        pTemp->entity.curstate.rendercolor.g = 255;
        pTemp->entity.curstate.rendercolor.b = 255;
    }
    
    iWaterSplash = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/splash2.spr");
    pTemp = gEngfuncs.pEfxAPI->R_TempSprite( Vector( x, y, z ),
    Vector( 0, 0, 0 ),
    /*ScaleSplash2*/0.08, iWaterSplash, kRenderTransAdd, kRenderFxNone, 1.0, 0.5, FTENT_SPRANIMATE | FTENT_FADEOUT | FTENT_COLLIDEKILL );
    
    if(pTemp)
    {
        pTemp->fadeSpeed = 60.0;
        pTemp->entity.curstate.framerate = 60.0;
        pTemp->entity.curstate.renderamt = 150;
        pTemp->entity.curstate.rendercolor.r = 255;
        pTemp->entity.curstate.rendercolor.g = 255;
        pTemp->entity.curstate.rendercolor.b = 255;
        pTemp->entity.angles = Vector( 90, 0, 0 );
    }
}

void EV_HLDM_NewExplode( float x, float y, float z, float ScaleExplode1 )
{
	float rnd = gEngfuncs.pfnRandomFloat( -0.03, 0.03 );   
	int  iNewExplode = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/dexplo.spr");
	TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->R_TempSprite( Vector( x, y, z + 15 ), Vector( 0, 0, 0 ),
	ScaleExplode1, iNewExplode, kRenderTransAdd, kRenderFxNone, 1.0, 0.5, FTENT_SPRANIMATE | FTENT_FADEOUT );
    
	if( pTemp )
	{
		pTemp->fadeSpeed = 90.0;
		pTemp->entity.curstate.framerate = 37.0;
		pTemp->entity.curstate.renderamt = 155;
		pTemp->entity.curstate.rendercolor.r = 255;
		pTemp->entity.curstate.rendercolor.g = 255;
		pTemp->entity.curstate.rendercolor.b = 255;
	}
    
	iNewExplode = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/fexplo.spr");
	pTemp = gEngfuncs.pEfxAPI->R_TempSprite( Vector( x, y, z + 15), Vector( 0, 0, 0 ),
	ScaleExplode1, iNewExplode, kRenderTransAdd, kRenderFxNone, 1.0, 0.5, FTENT_SPRANIMATE | FTENT_FADEOUT );   
    
	if( pTemp )
	{
		pTemp->fadeSpeed = 100.0;
		pTemp->entity.curstate.framerate = 35.0;
		pTemp->entity.curstate.renderamt = 150;
		pTemp->entity.curstate.rendercolor.r = 255;
		pTemp->entity.curstate.rendercolor.g = 255;
		pTemp->entity.curstate.rendercolor.b = 255;
		pTemp->entity.angles = Vector( 90, 0, 0 );
	}

	iNewExplode = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/smokeball.spr");
	pTemp = gEngfuncs.pEfxAPI->R_TempSprite( Vector( x, y, z + 16), Vector( 0, 0, 0 ),
	ScaleExplode1, iNewExplode, kRenderTransAdd, kRenderFxNone, 1.0, 0.5, FTENT_SPRANIMATE | FTENT_FADEOUT );   
    
	if( pTemp )
	{
		pTemp->fadeSpeed = 50.0;
		pTemp->entity.curstate.framerate = 22.0;
		pTemp->entity.curstate.renderamt = 120;
		pTemp->entity.curstate.rendercolor.r = 255;
		pTemp->entity.curstate.rendercolor.g = 255;
		pTemp->entity.curstate.rendercolor.b = 255;
		pTemp->entity.angles = Vector( 90, 0, 0 );
	}

	EV_ExplodeSmoke( Vector( x, y, z + 4.0f ));
}