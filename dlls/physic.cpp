/*
physic.cpp - common physics code
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include	"extdll.h"
#include  "util.h"
#include	"cbase.h"
#include	"saverestore.h"
#include	"client.h"
#include  "nodes.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"game.h"
#include	"com_model.h"
#include	"features.h"
#include  "triangleapi.h"
#include  "render_api.h"
#include  "pm_shared.h"
#include  "pm_defs.h"
#include  "trace.h"
#include  "stringlib.h"
#include	"weapons.h"

unsigned int EngineSetFeatures( void )
{
	unsigned int flags = (ENGINE_WRITE_LARGE_COORD|ENGINE_COMPUTE_STUDIO_LERP|ENGINE_LOAD_DELUXEDATA);

	if( g_iXashEngineBuildNumber >= 2148 )
		flags |= ENGINE_LARGE_LIGHTMAPS|ENGINE_DISABLE_HDTEXTURES;	// required for right trace alpha-surfaces on studiomodels

	return flags;
}

void Physic_SweepTest( CBaseEntity *pTouch, const Vector &start, const Vector &mins, const Vector &maxs, const Vector &end, trace_t *tr )
{
	if( !pTouch->pev->modelindex || !GET_MODEL_PTR( pTouch->edict() ))
	{
		// bad model?
		tr->allsolid = false;
		return;
	}

	Vector trace_mins, trace_maxs;
	UTIL_MoveBounds( start, mins, maxs, end, trace_mins, trace_maxs );

	// NOTE: pmove code completely ignore a bounds checking. So we need to do it here
	if( !BoundsIntersect( trace_mins, trace_maxs, pTouch->pev->absmin, pTouch->pev->absmax ))
	{
		tr->allsolid = false;
		return;
	}

	CMeshDesc	*bodyMesh = UTIL_GetCollisionMesh( pTouch->pev->modelindex );

	if( !bodyMesh )
	{
		// failed to build mesh for some reasons, so skip them
		tr->allsolid = false;
		return;
	}

	mmesh_t *pMesh = bodyMesh->GetMesh();
	areanode_t *pHeadNode = bodyMesh->GetHeadNode();
	entvars_t *pev = pTouch->pev;

	if( !pMesh )
	{
		// failed to build mesh for some reasons, so skip them
		tr->allsolid = false;
		return;
	}

	TraceMesh	trm;	// a name like Doom3 :-)

	trm.SetTraceMesh( pMesh, pHeadNode, pTouch->pev->modelindex );
	trm.SetMeshOrientation( pev->origin, pev->angles, pev->startpos );
	trm.SetupTrace( start, mins, maxs, end, tr );

	if( trm.DoTrace( ))
	{
		if( tr->fraction < 1.0f || tr->startsolid )
			tr->ent = pTouch->edict();
		tr->surf = trm.GetLastHitSurface();
	}
}

void SV_ClipMoveToEntity( edict_t *ent, const float *start, float *mins, float *maxs, const float *end, trace_t *trace )
{
	// convert edict_t to base entity
	CBaseEntity *pTouch = CBaseEntity::Instance( ent );

	if( !pTouch )
	{
		// removed entity?
		trace->allsolid = false;
		return;
	}

	Physic_SweepTest( pTouch, start, mins, maxs, end, trace );
}

void SV_ClipPMoveToEntity( physent_t *pe, const float *start, float *mins, float *maxs, const float *end, pmtrace_t *tr )
{
	// convert physent_t to base entity
	CBaseEntity *pTouch = CBaseEntity::Instance( INDEXENT( pe->info ));
	trace_t trace;

	if( !pTouch )
	{
		// removed entity?
		tr->allsolid = false;
		return;
	}

	// make trace default
	memset( &trace, 0, sizeof( trace ));
	trace.allsolid = true;
	trace.fraction = 1.0f;
	trace.endpos = end;

	Physic_SweepTest( pTouch, start, mins, maxs, end, &trace );

	// convert trace_t into pmtrace_t
	memcpy( tr, &trace, 48 );
	tr->surf = trace.surf;

	if( trace.ent != NULL && PM_GetPlayerMove( ))
		tr->ent = pe - PM_GetPlayerMove()->physents;
	else tr->ent = -1;
}

int SV_RestoreDecal( decallist_t *entry, edict_t *pEdict, qboolean adjacent )
{
	int	flags = entry->flags;
	int	entityIndex = ENTINDEX( pEdict );
	int	cacheID = 0, modelIndex = 0;
	Vector	scale = g_vecZero;

	if( flags & FDECAL_STUDIO )
	{
		if( FBitSet( pEdict->v.iuser1, CF_STATIC_ENTITY ))
		{
			cacheID = pEdict->v.colormap;
			scale = pEdict->v.startpos;
		}

		UTIL_RestoreStudioDecal( entry->position, entry->impactPlaneNormal, entityIndex,
		pEdict->v.modelindex, entry->name, flags, &entry->studio_state, cacheID, scale );
		return TRUE;
          }

	if( adjacent && entry->entityIndex != 0 && ( !pEdict || pEdict->free ))
	{
		TraceResult tr;

		ALERT( at_error, "couldn't restore entity index %i\n", entry->entityIndex );

		Vector testspot = entry->position + entry->impactPlaneNormal * 5.0f;
		Vector testend = entry->position + entry->impactPlaneNormal * -5.0f;

		UTIL_TraceLine( testspot, testend, ignore_monsters, NULL, &tr );

		// NOTE: this code may does wrong result on moving brushes e.g. func_tracktrain
		if( tr.flFraction != 1.0f && !tr.fAllSolid )
		{
			// check impact plane normal
			float	dot = DotProduct( entry->impactPlaneNormal, tr.vecPlaneNormal );

			if( dot >= 0.95f )
			{
				entityIndex = ENTINDEX( tr.pHit );
				if( entityIndex > 0 ) modelIndex = tr.pHit->v.modelindex;
				UTIL_RestoreCustomDecal( tr.vecEndPos, entry->impactPlaneNormal, entityIndex, modelIndex, entry->name, flags, entry->scale );
			}
		}
	}
	else
	{
		// global entity is exist on new level so we can apply decal in local space
		// NOTE: this case also used for transition world decals
		UTIL_RestoreCustomDecal( entry->position, entry->impactPlaneNormal, entityIndex, pEdict->v.modelindex, entry->name, flags, entry->scale );
	}

	return TRUE;
}

// point to create generic entities
int SV_CreateEntity( edict_t *pent, const char *szName )
{
	if( !Q_strnicmp( szName, "ammo_", 5 ))
	{
		GetClassPtr(( CBasePlayerAmmo *)&pent->v );		// alloc private data
		pent->v.netname = pent->v.classname;			// member virtual name
		pent->v.classname = MAKE_STRING( "ammo_generic" );	// to allow save\restore

		return 0;
	}
	else if( !Q_strnicmp( szName, "weapon_", 7 ))
	{
		GetClassPtr(( CBasePlayerItem *)&pent->v );		// alloc private data
		pent->v.netname = pent->v.classname;			// member virtual name
		pent->v.classname = MAKE_STRING( "weapon_generic" );	// to allow save\restore

		return 0;
	}

	return -1;
}

int EngineAllowSaveGame( void )
{
	return g_fAllowSaves;
}

void SV_ProcessModelData( model_t *mod, qboolean create, const byte *buffer )
{
	CRC32_t ulCrc;

	// g-cont. probably this is redundant :-)
	if( !IS_DEDICATED_SERVER( ))
		return;

	if( FBitSet( mod->flags, MODEL_WORLD ))
		SV_ProcessWorldData( mod, create, buffer );

	if( mod->type == mod_studio )
	{
		if( create )
		{
			studiohdr_t *src = (studiohdr_t *)buffer;
			CRC32_INIT( &ulCrc );
			CRC32_PROCESS_BUFFER( &ulCrc, (byte *)buffer, src->length );
			mod->modelCRC = CRC32_FINAL( ulCrc );
		}
		else
		{
			// release collision mesh
			if( mod->bodymesh != NULL )
			{
				mod->bodymesh->CMeshDesc::~CMeshDesc();
				Mem_Free( mod->bodymesh );
				mod->bodymesh = NULL;
			}
		}
	}
}

//
// Xash3D physics interface
//
static physics_interface_t gPhysicsInterface = 
{
	SV_PHYSICS_INTERFACE_VERSION,
	SV_CreateEntity,
	NULL,
	NULL,
	NULL,
	EngineAllowSaveGame,
	NULL,	// not needs
	EngineSetFeatures,
	NULL,
	NULL,
	NULL,
	SV_ClipMoveToEntity,
	SV_ClipPMoveToEntity,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SV_RestoreDecal,
	NULL,
	SV_ProcessModelData,
};

int Server_GetPhysicsInterface( int iVersion, server_physics_api_t *pfuncsFromEngine, physics_interface_t *pFunctionTable )
{
	if ( !pFunctionTable || !pfuncsFromEngine || iVersion != SV_PHYSICS_INTERFACE_VERSION )
	{
		return FALSE;
	}

	size_t iExportSize = sizeof( server_physics_api_t );
	size_t iImportSize = sizeof( physics_interface_t );

	// NOTE: the very old versions NOT have information about current build in any case
	if( g_iXashEngineBuildNumber <= 1910 )
	{
		if( g_fXashEngine )
			ALERT( at_console, "very oldest version of Xash3D was detected. Engine features was disabled.\n" );

		// interface sizes for build 1905 and older
		iExportSize = 28;
		iImportSize = 24;
	}

	if( g_iXashEngineBuildNumber <= 3584 )
	{
		if( g_fXashEngine )
			ALERT( at_console, "old version of Xash3D was detected. Some engine features was disabled.\n" );

		// interface sizes for build 3584 and older
		iExportSize = 80;
		iImportSize = 80;
	}

	// copy new physics interface
	memcpy( &g_physfuncs, pfuncsFromEngine, iExportSize );

	// fill engine callbacks
	memcpy( pFunctionTable, &gPhysicsInterface, iImportSize );

	g_fPhysicInitialized = TRUE;

	return TRUE;
}