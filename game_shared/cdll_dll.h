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
//  cdll_dll.h

// this file is included by both the game-dll and the client-dll,

#ifndef CDLL_DLL_H
#define CDLL_DLL_H

#define MAX_WEAPONS			32	// ???

#define MAX_WEAPON_SLOTS		10	// hud item selection slots
#define MAX_ITEM_TYPES		6	// hud item selection slots

#define MAX_ITEMS			5	// hard coded item types
#define ITEM_ANTIDOTE		0
#define ITEM_SECURITY		1

#define HIDEHUD_WEAPONS		( 1<<0 )
#define HIDEHUD_FLASHLIGHT		( 1<<1 )
#define HIDEHUD_ALL			( 1<<2 )
#define HIDEHUD_HEALTH		( 1<<3 )
#define ITEM_SUIT			( 1<<4 )
#define ITEM_HEADSHIELD		( 1<<5 )
#define ITEM_GASMASK		( 1<<6 )

#define MAX_AMMO_SLOTS  		32		// not really slots

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE		2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

// decal flags
#define FDECAL_PERMANENT		0x01	// This decal should not be removed in favor of any new decals
#define FDECAL_USE_LANDMARK		0x02	// This is a decal applied on a bmodel without origin-brush so we done in absoulute pos
#define FDECAL_CUSTOM		0x04	// This is a custom clan logo and should not be saved/restored
#define FDECAL_PUDDLE		0x08	// Decal is a puddle (use special shader)
#define FDECAL_NORANDOM		0x10	// Decal came from save\restore, so we don't use random select
#define FDECAL_DONTSAVE		0x20	// Decal was loaded from adjacent level, don't save it for this level
#define FDECAL_STUDIO		0x40	// Indicates a studio decal
#define FDECAL_LOCAL_SPACE		0x80	// Decal is in local space (any decal after serialization)

#endif