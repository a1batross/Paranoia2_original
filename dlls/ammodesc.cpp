/*
ammodesc.cpp - virtual generic ammo that not needs to be hardcoded
Copyright (C) 2014 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"

extern int gEvilImpulse101;

AmmoInfo CBasePlayerAmmo :: AmmoInfoArray[MAX_AMMO_SLOTS];
AmmoDesc CBasePlayerAmmo :: AmmoDescArray[MAX_AMMO_DESC];
int giAmmoIndex = 0;
int giAmmoEnts = 0;

// Precaches the ammo and queues the ammo info for sending to clients
void AddAmmoNameToAmmoRegistry( const char *szAmmoname )
{
	// make sure it's not already in the registry
	for( int i = 1; i < MAX_AMMO_SLOTS; i++ )
	{
		if( !CBasePlayerAmmo :: AmmoInfoArray[i].pszName )
			continue;

		if( !Q_stricmp( CBasePlayerAmmo :: AmmoInfoArray[i].pszName, szAmmoname ))
			return; // ammo already in registry, just quite
	}

	if( giAmmoIndex >= MAX_AMMO_SLOTS )
	{
		ALERT( at_error, "Too many ammo types. ammotype %s was ignored\n", szAmmoname );
		return;
	}

	CBasePlayerAmmo :: AmmoInfoArray[giAmmoIndex].pszName = szAmmoname;
	CBasePlayerAmmo :: AmmoInfoArray[giAmmoIndex].iId = giAmmoIndex;   // yes, this info is redundant
	giAmmoIndex++;
}

void AddAmmoNameToAmmoRegistry( const AmmoInfo *pInfo )
{
	if( !pInfo || !pInfo->pszName || !*pInfo->pszName )
		return; // bad name specified

	// make sure it's not already in the registry
	for( int i = 1; i < MAX_AMMO_SLOTS; i++ )
	{
		if( !CBasePlayerAmmo :: AmmoInfoArray[i].pszName )
			continue;

		if( !Q_stricmp( CBasePlayerAmmo :: AmmoInfoArray[i].pszName, pInfo->pszName ))
		{
			ALERT( at_warning, "AmmoInfo: duplicate info for ammotype %s was ignored\n", pInfo->pszName );
			return; // ammo already in registry
		}
	}

	if( giAmmoIndex >= MAX_AMMO_SLOTS )
	{
		ALERT( at_error, "Too many ammo types. ammotype %s was ignored\n", pInfo->pszName );
		return;
	}

	CBasePlayerAmmo :: AmmoInfoArray[giAmmoIndex] = *pInfo;
	CBasePlayerAmmo :: AmmoInfoArray[giAmmoIndex].iId = giAmmoIndex;   // yes, this info is redundant
	giAmmoIndex++;
}

AmmoInfo *UTIL_FindAmmoType( const char *szAmmoname )
{
	if( !szAmmoname || !*szAmmoname )
	{
		ALERT( at_error, "FindAmmoType: NULL type\n" );
		return NULL; // bad name specified
          }

	if( !Q_stricmp( szAmmoname, "none" ))
		return NULL; // no ammo specified

	for( int i = 1; i < MAX_AMMO_SLOTS; i++ )
	{
		if( !CBasePlayerAmmo :: AmmoInfoArray[i].pszName )
			continue;

		if( !Q_stricmp( CBasePlayerAmmo :: AmmoInfoArray[i].pszName, szAmmoname ))
			return &CBasePlayerAmmo :: AmmoInfoArray[i];
	}

	ALERT( at_error, "FindAmmoType: coudn't find ammo type '%s'\n", szAmmoname );

	return NULL;
}

AmmoDesc *UTIL_FindAmmoDesc( const char *szClassname )
{
	if( !szClassname || !*szClassname )
	{
		ALERT( at_error, "FindAmmoDesc: NULL classname\n" );
		return NULL; // bad classname specified
          }

	for( int i = 0; i < giAmmoEnts; i++ )
	{
		if( !CBasePlayerAmmo :: AmmoDescArray[i].classname )
			continue;

		if( !Q_stricmp( STRING( CBasePlayerAmmo :: AmmoDescArray[i].classname ), szClassname ))
			return &CBasePlayerAmmo :: AmmoDescArray[i];
	}

	ALERT( at_error, "FindAmmoDesc: coudn't find entity '%s'\n", szClassname );

	return NULL;
}

void AddAmmoEntityToArray( const AmmoDesc *pDesc )
{
	if( !pDesc || !pDesc->classname || !pDesc->type )
		return; // bad name specified or type not found

	if( giAmmoEnts >= MAX_AMMO_DESC )
	{
		ALERT( at_error, "Too many ammo entities specified. %s was ignored\n", STRING( pDesc->classname ));
		return;
	}

	// make sure it's not already in the registry
	for( int i = 0; i < giAmmoEnts; i++ )
	{
		if( !Q_stricmp( STRING( CBasePlayerAmmo :: AmmoDescArray[i].classname ), STRING( pDesc->classname )))
		{
			ALERT( at_warning, "AmmoDesc: duplicate info for %s was ignored\n", STRING( pDesc->classname ));
			return; // ammo already in registry
		}
	}

	// add new entry
	CBasePlayerAmmo :: AmmoDescArray[giAmmoEnts] = *pDesc;
	giAmmoEnts++;
}

void UTIL_ParseAmmoInfo( char *&pfile )
{
	AmmoInfo tmpInfo;
	char token[256];
	int section = 0;

	memset( &tmpInfo, 0, sizeof( AmmoInfo ));

	// apply default params
	tmpInfo.iMaxCarry = 255;
	tmpInfo.flDistance = 8192.0f;
	tmpInfo.iNumShots = 1;

	while( pfile != NULL )
	{
		pfile = COM_ParseFile( pfile, token );

		if( !Q_stricmp( token, "{" ))
			section++;
		else if( !Q_stricmp( token, "}" ))
		{
			section--;
			break;
		}
		else if( section > 0 )
		{
			if( !Q_stricmp( token, "name" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpInfo.pszName = STRING( ALLOC_STRING( token )); // zone memory
			}
			else if( !Q_stricmp( token, "MaxCarry" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpInfo.iMaxCarry = bound( 1, Q_atoi( token ), 255 ); // a byte limit
			}
			else if( !Q_stricmp( token, "ShellModel" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpInfo.iShellIndex = PRECACHE_MODEL( token ); // shell model
			}
			else if( !Q_stricmp( token, "Damage" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpInfo.flPlayerDamage = tmpInfo.flMonsterDamage = Q_atof( token ); // healing damage is allowed
			}
			else if( !Q_stricmp( token, "PlayerDamage" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpInfo.flPlayerDamage = Q_atof( token ); // healing damage is allowed
			}
			else if( !Q_stricmp( token, "MonsterDamage" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpInfo.flMonsterDamage = Q_atof( token ); // healing damage is allowed
			}
			else if( !Q_stricmp( token, "NumShots" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpInfo.iNumShots = bound( 1, Q_atoi( token ), 32 ); // shots per one shot
			}
			else if( !Q_stricmp( token, "Distance" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpInfo.flDistance = bound( 64.0f, Q_atof( token ), 32768.0f ); // shots per one shot
			}
			else if( !Q_stricmp( token, "Missile" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpInfo.iMissileClassName = ALLOC_STRING( token );
				UTIL_PrecacheOther( token );
			}
		}
	}

	if( section ) ALERT( at_warning, "invalid braced sections %i in ammodescripton\n", section );
	else AddAmmoNameToAmmoRegistry( &tmpInfo );
}

void UTIL_ParseAmmoEntity( char *&pfile, const char *classname )
{
	AmmoDesc tmpDesc;
	char token[256];
	int section = 0;

	memset( &tmpDesc, 0, sizeof( AmmoDesc ));

	tmpDesc.classname = ALLOC_STRING( classname );

	while( pfile != NULL )
	{
		pfile = COM_ParseFile( pfile, token );

		if( !Q_stricmp( token, "{" ))
			section++;
		else if( !Q_stricmp( token, "}" ))
		{
			section--;
			break;
		}
		else if( section > 0 )
		{
			if( !Q_stricmp( token, "model" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpDesc.ammomodel = ALLOC_STRING( token );
				PRECACHE_MODEL( STRING( tmpDesc.ammomodel ));
			}
			else if( !Q_stricmp( token, "sound" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpDesc.clipsound = ALLOC_STRING( token );
				PRECACHE_SOUND( STRING( tmpDesc.clipsound ));
			}
			else if( !Q_stricmp( token, "type" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpDesc.type = UTIL_FindAmmoType( token );
			}
			else if( !Q_stricmp( token, "count" ))
			{
				pfile = COM_ParseFile( pfile, token );
				tmpDesc.count = RandomRange( token );
			}
		}
	}

	if( section ) ALERT( at_warning, "invalid braced sections %i in %s\n", section, classname );
	else AddAmmoEntityToArray( &tmpDesc );
}

void UTIL_InitAmmoDescription( const char *filename )
{
	char *afile = (char *)LOAD_FILE( filename, NULL );

	giAmmoIndex = 1;
	giAmmoEnts = 0;

	if( !afile )
	{
 		ALERT( at_error, "ammo description file %s not found!\n", filename );
		return;
	}

	char *pfile = afile;
	char token[256];

	// parse all the ammoinfos
	while( pfile != NULL )
	{
		pfile = COM_ParseFile( pfile, token );

		if( !Q_stricmp( token, "ammoinfo" ))
			UTIL_ParseAmmoInfo( pfile );
		else pfile = COM_SkipBracedSection( pfile );
	}

	pfile = afile; // reset pointer

	// parse ammo_ virtual entities
	while( pfile != NULL )
	{
		pfile = COM_ParseFile( pfile, token );

		if( Q_stricmp( token, "ammoinfo" ))
			UTIL_ParseAmmoEntity( pfile, token );
		else pfile = COM_SkipBracedSection( pfile );
	}

	FREE_FILE( afile );
}

// ====================== AMMO_GENERIC ================================

LINK_ENTITY_TO_CLASS( ammo_generic, CBasePlayerAmmo );

// Wargon: SaveData для юзабельности патронов.
TYPEDESCRIPTION CBasePlayerAmmo::m_SaveData[] = 
{
	DEFINE_FIELD( CBasePlayerAmmo, m_bCustomAmmo, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayerAmmo, m_iAmmoCaps, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerAmmo, m_rAmmoCount, FIELD_RANGE ),
	DEFINE_FIELD( CBasePlayerAmmo, m_iAmmoType, FIELD_STRING ),
}; IMPLEMENT_SAVERESTORE( CBasePlayerAmmo, CBaseEntity );

void CBasePlayerAmmo :: Precache( void )
{
	if( IsGenericAmmo() && !pev->model )
	{
		if( !InitGenericAmmo( ))
			return;
	}

	PRECACHE_MODEL( STRING( pev->model ));
	PRECACHE_SOUND( STRING( pev->noise ));
}

void CBasePlayerAmmo :: KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "count" ))
	{
		m_rAmmoCount = RandomRange( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else CBaseEntity :: KeyValue( pkvd );
}

BOOL CBasePlayerAmmo :: InitGenericAmmo( void )
{
	// two way to create this - manually fill field 'netname' or just to create virtual entity
	if( IsGenericAmmo( ))
	{
		// find description of virtual entity
		AmmoDesc *pDesc = UTIL_FindAmmoDesc( STRING( pev->netname ));		

		if( !pDesc )
		{
			REMOVE_ENTITY( edict( ));
			return FALSE; // description is missed
		}

		// copy the description into entity struct
		m_iAmmoType = ALLOC_STRING( pDesc->type->pszName );
		pev->model = pDesc->ammomodel;
		pev->noise = pDesc->clipsound;

		// level-designer specified ammocount for current entity
		if( !m_rAmmoCount.IsDefined( ))
			m_rAmmoCount = pDesc->count;
		m_bCustomAmmo = TRUE;

		return TRUE;
	}
	return FALSE; // not a generic
}

void CBasePlayerAmmo :: Spawn( void )
{
	if( IsGenericAmmo( ))
	{
		// try to initialize virtual entity
		if( !InitGenericAmmo( ))
			return;
		Precache();
		SET_MODEL(ENT(pev), STRING( pev->model ));
	}

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ));
	UTIL_SetOrigin( this, pev->origin );

	if( !FBitSet( ObjectCaps(), FCAP_USE_ONLY ) || FBitSet( pev->spawnflags, SF_NORESPAWN ))
		SetTouch( &CBasePlayerAmmo:: DefaultTouch );

	// Wargon: Патроны юзабельны.
	SetUse( &CBasePlayerAmmo :: DefaultUse );
	m_iAmmoCaps = CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE;
}

BOOL CBasePlayerAmmo :: AddAmmo( CBaseEntity *pOther ) 
{ 
	if( pOther->GiveAmmo( m_rAmmoCount.Random(), STRING( m_iAmmoType )) != -1 )
	{
		EMIT_SOUND( ENT(pev), CHAN_ITEM, STRING( pev->noise ), 1, ATTN_NORM );
		return TRUE;
	}

	return FALSE;
}

CBaseEntity *CBasePlayerAmmo :: Respawn( void )
{
	SetBits( pev->effects, EF_NODRAW );
	SetTouch( NULL );

	// Wargon: Патроны неюзабельны.
	SetUse( NULL );
	m_iAmmoCaps = CBaseEntity::ObjectCaps();

	// move to wherever I'm supposed to respawn.
	UTIL_SetOrigin( this, g_pGameRules->VecAmmoRespawnSpot( this ));

	SetThink( &CBasePlayerAmmo:: Materialize );
	AbsoluteNextThink( g_pGameRules->FlAmmoRespawnTime( this ));

	return this;
}

void CBasePlayerAmmo :: Materialize( void )
{
	if( FBitSet( pev->effects, EF_NODRAW ))
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	if( !FBitSet( ObjectCaps(), FCAP_USE_ONLY ))
		SetTouch( &CBasePlayerAmmo:: DefaultTouch );

	// Wargon: Патроны юзабельны.
	SetUse( &CBasePlayerAmmo :: DefaultUse );
	m_iAmmoCaps = CBaseEntity :: ObjectCaps() | FCAP_IMPULSE_USE;
}

void CBasePlayerAmmo :: DefaultTouch( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer( ))
	{
		return;
	}

	if( AddAmmo( pOther ))
	{
		if( g_pGameRules->AmmoShouldRespawn( this ) == GR_AMMO_RESPAWN_YES )
		{
			Respawn();
		}
		else
		{
			SetTouch( NULL );

			// Wargon: Патроны неюзабельны.
			SetUse( NULL );
			m_iAmmoCaps = CBaseEntity :: ObjectCaps();
			SetThink( &CBasePlayerAmmo :: SUB_Remove );
			SetNextThink( 0.1 );
		}
	}
	else if( gEvilImpulse101 )
	{
		// evil impulse 101 hack, kill always
		SetTouch( NULL );

		// Wargon: Патроны неюзабельны.
		SetUse( NULL );
		m_iAmmoCaps = CBaseEntity :: ObjectCaps();

		SetThink( &CBasePlayerAmmo :: SUB_Remove );
		SetNextThink( 0.1 );
	}
}