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
#include "animation.h"
#include "effects.h"
#include "nodes.h"
#include "explode.h"
#include "player.h"

#define FIRE_SPRITE_NAME		"sprites/flame2.spr"
#define BURNIND_SOUND_NAME		"ambience/burning1.wav"

// must match definition in modelgen.h
enum synctype_t
{
	ST_SYNC=0, 
	ST_RAND 
};

// TODO: shorten these?
typedef struct {
	int			ident;
	int			version;
	int			type;
	int			texFormat;
	float		boundingradius;
	int			width;
	int			height;
	int			numframes;
	float		beamlength;
	synctype_t	synctype;
} dsprite_t;

class CPropExplosion : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	int	 BloodColor( void ) { return DONT_BLEED; };

	void EXPORT PropThink( void );

	void RunFire( void );
	void StartExplode( void );

	float CalculationSpriteScale( );

	void KeyValue( KeyValueData *pkvd );

	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	string_t m_iszParentName;

	int		m_startHealth;
	float	m_firetime;
	float	m_explosiontime;
	float	m_dmgradius;
	bool	m_isfire;
};

LINK_ENTITY_TO_CLASS( prop_explosion, CPropExplosion );

TYPEDESCRIPTION	CPropExplosion::m_SaveData[] = 
{
	DEFINE_FIELD( CPropExplosion, m_startHealth, FIELD_INTEGER ),
	DEFINE_FIELD( CPropExplosion, m_firetime, FIELD_FLOAT ),
	DEFINE_FIELD( CPropExplosion, m_explosiontime, FIELD_FLOAT ),
	DEFINE_FIELD( CPropExplosion, m_dmgradius, FIELD_FLOAT ),
	DEFINE_FIELD( CPropExplosion, m_isfire, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CPropExplosion, CGrenade );

void CPropExplosion :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "health"))//skin is used for content type
	{
		m_startHealth = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firetime"))
	{
		m_firetime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damageradius"))
	{
		m_dmgradius = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CGrenade::KeyValue( pkvd );
}

void CPropExplosion :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT(pev), STRING(pev->model) );

	Vector mins;
	Vector maxs;

	ExtractBbox( pev->sequence, mins, maxs );

	UTIL_SetSize(pev, mins, maxs ); //Whi it is not using for other entities ?
	UTIL_SetOrigin( this, pev->origin );

	DROP_TO_FLOOR( edict() );

	pev->nextthink = gpGlobals->time + 0.1;

//	SetTouch( PropTouch ); //maybe later
	SetThink( PropThink );

	//If not defined, add standart value
	if(!m_startHealth )
	{
		m_startHealth = 50;
	}

	if(!m_firetime)
	{
		m_firetime = 4;
	}

	if(!m_dmgradius )
	{
		m_dmgradius = 100;
	}

	m_isfire = false;

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_YES;
	pev->health			= m_startHealth;
	pev->dmg			= 50;
}

void CPropExplosion::Precache( void )
{
	PRECACHE_MODEL((char *)STRING(pev->model));
	PRECACHE_MODEL( FIRE_SPRITE_NAME );
	PRECACHE_SOUND( BURNIND_SOUND_NAME );
}

void CPropExplosion::PropThink( void )
{
	if( pev->health	< m_startHealth )
	{
		if(!m_explosiontime)
		{
			m_explosiontime = gpGlobals->time + m_firetime;
		}
		else
		{
			if( m_explosiontime < gpGlobals->time )
			{
				StartExplode( );
			}
			else
			{
				RunFire( );
			}
		}
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CPropExplosion::RunFire( void )
{
	if(!m_isfire )
	{
		CSprite *pSprite = CSprite::SpriteCreate( FIRE_SPRITE_NAME, pev->origin, TRUE );

		pSprite->AnimateAndDie( m_firetime );
		pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
		pSprite->SetScale( CalculationSpriteScale() );

		EMIT_SOUND(ENT(pev), CHAN_WEAPON, BURNIND_SOUND_NAME, 1, ATTN_NORM);

		m_isfire = true;
	}
}

void CPropExplosion::StartExplode( void )
{
	STOP_SOUND( ENT(pev), CHAN_WEAPON, BURNIND_SOUND_NAME );

	UTIL_ScreenShake( pev->origin, 16, 200, 5, m_dmgradius );

	ExplosionCreate( pev->origin, pev->angles, edict(), (int)m_dmgradius, true );

	UTIL_Remove( this );
}

float CPropExplosion::CalculationSpriteScale( )
{
	dsprite_t *szSprite = NULL;

	float SpriteX = 0;
	float SpriteY = 0;

	float ModelX = 0;
	float ModelY = 0;

	szSprite = (dsprite_t *)g_engfuncs.pfnLoadFileForMe( FIRE_SPRITE_NAME, NULL );

	if (!szSprite)
	{
		ALERT( at_aiconsole,"Couldn't open fire sprites !\n");

		return 1.0;
	}

	SpriteX = szSprite->width;
	SpriteY = szSprite->height;

	g_engfuncs.pfnFreeFile( szSprite );

	Vector mins;
	Vector maxs;

	ExtractBbox( pev->sequence, mins, maxs );

	ModelX = maxs.x - mins.x;
	ModelY = maxs.z - mins.z;

	float FullSprSizeX = ModelX * 3; //pixel to unit ~ 2 ???
	float FullSprSizeY = ModelY * 3;

	float ScaleX = FullSprSizeX / SpriteX;
	float ScaleY = FullSprSizeY / SpriteY;

	float Averagescale = ( ScaleX + ScaleY ) / 2;

	//Debug
	/*
	ALERT( at_console, "Sprite: x = %f, y = %f\n", SpriteX, SpriteY  );
	ALERT( at_console, "Model:  x = %f, y = %f\n", ModelX,  ModelY   );

	ALERT( at_console, "Full Sprite size:  x = %f, y = %f\n", SpriteX * Averagescale,  SpriteY * Averagescale );
	ALERT( at_console, "Scale:  x = %f, y = %f\n", ScaleX,  ScaleY  );
	ALERT( at_console, "Average Scale: %f", Averagescale );
	*/

	return Averagescale;
}