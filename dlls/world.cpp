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

===== world.cpp ========================================================

  precaches and defs for entities and other data that must always be available.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "nodes.h"
#include "soundent.h"
#include "client.h"
#include "decals.h"
#include "skill.h"
#include "effects.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "movewith.h" //LRC

extern CGraph WorldGraph;
extern CSoundEnt *pSoundEnt;

extern CBaseEntity				*g_pLastSpawn;
DLL_GLOBAL edict_t				*g_pBodyQueueHead;
CGlobalState				gGlobalState;
extern DLL_GLOBAL int			gDisplayTitle;
DLL_GLOBAL unsigned short			m_usSmokePuff; // buz - smoke event
extern unsigned short			g_usShootEvent;

//
// This must match the list in util.h
//
DLL_DECALLIST gDecals[] = {
	{ "{shot1",	0 },		// DECAL_GUNSHOT1 
	{ "{shot2",	0 },		// DECAL_GUNSHOT2
	{ "{shot3",0 },			// DECAL_GUNSHOT3
	{ "{shot4",	0 },		// DECAL_GUNSHOT4
	{ "{shot5",	0 },		// DECAL_GUNSHOT5
	{ "{lambda01", 0 },		// DECAL_LAMBDA1
	{ "{lambda02", 0 },		// DECAL_LAMBDA2
	{ "{lambda03", 0 },		// DECAL_LAMBDA3
	{ "{lambda04", 0 },		// DECAL_LAMBDA4
	{ "{lambda05", 0 },		// DECAL_LAMBDA5
	{ "{lambda06", 0 },		// DECAL_LAMBDA6
	{ "{scorch1", 0 },		// DECAL_SCORCH1
	{ "{scorch2", 0 },		// DECAL_SCORCH2
	{ "{blood1", 0 },		// DECAL_BLOOD1
	{ "{blood2", 0 },		// DECAL_BLOOD2
	{ "{blood3", 0 },		// DECAL_BLOOD3
	{ "{blood4", 0 },		// DECAL_BLOOD4
	{ "{blood5", 0 },		// DECAL_BLOOD5
	{ "{blood6", 0 },		// DECAL_BLOOD6
	{ "{yblood1", 0 },		// DECAL_YBLOOD1
	{ "{yblood2", 0 },		// DECAL_YBLOOD2
	{ "{yblood3", 0 },		// DECAL_YBLOOD3
	{ "{yblood4", 0 },		// DECAL_YBLOOD4
	{ "{yblood5", 0 },		// DECAL_YBLOOD5
	{ "{yblood6", 0 },		// DECAL_YBLOOD6
	{ "{break1", 0 },		// DECAL_GLASSBREAK1
	{ "{break2", 0 },		// DECAL_GLASSBREAK2
	{ "{break3", 0 },		// DECAL_GLASSBREAK3
	{ "{bigshot1", 0 },		// DECAL_BIGSHOT1
	{ "{bigshot2", 0 },		// DECAL_BIGSHOT2
	{ "{bigshot3", 0 },		// DECAL_BIGSHOT3
	{ "{bigshot4", 0 },		// DECAL_BIGSHOT4
	{ "{bigshot5", 0 },		// DECAL_BIGSHOT5
	{ "{spit1", 0 },		// DECAL_SPIT1
	{ "{spit2", 0 },		// DECAL_SPIT2
	{ "{bproof1", 0 },		// DECAL_BPROOF1
	{ "{gargstomp", 0 },	// DECAL_GARGSTOMP1,	// Gargantua stomp crack
	{ "{smscorch1", 0 },	// DECAL_SMALLSCORCH1,	// Small scorch mark
	{ "{smscorch2", 0 },	// DECAL_SMALLSCORCH2,	// Small scorch mark
	{ "{smscorch3", 0 },	// DECAL_SMALLSCORCH3,	// Small scorch mark
	{ "{mommablob", 0 },	// DECAL_MOMMABIRTH		// BM Birth spray
	{ "{mommablob", 0 },	// DECAL_MOMMASPLAT		// BM Mortar spray?? need decal
};

