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
/*

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

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
#include "animation.h"
#include "game.h"

extern int gEvilImpulse101;

DLL_GLOBAL unsigned short	g_usShootEvent;
DLL_GLOBAL short		g_sModelIndexLaser;// holds the index for the laser beam
DLL_GLOBAL const char	*g_pModelNameLaser = "sprites/laserbeam.spr";
DLL_GLOBAL short		g_sModelIndexLaserDot;// holds the index for the laser beam dot
DLL_GLOBAL short		g_sModelIndexFireball;// holds the index for the fireball
DLL_GLOBAL short		g_sModelIndexSmoke;// holds the index for the smoke cloud
DLL_GLOBAL short		g_sModelIndexWExplosion;// holds the index for the underwater explosion
DLL_GLOBAL short		g_sModelIndexBubbles;// holds the index for the bubbles model
DLL_GLOBAL short		g_sModelIndexBloodDrop;// holds the sprite index for the initial blood
DLL_GLOBAL short		g_sModelIndexBloodSpray;// holds the sprite index for splattered blood
DLL_GLOBAL short		g_sModelIndexWaterSplash;
DLL_GLOBAL short		g_sModelIndexSmokeTrail; 
DLL_GLOBAL short		g_sModelIndexNull;

ItemInfo CBasePlayerItem :: ItemInfoArray[MAX_WEAPONS];
int CBasePlayerItem :: m_iGlobalID;

extern int gmsgCurWeapon;

// Precaches the weapon and queues the weapon info for sending to clients
void UTIL_PrecacheWeapon( const char *szClassname )
{
	edict_t *pent = CREATE_NAMED_ENTITY( ALLOC_STRING( szClassname ) );

	if ( FNullEnt( pent ))
	{
		ALERT( at_error, "NULL Ent in UTIL_PrecacheOtherWeapon\n" );
		return;
	}
	
	CBaseEntity *pEntity = CBaseEntity::Instance (VARS( pent ));

	if (pEntity)
	{
		ItemInfo II;

		memset( &II, 0, sizeof II );

		if((( CBasePlayerItem *)pEntity)->GetItemInfo( &II ))
		{
			CBasePlayerItem::ItemInfoArray[II.iId] = II;
		}

		// g-cont. reduce loading time because v_models is too heaviliy ~ 35 Mb
//		pEntity->Precache( );
	}

	REMOVE_ENTITY( pent );
}

void UTIL_PrecacheSpecialItems( void ) // buz
{
	ItemInfo II;		

	// add painkiller to weapons list
	memset( &II, 0, sizeof II );
	II.pszName = "painkiller";
	II.pszAmmo1 = "painkillers";
	II.iMaxAmmo1 = 10;
	II.iSlot = 0;
	II.iPosition = 1;
 	II.pszAmmo2 = "none";
	II.iMaxAmmo2 = -1;
	II.iId = WEAPON_PAINKILLER;
	CBasePlayerItem::ItemInfoArray[II.iId] = II;
}

void UTIL_InitWeaponDescription( const char *pattern )
{
	int numFiles = 0;

	char **filenames = GET_FILES_LIST( pattern, &numFiles, FALSE );
	char classname[256]; // convert path into name

	for( int i = 0; i < numFiles; i++ )
	{
		COM_FileBase( filenames[i], classname );
		UTIL_PrecacheWeapon( classname );
	}

	UTIL_PrecacheSpecialItems();
}

TYPEDESCRIPTION CBasePlayerItem :: m_SaveData[] = 
{
	DEFINE_FIELD( CBasePlayerItem, m_pPlayer, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayerItem, m_pNext, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayerItem, m_iId, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iItemCaps, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iDefaultAmmo1, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iDefaultAmmo2, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerItem, m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerItem, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerItem, m_flTimeUpdate, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerItem, m_flSpreadTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerItem, m_flLastShotTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerItem, m_flLastSpreadPower, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerItem, m_iPrimaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iSecondaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iWeaponAutoFire, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_cActiveRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iStepReload, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iIronSight, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayerItem, m_fWaitForHolster, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayerItem, m_iZoom, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iClip, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iBody, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iSkin, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iSpot, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_iHandModel, FIELD_MODELNAME ),
}; IMPLEMENT_SAVERESTORE( CBasePlayerItem, CBaseAnimating );

LINK_ENTITY_TO_CLASS( weapon_generic, CBasePlayerItem );

void CBasePlayerItem :: SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector( -24.0f, -24.0f, 0.0f );
	pev->absmax = pev->origin + Vector( 24.0f,  24.0f, 16.0f ); 
}

//=========================================================
// Sets up movetype, size, solidtype for a new weapon. 
//=========================================================
void CBasePlayerItem :: FallInit( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	UTIL_SetOrigin( this, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero ); // pointsize until it lands on the ground.
	
	// Wargon: Оружие юзабельно.
	SetUse( &CBasePlayerItem::DefaultUse );
	m_iItemCaps = CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE;

	if( !FBitSet( ObjectCaps(), FCAP_USE_ONLY ) || FBitSet( pev->spawnflags, SF_NORESPAWN ))
		SetTouch( &CBasePlayerItem :: DefaultTouch );
	SetThink( &CBasePlayerItem :: FallThink );

	SetNextThink( 0.1 );
}

//=========================================================
// AttemptToMaterialize - the item is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
void CBasePlayerItem :: AttemptToMaterialize( void )
{
	float time = g_pGameRules->FlWeaponTryRespawn( this );

	if ( time == 0.0f )
	{
		Materialize();
		return;
	}

	SetNextThink( time );
}

//=========================================================
// Materialize - make a CBasePlayerItem visible and tangible
//=========================================================
void CBasePlayerItem :: Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin( this, pev->origin ); // link into world.

	// Wargon: Оружие юзабельно.
	SetUse( &CBasePlayerItem::DefaultUse );
	m_iItemCaps = CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE;

	if( !FBitSet( ObjectCaps(), FCAP_USE_ONLY ))
		SetTouch( &CBasePlayerItem :: DefaultTouch );
	SetThink( NULL );

}

//=========================================================
// CheckRespawn - a player is taking this weapon, should 
// it respawn?
//=========================================================
void CBasePlayerItem :: CheckRespawn ( void )
{
	switch ( g_pGameRules->WeaponShouldRespawn( this ) )
	{
	case GR_WEAPON_RESPAWN_YES:
		Respawn();
		break;
	case GR_WEAPON_RESPAWN_NO:
		break;
	}
}

//=========================================================
// Respawn- this item is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
CBaseEntity* CBasePlayerItem :: Respawn( void )
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	const char *pszClassname = STRING( pev->classname );
	CBaseEntity *pNewWeapon = CBaseEntity :: Create( pszClassname, g_pGameRules->VecWeaponRespawnSpot( this ), pev->angles, pev->owner );

	if ( pNewWeapon )
	{
		pNewWeapon->pev->effects |= EF_NODRAW;// invisible for now

		// Wargon: Оружие неюзабельно.
		pNewWeapon->SetUse( NULL );
		m_iItemCaps = CBaseEntity :: ObjectCaps();

		pNewWeapon->SetTouch( NULL );// no touch
		pNewWeapon->SetThink( &CBasePlayerItem :: AttemptToMaterialize );

		DROP_TO_FLOOR( edict() );

		// not a typo! We want to know when the weapon the player just picked up should respawn!
		// this new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->AbsoluteNextThink( g_pGameRules->FlWeaponRespawnTime( this ));
	}
	else
	{
		ALERT ( at_debug, "Respawn failed to create %s!\n", STRING( pev->classname ) );
	}

	return pNewWeapon;
}


//=========================================================
// FallThink - Items that have just spawned run this think
// to catch them when they hit the ground. Once we're sure
// that the object is grounded, we change its solid type
// to trigger and set it in a large box that helps the
// player get it.
//=========================================================
void CBasePlayerItem :: FallThink ( void )
{
	SetNextThink( 0.1 );

	if ( pev->flags & FL_ONGROUND )
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if ( !FNullEnt( pev->owner ) )
		{
			int pitch = 95 + RANDOM_LONG( 0, 29 );
			EMIT_SOUND_DYN( edict(), CHAN_VOICE, "items/weapondrop1.wav", 1, ATTN_NORM, 0, pitch );	
		}

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize(); 
	}
}

void CBasePlayerItem :: KnifeDecal1( void )
{
	UTIL_StudioDecalTrace( &m_trHit, pszDecalName( 0 ));
	UTIL_TraceCustomDecal( &m_trHit, pszDecalName( 0 ));
}

void CBasePlayerItem :: KnifeDecal2( void )
{
	UTIL_StudioDecalTrace( &m_trHit, pszDecalName( 1 ));
	UTIL_TraceCustomDecal( &m_trHit, pszDecalName( 1 ));
}

BOOL CBasePlayerItem :: HasAmmo( void )
{
	BOOL bHasAmmo = 0;

	if ( pszAmmo1() )
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0);

	if ( pszAmmo2() )
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] != 0);

	if (m_iClip > 0)
		bHasAmmo |= 1;

	return bHasAmmo;
}

BOOL CBasePlayerItem :: CanDeploy( void )
{
	if( FBitSet( iFlags(), ITEM_FLAG_SELECTONEMPTY ))
		return TRUE;

	if ( !Q_stricmp( pszAmmo1(), "none" ) && !Q_stricmp( pszAmmo2(), "none" ))
	{
		// this weapon doesn't use ammo, can always deploy.
		return TRUE;
	}

	return HasAmmo();
}

BOOL CBasePlayerItem :: CanHolster( void )
{
	// can't put away while guiding a missile.
	if( m_iSpot && m_cActiveRockets )
		return FALSE;
	return TRUE;
}

void CBasePlayerItem :: Precache( void )
{
	int i;

	m_iClientAnim = -1;
	m_iClientSkin = -1;
	m_iClientBody = -1;

	if( iViewModel() != iStringNull )
		PRECACHE_MODEL( STRING( iViewModel() ));

	if( iHandModel() != iStringNull )
		PRECACHE_MODEL( STRING( iHandModel() ));

	if( iWorldModel() != iStringNull )
		PRECACHE_MODEL( STRING( iWorldModel() ));

	for( i = 0; i < sndcnt1(); i++ )
		PRECACHE_SOUND( STRING( ItemInfoArray[m_iId].shootsound1[i] ));

	for( i = 0; i < sndcnt2(); i++ )
		PRECACHE_SOUND( STRING( ItemInfoArray[m_iId].shootsound2[i] ));

	for( i = 0; i < emptycnt(); i++ )
		PRECACHE_SOUND( STRING( ItemInfoArray[m_iId].emptysounds[i] ));

	// FIXME: add reload sounds
}

int CBasePlayerItem :: GetItemInfo( ItemInfo *p )
{
	// support for half-virtual weapons
	if( FStringNull( pev->netname ))
		pev->netname = pev->classname;

	if( ParseWeaponFile( p, STRING( pev->netname )))
	{                                                     
		GenerateID();
		ALERT( at_aiconsole, "ID %i for %s\n", m_iId, STRING( pev->netname ));
		p->iId = m_iId;
 		return 1;
	}

	return 0;
}

void CBasePlayerItem :: Spawn( void )
{
	// support for half-virtual weapons
	if( FStringNull( pev->netname ))
		pev->netname = pev->classname;

	if( !FindWeaponID( )) // get actual ID
	{
		ALERT( at_error, "No spawn function for %s\n", STRING( pev->netname ));
		UTIL_Remove( this );
		return;
	}

	Precache();            

	// init default ammo
	m_iDefaultAmmo1 = iDefaultAmmo1();
	m_iDefaultAmmo2 = iDefaultAmmo2();
	m_iHandModel = iHandModel();
          
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	pev->framerate = 1.0f;
	
	UTIL_SetOrigin( this, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero ); // pointsize until it lands on the ground.

	if( !FBitSet( ObjectCaps(), FCAP_USE_ONLY ) || FBitSet( pev->spawnflags, SF_NORESPAWN ))	
		SetTouch( DefaultTouch );
	SetThink( FallThink );

	SET_MODEL( edict(), STRING( iWorldModel( )));

	if( GetSequenceCount( ) > 1 )
		pev->sequence = 1; // set world animation

	pev->animtime = gpGlobals->time + 0.1;
	m_iSpot = 0;
	
	SetNextThink( 0.1 );
}

bool CBasePlayerItem :: FindWeaponID( void )
{
	for( int i = 0; i < m_iGlobalID; i++ )
	{
		if( FStrEq( STRING( pev->netname ), ItemInfoArray[i].pszName ))
		{               
			// already exist
			m_iId = ItemInfoArray[i].iId;
			return true;
 		}
	}

	return false;
}

void CBasePlayerItem :: GenerateID( void )
{
	if( FindWeaponID() )
		return; // already exist

	if( m_iGlobalID >= WEAPON_CUSTOM_COUNT )
	{
		ALERT( at_error, "GenerateID: unique weapon ID's is out. Limit is %i weapons\n", WEAPON_CUSTOM_COUNT );
		m_iId = 0;
		return;
	}

	m_iId = m_iGlobalID++;
}

void CBasePlayerItem :: DefaultTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer( ))
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// can I have this?
	if ( !g_pGameRules->CanHavePlayerItem( pPlayer, this ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( this );
		}
		return;
	}

	if ( pOther->AddPlayerItem( this ))
	{
		EMIT_SOUND( edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
		AttachToPlayer( pPlayer );
	}

	SUB_UseTargets( pOther, USE_TOGGLE, 0 ); // UNDONE: when should this happen?
}

void CBasePlayerItem :: SetDefaultParams( ItemInfo *II )
{
	II->iSlot = II->iPosition = 0;
	II->iViewModel = MAKE_STRING( "models/view_glock.mdl" );
	II->iHandModel = iStringNull;
	II->iWorldModel = MAKE_STRING( "models/world_glock.mdl" );
	Q_strcpy( II->szAnimExt, "onehanded" );
 	II->pszAmmo1 = II->pszAmmo2 = "none";
	II->iMaxAmmo1 = II->iMaxAmmo2 = -1;
	II->iMaxClip = -1;
	II->iFlags = II->iWeight = 0;
	II->attack1 = ATTACK_NONE;
	II->attack2 = ATTACK_NONE;
	II->fNextAttack1 = II->fNextAttack2 = 0.5f;
	memset( II->shootsound1, 0, sizeof( II->shootsound1 ));
	memset( II->shootsound2, 0, sizeof( II->shootsound2 ));
	memset( II->emptysounds, 0, sizeof( II->emptysounds ));
	II->sndcount1 = II->sndcount2 = II->emptysndcount = 0;
	II->feedback1[0].punchangle[0] = II->feedback1[0].punchangle[2] = II->feedback1[0].punchangle[2] = RandomRange( 0.0f );
	II->feedback1[1].punchangle[0] = II->feedback1[1].punchangle[2] = II->feedback1[1].punchangle[2] = RandomRange( 0.0f );
	II->feedback2[0].punchangle[0] = II->feedback2[0].punchangle[2] = II->feedback2[0].punchangle[2] = RandomRange( 0.0f );
	II->feedback2[1].punchangle[0] = II->feedback2[1].punchangle[2] = II->feedback2[1].punchangle[2] = RandomRange( 0.0f );
	II->feedback1[0].recoil = II->feedback1[1].recoil = RandomRange( 0.0f );
	II->feedback2[0].recoil = II->feedback2[1].recoil = RandomRange( 0.0f );
	II->plr_settings[0].jumpHeight = -1.0f;
	II->plr_settings[1].jumpHeight = -1.0f;
	II->plr_settings[0].maxSpeed = 0;
	II->plr_settings[1].maxSpeed = 0;
	II->iVolume = NORMAL_GUN_VOLUME;
	II->iFlash = NORMAL_GUN_FLASH;
	II->recoil1 = RandomRange( 0.0f );
	II->recoil2 = RandomRange( 0.0f );
	II->vecThrowOffset = g_vecZero;
	II->spread1[0].range = RandomRange( 6.0f, 6.0f );
	II->spread1[0].type = SPREAD_LINEAR;
	II->spread1[0].expand = 0.25f;
	II->spread1[1].range = RandomRange( 2.0f, 4.0f );
	II->spread1[1].type = SPREAD_LINEAR;
	II->spread1[1].expand = 0.2f;
	II->spread2[0].range = RandomRange( 6.0f, 6.0f );
	II->spread2[0].type = SPREAD_LINEAR;
	II->spread2[0].expand = 0.25f;
	II->spread2[1].range = RandomRange( 2.0f, 4.0f );
	II->spread2[1].type = SPREAD_LINEAR;
	II->spread2[1].expand = 0.2f;
	II->spreadtime = 1.5f;
	m_iDefaultAmmo1 = 0;
	m_iDefaultAmmo2 = 0;
	m_iHandModel = 0;
	m_iId = 0; // will be overwritten with GenerateID()
}

int CBasePlayerItem :: ParseWeaponFile( ItemInfo *II, const char *filename )
{
	char path[256];
	int iResult = 0;	

	Q_snprintf( path, sizeof( path ), "scripts/weapons/%s", filename );
	COM_DefaultExtension( path, ".txt" );

	char *afile = (char *)LOAD_FILE( path, NULL );
	SetDefaultParams( II );
	
	if( !afile )
	{
 		ALERT( at_warning, "weapon info file for %s not found! Entity removed from map.\n", STRING( pev->netname ));
 		UTIL_Remove( this );
 		return 0;
	}
	else
	{
		II->pszName = STRING( pev->netname );
		ALERT( at_aiconsole, "parse %s.txt\n", II->pszName );
		// parses the type, moves the file pointer
		iResult = ParseWeaponData( II, afile );
		ALERT( at_aiconsole, "Parsing: WeaponData{} %s\n", iResult ? "OK" : "ERROR" );
		iResult = ParsePrimaryAttack( II, afile );
		ALERT( at_aiconsole, "Parsing: PrimaryAttack{} %s\n", iResult ? "OK" : "ERROR" );
		iResult = ParseSecondaryAttack( II, afile );
		ALERT( at_aiconsole, "Parsing: SecondaryAttack{} %s\n", iResult ? "OK" : "ERROR" );
		iResult = ParseSoundData( II, afile );
		ALERT( at_aiconsole, "Parsing: SoundData{} %s\n", iResult ? "OK" : "ERROR" );
		FREE_FILE( afile );

		return 1;
 	}
}

int CBasePlayerItem :: ParseItemFlags( char *pfile )
{
	char token[256];
	int iFlags = 0;

	if( !pfile || !*pfile )
		return iFlags;

	while( pfile != NULL )
	{
		pfile = COM_ParseLine( pfile, token );

		if( !Q_stricmp( token, "SelectOnEmpty" )) 
			iFlags |= ITEM_FLAG_SELECTONEMPTY;
		else if( !Q_stricmp( token, "NoAutoReload" )) 
			iFlags |= ITEM_FLAG_NOAUTORELOAD;
		else if( !Q_stricmp( token, "NoAutoSwitch" )) 
			iFlags |= ITEM_FLAG_NOAUTOSWITCHEMPTY;
		else if( !Q_stricmp( token, "LimitInWorld" )) 
			iFlags |= ITEM_FLAG_LIMITINWORLD;
		else if( !Q_stricmp( token, "Exhaustible" )) 
			iFlags |= ITEM_FLAG_EXHAUSTIBLE;
		else if( !Q_stricmp( token, "NoDuplicate" )) 
			iFlags |= ITEM_FLAG_NODUPLICATE;
		else if( !Q_stricmp( token, "AutoAim" )) 
			iFlags |= ITEM_FLAG_USEAUTOAIM;
		else if( !Q_stricmp( token, "AllowFireMode" )) 
			iFlags |= ITEM_FLAG_ALLOW_FIREMODE;
		else if( !Q_stricmp( token, "UnderWater" )) 
			iFlags |= ITEM_FLAG_SHOOT_UNDERWATER;
		else if( !Q_stricmp( token, "IronSight" )) 
			iFlags |= ITEM_FLAG_IRONSIGHT;
		else if( !Q_stricmp( token, "AutoFire" ))
			iFlags |= ITEM_FLAG_AUTOFIRE;
		else if( !Q_stricmp( token, "Scope" ))
			iFlags |= ITEM_FLAG_SCOPE;
		else if( !Q_stricmp( token, "NoDrop" ))
			iFlags |= ITEM_FLAG_NODROP;
		else if( pfile && token[0] != '|' )
			ALERT( at_warning, "unknown value %s for 'item_flags'\n", token );
	}

	return iFlags;
}

char *CBasePlayerItem :: ParseViewPunch( char *pfile, feedback_t *pFeed )
{
	char token[256];

	for( int i = 0; i < 3 && pfile != NULL; i++ )
	{
		pfile = COM_ParseLine( pfile, token );
		pFeed->punchangle[i] = RandomRange( token );
	}

	return pfile;
}

int CBasePlayerItem :: ParseWeaponData( ItemInfo *II, char *pfile )
{
	char token[2048];

	while( Q_stricmp( token, "WeaponData" ))
	{
		if( !pfile ) return 0;
		pfile = COM_ParseFile( pfile, token );
	}

	while( Q_stricmp( token, "{" ))
	{
		if( !pfile ) return 0;
		pfile = COM_ParseFile( pfile, token );
	}

	pfile = COM_ParseFile( pfile, token );

	while( Q_stricmp( token, "}" ))
	{
		if( !pfile ) return 0;

		if ( !Q_stricmp( token, "viewmodel" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->iViewModel = ALLOC_STRING( token );
		}
		else if ( !Q_stricmp( token, "handmodel" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->iHandModel = ALLOC_STRING( token );
		}
		else if( !Q_stricmp( token, "playermodel" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->iWorldModel = ALLOC_STRING( token );
		}
		else if( !Q_stricmp( token, "anim_prefix" )) 
		{                                          
			pfile = COM_ParseFile( pfile, token );
			Q_strncpy( II->szAnimExt, token, sizeof( II->szAnimExt ));
		}
		else if( !Q_stricmp( token, "bucket" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->iSlot = Q_atoi( token );  
		} 
		else if( !Q_stricmp( token, "bucket_position" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->iPosition = Q_atoi( token ); 
		} 
		else if( !Q_stricmp( token, "clip_size" )) 
		{
  			pfile = COM_ParseFile( pfile, token );
			if( !Q_stricmp( token, "noclip" ))
				II->iMaxClip = WEAPON_NOCLIP; 
			else II->iMaxClip = Q_atoi( token ); 
  	 	}  
		else if( !Q_stricmp( token, "primary_ammo" )) 
		{
  			pfile = COM_ParseFile( pfile, token );    

			if( Q_stricmp( token, "none" ))
			{
				AmmoInfo *pAmmo = UTIL_FindAmmoType( token );
				if( pAmmo ) II->iMaxAmmo1 = pAmmo->iMaxCarry;
				else ALERT( at_error, "ParseWeaponData: unknown ammo type %s in 'primary_ammo'\n", token );
			}
			else II->iMaxAmmo1 = -1;

			II->pszAmmo1 = STRING( ALLOC_STRING( token ));
		}
		else if( !Q_stricmp( token, "secondary_ammo" )) 
		{
  			pfile = COM_ParseFile( pfile, token );    

			if( Q_stricmp( token, "none" ))
			{
				AmmoInfo *pAmmo = UTIL_FindAmmoType( token );
				if( pAmmo ) II->iMaxAmmo2 = pAmmo->iMaxCarry;
				else ALERT( at_error, "ParseWeaponData: unknown ammo type %s in 'secondary_ammo'\n", token );
			}
			else II->iMaxAmmo2 = -1;

			II->pszAmmo2 = STRING( ALLOC_STRING( token ));
		} 
		else if( !Q_stricmp( token, "defaultammo" ) || !Q_stricmp( token, "defaultammo1" )) 
		{
  			pfile = COM_ParseFile( pfile, token );
			II->iDefaultAmmo1 = RandomRange( token );
		}
		else if( !Q_stricmp( token, "defaultammo2" )) 
		{
  			pfile = COM_ParseFile( pfile, token );
			II->iDefaultAmmo2 = RandomRange( token );
		}
		else if( !Q_stricmp( token, "weight" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->iWeight = Q_atoi( token ); 
		}
		else if( !Q_stricmp( token, "ThrowOffset" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			UTIL_StringToVector( II->vecThrowOffset, token );
		}
		else if( !Q_stricmp( token, "volume" )) 
		{
			pfile = COM_ParseFile( pfile, token );

			if( !Q_stricmp( token, "none" )) 
				II->iVolume = NO_GUN_VOLUME; 
			else if( !Q_stricmp( token, "quiet" )) 
				II->iVolume = QUIET_GUN_VOLUME; 
			else if( !Q_stricmp( token, "normal" )) 
				II->iVolume = NORMAL_GUN_VOLUME; 
			else if( !Q_stricmp( token, "loud" )) 
				II->iVolume = LOUD_GUN_VOLUME; 
		}
		else if( !Q_stricmp( token, "flash" )) 
		{
			pfile = COM_ParseFile( pfile, token );

			if( !Q_stricmp( token, "none" )) 
				II->iVolume = NO_GUN_FLASH; 
			else if( !Q_stricmp( token, "dim" )) 
				II->iVolume = DIM_GUN_FLASH; 
			else if( !Q_stricmp( token, "normal" )) 
				II->iVolume = NORMAL_GUN_FLASH; 
			else if( !Q_stricmp( token, "bright" )) 
				II->iVolume = BRIGHT_GUN_FLASH; 
		}
		else if( !Q_stricmp( token, "SpreadTime" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->spreadtime = Q_atof( token ); 
		}
		else if( !Q_stricmp( token, "MaxSpeed" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->plr_settings[0].maxSpeed = Q_atof( token ); 
		}
		else if( !Q_stricmp( token, "JumpHeight" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->plr_settings[0].jumpHeight = Q_atof( token ); 
		}
		else if( !Q_stricmp( token, "MaxSpeedIS" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->plr_settings[1].maxSpeed = Q_atof( token ); 
		}
		else if( !Q_stricmp( token, "JumpHeightIS" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->plr_settings[1].jumpHeight = Q_atof( token ); 
		}
		else if( !Q_stricmp( token, "item_flags" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->iFlags = ParseItemFlags( token ); 
		}

 		pfile = COM_ParseFile( pfile, token );
 	}

	return 1;
}

int CBasePlayerItem :: ParsePrimaryAttack( ItemInfo *II, char *pfile )
{
	char token[256];

	while( Q_stricmp( token, "PrimaryAttack" ))
	{
		if( !pfile ) return 0;
		pfile = COM_ParseFile( pfile, token );
	}

	while( Q_stricmp( token, "{" ))
	{
		if( !pfile ) return 0;
		pfile = COM_ParseFile( pfile, token );
	}
	
	pfile = COM_ParseFile( pfile, token );

	while( Q_stricmp( token, "}" ))
	{
		if( !pfile ) return 0;

		if( !Q_stricmp( token, "action" )) 
		{
  			pfile = COM_ParseFile( pfile, token );    
			if( !Q_stricmp( token, "none" ))
				II->attack1 = ATTACK_NONE;
			else if( !Q_stricmp( token, "ammo1" ))
				II->attack1 = ATTACK_AMMO1;
			else if( !Q_stricmp( token, "ammo2" ))
				II->attack1 = ATTACK_AMMO2;
			else if( !Q_stricmp( token, "laserdot" ))
				II->attack1 = ATTACK_LASER_DOT;
			else if( !Q_stricmp( token, "zoom" ))
				II->attack1 = ATTACK_ZOOM;
			else if( !Q_stricmp( token, "flashlight" ))
				II->attack1 = ATTACK_FLASHLIGHT;
			else if( !Q_stricmp( token, "switchmode" ))
				II->attack1 = ATTACK_SWITCHMODE;
			else if( !Q_stricmp( token, "swing" ))
				II->attack1 = ATTACK_SWING;
			else if( !Q_stricmp( token, "ironsight" ))
				II->attack1 = ATTACK_IRONSIGHT;
			else if( !Q_stricmp( token, "scope" ))
				II->attack1 = ATTACK_SCOPE;
			else II->attack1 = ALLOC_STRING( token ); // client command
		}
		else if( !Q_stricmp( token, "SpreadRange" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->spread1[0].range = RandomRange( token );
		}
		else if( !Q_stricmp( token, "SpreadRangeIS" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->spread1[1].range = RandomRange( token );
		}
		else if( !Q_stricmp( token, "SpreadType" )) 
		{
			pfile = COM_ParseFile( pfile, token );

			if( !Q_stricmp( "E_LINEAR", token ))
				II->spread1[0].type = SPREAD_LINEAR;
			else if( !Q_stricmp( "E_QUAD", token ))
				II->spread1[0].type = SPREAD_QUAD;
			else if( !Q_stricmp( "E_CUBE", token ))
				II->spread1[0].type = SPREAD_CUBE;
			else if( !Q_stricmp( "E_SQRT", token ))
				II->spread1[0].type = SPREAD_SQRT;
			else ALERT( at_warning, "ParsePrimaryAttack: unknown spread equalize type '%s'\n", token );
		}
		else if( !Q_stricmp( token, "SpreadTypeIS" )) 
		{
			pfile = COM_ParseFile( pfile, token );

			if( !Q_stricmp( "E_LINEAR", token ))
				II->spread1[1].type = SPREAD_LINEAR;
			else if( !Q_stricmp( "E_QUAD", token ))
				II->spread1[1].type = SPREAD_QUAD;
			else if( !Q_stricmp( "E_CUBE", token ))
				II->spread1[1].type = SPREAD_CUBE;
			else if( !Q_stricmp( "E_SQRT", token ))
				II->spread1[1].type = SPREAD_SQRT;
			else ALERT( at_warning, "ParsePrimaryAttack: unknown spread equalize type '%s'\n", token );
		}
		else if( !Q_stricmp( token, "SpreadExpand" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->spread1[0].expand = Q_atof( token );
		}
		else if( !Q_stricmp( token, "SpreadExpandIS" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->spread1[1].expand = Q_atof( token );
		}
		else if( !Q_stricmp( token, "PunchAngle" )) 
		{
			pfile = ParseViewPunch( pfile, &II->feedback1[0] );
		}
		else if( !Q_stricmp( token, "PunchAngleIS" )) 
		{
			pfile = ParseViewPunch( pfile, &II->feedback1[1] );
		}
		else if( !Q_stricmp( token, "recoil" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->recoil1 = RandomRange( token );
		}
		else if( !Q_stricmp( token, "nextattack" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->fNextAttack1 = Q_atof( token );
		}
		else if( !Q_stricmp( token, "SmashDecal" ))
		{
			pfile = COM_ParseFile( pfile, token );
			II->smashDecals[0] = ALLOC_STRING( token );
		}

		pfile = COM_ParseFile( pfile, token );
	}

	return 1;
}

int CBasePlayerItem :: ParseSecondaryAttack( ItemInfo *II, char *pfile )
{
	char token[256];

	while( Q_stricmp( token, "SecondaryAttack" ))
	{
		if( !pfile ) return 0;
		pfile = COM_ParseFile( pfile, token );
	}

	while( Q_stricmp( token, "{" ))
	{
		if( !pfile ) return 0;
		pfile = COM_ParseFile( pfile, token );
	}
	
	pfile = COM_ParseFile( pfile, token );

	while( Q_stricmp( token, "}" ))
	{
		if( !pfile ) return 0;

		if( !Q_stricmp( token, "action" )) 
		{
  			pfile = COM_ParseFile( pfile, token );    
			if( !Q_stricmp( token, "none" ))
				II->attack2 = ATTACK_NONE;
			else if( !Q_stricmp( token, "ammo1" ))
				II->attack2 = ATTACK_AMMO1;
			else if( !Q_stricmp( token, "ammo2" ))
				II->attack2 = ATTACK_AMMO2;
			else if( !Q_stricmp( token, "laserdot" ))
				II->attack2 = ATTACK_LASER_DOT;
			else if( !Q_stricmp( token, "zoom" ))
				II->attack2 = ATTACK_ZOOM;
			else if( !Q_stricmp( token, "flashlight" ))
				II->attack2 = ATTACK_FLASHLIGHT;
			else if( !Q_stricmp( token, "switchmode" ))
				II->attack2 = ATTACK_SWITCHMODE;
			else if( !Q_stricmp( token, "swing" ))
				II->attack2 = ATTACK_SWING;
			else if( !Q_stricmp( token, "ironsight" ))
				II->attack2 = ATTACK_IRONSIGHT;
			else if( !Q_stricmp( token, "scope" ))
				II->attack2 = ATTACK_SCOPE;
			else II->attack1 = ALLOC_STRING( token ); // client command
		}
		else if( !Q_stricmp( token, "SpreadRange" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->spread2[0].range = RandomRange( token );
		}
		else if( !Q_stricmp( token, "SpreadRangeIS" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->spread2[1].range = RandomRange( token );
		}
		else if( !Q_stricmp( token, "SpreadType" )) 
		{
			pfile = COM_ParseFile( pfile, token );

			if( !Q_stricmp( "E_LINEAR", token ))
				II->spread2[0].type = SPREAD_LINEAR;
			else if( !Q_stricmp( "E_QUAD", token ))
				II->spread2[0].type = SPREAD_QUAD;
			else if( !Q_stricmp( "E_CUBE", token ))
				II->spread2[0].type = SPREAD_CUBE;
			else if( !Q_stricmp( "E_SQRT", token ))
				II->spread2[0].type = SPREAD_SQRT;
			else ALERT( at_warning, "ParseSecondaryAttack: unknown spread equalize type '%s'\n", token );
		}
		else if( !Q_stricmp( token, "SpreadTypeIS" )) 
		{
			pfile = COM_ParseFile( pfile, token );

			if( !Q_stricmp( "E_LINEAR", token ))
				II->spread2[1].type = SPREAD_LINEAR;
			else if( !Q_stricmp( "E_QUAD", token ))
				II->spread2[1].type = SPREAD_QUAD;
			else if( !Q_stricmp( "E_CUBE", token ))
				II->spread2[1].type = SPREAD_CUBE;
			else if( !Q_stricmp( "E_SQRT", token ))
				II->spread2[1].type = SPREAD_SQRT;
			else ALERT( at_warning, "ParseSecondaryAttack: unknown spread equalize type '%s'\n", token );
		}
		else if( !Q_stricmp( token, "SpreadExpand" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->spread2[0].expand = Q_atof( token );
		}
		else if( !Q_stricmp( token, "SpreadExpandIS" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->spread2[1].expand = Q_atof( token );
		}
		else if( !Q_stricmp( token, "PunchAngle" )) 
		{
			pfile = ParseViewPunch( pfile, &II->feedback2[0] );
		}
		else if( !Q_stricmp( token, "PunchAngleIS" )) 
		{
			pfile = ParseViewPunch( pfile, &II->feedback2[1] );
		}
		else if( !Q_stricmp( token, "recoil" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->recoil2 = RandomRange( token );
		}
		else if( !Q_stricmp( token, "nextattack" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			II->fNextAttack2 = Q_atof( token );
		}
		else if( !Q_stricmp( token, "SmashDecal" ))
		{
			pfile = COM_ParseFile( pfile, token );
			II->smashDecals[1] = ALLOC_STRING( token );
		}

		pfile = COM_ParseFile( pfile, token );
	}

	return 1;
}

int CBasePlayerItem :: ParseSoundData( ItemInfo *II, char *pfile )
{
	char token[256];
	int i = 0;
	int j = 0;

	while( Q_stricmp( token, "SoundData" ))
	{
		if( !pfile ) return 0;
		pfile = COM_ParseFile( pfile, token );
	}

	while( Q_stricmp( token, "{" ))
	{
		if( !pfile ) return 0;
		pfile = COM_ParseFile( pfile, token );
	}

	pfile = COM_ParseFile( pfile, token );

	while( Q_stricmp( token, "}" ))
	{
		if( !pfile ) return 0;

		if( !Q_stricmp( token, "shootsound1" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			if( II->sndcount1 < MAX_SHOOTSOUNDS ) 
			{
				II->shootsound1[II->sndcount1] = ALLOC_STRING( token );
				PRECACHE_SOUND( STRING( II->shootsound1[II->sndcount1] ));
				II->sndcount1++;
			}
			else ALERT( at_warning, "Too many shoot sounds for %s\n", STRING( pev->netname ));
		}
		else if ( !stricmp( token, "shootsound2" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			if( II->sndcount2 < MAX_SHOOTSOUNDS ) 
			{
				II->shootsound2[II->sndcount2] = ALLOC_STRING( token );
				PRECACHE_SOUND( STRING( II->shootsound2[II->sndcount2] ));
				II->sndcount2++;
			}
			else ALERT( at_warning, "Too many shoot sounds for %s\n", STRING( pev->netname ));
		}
		else if( !stricmp( token, "emptysound" )) 
		{
			pfile = COM_ParseFile( pfile, token );
			if( II->emptysndcount < MAX_SHOOTSOUNDS ) 
			{
				II->emptysounds[II->emptysndcount] = ALLOC_STRING( token );
				PRECACHE_SOUND( STRING( II->emptysounds[II->emptysndcount] ));
				II->emptysndcount++;
			}
			else ALERT( at_warning, "Too many empty sounds for %s\n", STRING( pev->netname ));
		}

		pfile = COM_ParseFile( pfile, token );
	}

	return 1;
}

int CBasePlayerItem :: GetAnimation( Activity activity )
{
	if( !FStrEq( STRING( pev->model ), STRING( iViewModel( ))))
		return -1;

	if( m_iIronSight )
	{
		// translate activity in case Iron Sight is enabled
		Activity new_activity = GetIronSightActivity( activity );

		// make sure what iron sight activity is found
		int iAnim = LookupActivity( new_activity );

		if( iAnim != -1 ) return iAnim;
		// fallback to normal activity
	}

	return LookupActivity( activity );
}

int CBasePlayerItem :: SetAnimation( Activity activity, float fps )
{
	int iSequence = GetAnimation( activity );

	if( iSequence != -1 ) SendWeaponAnim( iSequence, fps );
	else ALERT( at_aiconsole, "%s not found\n", activity_map[activity - 1].name );

	return iSequence;
} 

int CBasePlayerItem :: SetAnimation( char *name, float fps )
{
	if( !FStrEq( STRING( pev->model ), STRING( iViewModel( ))))
		return -1;

	int iSequence = LookupSequence( name );

	if( iSequence != -1 ) SendWeaponAnim( iSequence, fps );
	else ALERT( at_aiconsole, "sequence \"%s\" not found\n", name );

	return iSequence;
}

void CBasePlayerItem :: SendWeaponAnim( int iAnim, float framerate )
{
	if( iAnim == -1 ) return; // sequence not found

	m_pPlayer->pev->weaponanim = iAnim;
	pev->framerate = framerate;
	SetNextIdle( SequenceDuration( )); // auto-update idle activity
	m_iClientAnim = -1;	// force to send new sequence
}

Activity CBasePlayerItem :: GetIronSightActivity( Activity act )
{
	switch( act )
	{
	case ACT_VM_IDLE: return ACT_VM_IDLE_IS;
	case ACT_VM_IDLE_EMPTY: return ACT_VM_IDLE_EMPTY_IS;
	case ACT_VM_RANGE_ATTACK: return ACT_VM_RANGE_ATTACK_IS;
	case ACT_VM_MELEE_ATTACK: return ACT_VM_MELEE_ATTACK_IS;
	case ACT_VM_SHOOT_LAST: return ACT_VM_SHOOT_LAST_IS;
	case ACT_VM_LAST_MELEE_ATTACK: return ACT_VM_LAST_MELEE_ATTACK_IS;
	case ACT_VM_SHOOT_EMPTY: return ACT_VM_SHOOT_EMPTY_IS;
	case ACT_VM_RELOAD_EMPTY: return ACT_VM_RELOAD_EMPTY_IS;
	case ACT_VM_PUMP_EMPTY: return ACT_VM_PUMP_EMPTY_IS;
	case ACT_VM_RELOAD: return ACT_VM_RELOAD_IS;
	case ACT_VM_PUMP: return ACT_VM_PUMP_IS;
	}

	return act; // default
}

void CBasePlayerItem :: ApplyPlayerSettings( bool bReset )
{
	if (bReset)
	{
		// buz: Paranoia's speed adjustment
		// return player speed to normal
		m_pPlayer->pev->maxspeed = gSkillData.plrPrimaryMaxSpeed;

		// buz: set jump force
		m_pPlayer->SetJumpHeight( 100.0f );
	}
	else
	{
		// buz: Paranoia's speed adjustment
		if (ClientMaxSpeed( )) m_pPlayer->pev->maxspeed = ClientMaxSpeed();
		else m_pPlayer->pev->maxspeed = gSkillData.plrPrimaryMaxSpeed;

		// buz: set jump force
		if (ClientJumpHeight() != -1.0f)
			m_pPlayer->SetJumpHeight( ClientJumpHeight( ));
		else m_pPlayer->SetJumpHeight( 100.0f );
	}
}

//=========================================================
//	Spread system from Paranoia
//=========================================================
Vector CBasePlayerItem :: GetConeVectorForDegree( int degree )
{
	switch( degree )
	{
	case 0: return g_vecZero;
	case 1: return VECTOR_CONE_1DEGREES;
	case 2: return VECTOR_CONE_2DEGREES;
	case 3: return VECTOR_CONE_3DEGREES;
	case 4: return VECTOR_CONE_4DEGREES;
	case 5: return VECTOR_CONE_5DEGREES;
	case 6: return VECTOR_CONE_6DEGREES;
	case 7: return VECTOR_CONE_7DEGREES;
	case 8: return VECTOR_CONE_8DEGREES;
	case 9: return VECTOR_CONE_9DEGREES;
	case 10: return VECTOR_CONE_10DEGREES;
	case 15: return VECTOR_CONE_15DEGREES;
	case 20: return VECTOR_CONE_20DEGREES;
	}

	return g_vecZero;
}

void CBasePlayerItem :: DoEqualizeSpread( int type, float &spread )
{
	switch( type )
	{
	case SPREAD_QUAD:
		spread = spread * spread;
		break;
	case SPREAD_CUBE:
		spread = spread * spread * spread;
		break;
	case SPREAD_SQRT:
		spread = sqrt( spread );
		break;
	}
}

// returns spread expand power [0..1] based on current time
float CBasePlayerItem :: CalcSpread( void )
{
	float decay = ( gpGlobals->time - m_flLastShotTime ) / ( m_flSpreadTime * m_flLastSpreadPower );

	if( decay > 1.0f ) return 0.0f;

	return ( 1.0f - decay ) * m_flLastSpreadPower;
}

float CBasePlayerItem :: ExpandSpread( float expandPower )
{
	// buz: in ducking more accuracy
	// g-cont. make sure what is not a duck in the jump
	if( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) && FBitSet( pev->flags, FL_ONGROUND ))
		expandPower *= 0.7f;

	float curspread = CalcSpread();

	m_flLastShotTime = gpGlobals->time;
	m_flLastSpreadPower = curspread + expandPower;

	if( m_flLastSpreadPower > 2.0f )
		m_flLastSpreadPower = 2.0f;
	
	return curspread;
}

Vector CBasePlayerItem :: GetSpreadVec( void )
{
	const spread_t *info = pSpread1(); // auto-select between normal and IronSight settings

	float spread = CalcSpread();
	DoEqualizeSpread( info->type, spread );

	Vector minspread = GetConeVectorForDegree( (int)info->range.m_flMin );
	Vector addspread = GetConeVectorForDegree( (int)info->range.m_flMax );
	Vector vecSpread = minspread + ( addspread * spread );

	vecSpread.z = spread; // scale

	return vecSpread;
}

Vector CBasePlayerItem :: CalcSpreadVec( const spread_t *info, float &spread )
{
	spread = ExpandSpread( info->expand );

	DoEqualizeSpread( info->type, spread );
	Vector minspread = GetConeVectorForDegree( (int)info->range.m_flMin );
	Vector addspread = GetConeVectorForDegree( (int)info->range.m_flMax );

	return ( minspread + ( addspread * spread ));
}

void CBasePlayerItem :: PlayerJump( void )
{
	ExpandSpread( 0.7f );
}

void CBasePlayerItem :: PlayerRun( void )
{
	ExpandSpread( 0.1f );
}

void CBasePlayerItem :: PlayerWalk( void )
{
	ExpandSpread( 0.03f );
}

//=========================================================
//	Zoom In\Out
//=========================================================
void CBasePlayerItem :: ZoomUpdate( void )
{
	BOOL m_bUseZoom = FALSE;

	if( iAttack1() == ATTACK_ZOOM && FBitSet( m_pPlayer->pev->button, IN_ATTACK ))
		m_bUseZoom = TRUE;

	if( iAttack2() == ATTACK_ZOOM && FBitSet( m_pPlayer->pev->button, IN_ATTACK2 ))
		m_bUseZoom = TRUE;

	if( m_bUseZoom )
	{
		if( m_iZoom == NOT_IN_ZOOM )
		{
			if( m_flHoldTime > UTIL_WeaponTimeBase( ))
				return;

			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/zoom.wav", 1, ATTN_NORM );
			m_flTimeUpdate = UTIL_WeaponTimeBase() + 0.8;
			m_iZoom = NORMAL_ZOOM;
		}

		if( m_iZoom == NORMAL_ZOOM )
		{
			m_pPlayer->m_iFOV = ZOOM_DEFAULT; // FIXME: get zoom values from weapon settings
			m_pPlayer->pev->viewmodel = iStringNull;
			m_iZoom = UPDATE_ZOOM; // ready to zooming, wait for 0.8 secs
		}

		if( m_iZoom == UPDATE_ZOOM && m_pPlayer->m_iFOV > ZOOM_MAXIMUM )
		{
			if( m_flTimeUpdate < UTIL_WeaponTimeBase( ))
			{
				m_flTimeUpdate = UTIL_WeaponTimeBase() + 0.02f;
				m_pPlayer->m_iFOV--;
			}
		}

		if( m_iZoom == RESET_ZOOM )
			ZoomReset();
	}
	else if( m_iZoom > NORMAL_ZOOM )
	{
		m_iZoom = RESET_ZOOM;
	}

#if 0
	MESSAGE_BEGIN( MSG_ONE, gmsgZoomHUD, NULL, m_pPlayer->pev );
		WRITE_BYTE( m_iZoom );
	MESSAGE_END();
#endif
}

void CBasePlayerItem :: ZoomReset( void )
{
	// return viewmodel
	if( m_iZoom != NOT_IN_ZOOM )
	{
		m_pPlayer->pev->viewmodel = iViewModel();
		m_pPlayer->pev->iuser3 = iHandModel();
		m_flHoldTime = UTIL_WeaponTimeBase() + 0.5;
		m_pPlayer->m_iFOV = 90;
		m_iZoom = NOT_IN_ZOOM; // clear zoom
#if 0
		MESSAGE_BEGIN( MSG_ONE, gmsgZoomHUD, NULL, m_pPlayer->pev );
			WRITE_BYTE( m_iZoom );
		MESSAGE_END();
#endif
		// update client data manually
		m_pPlayer->UpdateClientData();
	}
}

//=========================================================
//	generic base functions
//=========================================================
BOOL CBasePlayerItem :: DefaultDeploy( Activity sequence )
{                                                       
	m_iClientAnim = -1;
	m_iClientSkin = -1;
	m_iClientBody = -1;

	// init spread system
	m_flSpreadTime = fSpreadTime();
	m_flLastSpreadPower = 0.0f;
	m_flLastShotTime = 0.0f;

	if( PLAYER_HAS_SUIT )
		pev->body |= MILITARY_SUIT;
	else pev->body &= ~MILITARY_SUIT;

	m_pPlayer->pev->viewmodel = iViewModel();
	m_pPlayer->pev->iuser3 = iHandModel();
	m_pPlayer->pev->weaponmodel = iWorldModel();
	Q_strncpy( m_pPlayer->m_szAnimExtention, szAnimExt(), sizeof( m_pPlayer->m_szAnimExtention ));

	SET_MODEL( edict(), STRING( iViewModel( )));

	float fps = 1.0f;

	if( g_pGameRules->IsMultiplayer( ))
		fps *= 1.5f; // speed up 1.5x

	if ( SetAnimation( sequence, fps ) != -1 )
	{
		// make some delay before idle playing
		SetNextAttack( SequenceDuration() - 0.1f );
		return TRUE;        
	}

	// animation missed
	return FALSE;
}

BOOL CBasePlayerItem :: DefaultHolster( Activity sequence, bool force )
{
	m_fInReload = FALSE;
	int iResult = 0;

	if( m_pSpot )
	{
		// disable laser dot
		EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM, "weapons/spot_off.wav", 1, ATTN_NORM );
		m_pSpot->Killed( NULL, GIB_NEVER );
		m_pSpot = NULL;
	}

	m_iSkin = 0; // reset screen
	ZoomReset();

	if( iAttack1() == ATTACK_FLASHLIGHT || iAttack2() == ATTACK_FLASHLIGHT )
		ClearBits( m_pPlayer->pev->effects, EF_DIMLIGHT ); // FIXME: create new flag for weapon flashlight

	// disbale IronSight before switching to next weapon
	if( m_iIronSight )
	{
		m_iIronSight = 0;

		// can play full animation
		if( !force )
		{
			int iAnim = -1;
			if( !HasAmmo( )) iAnim = GetAnimation( ACT_VM_IRONSIGHT_OFF_EMPTY );
			if( iAnim == -1 ) iAnim = GetAnimation( ACT_VM_IRONSIGHT_OFF );
			SendWeaponAnim( iAnim );
			SetNextAttack( SequenceDuration() + 0.1f );
			m_fWaitForHolster = TRUE; // queue enabled
			return 0;
		}
	}

	// disable queue
	m_fWaitForHolster = FALSE;

	float fps = 1.0f;

	if( g_pGameRules->IsMultiplayer( ))
		fps *= 2.5f;

	iResult = SetAnimation( sequence, fps );
	if( iResult != -1 )
	{
		// delay before switching      
		SetNextAttack( SequenceDuration() + 0.1f );
          	iResult = 1;
          }

	if( FBitSet( iFlags(), ITEM_FLAG_EXHAUSTIBLE ) && !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] && !m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] )
	{
		// no more ammo!
		ClearBits( m_pPlayer->pev->weapons, BIT( m_iId ));
		m_pPlayer->pev->viewmodel = iStringNull;
		m_pPlayer->pev->weaponmodel = iStringNull;
		SetThink( DestroyItem );
		SetNextThink( 0.5 );
	}

	// animation not found
	return iResult;
}

void CBasePlayerItem :: DefaultIdle( void )
{ 
	// weapon have clip and ammo or just have ammo
	if(( iMaxClip() && m_iClip ) || m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 || iMaxAmmo1() == -1 )
	{
		// play random idle animation
		float flRand = RANDOM_FLOAT( 0, 1.0f );
		if( flRand < 0.5f ) SetAnimation( ACT_VM_IDLE );
		else m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10.0f, 15.0f );
	}
	else
	{
		SetAnimation( ACT_VM_IDLE_EMPTY );
	}
}

BOOL CBasePlayerItem :: DefaultReload( Activity sequence )
{
	if( m_flNextPrimaryAttack > UTIL_WeaponTimeBase( ))
		return FALSE;

	if( m_flNextSecondaryAttack > UTIL_WeaponTimeBase( ))
		return FALSE;

	if( m_cActiveRockets && m_iSpot )
	{
		// no reloading when there are active missiles tracking the designator.
		// ward off future autoreload attempts by setting next attack time into the future for a bit. 
		SetNextAttack( 0.5f );
		return FALSE;
	}

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 ) // have ammo?
	{
		if( iMaxClip() == m_iClip )
			return FALSE;

		if( m_iStepReload == NOT_IN_RELOAD )
		{
			if( SetAnimation( ACT_VM_START_RELOAD ) != -1 )
			{
				// found anim, continue
				m_iStepReload = START_RELOAD;
				m_fInReload = TRUE; // disable reload button
				return TRUE; // start reload cycle. See also ItemPostFrame 
			}
			else // init default reload
			{
				int i = Q_min( iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] );	
				if( i == 0 ) return FALSE;
				int iResult = -1;

				ZoomReset(); // reset zoom

				if( m_iClip <= 0 ) // empty clip ?
				{
					// iResult is error code
					iResult = SetAnimation( ACT_VM_RELOAD_EMPTY );
					m_iStepReload = EMPTY_RELOAD; // it's empty reload
				}

				if( iResult == -1 ) 
				{
					SetAnimation( sequence );
					m_iStepReload = NORMAL_RELOAD; // not empty reload or sequence not found
				}

				if( m_pSpot ) m_pSpot->Suspend( SequenceDuration( )); // suspend laserdot
				SetNextAttack( SequenceDuration( ));
				m_fInReload = TRUE; // disable reload button
				
				return TRUE;
			}
		}
		else if( m_iStepReload == START_RELOAD )
		{
			// continue step reload
			if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase( ))
				return FALSE;

			// was waiting for gun to move to side
			SetAnimation( sequence );
			m_iStepReload = CONTINUE_RELOAD;
		}
		else if( m_iStepReload == CONTINUE_RELOAD )
		{
			// Add them to the clip
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
			m_iStepReload = START_RELOAD;
			m_iClip++;

			if( m_iClip == iMaxClip( ))
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		}

		return TRUE;
	}   

	return FALSE;
}

BOOL CBasePlayerItem :: DefaultSwing( int primary )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32.0f;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &tr );

		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				UTIL_FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos; // This is the point on the actual surface (the hull could have hit space)
		}
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	if ( tr.flFraction >= 1.0f )
	{
		if( emptycnt( ))
			EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, STRING( EmptySnd( )), 1.0, ATTN_NORM );

		if (primary)
		{ 
			// HACKHACK: ACT_VM_SHOOT_EMPTY_IS as miss attack for knife
			if( LookupActivity( ACT_VM_SHOOT_EMPTY_IS ) != ACTIVITY_NOT_AVAILABLE )
				SetAnimation( ACT_VM_SHOOT_EMPTY_IS );
			else SetAnimation( ACT_VM_RANGE_ATTACK );
		}
		else SetAnimation( ACT_VM_SHOOT_EMPTY );
	}
	else
	{
		if (primary) SetAnimation( ACT_VM_RANGE_ATTACK );
		else SetAnimation( ACT_VM_MELEE_ATTACK );

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage( );

		// Wargon: Исправлено гибание ножем.
		if( primary ) pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_CLUB | DMG_NEVERGIB );
		else pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbarSec, gpGlobals->v_forward, &tr, DMG_CLUB | DMG_NEVERGIB );

		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		// play thwack, smack, or dong sound
		float flVol = 1.0f;
		int fHitWorld = TRUE;       

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				if( primary )
				{
					if( sndcnt1( ))
						EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, STRING( ShootSnd1( )), 1.0, ATTN_NORM );
				}
				else
				{
					// FIXME: hardcoded sound
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_stab.wav", 1, ATTN_NORM);
				}

				m_pPlayer->m_iWeaponVolume = KNIFE_BODYHIT_VOLUME;

				if ( !pEntity->IsAlive() )
					return TRUE;
				else flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line
		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + (vecEnd-vecSrc) * 2.0f, BULLET_STAB );            

			if( sndcnt2( ))
			{
				const char *pszSound = STRING( ShootSnd2());

				EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, pszSound, fvolbar, ATTN_NORM, 0, RANDOM_LONG( 98, 102 ));
			}
		}

		// delay the decal a bit
		m_trHit = tr;

		m_pPlayer->m_iWeaponVolume = flVol * KNIFE_WALLHIT_VOLUME;

		if (primary)
		{
			SetThink( &CBasePlayerItem :: KnifeDecal1 );
			SetNextThink( 0.1f );
		}
		else
		{
			SetThink( &CBasePlayerItem :: KnifeDecal2 );
			SetNextThink( 0.15f );
		}		
	}

	return fDidHit;
}

BOOL CBasePlayerItem :: PlayEmptySound( void )
{
	if( m_iPlayEmptySound )
	{
		if( emptycnt( ))
			EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, STRING( EmptySnd( )), 0.8, ATTN_NORM );
		m_iPlayEmptySound = 0;
		return TRUE;
	}

	return FALSE;
}

void CBasePlayerItem :: ResetEmptySound( void )
{
	m_iPlayEmptySound = 1;
}

void CBasePlayerItem :: PlayAttackSound( int primary )
{
	if( primary )
	{
		if( sndcnt1( )) EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, STRING( ShootSnd1( )), 1.0, ATTN_NORM );
	}
	else
	{
		if( sndcnt2( )) EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, STRING( ShootSnd2( )), 1.0, ATTN_NORM );
	}
}

int CBasePlayerItem :: GetCurrentAttack( const char *ammo, int primary )
{
	AmmoInfo *pAmmo = UTIL_FindAmmoType( ammo );

	int iResult;

	if( !pAmmo || !Q_stricmp( ammo, "none" )) // no ammo or ammo not found
	{
		// just play animation and sound
		if( primary ) iResult = ( SetAnimation( ACT_VM_RANGE_ATTACK ) == -1 ) ? 0 : 1;
		else iResult = ( SetAnimation( ACT_VM_MELEE_ATTACK ) == -1 ) ? 0 : 1;

                    PlayAttackSound( primary );
		SetPlayerEffects();
	}
	else if( pAmmo->iMissileClassName != iStringNull )
	{
		// missile attack (throw entity)
		iResult = ThrowGeneric( ammo, primary );
	}
	else
	{
		// shoot bullets (default case)
		iResult = ShootGeneric( ammo, primary );
	}

	return iResult;
}

int CBasePlayerItem :: PlayCurrentAttack( int action, int primary )
{
	int iResult = 0;

	if( action == ATTACK_ZOOM )
		iResult = 1; // See void ZoomUpdate( void ); for details
	else if( action == ATTACK_AMMO1 )
		iResult = GetCurrentAttack( pszAmmo1(), primary );
	else if( action == ATTACK_AMMO2 )
		iResult = GetCurrentAttack( pszAmmo2(), primary );
	else if( action == ATTACK_LASER_DOT )
	{
		m_iSpot = !m_iSpot;
		if( !m_iSpot && m_pSpot )
		{
			// disable laser dot
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/spot_off.wav", 1, ATTN_NORM );
			m_pSpot->Killed( NULL, GIB_NEVER );
			m_pSpot = NULL;
			m_iSkin = 0; // disable screen
		}
		iResult = 1;
	}
	else if( action == ATTACK_FLASHLIGHT )
	{
		if( FBitSet( m_pPlayer->pev->effects, EF_DIMLIGHT ))
		{
			ClearBits( m_pPlayer->pev->effects, EF_DIMLIGHT );
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM );
		}
		else 
		{
			SetBits( m_pPlayer->pev->effects, EF_DIMLIGHT );
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM );
		}
		iResult = 1;
	}
	else if( action == ATTACK_SWITCHMODE )
	{
		if( !m_iBody )
		{
			m_iBody = 1;
			pev->button = 1; // enable custom firemode
			SetAnimation( ACT_VM_TURNON );
		}
		else
		{
			SetAnimation( ACT_VM_TURNOFF );
			pev->button = 0; // disable custom firemode
			m_iClientBody = m_iBody = 0; // make delay before change
		}
		iResult = 1;
	}
	else if( action == ATTACK_SWING )
	{
		iResult = DefaultSwing( primary );
	}
	else if( action == ATTACK_IRONSIGHT )
	{
		int iAnim = -1;

		if( !m_iIronSight )
		{
			if( !HasAmmo( )) iAnim = GetAnimation( ACT_VM_IRONSIGHT_ON_EMPTY );
			if( iAnim == -1 ) iAnim = GetAnimation( ACT_VM_IRONSIGHT_ON );
			m_iIronSight = 1;
		}
		else
		{
			if( !HasAmmo( )) iAnim = GetAnimation( ACT_VM_IRONSIGHT_OFF_EMPTY );
			if( iAnim == -1 ) iAnim = GetAnimation( ACT_VM_IRONSIGHT_OFF );
			m_iIronSight = 0;
		}

		ApplyPlayerSettings( FALSE );
		SendWeaponAnim( iAnim );
		iResult = 1;
	}
	else if( action == ATTACK_SCOPE )
	{
		int iAnim = -1;

		if( m_pPlayer->m_iGasMaskOn )
		{
			UTIL_ShowMessage( "#GAS_AND_SCOPE", m_pPlayer );
			SetNextAttack( 0.5f );
			return -1;
		}
		else if( m_pPlayer->m_iHeadShieldOn )
		{
			UTIL_ShowMessage( "#SCOPE_AND_SHIELD", m_pPlayer );
			SetNextAttack( 0.5f );
			return -1;
		}

		if( !m_iIronSight )
		{
			if( !HasAmmo( )) iAnim = GetAnimation( ACT_VM_IRONSIGHT_ON_EMPTY );
			if( iAnim == -1 ) iAnim = GetAnimation( ACT_VM_IRONSIGHT_ON );
			m_iIronSight = 1;
		}
		else
		{
			if( !HasAmmo( )) iAnim = GetAnimation( ACT_VM_IRONSIGHT_OFF_EMPTY );
			if( iAnim == -1 ) iAnim = GetAnimation( ACT_VM_IRONSIGHT_OFF );
			m_iIronSight = 0;
		}

		ApplyPlayerSettings( FALSE );
		SendWeaponAnim( iAnim );
		iResult = 1;
	}
	else if( action == ATTACK_NONE )
	{
		return -1;
	}
	else
	{
		// just command
		char command[64];

		if( !strncmp( STRING( action ), "fire ", 5 ))
		{
			char *target  = (char *)STRING( action );
			target = target + 5; // remove "fire "
			FireTargets( target, m_pPlayer, this, USE_TOGGLE, 1.0f ); // activate target
		}
		else
		{
			Q_snprintf( command, sizeof( command ), "%s\n", STRING( action ));
			SERVER_COMMAND( command );
		}		

		// just play animation and sound
		if( primary ) SetAnimation( ACT_VM_RANGE_ATTACK );
		else SetAnimation( ACT_VM_MELEE_ATTACK );

		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		PlayAttackSound( primary );
	}

	return iResult;
}

//=========================================================
//	Get Atatck Info by AmmoInfo
//=========================================================
int CBasePlayerItem :: GetAmmoType( const char *ammo )
{
	if( !Q_stricmp( ammo, pszAmmo1() ))
		return AMMO_PRIMARY; // primary ammo

	if( !Q_stricmp( ammo, pszAmmo2() ))
		return AMMO_SECONDARY; // secondary ammo

	return AMMO_UNKNOWN; // no ammo
}

int CBasePlayerItem :: UseAmmo( const char *ammo, int count )
{
	int ammoType = GetAmmoType( ammo );

	if( ammoType == AMMO_UNKNOWN )
	{
		return -1;
	}
	else if( ammoType == AMMO_PRIMARY )
	{
		if( iMaxClip() == -1 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
		{
			// noclip
			if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < count )
				count = 1;
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= count;			
		}
		else if( m_iClip > 0 )
		{
			// have clip 
			if( m_iClip < count )
				count = 1;
			m_iClip -= count;
		}
		else
		{
			// ammo is out
			return 0;
		}
	}
	else if( ammoType == AMMO_SECONDARY )
	{
		if( m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] > 0 )
		{
			if( m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] < count )
				count = 1;
			m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] -= count;
		}
		else
		{
			// ammo is out
			return 0;
		}
	}

	return 1;
}

// client effects (muzzleflash, decals, shell, etc)
void CBasePlayerItem :: PlayClientFire( const Vector &vecDir, float spread, int iAnim, int shellidx, string_t shootsnd, int cShots )
{
	int soundidx = 0;

	// params sent table:
	// args->fparam1 = vecDir.x
	// args->fparam2 = vecDir.y
	// args->iparam1 = iAnim
	// args->iparam2 = spread * 255
	// args->bparam1 = shellindex (bool expanded up to 16 bits)
	// args->bparam2 = soundindex (bool expanded up to 16 bits)

	if( shootsnd != iStringNull )
		soundidx = PRECACHE_SOUND( STRING( shootsnd ));

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), g_usShootEvent, 0.0, (float *)&g_vecZero, (float *)&g_vecZero,
	vecDir.x, vecDir.y, (iAnim & 0x0FFF)|((cShots & 0xF )<<12 ), (int)( spread * 255 ), shellidx, soundidx );
}

void CBasePlayerItem :: SetPlayerEffects( void )
{
	m_pPlayer->m_iWeaponVolume = iVolume();
	m_pPlayer->m_iWeaponFlash = iFlash();

	if( m_pPlayer->m_iWeaponFlash )
		SetBits( m_pPlayer->pev->effects, EF_MUZZLEFLASH );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 ); // player animation
}

float CBasePlayerItem :: AutoAimDelta( int primary )
{
	const spread_t *info = (primary) ? pSpread1() : pSpread2();

	return GetConeVectorForDegree( (int)info->range.m_flMax ).x;
}

int CBasePlayerItem :: ShootGeneric( const char *ammo, int primary, int cShots )
{
	if( m_pPlayer->pev->waterlevel != 3 || FBitSet( iFlags(), ITEM_FLAG_SHOOT_UNDERWATER ))
	{
		// have ammo and player not underwater
		if( !UseAmmo( ammo, cShots ))
			return 0;

		SetPlayerEffects();

		// viewmodel animation
		ZoomReset();

		int iAnim = -1;

		if( primary )
		{
			if( iMaxClip() && !m_iClip )
				iAnim = GetAnimation( ACT_VM_SHOOT_LAST );
			if( iAnim == -1 )
				iAnim = GetAnimation( ACT_VM_RANGE_ATTACK );
		}
		else
		{
			if( !m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] )
				iAnim = GetAnimation( ACT_VM_LAST_MELEE_ATTACK );
			if( iAnim == -1 )
				iAnim = GetAnimation( ACT_VM_MELEE_ATTACK );
		}

		float spread, flDistance = 8192.0f; // set max distance
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = gpGlobals->v_forward;
		Vector vecSpread = CalcSpreadVec((primary) ? pSpread1() : pSpread2(), spread );

		if( FBitSet( iFlags(), ITEM_FLAG_USEAUTOAIM ))
		{
			vecAiming = m_pPlayer->GetAutoaimVector( AutoAimDelta( primary ));
		}
		else
		{
			UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
			vecAiming = gpGlobals->v_forward;
		}

		AmmoInfo *pInfo = UTIL_FindAmmoType( ammo );
		int shellIndex = 0;
		float flDamage = 0.0f;
		int numShots = 1;

		if( pInfo != NULL )
		{
			shellIndex = pInfo->iShellIndex;
			flDamage = pInfo->flPlayerDamage;
			flDistance = pInfo->flDistance;
			numShots = pInfo->iNumShots;
		}

		Vector vecDir = m_pPlayer->FireBulletsPlayer( cShots * numShots, vecSrc, vecAiming, vecSpread, flDistance, flDamage,
		m_pPlayer->pev, m_pPlayer->random_seed );

		PlayClientFire( vecDir, spread, iAnim, shellIndex, (primary) ? ShootSnd1() : ShootSnd2( ), cShots * numShots );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT ( 10, 15 );

		return 1;
	}

	return 0;
}

int CBasePlayerItem :: ThrowGeneric( const char *ammo, int primary, int cShots )
{
	// have ammo and player not underwater
	if( !UseAmmo( ammo, cShots ))
		return 0;

	SetPlayerEffects();

	// viewmodel animation
	ZoomReset();

	int iAnim = -1;

	if( primary )
	{
		if( iMaxClip() && !m_iClip )
			iAnim = GetAnimation( ACT_VM_SHOOT_LAST );
		if( iAnim == -1 )
			iAnim = GetAnimation( ACT_VM_RANGE_ATTACK );

		if( sndcnt1( ))
			EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, STRING( ShootSnd1( )), 1.0, ATTN_NORM );
	}
	else
	{
		if( !m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] )
			iAnim = GetAnimation( ACT_VM_LAST_MELEE_ATTACK );
		if( iAnim == -1 )
			iAnim = GetAnimation( ACT_VM_MELEE_ATTACK );

		if( sndcnt2( ))
			EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, STRING( ShootSnd2( )), 1.0, ATTN_NORM );
	}

	SendWeaponAnim( iAnim );

	AmmoInfo *pInfo = UTIL_FindAmmoType( ammo );

	if( !pInfo ) return -1; // ammo not found

	float flDamage = pInfo->flPlayerDamage;
	float flDistance = pInfo->flDistance;
	float spread;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
#if 1
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * vecThrowOffset().x
	+ gpGlobals->v_right * vecThrowOffset().y + gpGlobals->v_up * vecThrowOffset().z;
#else
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * weapon_x.value
	+ gpGlobals->v_right * weapon_y.value + gpGlobals->v_up * weapon_z.value;
#endif
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSpread = CalcSpreadVec((primary) ? pSpread1() : pSpread2(), spread );

	if( FBitSet( iFlags(), ITEM_FLAG_USEAUTOAIM ))
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AutoAimDelta( primary ));
	}
	else
	{
		UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
		vecAiming = gpGlobals->v_forward;
	}

	// FIXME: this is a limitation of weapon system. Make generic tuneable projectile someday...
	if( FStrEq( STRING( pInfo->iMissileClassName ), "rpg_rocket" ))
	{
		CRpgRocket *pRocket = CRpgRocket :: CreateRpgRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this );
		pRocket->pev->velocity = vecAiming * flDistance;
		pRocket->pev->velocity += vecAiming * DotProduct( m_pPlayer->pev->velocity, vecAiming );
		pRocket->pev->dmg = flDamage;

		return 1;
	}
	else if( FStrEq( STRING( pInfo->iMissileClassName ), "grenade_vog25" ))
	{
		CGrenade *pGrenade = CGrenade :: ShootGeneric( m_pPlayer, vecSrc, vecAiming * flDistance, pInfo->iMissileClassName );
		pGrenade->pev->gravity = 0.5;// lower gravity since grenade is aerodynamic and engine doesn't know it.
		pGrenade->pev->dmg = flDamage;

		return 1;
	}

	return 0;
}

//=========================================================
// called by the new item with the existing item as parameter
//
// if we call ExtractAmmo(), it's because the player is picking up this type of weapon for 
// the first time. If it is spawned by the world, m_iDefaultAmmo will have a default ammo amount in it.
// if  this is a weapon dropped by a dying player, has 0 m_iDefaultAmmo, which means only the ammo in 
// the weapon clip comes along. 
//=========================================================
int CBasePlayerItem :: ExtractAmmo( CBasePlayerItem *pWeapon, BOOL duplicate )
{
	int iResult = 0;

	if( pszAmmo1( ))
	{
		iResult |= pWeapon->AddPrimaryAmmo( m_iDefaultAmmo1, pszAmmo1(), iMaxClip(), iMaxAmmo1(), duplicate );
		m_iDefaultAmmo1 = 0;
	}

	if( pszAmmo2( ))
	{
		iResult |= pWeapon->AddSecondaryAmmo( m_iDefaultAmmo2, pszAmmo2(), iMaxAmmo2() );
		m_iDefaultAmmo2 = 0;
	}

	return iResult;
}

//=========================================================
// called by the new item's class with the existing item as parameter
//=========================================================
int CBasePlayerItem :: ExtractClipAmmo( CBasePlayerItem *pWeapon )
{
	int	iAmmo;

	if( m_iClip == WEAPON_NOCLIP )
		iAmmo = 0; // guns with no clips always come empty if they are second-hand
	else iAmmo = m_iClip;
	
	return pWeapon->m_pPlayer->GiveAmmo( iAmmo, pszAmmo1()); // , &m_iPrimaryAmmoType
}

BOOL CBasePlayerItem :: AddPrimaryAmmo( int iCount, const char *szName, int iMaxClip, int iMaxCarry, BOOL duplicate )
{
	int iIdAmmo;

	if( iMaxClip < 1 )
	{
		m_iClip = WEAPON_NOCLIP;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName );
	}
	else if( m_iClip == 0 && duplicate == FALSE )
	{
		int i = Q_min( m_iClip + iCount, iMaxClip ) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount - i, szName );
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName );
	}

	if( iIdAmmo > 0 )
	{
		m_iPrimaryAmmoType = iIdAmmo;

		if( m_pPlayer->HasPlayerItem( this ))
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		}
	}

	return iIdAmmo > 0 ? TRUE : FALSE;
}


BOOL CBasePlayerItem :: AddSecondaryAmmo( int iCount, const char *szName, int iMax )
{
	int iIdAmmo;

	iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName );

	//m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] = iMax; // hack for testing

	if (iIdAmmo > 0)
	{
		m_iSecondaryAmmoType = iIdAmmo;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
	}
	return iIdAmmo > 0 ? TRUE : FALSE;
}

// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
int CBasePlayerItem :: AddDuplicate( CBasePlayerItem *pOriginal )
{
	if( FBitSet( iFlags(), ITEM_FLAG_NODUPLICATE ))
		return FALSE;

	if( m_iDefaultAmmo1 || m_iDefaultAmmo2 )
		return ExtractAmmo( pOriginal, TRUE );
	return ExtractClipAmmo( pOriginal );
}

void CBasePlayerItem :: ItemPostFrame( void )
{
	if( m_iSpot && !m_pSpot )
	{
		// enable laser dot
		m_pSpot = CLaserSpot :: CreateSpot();
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/spot_on.wav", 1, ATTN_NORM );
	}

	if( !m_iSpot && m_pSpot )
	{
		// disable laser dot
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/spot_off.wav", 1, ATTN_NORM );
		m_pSpot->Killed( NULL, GIB_NEVER );
		m_pSpot = NULL;
	}

	if( m_pSpot )
          	m_pSpot->Update( m_pPlayer );

          ZoomUpdate(); // update zoom

	if( m_iStepReload == NOT_IN_RELOAD )
		m_fInReload = FALSE; // finished
          
	if( m_flTimeUpdate < UTIL_WeaponTimeBase( ))
		PostIdle(); // catch all

	if( m_fInReload && m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase( ))
	{
		if( m_iStepReload > CONTINUE_RELOAD )
		{
			// complete the reload. 
			int j = Q_min( iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);	

			// Add them to the clip
			m_iClip += j;
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.01; // play PostReload
			m_fInReload = FALSE;
		}
	}

	if( FBitSet( m_pPlayer->pev->button, IN_ATTACK2 ) && ( m_flNextSecondaryAttack <= gpGlobals->time ))
	{
		SecondaryAttack();
//		ClearBits( m_pPlayer->pev->button, IN_ATTACK2 );
	}
	else if( FBitSet( m_pPlayer->pev->button, IN_ATTACK ) && ( m_flNextPrimaryAttack <= gpGlobals->time ))
	{
		PrimaryAttack();
//		ClearBits( m_pPlayer->pev->button, IN_ATTACK );
	}
	else if( FBitSet( m_pPlayer->pev->button, IN_RELOAD ) && iMaxClip() != WEAPON_NOCLIP && !m_fInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if( !FBitSet( m_pPlayer->pev->button, IN_ATTACK | IN_ATTACK2 ))
	{
		// play sequence holster / deploy if player find or lose suit
		if(( PLAYER_HAS_SUIT && !PLAYER_DRAW_SUIT ) || ( !PLAYER_HAS_SUIT && PLAYER_DRAW_SUIT ))
		{
			m_pPlayer->m_pActiveItem->Holster( true );
			m_pPlayer->QueueItem( this );
			if( m_pPlayer->m_pActiveItem ) 
				m_pPlayer->m_pActiveItem->Deploy();
		}

		if( !CanDeploy() && m_flNextPrimaryAttack < gpGlobals->time ) 
		{
			// weapon isn't useable, switch.
			if( !FBitSet( iFlags(), ITEM_FLAG_NOAUTOSWITCHEMPTY ) && g_pGameRules->GetNextBestWeapon( m_pPlayer, this ))
			{
				m_flNextPrimaryAttack = gpGlobals->time + 0.3;
				return;
			}
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if( m_iClip == 0 && !FBitSet( iFlags(), ITEM_FLAG_NOAUTORELOAD ) && m_flNextPrimaryAttack < gpGlobals->time )
			{
				Reload();
				return;
			}
		}

		m_iPlayEmptySound = 1; // reset empty sound
		if( FBitSet( iFlags(), ITEM_FLAG_USEAUTOAIM ))
			m_pPlayer->GetAutoaimVector( AutoAimDelta( TRUE ));

		if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase( ))
		{
			// step reload
			if( m_iStepReload > NOT_IN_RELOAD )
			{
				if( m_iClip != iMaxClip() && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
				{
					Reload();
				}
				else
				{
					// reload debounce has timed out
					if( IsEmptyReload( ))
					{
						if( SetAnimation( ACT_VM_PUMP_EMPTY ) == -1 )
							SetAnimation( ACT_VM_PUMP );
					}
					else SetAnimation( ACT_VM_PUMP );
					PostReload(); // post effects
					m_iStepReload = NOT_IN_RELOAD;
				}
			}
			else
			{
				WeaponIdle();
			}
			return;
		}
	}
	
	// catch all
	if( ShouldWeaponIdle( ))
	{
		WeaponIdle();
	}
}

void CBasePlayerItem :: DestroyItem( void )
{
	if( m_pPlayer )
	{
		// if attached to a player, remove. 
		m_pPlayer->RemovePlayerItem( this );
	}

	Kill( );
}

int CBasePlayerItem :: AddToPlayer( CBasePlayer *pPlayer )
{
	m_pPlayer = pPlayer;

	pPlayer->pev->weapons |= BIT( m_iId );

	pev->sequence = 0; // to avoid some strange problems

	if( !m_iPrimaryAmmoType )
		m_iPrimaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo1() );

	if( !m_iSecondaryAmmoType )
		m_iSecondaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo2() );

	if( FBitSet( iFlags(), ITEM_FLAG_AUTOFIRE ))
		m_iWeaponAutoFire = TRUE; // enable auto-fire as default

	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
		WRITE_BYTE( m_iId );
	MESSAGE_END();

	return AddWeapon();
}

void CBasePlayerItem :: Drop( void )
{
	// Wargon: Оружие неюзабельно.
	SetUse( NULL );
	m_iItemCaps = CBaseEntity :: ObjectCaps();

	SetTouch( NULL );
	SetThink( &CBasePlayerItem :: SUB_Remove );
	SetNextThink( 0.1 );
}

void CBasePlayerItem :: Kill( void )
{
	// Wargon: Оружие неюзабельно.
	SetUse( NULL );
	m_iItemCaps = CBaseEntity :: ObjectCaps();

	SetTouch( NULL );
	SetThink( &CBasePlayerItem :: SUB_Remove );
	SetNextThink( 0.1 );
}

void CBasePlayerItem :: Deploy( void )
{                              
	ApplyPlayerSettings( FALSE );

	m_iStepReload = 0;
	if( iMaxClip() > 0 && m_iClip <= 0 )			// weapon have clip and clip is empty ?
	{
		if( !DefaultDeploy( ACT_VM_DEPLOY_EMPTY ))	// try to playing "deploy_empty" anim
			DefaultDeploy( ACT_VM_DEPLOY );	// custom animation not found, play standard animation
	}
	else DefaultDeploy( ACT_VM_DEPLOY );			// just playing standard anim
}

void CBasePlayerItem :: Holster( bool force )
{                              
	ApplyPlayerSettings( TRUE );

	if( iMaxClip() > 0 && m_iClip <= 0 )				// weapon have clip and clip is empty ?
	{         
		if( !DefaultHolster( ACT_VM_HOLSTER_EMPTY, force ))	// try to playing "holster_empty" anim
			DefaultHolster( ACT_VM_HOLSTER, force );
	}
	else DefaultHolster( ACT_VM_HOLSTER, force );			// just playing standard anim
}

void CBasePlayerItem :: WeaponToggleMode( void )
{
	if( !FBitSet( iFlags(), ITEM_FLAG_ALLOW_FIREMODE ))
		return;

	if( m_iWeaponAutoFire )
	{
		UTIL_ShowMessage( "#WEAPON_SEMI_AUTO", m_pPlayer );
		m_iWeaponAutoFire = 0;
	}
	else
	{
		UTIL_ShowMessage( "#WEAPON_FULL_AUTO", m_pPlayer );
		m_iWeaponAutoFire = 1;
	}

	EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "weapons/weapon_chengemode.wav", 0.8, ATTN_NORM );

	// FIXME: play anim here
}

void CBasePlayerItem :: AttachToPlayer ( CBasePlayer *pPlayer )
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->aiment = pPlayer->edict();
	pev->effects = EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->targetname = iStringNull;
	pev->owner = pPlayer->edict();

	SetNextThink( 0.1 );
	SetTouch( NULL );
	SetThink( NULL );
	SetUse( NULL );

	m_iItemCaps = CBaseEntity :: ObjectCaps();
}

int CBasePlayerItem :: UpdateClientData( CBasePlayer *pPlayer )
{
	BOOL bSend = FALSE;
	int state = 0;

	// update weapon body
	if( m_iClientBody != m_iBody )
	{
		pev->body = (pev->body % NUM_HANDS) + NUM_HANDS * m_iBody;
		MESSAGE_BEGIN( MSG_ONE, gmsgWeaponBody, NULL, m_pPlayer->pev );
			WRITE_BYTE( pev->body );
		MESSAGE_END();
		m_iClientBody = m_iBody;
	}

	// update weapon skin
	if( m_iClientSkin != m_iSkin )
	{
		pev->skin = m_iSkin;
		MESSAGE_BEGIN( MSG_ONE, gmsgWeaponSkin, NULL, m_pPlayer->pev );
			WRITE_BYTE( pev->skin );
		MESSAGE_END();
		m_iClientSkin = m_iSkin;
	}

	// update weapon anim
	if( pPlayer->pev->weaponanim != m_iClientAnim )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeaponAnim, NULL, m_pPlayer->pev );
			WRITE_BYTE( pPlayer->pev->weaponanim );	// sequence number
			WRITE_BYTE( pev->framerate * 8 );	// playing speed
		MESSAGE_END();

		m_iClientAnim = pPlayer->pev->weaponanim;
	}

	if( pPlayer->m_pActiveItem == this )
	{
		if( pPlayer->m_fOnTarget )
			state = WEAPON_IS_ONTARGET;
		else state = 1;
	}

	// forcing send of all data!
	if( !pPlayer->m_fWeapon )
		bSend = TRUE;
	
	// This is the current or last weapon, so the state will need to be updated
	if( this == pPlayer->m_pActiveItem || this == pPlayer->m_pClientActiveItem )
	{
		if( pPlayer->m_pActiveItem != pPlayer->m_pClientActiveItem )
		{
			bSend = TRUE;
		}
	}

	// if the ammo, state, or fov has changed, update the weapon
	if( m_iClip != m_iClientClip || state != m_iClientWeaponState || pPlayer->m_iFOV != pPlayer->m_iClientFOV )
	{
		bSend = TRUE;
	}

	if( bSend )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pPlayer->pev );
			WRITE_BYTE( state );
			WRITE_BYTE( m_iId );
			WRITE_BYTE( m_iClip );
		MESSAGE_END();

		m_iClientClip = m_iClip;
		m_iClientWeaponState = state;
		pPlayer->m_fWeapon = TRUE;
	}

	if( m_pNext )
		m_pNext->UpdateClientData( pPlayer );

	return 1;
}

//=========================================================
// IsUseable - this function determines whether or not a 
// weapon is useable by the player in its current state. 
// (does it have ammo loaded? do I have any ammo for the 
// weapon?, etc)
//=========================================================
BOOL CBasePlayerItem :: IsUseable( void )
{
	if( m_iClip <= 0 && ( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] <= 0 && iMaxAmmo1() != -1 ))
	{
		return FALSE;
	}
	return TRUE;
}

//=========================================================
//=========================================================
int CBasePlayerItem :: PrimaryAmmoIndex( void )
{
	return m_iPrimaryAmmoType;
}

//=========================================================
//=========================================================
int CBasePlayerItem :: SecondaryAmmoIndex( void )
{
	return -1;
}

//=========================================================
// RetireWeapon - no more ammo for this gun, put it away.
//=========================================================
void CBasePlayerItem :: RetireWeapon( void )
{
	// first, no viewmodel at all.
	m_pPlayer->pev->viewmodel = iStringNull;
	m_pPlayer->pev->weaponmodel = iStringNull;
	//m_pPlayer->pev->viewmodelindex = NULL;

	g_pGameRules->GetNextBestWeapon( m_pPlayer, this );
}

//*********************************************************
// weaponbox code:
//*********************************************************
LINK_ENTITY_TO_CLASS( weaponbox, CWeaponBox );

TYPEDESCRIPTION CWeaponBox :: m_SaveData[] = 
{
	DEFINE_ARRAY( CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_ARRAY( CWeaponBox, m_rgiszAmmo, FIELD_STRING, MAX_AMMO_SLOTS ),
	DEFINE_ARRAY( CWeaponBox, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CWeaponBox, m_cAmmoTypes, FIELD_INTEGER ),
}; IMPLEMENT_SAVERESTORE( CWeaponBox, CBaseEntity );

//=========================================================
//
//=========================================================
void CWeaponBox :: Precache( void )
{
	if( pev->model != iStringNull )
		PRECACHE_MODEL( STRING( pev->model ));
}

//=========================================================
//=========================================================
void CWeaponBox :: KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "reflection" ))
	{
		switch(atoi(pkvd->szValue))
		{
		case 1: pev->effects |= EF_NOREFLECT; break;
		case 2: pev->effects |= EF_REFLECTONLY; break;
		}
	}
	else if ( m_cAmmoTypes < MAX_AMMO_SLOTS )
	{
		PackAmmo( ALLOC_STRING(pkvd->szKeyName), atoi(pkvd->szValue) );
		m_cAmmoTypes++;// count this new ammo type.

		pkvd->fHandled = TRUE;
	}
	else
	{
		ALERT( at_error, "WeaponBox too full! only %d ammotypes allowed\n", MAX_AMMO_SLOTS );
	}
}

//=========================================================
// CWeaponBox - Spawn 
//=========================================================
void CWeaponBox :: Spawn( void )
{
	if( GET_SERVER_STATE() == SERVER_LOADING )
		pev->model = MAKE_STRING( "models/w_weaponbox.mdl" );

	Precache( );

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	if( pev->model != iStringNull )
		SET_MODEL( ENT(pev), STRING( pev->model ));
}

//=========================================================
// CWeaponBox - Kill - the think function that removes the
// box from the world.
//=========================================================
void CWeaponBox :: Kill( void )
{
	CBasePlayerItem *pWeapon;
	int i;

	// destroy the weapons
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pWeapon = m_rgpPlayerItems[ i ];

		while ( pWeapon )
		{
			pWeapon->SetThink(&CBasePlayerItem::SUB_Remove);
			pWeapon->SetNextThink( 0.1 );
			pWeapon = pWeapon->m_pNext;
		}
	}

	// remove the box
	UTIL_Remove( this );
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================
void CWeaponBox :: Touch( CBaseEntity *pOther )
{
	if ( !(pev->flags & FL_ONGROUND ) )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		// only players may touch a weaponbox.
		return;
	}

	if ( !pOther->IsAlive() )
	{
		// no dead guys.
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	int i;

	// dole out ammo
	for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
	{
		if ( !FStringNull( m_rgiszAmmo[ i ] ) )
		{
			// there's some ammo of this type. 
			pPlayer->GiveAmmo( m_rgAmmo[ i ], STRING( m_rgiszAmmo[ i ] ));

			// now empty the ammo from the weaponbox since we just gave it to the player
			m_rgiszAmmo[ i ] = iStringNull;
			m_rgAmmo[ i ] = 0;
		}
	}

// go through my weapons and try to give the usable ones to the player. 
// it's important the the player be given ammo first, so the weapons code doesn't refuse 
// to deploy a better weapon that the player may pick up because he has no ammo for it.
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			CBasePlayerItem *pItem;

			// have at least one weapon in this slot
			while ( m_rgpPlayerItems[ i ] )
			{
				pItem = m_rgpPlayerItems[ i ];
				m_rgpPlayerItems[ i ] = m_rgpPlayerItems[ i ]->m_pNext;// unlink this weapon from the box

				if ( pPlayer->AddPlayerItem( pItem ) )
				{
					pItem->AttachToPlayer( pPlayer );
				}
			}
		}
	}

	EMIT_SOUND( pOther->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
	SetTouch(NULL);
	UTIL_Remove(this);
}

//=========================================================
// CWeaponBox - PackWeapon: Add this weapon to the box
//=========================================================
BOOL CWeaponBox :: PackWeapon( CBasePlayerItem *pWeapon )
{
	// is one of these weapons already packed in this box?
	if ( HasWeapon( pWeapon ) )
	{
		return FALSE;// box can only hold one of each weapon type
	}

	if ( pWeapon->m_pPlayer )
	{
		if ( !pWeapon->m_pPlayer->RemovePlayerItem( pWeapon ) )
		{
			// failed to unhook the weapon from the player!
			return FALSE;
		}
	}

	int iWeaponSlot = pWeapon->iItemSlot();
	
	if ( m_rgpPlayerItems[ iWeaponSlot ] )
	{
		// there's already one weapon in this slot, so link this into the slot's column
		pWeapon->m_pNext = m_rgpPlayerItems[ iWeaponSlot ];	
		m_rgpPlayerItems[ iWeaponSlot ] = pWeapon;
	}
	else
	{
		// first weapon we have for this slot
		m_rgpPlayerItems[ iWeaponSlot ] = pWeapon;
		pWeapon->m_pNext = NULL;	
	}

	pWeapon->pev->spawnflags |= SF_NORESPAWN;// never respawn
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects |= EF_NODRAW;
	pWeapon->pev->modelindex = 0;
	pWeapon->pev->model = iStringNull;
	pWeapon->pev->owner = edict();
	pWeapon->SetThink( NULL );// crowbar may be trying to swing again, etc.
	pWeapon->SetTouch( NULL );
	pWeapon->m_pPlayer = NULL;

	return TRUE;
}

//=========================================================
// MaxAmmoCarry - pass in a name and this function will tell
// you the maximum amount of that type of ammunition that a 
// player can carry.
//=========================================================
int CWeaponBox :: MaxAmmoCarry( int iszName )
{
	for ( int i = 0;  i < MAX_WEAPONS; i++ )
	{
		if ( CBasePlayerItem::ItemInfoArray[i].pszAmmo1 && !Q_strcmp( STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo1 ) )
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo1;
		if ( CBasePlayerItem::ItemInfoArray[i].pszAmmo2 && !Q_strcmp( STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo2 ) )
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo2;
	}

	ALERT( at_error, "MaxAmmoCarry() doesn't recognize '%s'!\n", STRING( iszName ) );

	return -1;
}

//=========================================================
// CWeaponBox - PackAmmo
//=========================================================
BOOL CWeaponBox :: PackAmmo( int iszName, int iCount )
{
	int iMaxCarry;

	if ( FStringNull( iszName ) )
	{
		// error here
		ALERT ( at_debug, "NULL String in PackAmmo!\n" );
		return FALSE;
	}
	
	iMaxCarry = MaxAmmoCarry( iszName );

	if ( iMaxCarry != -1 && iCount > 0 )
	{
		GiveAmmo( iCount, STRING( iszName ));
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CWeaponBox - GiveAmmo
//=========================================================
int CWeaponBox :: GiveAmmo( int iCount, const char *szName )
{
	int i;

	for (i = 1; i < MAX_AMMO_SLOTS && !FStringNull( m_rgiszAmmo[i] ); i++)
	{
		if (!Q_stricmp( szName, STRING( m_rgiszAmmo[i] )))
		{
			AmmoInfo *pAmmo = UTIL_FindAmmoType( szName );
	
			if( !pAmmo ) return -1;

			int iAdd = Q_min( iCount, pAmmo->iMaxCarry - m_rgAmmo[i] );
			if ( iCount == 0 || iAdd > 0 )
			{
				m_rgAmmo[i] += iAdd;
				return i;
			}

			return -1;
		}
	}

	if (i < MAX_AMMO_SLOTS)
	{
		m_rgiszAmmo[i] = MAKE_STRING( szName );
		m_rgAmmo[i] = iCount;
		return i;
	}

	ALERT( at_debug, "out of named ammo slots\n");

	return i;
}

//=========================================================
// CWeaponBox::HasWeapon - is a weapon of this type already
// packed in this box?
//=========================================================
BOOL CWeaponBox :: HasWeapon( CBasePlayerItem *pCheckItem )
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while (pItem)
	{
		if (FClassnameIs( pItem->pev, STRING( pCheckItem->pev->classname) ))
		{
			return TRUE;
		}
		pItem = pItem->m_pNext;
	}

	return FALSE;
}

//=========================================================
// CWeaponBox::IsEmpty - is there anything in this box?
//=========================================================
BOOL CWeaponBox :: IsEmpty( void )
{
	int i;

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			return FALSE;
		}
	}

	for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
	{
		if ( !FStringNull( m_rgiszAmmo[ i ] ) )
		{
			// still have a bit of this type of ammo
			return FALSE;
		}
	}

	return TRUE;
}

//=========================================================
//=========================================================
void CWeaponBox :: SetObjectCollisionBox( void )
{
	if( !Q_stricmp( STRING( pev->model ), "models/w_weaponbox.mdl" ))
	{
		pev->absmin = pev->origin + Vector(-16, -16, 0);
		pev->absmax = pev->origin + Vector(16, 16, 16); 
	}
	else
	{
		pev->absmin = pev->origin + Vector(-24, -24, 0);
		pev->absmax = pev->origin + Vector(24, 24, 16); 
	}
}

// buz: painkiller item
class CPainkiller : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_painkiller.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_painkiller.mdl");
		PRECACHE_SOUND("items/painkiller_pickup.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 1, "painkillers" ) != -1 );
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/painkiller_pickup.wav", 1, ATTN_NORM);
			pOther->pev->weapons |= (1<<WEAPON_PAINKILLER);
			// Wargon: Возможность таргетить энтити подбором паинкиллера.
			SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( item_painkiller, CPainkiller );