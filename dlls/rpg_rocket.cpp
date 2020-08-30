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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( laser_spot, CLaserSpot );

//=========================================================
//=========================================================
CLaserSpot *CLaserSpot :: CreateSpot( void )
{
	CLaserSpot *pSpot = GetClassPtr((CLaserSpot *)NULL );
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING( "laser_spot" );

	return pSpot;
}

//=========================================================
//=========================================================
CLaserSpot *CLaserSpot :: CreateSpot( const char *spritename )
{
	CLaserSpot *pSpot = CreateSpot();
	SET_MODEL( pSpot->edict(), spritename );

	return pSpot;
}

//=========================================================
//=========================================================
void CLaserSpot :: Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/laserdot.spr");
	UTIL_SetOrigin( this, pev->origin );
}

//=========================================================
// Suspend- make the laser sight invisible. 
//=========================================================
void CLaserSpot :: Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	
	if( flSuspendTime == -1.0f )
	{
		SetThink( NULL );
	}
	else
	{
		SetThink( &CLaserSpot:: Revive );
		SetNextThink( flSuspendTime );
	}
}

void CLaserSpot :: Update( CBasePlayer *pPlayer )
{
	UTIL_MakeVectors( pPlayer->pev->v_angle );
	Vector vecSrc = pPlayer->GetGunPosition( );
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;
	UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, pPlayer->edict(), &tr );
		
	UTIL_SetOrigin( this, tr.vecEndPos );
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CLaserSpot :: Revive( void )
{
	pev->effects &= ~EF_NODRAW;

	SetThink( NULL );
}

void CLaserSpot::Precache( void )
{
	PRECACHE_MODEL( "sprites/laserdot.spr" );
}

LINK_ENTITY_TO_CLASS( rpg_rocket, CRpgRocket );

// Wargon: SaveData перемещен из weapons.cpp.
TYPEDESCRIPTION CRpgRocket :: m_SaveData[] = 
{
	DEFINE_FIELD( CRpgRocket, m_flIgniteTime, FIELD_TIME ),
	DEFINE_FIELD( CRpgRocket, m_pLauncher, FIELD_CLASSPTR ),
}; IMPLEMENT_SAVERESTORE( CRpgRocket, CGrenade );

//=========================================================
//=========================================================
CRpgRocket *CRpgRocket :: CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CBasePlayerItem *pLauncher )
{
	CRpgRocket *pRocket = GetClassPtr((CRpgRocket *)NULL );

	EMIT_SOUND( pLauncher->m_pPlayer->edict(), CHAN_ITEM, "weapons/rocketfire1.wav", 1, 0.9 );

	UTIL_SetOrigin( pRocket, vecOrigin );
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch(& CRpgRocket::RocketTouch );
	pRocket->m_pLauncher = pLauncher;// remember what RPG fired me. 
	pRocket->m_pLauncher->m_cActiveRockets++;// register this missile as active for the launcher
	pRocket->pev->owner = pOwner->edict();

	return pRocket;
}

//=========================================================
//=========================================================
void CRpgRocket :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( edict(), "models/rpgrocket.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( this, pev->origin );

	pev->classname = MAKE_STRING( "rpg_rocket" );

	SetThink( &CRpgRocket :: IgniteThink );
	SetTouch( &CRpgRocket :: ExplodeTouch );

	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -pev->angles.x;
	pev->gravity = 0.5;

	SetNextThink( 0.05 );

	pev->dmg = gSkillData.plrDmgRPG;
}

//=========================================================
//=========================================================
void CRpgRocket :: RocketTouch ( CBaseEntity *pOther )
{
	// my launcher is still around, tell it I'm dead.
	if( m_pLauncher ) m_pLauncher->m_cActiveRockets--;
	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );
	ExplodeTouch( pOther );
}