/*
==============================================================================

BODY QUE

==============================================================================
*/
// Body queue class here.... It's really just CBaseEntity
class CCorpse : public CBaseEntity
{
	virtual int ObjectCaps( void ) { return FCAP_DONT_SAVE; }	
};

LINK_ENTITY_TO_CLASS( bodyque, CCorpse );

static void InitBodyQue(void)
{
	string_t	istrClassname = MAKE_STRING("bodyque");

	g_pBodyQueueHead = CREATE_NAMED_ENTITY( istrClassname );
	entvars_t *pev = VARS(g_pBodyQueueHead);
	
	// Reserve 3 more slots for dead bodies
	for ( int i = 0; i < 3; i++ )
	{
		pev->owner = CREATE_NAMED_ENTITY( istrClassname );
		pev = VARS(pev->owner);
	}
	
	pev->owner = g_pBodyQueueHead;
}


//
// make a body que entry for the given ent so the ent can be respawned elsewhere
//
// GLOBALS ASSUMED SET:  g_eoBodyQueueHead
//
void CopyToBodyQue(entvars_t *pev) 
{
	if (pev->effects & EF_NODRAW)
		return;

	entvars_t *pevHead	= VARS(g_pBodyQueueHead);

	pevHead->angles		= pev->angles;
	pevHead->model		= pev->model;
	pevHead->modelindex	= pev->modelindex;
	pevHead->frame		= pev->frame;
	pevHead->colormap	= pev->colormap;
	pevHead->movetype	= MOVETYPE_TOSS;
	pevHead->velocity	= pev->velocity;
	pevHead->flags		= 0;
	pevHead->deadflag	= pev->deadflag;
	pevHead->renderfx	= kRenderFxDeadPlayer;
	pevHead->renderamt	= ENTINDEX( ENT( pev ) );

	pevHead->effects    = pev->effects | EF_NOINTERP;
	//pevHead->goalstarttime = pev->goalstarttime;
	//pevHead->goalframe	= pev->goalframe;
	//pevHead->goalendtime = pev->goalendtime ;
	
	pevHead->sequence = pev->sequence;
	pevHead->animtime = pev->animtime;

	UTIL_SetEdictOrigin(g_pBodyQueueHead, pev->origin);
	UTIL_SetSize(pevHead, pev->mins, pev->maxs);
	g_pBodyQueueHead = pevHead->owner;
}


CGlobalState::CGlobalState( void )
{
	Reset();
}

void CGlobalState::Reset( void )
{
	m_pList = NULL; 
	m_listCount = 0;
}

globalentity_t *CGlobalState :: Find( string_t globalname )
{
	if ( !globalname )
		return NULL;

	globalentity_t *pTest;
	const char *pEntityName = STRING(globalname);

	
	pTest = m_pList;
	while ( pTest )
	{
		if ( FStrEq( pEntityName, pTest->name ) )
			break;
	
		pTest = pTest->pNext;
	}

	return pTest;
}


// This is available all the time now on impulse 104, remove later
//#ifdef _DEBUG
void CGlobalState :: DumpGlobals( void )
{
	static char *estates[] = { "Off", "On", "Dead" };
	globalentity_t *pTest;

	ALERT( at_debug, "-- Globals --\n" );
	pTest = m_pList;
	while ( pTest )
	{
		ALERT( at_debug, "%s: %s (%s)\n", pTest->name, pTest->levelName, estates[pTest->state] );
		pTest = pTest->pNext;
	}
}
//#endif


void CGlobalState :: EntityAdd( string_t globalname, string_t mapName, GLOBALESTATE state, float time )
{
	ASSERT( !Find(globalname) );

	globalentity_t *pNewEntity = (globalentity_t *)calloc( sizeof( globalentity_t ), 1 );
	ASSERT( pNewEntity != NULL );
	pNewEntity->pNext = m_pList;
	m_pList = pNewEntity;
	strcpy( pNewEntity->name, STRING( globalname ) );
	strcpy( pNewEntity->levelName, STRING(mapName) );
	pNewEntity->global_time = time;
	pNewEntity->state = state;
	m_listCount++;
}


void CGlobalState :: EntitySetState( string_t globalname, GLOBALESTATE state )
{
	globalentity_t *pEnt = Find( globalname );

	if ( !pEnt ) return;

	pEnt->state = state;
}

void CGlobalState :: EntitySetTime( string_t globalname, float time )
{
	globalentity_t *pEnt = Find( globalname );

	if ( !pEnt ) return;

	pEnt->global_time = time;
}

const globalentity_t *CGlobalState :: EntityFromTable( string_t globalname )
{
	globalentity_t *pEnt = Find( globalname );

	return pEnt;
}


GLOBALESTATE CGlobalState :: EntityGetState( string_t globalname )
{
	globalentity_t *pEnt = Find( globalname );
	if ( pEnt )
		return pEnt->state;

	return GLOBAL_OFF;
}

float CGlobalState :: EntityGetTime( string_t globalname )
{
	globalentity_t *pEnt = Find( globalname );
	if ( pEnt )
		return pEnt->global_time;

	return -1.0f;
}


// Global Savedata for Delay
TYPEDESCRIPTION	CGlobalState::m_SaveData[] = 
{
	DEFINE_FIELD( CGlobalState, m_listCount, FIELD_INTEGER ),
};

// Global Savedata for Delay
TYPEDESCRIPTION	gGlobalEntitySaveData[] = 
{
	DEFINE_ARRAY( globalentity_t, name, FIELD_CHARACTER, 64 ),
	DEFINE_ARRAY( globalentity_t, levelName, FIELD_CHARACTER, 32 ),
	DEFINE_FIELD( globalentity_t, state, FIELD_INTEGER ),
	DEFINE_FIELD( globalentity_t, global_time, FIELD_FLOAT ), // to save global time instead of state
};


int CGlobalState::Save( CSave &save )
{
	int i;
	globalentity_t *pEntity;

	if ( !save.WriteFields( "cGLOBAL", "GLOBAL", this, m_SaveData, ARRAYSIZE(m_SaveData) ) )
		return 0;
	
	pEntity = m_pList;
	for ( i = 0; i < m_listCount && pEntity; i++ )
	{
		if ( !save.WriteFields( "cGENT", "GENT", pEntity, gGlobalEntitySaveData, ARRAYSIZE(gGlobalEntitySaveData) ) )
			return 0;

		pEntity = pEntity->pNext;
	}
	
	return 1;
}

int CGlobalState::Restore( CRestore &restore )
{
	int i, listCount;
	globalentity_t tmpEntity;

	ClearStates();
	if ( !restore.ReadFields( "GLOBAL", this, m_SaveData, ARRAYSIZE(m_SaveData) ) )
		return 0;
	
	listCount = m_listCount;	// Get new list count
	m_listCount = 0;				// Clear loaded data

	for ( i = 0; i < listCount; i++ )
	{
		if ( !restore.ReadFields( "GENT", &tmpEntity, gGlobalEntitySaveData, ARRAYSIZE(gGlobalEntitySaveData) ) )
			return 0;
		EntityAdd( MAKE_STRING(tmpEntity.name), MAKE_STRING(tmpEntity.levelName), tmpEntity.state, tmpEntity.global_time );
	}
	return 1;
}

void CGlobalState::EntityUpdate( string_t globalname, string_t mapname )
{
	globalentity_t *pEnt = Find( globalname );

	if ( pEnt )
		strcpy( pEnt->levelName, STRING(mapname) );
}


void CGlobalState::ClearStates( void )
{
	globalentity_t *pFree = m_pList;
	while ( pFree )
	{
		globalentity_t *pNext = pFree->pNext;
		free( pFree );
		pFree = pNext;
	}
	Reset();
}