//=========================================================
//=========================================================
void CRpgRocket :: Precache( void )
{
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	m_iFireTrail = PRECACHE_MODEL( "sprites/muz3.spr" ); // Wargon: —прайт дл€ огненного следа у ракет.
	PRECACHE_SOUND ("weapons/rocket1.wav");
}

void CRpgRocket :: IgniteThink( void  )
{
	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	pev->body = 1;

	// make rocket sound
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );

	// rocket trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT(entindex()); // entity
		WRITE_SHORT( m_iTrail ); // model
		WRITE_BYTE( 30 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( 160 ); // r, g, b
		WRITE_BYTE( 170 ); // r, g, b
		WRITE_BYTE( 170 ); // r, g, b
		WRITE_BYTE( 200 ); // brightness
	MESSAGE_END(); //move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
	m_flIgniteTime = gpGlobals->time;

	// set to follow laser spot
	SetThink(&CRpgRocket :: FollowThink );
	SetNextThink( 0 );
}

void CRpgRocket :: FollowThink( void )
{
	CBaseEntity *pOther = NULL;
	Vector vecTarget;
	Vector vecDir;
	float flDist, flMax, flDot;
	TraceResult tr;

	UTIL_MakeAimVectors( pev->angles );

	vecTarget = gpGlobals->v_forward;
	flMax = 4096;

	// Examine all entities within a reasonable radius
	while ((pOther = UTIL_FindEntityByClassname( pOther, "laser_spot" )) != NULL)
	{
		UTIL_TraceLine ( pev->origin, pOther->pev->origin, dont_ignore_monsters, ENT(pev), &tr );

		if( tr.flFraction >= 0.90f )
		{
			vecDir = pOther->pev->origin - pev->origin;
			flDist = vecDir.Length( );
			vecDir = vecDir.Normalize( );
			flDot = DotProduct( gpGlobals->v_forward, vecDir );
			if(( flDot > 0.0f ) && ( flDist * ( 1.0f - flDot ) < flMax ))
			{
				flMax = flDist * ( 1.0f - flDot );
				vecTarget = vecDir;
			}
		}
	}

	pev->angles = UTIL_VecToAngles( vecTarget );

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();

	if( gpGlobals->time - m_flIgniteTime < 1.0f )
	{
		pev->velocity = pev->velocity * 0.2f + vecTarget * ( flSpeed * 0.8f + 400.0f );
		if( pev->waterlevel == 3 && pev->watertype > CONTENTS_FLYFIELD )
		{
			// go slow underwater
			if( pev->velocity.Length() > 300.0f )
			{
				pev->velocity = pev->velocity.Normalize() * 300.0f;
			}
			UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 4 );
		} 
		else 
		{
			if( pev->velocity.Length() > 2000.0f )
			{
				pev->velocity = pev->velocity.Normalize() * 2000.0f;
			}
		}
	}
	else
	{
		if( pev->effects & EF_LIGHT )
		{
			pev->effects &= ~EF_LIGHT;
			STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav" );
		}

		pev->velocity = pev->velocity * 0.2 + vecTarget * flSpeed * 0.798;

		if(( pev->waterlevel == 0 || pev->watertype == CONTENTS_FOG ) && pev->velocity.Length() < 1500 )
		{
			// my launcher is still around, tell it I'm dead.
			if( m_pLauncher ) m_pLauncher->m_cActiveRockets--;
			Detonate( );
		}
	}

	// Wargon: Ќепон€тно, почему у ракет не рисуетс€ glow-спрайт. »справил положение, сделав свой огонь дл€ ракеты через мессагу.
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iFireTrail );
		WRITE_BYTE( 4 );
		WRITE_BYTE( 150 );
	MESSAGE_END();

	// lev
	extern int gmsgCustomDLight; 

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPARKS );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, gmsgCustomDLight, pev->origin );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_BYTE( 10 );	// radius / 10
		WRITE_BYTE( 1 );	// life * 10
		WRITE_BYTE( 10 );	// decay / 10
	MESSAGE_END();

	SetNextThink( 0.02 );
}