void SaveGlobalState( SAVERESTOREDATA *pSaveData )
{
	CSave saveHelper( pSaveData );
	gGlobalState.Save( saveHelper );
}


void RestoreGlobalState( SAVERESTOREDATA *pSaveData )
{
	CRestore restoreHelper( pSaveData );
	gGlobalState.Restore( restoreHelper );
}


void ResetGlobalState( void )
{
	gGlobalState.ClearStates();
	gInitHUD = TRUE;	// Init the HUD on a new game / load game
}

// called by worldspawn
void W_Precache( void )
{
	memset( CBasePlayerItem::ItemInfoArray, 0, sizeof(CBasePlayerItem::ItemInfoArray) );
	memset( CBasePlayerAmmo::AmmoInfoArray, 0, sizeof(CBasePlayerAmmo::AmmoInfoArray) );
	memset( CBasePlayerAmmo::AmmoDescArray, 0, sizeof(CBasePlayerAmmo::AmmoDescArray) );
	CBasePlayerItem::m_iGlobalID = 1; // weapon id's starts from 1

	// custom items...
	UTIL_InitAmmoDescription( "scripts/weapons/ammodesc.txt" );

	UTIL_InitWeaponDescription( "scripts/weapons/weapon_*.txt" ); // lookup all the weapon_???.txt files

	// common world objects
	UTIL_PrecacheOther( "item_suit" );
	UTIL_PrecacheOther( "item_gasmask" ); // buz
	UTIL_PrecacheOther( "item_headshield" ); // buz

	UTIL_PrecacheOther( "laser_spot" );
	UTIL_PrecacheOther( "rpg_rocket" );

	if ( g_pGameRules->IsDeathmatch() )
	{
		PRECACHE_SOUND( "items/suitchargeok1.wav" );//!!! temporary sound for respawning weapons.
		UTIL_PrecacheOther( "weaponbox" );// container for dropped deathmatch weapons
	}

	g_sModelIndexFireball = PRECACHE_MODEL ("sprites/zerogxplode.spr");// fireball
	g_sModelIndexWExplosion = PRECACHE_MODEL ("sprites/WXplo1.spr");// underwater fireball
	g_sModelIndexSmoke = PRECACHE_MODEL ("sprites/steam1.spr");// smoke
	g_sModelIndexWaterSplash = PRECACHE_MODEL ("sprites/wsplash_x.spr");//water splash
	g_sModelIndexBubbles = PRECACHE_MODEL ("sprites/bubble.spr");//bubbles
	g_sModelIndexBloodSpray = PRECACHE_MODEL ("sprites/bloodspray.spr"); // initial blood
	g_sModelIndexBloodDrop = PRECACHE_MODEL ("sprites/blood.spr"); // splattered blood
	g_sModelIndexSmokeTrail = PRECACHE_MODEL ("sprites/smokeball.spr");
	g_sModelIndexNull = PRECACHE_MODEL ("sprites/null.spr");
 
	g_sModelIndexLaser = PRECACHE_MODEL( (char *)g_pModelNameLaser );
	g_sModelIndexLaserDot = PRECACHE_MODEL("sprites/laserdot.spr");

	m_usSmokePuff = PRECACHE_EVENT( 1, "evSmokePuff" ); // buz
	g_usShootEvent = PRECACHE_EVENT( 1, "evFireGeneric" ); // g-cont

	// used by explosions
	PRECACHE_MODEL ("sprites/null.spr");

	PRECACHE_MODEL ("models/grenade.mdl");
	PRECACHE_MODEL ("sprites/explode1.spr");
	PRECACHE_MODEL ("models/m_flash1.mdl");

	PRECACHE_SOUND ("weapons/debris1.wav");// explosion aftermaths
	PRECACHE_SOUND ("weapons/debris2.wav");// explosion aftermaths
	PRECACHE_SOUND ("weapons/debris3.wav");// explosion aftermaths

	PRECACHE_SOUND ("weapons/grenade_hit1.wav");//grenade
	PRECACHE_SOUND ("weapons/grenade_hit2.wav");//grenade
	PRECACHE_SOUND ("weapons/grenade_hit3.wav");//grenade

	PRECACHE_SOUND ("weapons/bullet_hit1.wav");	// hit by bullet
	PRECACHE_SOUND ("weapons/bullet_hit2.wav");	// hit by bullet
	
	PRECACHE_SOUND ("items/weapondrop1.wav");// weapon falls to the ground
	PRECACHE_SOUND ("weapons/weapon_chengemode.wav");

	// buz
	PRECACHE_MODEL ("sprites/wsplash_x.spr");
	PRECACHE_MODEL ("sprites/smokeball.spr");
	PRECACHE_MODEL ("sprites/eexplo.spr");
	PRECACHE_MODEL ("sprites/fexplo.spr");
	PRECACHE_MODEL ("sprites/dexplo.spr");
	PRECACHE_MODEL ("sprites/richo1.spr");
}

// moved CWorld class definition to cbase.h
//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================

LINK_ENTITY_TO_CLASS( worldspawn, CWorld );

#define SF_WORLD_DARK		0x0001		// Fade from black at startup
#define SF_WORLD_TITLE		0x0002		// Display game title at startup
#define SF_WORLD_FORCETEAM	0x0004		// Force teams
//#define SF_WORLD_STARTSUIT	0x0008		// LRC- Start this level with an HEV suit!

extern DLL_GLOBAL BOOL		g_fGameOver;
float g_flWeaponCheat; 

//BOOL g_startSuit; //LRC

void CWorld :: Spawn( void )
{
	// if time was set
	if( pev->button && m_vecTime.x != -1.0f && m_vecTime.y != -1.0f )
	{
		int name = MAKE_STRING( "GLOBAL_TIME" );
		float global_time = bound( 0.0f, m_vecTime.x, 24.0f ) + bound( 0.0f, m_vecTime.y, 60.0f ) * (1.0f/60.0f);

		if ( !gGlobalState.EntityInTable( name ) ) // first initialize ?
			gGlobalState.EntityAdd( name, gpGlobals->mapname, (GLOBALESTATE)GLOBAL_ON, global_time );
	}

	LIGHT_STYLE( 20, "m" );
	g_fGameOver = FALSE;
	Precache( );
}

void CWorld :: Precache( void )
{
	// LRC - set up the world lists
	g_pWorld = this;
	m_pAssistLink = NULL;
	m_pFirstAlias = NULL;
	g_pLastSpawn = NULL;
	
	CVAR_SET_STRING( "sv_gravity", "800" ); // 67ft/sec
	CVAR_SET_STRING( "sv_stepsize", "18" );
	CVAR_SET_STRING( "room_type", "0" ); // clear DSP

	// Set up game rules
	if ( g_pGameRules )
	{
		delete g_pGameRules;
	}

	g_pGameRules = InstallGameRules( );

	//!!!UNDONE why is there so much Spawn code in the Precache function? I'll just keep it here 

	///!!!LATER - do we want a sound ent in deathmatch? (sjb)
	//pSoundEnt = CBaseEntity::Create( "soundent", g_vecZero, g_vecZero, edict() );
	pSoundEnt = GetClassPtr(( CSoundEnt *)NULL );
	pSoundEnt->Spawn();

	if ( !pSoundEnt )
	{
		ALERT ( at_debug, "**COULD NOT CREATE SOUNDENT**\n" );
	}

	InitBodyQue();
	
// init sentence group playback stuff from sentences.txt.
// ok to call this multiple times, calls after first are ignored.

	SENTENCEG_Init();

// init texture type array from materials.txt

	TEXTURETYPE_Init();


// the area based ambient sounds MUST be the first precache_sounds

// player precaches     
	W_Precache ();						// get weapon precaches

	ClientPrecache();

// sounds used from C physics code
	PRECACHE_SOUND("common/null.wav");				// clears sound channels

	PRECACHE_SOUND( "items/gunpickup2.wav" );// player picks up a gun.

	PRECACHE_SOUND( "common/bodydrop3.wav" );// dead bodies hitting the ground (animation events)
	PRECACHE_SOUND( "common/bodydrop4.wav" );
	
	g_Language = (int)CVAR_GET_FLOAT( "sv_language" );
	if ( g_Language == LANGUAGE_GERMAN )
	{
		PRECACHE_MODEL( "models/germangibs.mdl" );
	}
	else
	{
		PRECACHE_MODEL( "models/hgibs.mdl" );
		PRECACHE_MODEL( "models/agibs.mdl" );
	}

	PRECACHE_SOUND ("weapons/ric1.wav");
	PRECACHE_SOUND ("weapons/ric2.wav");
	PRECACHE_SOUND ("weapons/ric3.wav");
	PRECACHE_SOUND ("weapons/ric4.wav");
	PRECACHE_SOUND ("weapons/ric5.wav");

	PRECACHE_SOUND ("debris/wood1.wav");		// hit wood texture
	PRECACHE_SOUND ("debris/wood2.wav");
	PRECACHE_SOUND ("debris/wood3.wav");
	PRECACHE_SOUND ("debris/wood4.wav");

	PRECACHE_SOUND ("debris/metal1.wav");		// hit metal texture
	PRECACHE_SOUND ("debris/metal2.wav");
	PRECACHE_SOUND ("debris/metal3.wav");
	PRECACHE_SOUND ("debris/metal4.wav");

	PRECACHE_SOUND ("debris/glass1.wav");		// hit metal texture
	PRECACHE_SOUND ("debris/glass2.wav");
	PRECACHE_SOUND ("debris/glass3.wav");

	PRECACHE_SOUND( "player/water_splash1.wav" );
	PRECACHE_SOUND( "player/water_splash2.wav" );
	PRECACHE_SOUND( "player/water_splash3.wav" );

	PRECACHE_MODEL( "sprites/null.spr" ); //LRC

	PRECACHE_MODEL( "sprites/splash1.spr" );
	PRECACHE_MODEL( "sprites/splash2.spr" );

	PRECACHE_MODEL( "sprites/bexplo.spr" );
	PRECACHE_MODEL( "sprites/cexplo.spr" );
	PRECACHE_MODEL( "sprites/dexplo.spr" );
	PRECACHE_MODEL( "sprites/eexplo.spr" );
	PRECACHE_MODEL( "sprites/fexplo.spr" );
	PRECACHE_MODEL( "sprites/smokeball.spr" );               

	PRECACHE_MODEL( "sprites/gunsmoke.spr" );
 
//
// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
//
	int i;

	// 0 normal
	for (i = 0; i <= 13; i++)
	{
		LIGHT_STYLE(i, (char*)STRING(GetStdLightStyle(i)));
	}
	
	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	LIGHT_STYLE(63, "a");

#if 0	// Paranoia has fully seperate decal system
	for (i = 0; i < ARRAYSIZE(gDecals); i++ )
		gDecals[i].index = DECAL_INDEX( gDecals[i].name );
#endif

// init the WorldGraph.
	WorldGraph.InitGraph();

// make sure the .NOD file is newer than the .BSP file.
	if ( !WorldGraph.CheckNODFile ( ( char * )STRING( gpGlobals->mapname ) ) )
	{// NOD file is not present, or is older than the BSP file.
		WorldGraph.AllocNodes ();
	}
	else
	{// Load the node graph for this level
		if ( !WorldGraph.FLoadGraph ( (char *)STRING( gpGlobals->mapname ) ) )
		{// couldn't load, so alloc and prepare to build a graph.
			ALERT ( at_debug, "*Error opening .NOD file\n" );
			WorldGraph.AllocNodes ();
		}
		else
		{
			ALERT ( at_debug, "\n*Graph Loaded!\n" );
		}
	}

	if ( pev->speed > 0 )
		CVAR_SET_FLOAT( "sv_zmax", pev->speed );
	else
		CVAR_SET_FLOAT( "sv_zmax", 4096 );

	// g-cont. moved here to right restore global WaveHeight on save\restore level
	CVAR_SET_FLOAT( "sv_wateramp", pev->scale );

	if ( pev->netname )
	{
		ALERT( at_aiconsole, "Chapter title: %s\n", STRING(pev->netname) );
		CBaseEntity *pEntity = CBaseEntity::Create( "env_message", g_vecZero, g_vecZero, NULL );
		if ( pEntity )
		{
			pEntity->SetThink(&CWorld::SUB_CallUseToggle );
			pEntity->pev->message = pev->netname;
			pev->netname = 0;
			pEntity->SetNextThink( 0.3 );
			pEntity->pev->spawnflags = SF_MESSAGE_ONCE;
		}
	}

	if ( pev->spawnflags & SF_WORLD_DARK )
		CVAR_SET_FLOAT( "v_dark", 1.0 );
	else
		CVAR_SET_FLOAT( "v_dark", 0.0 );

	pev->spawnflags &= ~SF_WORLD_DARK;		// g-cont. don't apply fade after save\restore

	if ( pev->spawnflags & SF_WORLD_TITLE )
		gDisplayTitle = TRUE;		// display the game title if this key is set
	else
		gDisplayTitle = FALSE;

	pev->spawnflags &= ~SF_WORLD_TITLE;		// g-cont. don't show logo after save\restore

	if ( pev->spawnflags & SF_WORLD_FORCETEAM )
	{
		CVAR_SET_FLOAT( "mp_defaultteam", 1 );
	}
	else
	{
		CVAR_SET_FLOAT( "mp_defaultteam", 0 );
	}

	// g-cont. moved here so cheats will working on restore level
	g_flWeaponCheat = CVAR_GET_FLOAT( "sv_cheats" );  // Is the impulse 101 command allowed?

	if( g_iXashEngineBuildNumber >= 2009 )
		UPDATE_PACKED_FOG( pev->impulse );
}


//
// Just to ignore the "wad" field.
//
void CWorld :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq(pkvd->szKeyName, "skyname") )
	{
		// Sent over net now.
		CVAR_SET_STRING( "sv_skyname", pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "sounds") )
	{
		gpGlobals->cdAudioTrack = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "WaveHeight") )
	{
		// Sent over net now.
		pev->scale = atof(pkvd->szValue) * (1.0/8.0);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "MaxRange") )
	{
		pev->speed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "chaptertitle") )
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "fog") )
	{
		int fog_settings[4];
		UTIL_StringToIntArray( fog_settings, 4, pkvd->szValue );
		pev->impulse = (fog_settings[0]<<24)|(fog_settings[1]<<16)|(fog_settings[2]<<8)|fog_settings[3];
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "startdark") )
	{
		// UNDONE: This is a gross hack!!! The CVAR is NOT sent over the client/sever link
		// but it will work for single player
		int flag = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
		if ( flag )
			pev->spawnflags |= SF_WORLD_DARK;
	}
	else if ( FStrEq(pkvd->szKeyName, "newunit") )
	{
		// Single player only.  Clear save directory if set
		if ( atoi(pkvd->szValue) )
			CVAR_SET_FLOAT( "sv_newunit", 1 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "gametitle") )
	{
		if ( atoi(pkvd->szValue) )
			pev->spawnflags |= SF_WORLD_TITLE;

		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "mapteams") )
	{
		pev->team = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "defaultteam") )
	{
		if ( atoi(pkvd->szValue) )
		{
			pev->spawnflags |= SF_WORLD_FORCETEAM;
		}
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "allowmonsters") )
	{
		CVAR_SET_FLOAT( "mp_allowmonsters", atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "time") )
	{
		UTIL_StringToVector( (float*)(m_vecTime), pkvd->szValue );
		pkvd->fHandled = TRUE;
		pev->button = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}
