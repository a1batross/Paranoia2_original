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
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "pm_shared.h"
#include "stringlib.h"
#include <string.h>
#include <stdio.h>

#include "ammo.h"
#include "ammohistory.h"
#include "vgui_TeamFortressViewport.h"

WEAPON *gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
						// this points to the active weapon menu item
//WEAPON *gpLastSel;		// Last weapon menu selection buz: нах

client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);

WeaponsResource gWR;
extern int g_iGunMode; // buz

int g_ammoAdded = 0; // buz

int g_weaponselect = 0;

void WeaponsResource :: LoadAllWeaponSprites( void )
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if ( rgWeapons[i].iId )
			LoadWeaponSprites( &rgWeapons[i] );
	}
}

int WeaponsResource :: CountAmmo( int iId ) 
{ 
	if ( iId < 0 )
		return 0;

	return riAmmo[iId];
}

int WeaponsResource :: HasAmmo( WEAPON *p )
{
	if ( !p )
		return FALSE;

	// weapons with no max ammo can always be selected
	if ( p->iMax1 == -1 )
		return TRUE;

	return (p->iAmmoType == -1) || p->iClip > 0 || CountAmmo(p->iAmmoType) 
		|| CountAmmo(p->iAmmo2Type) || ( p->iFlags & WEAPON_FLAGS_SELECTONEMPTY );
}


void WeaponsResource :: LoadWeaponSprites( WEAPON *pWeapon )
{
	int i, iRes;

	if (ScreenWidth < 640)
		iRes = 320;
	else
		iRes = 640;

	char sz[128];

	if ( !pWeapon )
		return;

	memset( &pWeapon->rcActive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcNoAmmo, 0, sizeof(wrect_t) ); // buz
	memset( &pWeapon->rcInactive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo2, 0, sizeof(wrect_t) );
	pWeapon->hInactive = 0;
	pWeapon->hNoAmmo = 0; // buz
	pWeapon->hActive = 0;
	pWeapon->hAmmo = 0;
	pWeapon->hAmmo2 = 0;

	sprintf(sz, "scripts/weapons/%s.txt", pWeapon->szName);
	client_sprite_t *pList = SPR2_GetList(sz, &i);

	if (!pList)
		return;

	client_sprite_t *p;
	
	p = GetSpriteList( pList, "crosshair", iRes, i );
	if (p)
	{
		pWeapon->hCrosshair = SPR_Load( p->szSprite );
		pWeapon->rcCrosshair = p->rc;
	}
	else
		pWeapon->hCrosshair = NULL;

	p = GetSpriteList(pList, "autoaim", iRes, i);
	if (p)
	{
		pWeapon->hAutoaim = SPR_Load( p->szSprite );
		pWeapon->rcAutoaim = p->rc;
	}
	else
		pWeapon->hAutoaim = 0;

	p = GetSpriteList( pList, "zoom", iRes, i );
	if (p)
	{
		pWeapon->hZoomedCrosshair = SPR_Load( p->szSprite );
		pWeapon->rcZoomedCrosshair = p->rc;
	}
	else
	{
		pWeapon->hZoomedCrosshair = pWeapon->hCrosshair; //default to non-zoomed crosshair
		pWeapon->rcZoomedCrosshair = pWeapon->rcCrosshair;
	}

	p = GetSpriteList(pList, "zoom_autoaim", iRes, i);
	if (p)
	{
		pWeapon->hZoomedAutoaim = SPR_Load( p->szSprite );
		pWeapon->rcZoomedAutoaim = p->rc;
	}
	else
	{
		pWeapon->hZoomedAutoaim = pWeapon->hZoomedCrosshair;  //default to zoomed crosshair
		pWeapon->rcZoomedAutoaim = pWeapon->rcZoomedCrosshair;
	}

	p = GetSpriteList(pList, "weapon", iRes, i);
	if (p)
	{
		pWeapon->hInactive = SPR_Load( p->szSprite );
		pWeapon->rcInactive = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hInactive = 0;

	// buz
	p = GetSpriteList(pList, "noammo", iRes, i);
	if (p)
	{
		pWeapon->hNoAmmo = SPR_Load( p->szSprite );
		pWeapon->rcNoAmmo = p->rc;
	}
	else
		pWeapon->hNoAmmo = 0;

	p = GetSpriteList(pList, "weapon_s", iRes, i);
	if (p)
	{
		pWeapon->hActive = SPR_Load( p->szSprite );
		pWeapon->rcActive = p->rc;
	}
	else
		pWeapon->hActive = 0;

	p = GetSpriteList(pList, "ammo", iRes, i);
	if (p)
	{
		pWeapon->hAmmo = SPR_Load( p->szSprite );
		pWeapon->rcAmmo = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hAmmo = 0;

	p = GetSpriteList(pList, "ammo2", iRes, i);
	if (p)
	{
		pWeapon->hAmmo2 = SPR_Load( p->szSprite );
		pWeapon->rcAmmo2 = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hAmmo2 = 0;


	delete [] pList;
}

// Returns the first weapon for a given slot.
WEAPON *WeaponsResource :: GetFirstPos( int iSlot )
{
	WEAPON *pret = NULL;

	for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
	{
		if ( rgSlots[iSlot][i] && HasAmmo( rgSlots[iSlot][i] ) )
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}

// buz
WEAPON *WeaponsResource :: GetLastPos( int iSlot )
{
	WEAPON *pret = NULL;

	for (int i = MAX_WEAPON_POSITIONS - 1; i >= 0; i--)
	{
		if ( rgSlots[iSlot][i] && HasAmmo( rgSlots[iSlot][i] ) )
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}


WEAPON* WeaponsResource :: GetNextActivePos( int iSlot, int iSlotPos )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	WEAPON *p = gWR.rgSlots[ iSlot ][ iSlotPos+1 ];
	
	if ( !p || !gWR.HasAmmo(p) )
		return GetNextActivePos( iSlot, iSlotPos + 1 );

	return p;
}


WEAPON* WeaponsResource :: GetPrevActivePos( int iSlot, int iSlotPos ) // buz
{
	if ( iSlotPos < 1 || iSlot < 0 )
		return NULL;

	WEAPON *p = gWR.rgSlots[ iSlot ][ iSlotPos - 1 ];
	
	if ( !p || !gWR.HasAmmo(p) )
		return GetPrevActivePos( iSlot, iSlotPos - 1 );

	return p;
}


int giBucketHeight, giBucketWidth, giABHeight, giABWidth; // Ammo Bar width and height

HSPRITE ghsprBuckets;					// Sprite for top row of weapons menu

DECLARE_MESSAGE(m_Ammo, CurWeapon );	// Current weapon and clip
DECLARE_MESSAGE(m_Ammo, WeaponList);	// new weapon type
DECLARE_MESSAGE(m_Ammo, AmmoX);			// update known ammo type's count
DECLARE_MESSAGE(m_Ammo, AmmoPickup);	// flashes an ammo pickup record
DECLARE_MESSAGE(m_Ammo, WeapPickup);    // flashes a weapon pickup record
DECLARE_MESSAGE(m_Ammo, HideWeapon);	// hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE(m_Ammo, ItemPickup);

DECLARE_COMMAND(m_Ammo, Slot1);
DECLARE_COMMAND(m_Ammo, Slot2);
DECLARE_COMMAND(m_Ammo, Slot3);
DECLARE_COMMAND(m_Ammo, Slot4);
DECLARE_COMMAND(m_Ammo, Slot5);
DECLARE_COMMAND(m_Ammo, Slot6);
DECLARE_COMMAND(m_Ammo, Slot7);
DECLARE_COMMAND(m_Ammo, Slot8);
DECLARE_COMMAND(m_Ammo, Slot9);
DECLARE_COMMAND(m_Ammo, Slot10);
DECLARE_COMMAND(m_Ammo, Close);
DECLARE_COMMAND(m_Ammo, NextWeapon);
DECLARE_COMMAND(m_Ammo, PrevWeapon);

// width of ammo fonts
#define AMMO_SMALL_WIDTH 10
#define AMMO_LARGE_WIDTH 20

#define HISTORY_DRAW_TIME	"5"

int CHudAmmo::Init(void)
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(CurWeapon);
	HOOK_MESSAGE(WeaponList);
	HOOK_MESSAGE(AmmoPickup);
	HOOK_MESSAGE(WeapPickup);
	HOOK_MESSAGE(ItemPickup);
	HOOK_MESSAGE(HideWeapon);
	HOOK_MESSAGE(AmmoX);

	HOOK_COMMAND("slot1", Slot1);
	HOOK_COMMAND("slot2", Slot2);
	HOOK_COMMAND("slot3", Slot3);
	HOOK_COMMAND("slot4", Slot4);
	HOOK_COMMAND("slot5", Slot5);
	HOOK_COMMAND("slot6", Slot6);
	HOOK_COMMAND("slot7", Slot7);
	HOOK_COMMAND("slot8", Slot8);
	HOOK_COMMAND("slot9", Slot9);
	HOOK_COMMAND("slot10", Slot10);
	HOOK_COMMAND("cancelselect", Close);
	HOOK_COMMAND("invnext", NextWeapon);
	HOOK_COMMAND("invprev", PrevWeapon);

	Reset();

	CVAR_REGISTER( "hud_drawhistory_time", HISTORY_DRAW_TIME, 0 );
	CVAR_REGISTER( "hud_fastswitch", "0", FCVAR_ARCHIVE );		// controls whether or not weapons can be selected in one keypress

	m_iFlags |= HUD_ACTIVE; //!!!

	gWR.Init();
	gHR.Init();

	return 1;
};


// buz: loads menu settings from menu_settings.txt
void CHudAmmo::LoadMenuSettings( void )
{
	char token[64];
	char *pfile;
	char *pf2;

	m_menuSizeX = XRES(170);
	m_menuSizeY = YRES(45);
	m_menuScale = 0.4;
	m_menuMinAlpha = 0.35;
	m_menuAddAlpha = 0.8;
	m_menuSpeed = 5;
	m_menuRenderMode = kRenderTransAlpha;
	m_menuOfsX = XRES(10);
	m_menuOfsY = YRES(10);
	m_menuSpaceX = XRES(5);
	m_menuSpaceY = 0;
	m_menuNumberSizeX = XRES(12);
	m_menuNumberSizeY = YRES(12);

	pfile = (char *)gEngfuncs.COM_LoadFile( "scripts/menu_settings.txt", 5, NULL);
	pf2 = pfile;
	if (!pfile)
	{
		gEngfuncs.Con_Printf("Error: couldn't load menu_settings.txt, using default\n");
		m_menuAddAlpha = m_menuAddAlpha - m_menuMinAlpha;
		return;
	}

	while (true)
	{
		pfile = COM_ParseFile(pfile, token);
		if (!pfile)
		break;

		if (!stricmp(token, "SizeX"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuSizeX = XRES(atoi(token));
		}
		else if (!stricmp(token, "SizeY"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuSizeY = YRES(atoi(token));
		}
		else if (!stricmp(token, "NumberSizeX"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuNumberSizeX = XRES(atoi(token));
		}
		else if (!stricmp(token, "NumberSizeY"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuNumberSizeY = YRES(atoi(token));
		}
		else if (!stricmp(token, "OfsX"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuOfsX = XRES(atoi(token));
		}
		else if (!stricmp(token, "OfsY"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuOfsY = YRES(atoi(token));
		}
		else if (!stricmp(token, "SpaceX"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuSpaceX = XRES(atoi(token));
		}
		else if (!stricmp(token, "SpaceY"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuSpaceY = YRES(atoi(token));
		}
		else if (!stricmp(token, "Scale"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuScale = atof(token);
		}
		else if (!stricmp(token, "MinAlpha"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuMinAlpha = atof(token);
		}
		else if (!stricmp(token, "MaxAlpha"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuAddAlpha = atof(token);
		}
		else if (!stricmp(token, "Speed"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuSpeed = atof(token);
		}
		else if (!stricmp(token, "RenderMode"))
		{
			pfile = COM_ParseFile(pfile, token);
			m_menuRenderMode = atoi(token);
		}
		else
			gEngfuncs.Con_Printf("Error: unknown token %s in file menu_settings.txt\n", token);
	}

	m_menuAddAlpha = m_menuAddAlpha - m_menuMinAlpha;

/*	gEngfuncs.Con_Printf("m_menuSizeX %d\n",m_menuSizeX);
	gEngfuncs.Con_Printf("m_menuSizeY %d\n",m_menuSizeY);
	gEngfuncs.Con_Printf("m_menuScale %f\n",m_menuScale);
	gEngfuncs.Con_Printf("m_menuMinAlpha %f\n",m_menuMinAlpha);
	gEngfuncs.Con_Printf("m_menuAddAlpha %f\n",m_menuAddAlpha);
	gEngfuncs.Con_Printf("m_menuSpeed %f\n",m_menuSpeed);
	gEngfuncs.Con_Printf("m_menuRenderMode %d\n",m_menuRenderMode);*/

	gEngfuncs.COM_FreeFile( pf2 );
}


void CHudAmmo::Reset(void)
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!

	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;

	gWR.Reset();
	gHR.Reset();

	//	VidInit();

}

int CHudAmmo::VidInit(void)
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex( "bucket1" );
	m_HUD_selection = gHUD.GetSpriteIndex( "selection" );
	m_mach = gHUD.GetSpriteIndex( "machinegun" );

	ghsprBuckets = gHUD.GetSprite(m_HUD_bucket0);
	giBucketWidth = gHUD.GetSpriteRect(m_HUD_bucket0).right - gHUD.GetSpriteRect(m_HUD_bucket0).left;
	giBucketHeight = gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top;

	gHR.iHistoryGap = max( gHR.iHistoryGap, gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top);

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();

	if (ScreenWidth >= 640)
	{
		giABWidth = 20;
		giABHeight = 4;
	}
	else
	{
		giABWidth = 10;
		giABHeight = 2;
	}

	LoadMenuSettings(); // buz

	return 1;
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think(void)
{
	if ( gHUD.m_fPlayerDead )
		return;

	if ( gHUD.m_iWeaponBits != gWR.iOldWeaponBits )
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;

		for (int i = MAX_WEAPONS-1; i > 0; i-- )
		{
			WEAPON *p = gWR.GetWeapon(i);

			if ( p )
			{
				if ( gHUD.m_iWeaponBits & ( 1 << p->iId ) )
					gWR.PickupWeapon( p );
				else
					gWR.DropWeapon( p );
			}
		}
	}

//	if (!gpActiveSel)
	if (gWR.m_iSelectedColumn == -1) // buz
		return;

	// has the player selected one?
	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		if (gpActiveSel)
		{
			ServerCmd(gpActiveSel->szName);
			g_weaponselect = gpActiveSel->iId;
		}

	//	gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;
		gWR.m_iSelectedColumn = -1;

		PlaySound("common/wpn_select.wav", 1);
	}

}

//
// Helper function to return a Ammo pointer from id
//

HSPRITE* WeaponsResource :: GetAmmoPicFromWeapon( int iAmmoId, wrect_t& rect )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( rgWeapons[i].iAmmoType == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo;
			return &rgWeapons[i].hAmmo;
		}
		else if ( rgWeapons[i].iAmmo2Type == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo2;
			return &rgWeapons[i].hAmmo2;
		}
	}

	return NULL;
}


// Menu Selection Code

void WeaponsResource :: SelectSlot( int iSlot, int fAdvance, int iDirection )
{
	if ( gHUD.m_Menu.m_fMenuDisplayed && (fAdvance == FALSE) && (iDirection == 1) )
	{ // menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem( iSlot + 1 );  // slots are one off the key numbers
		return;
	}

	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	if ( gHUD.m_fPlayerDead || gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
		return;

	if ( !FBitSet( gHUD.m_iHideHUDDisplay, ITEM_SUIT ))
		return;	// no suit?

	if ( !gHUD.m_iWeaponBits )
		return;	// no weapons?

	WEAPON *p = NULL;
	bool fastSwitch = CVAR_GET_FLOAT( "hud_fastswitch" ) != 0;

	if ( m_iSelectedColumn == -1 )
	{
		m_iSelectedColumn = iSlot;
		memset(m_rgColumnSizes, 0, sizeof m_rgColumnSizes);
		m_rgColumnSizes[iSlot] = 1;
		gpActiveSel = GetFirstPos( iSlot );

		// Wargon: hud_fastswitch для нового HUD'а.
		if (gpActiveSel && fastSwitch && !(GetNextActivePos(gpActiveSel->iSlot, gpActiveSel->iSlotPos)) && (iSlot != 0))
		{
			ServerCmd(gpActiveSel->szName);
			g_weaponselect = gpActiveSel->iId;
			gpActiveSel = NULL;
			gWR.m_iSelectedColumn = -1;
			return;
		}

		PlaySound( "common/wpn_hudon.wav", 1 );
		return;
	}
	else
	{
		if (iSlot == m_iSelectedColumn)
		{
			if (!gpActiveSel)
				return;
			PlaySound("common/wpn_moveselect.wav", 1);
			gpActiveSel = GetNextActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );
			if (!gpActiveSel)
				gpActiveSel = GetFirstPos( iSlot );
			return;
		}
		else
		{
			m_iSelectedColumn = iSlot;
			gpActiveSel = GetFirstPos( iSlot );

			// Wargon: hud_fastswitch для нового HUD'а.
			if (gpActiveSel && fastSwitch && !(GetNextActivePos(gpActiveSel->iSlot, gpActiveSel->iSlotPos)) && (iSlot != 0))
			{
				ServerCmd(gpActiveSel->szName);
				g_weaponselect = gpActiveSel->iId;
				gpActiveSel = NULL;
				gWR.m_iSelectedColumn = -1;
				return;
			}

			PlaySound( "common/wpn_hudon.wav", 1 );
			return;
		}
	}
}

//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

//
// AmmoX  -- Update the count of a known type of ammo
// 
int CHudAmmo::MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	gWR.SetAmmo( iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();
	const char *ammoname = READ_STRING();

	// Add ammo to the history
//	gHR.AddToHistory( HISTSLOT_AMMO, iIndex, abs(iCount) );
	// buz: use new text-based vgui ammo history
	if (ammoname)
	{
		if (!strcmp(ammoname, "Hand Grenade"))
			ammoname = "handgrenade"; // prevent spaces in ammo name..

		char msgname[256];
		sprintf(msgname, "!%s", ammoname); // dont show message if not found in titles.txt
		g_ammoAdded = iCount;
		gHUD.m_Message.MessageAdd( msgname, gEngfuncs.GetClientTime() );
		g_ammoAdded = 0;
	//	CONPRINT("[%s] - %d\n", msgname, iCount);
	}

	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();

	// Add the weapon to the history
//	gHR.AddToHistory( HISTSLOT_WEAP, iIndex );
	
	// buz: use new text-based vgui ammo history
	WEAPON *weap = gWR.GetWeapon( iIndex );
	if (weap)
	{
		char msgname[256];
		sprintf(msgname, "!%s", weap->szName); // dont show message if not found in titles.txt
		gHUD.m_Message.MessageAdd( msgname, gEngfuncs.GetClientTime() );
	//	CONPRINT("[%s]\n", msgname);
	}

	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *szName = READ_STRING();

	// Add the weapon to the history
//	gHR.AddToHistory( HISTSLOT_ITEM, szName );
	// buz: use new text-based vgui ammo history
	if (szName)
	{
		char msgname[256];
		sprintf(msgname, "!%s", szName); // dont show message if not found in titles.txt
		gHUD.m_Message.MessageAdd( msgname, gEngfuncs.GetClientTime() );
	//	CONPRINT("[%s]\n", msgname);
	}

	return 1;
}

int CHudAmmo :: MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	gHUD.m_iHideHUDDisplay = READ_BYTE();

	// buz: dont draw fucking crosshairs!
	static wrect_t nullrc;
	SetCrosshair( 0, nullrc, 0, 0, 0 );

	return 1;
}

// 
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type 
//  numbers match a real ammo type.
//
int CHudAmmo::MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf )
{
	static wrect_t nullrc;
	int fOnTarget = FALSE;

	BEGIN_READ( pbuf, iSize );

	int iState = READ_BYTE();
	int iId = READ_CHAR();
	int iClip = READ_CHAR();

	// detect if we're also on target
	if ( iState > 1 )
	{
		fOnTarget = TRUE;
	}

	if ( iId < 1 )
	{
		SetCrosshair(0, nullrc, 0, 0, 0);
		m_pWeapon = NULL; //LRC
		return 0;
	}

	if ( g_iUser1 != OBS_IN_EYE )
	{
		// Is player dead???
		if ((iId == -1) && (iClip == -1))
		{
			gHUD.m_fPlayerDead = TRUE;
			gpActiveSel = NULL;
			return 1;
		}
		gHUD.m_fPlayerDead = FALSE;
	}

	WEAPON *pWeapon = gWR.GetWeapon( iId );

	if ( !pWeapon )
		return 0;

	if ( iClip < -1 )
		pWeapon->iClip = abs(iClip);
	else
		pWeapon->iClip = iClip;


	if ( iState == 0 )	// we're not the current weapon, so update no more
		return 1;

	m_pWeapon = pWeapon;

	// buz: dont draw fucking crosshairs!
	SetCrosshair( 0, nullrc, 0, 0, 0 );

	m_fFade = 200.0f; //!!!
	m_iFlags |= HUD_ACTIVE;
	
	return 1;
}

//
// WeaponList -- Tells the hud about a new weapon type.
//
int CHudAmmo::MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	WEAPON Weapon;

	strcpy( Weapon.szName, READ_STRING() );
	Weapon.iAmmoType = (int)READ_CHAR();	
	
	Weapon.iMax1 = READ_BYTE();
	if (Weapon.iMax1 == 255)
		Weapon.iMax1 = -1;

	Weapon.iAmmo2Type = READ_CHAR();
	Weapon.iMax2 = READ_BYTE();
	if (Weapon.iMax2 == 255)
		Weapon.iMax2 = -1;

	Weapon.iSlot = READ_CHAR();
	Weapon.iSlotPos = READ_CHAR();
	Weapon.iId = READ_CHAR();
	Weapon.iFlags = READ_SHORT();
	Weapon.iClip = 0;

	gWR.AddWeapon( &Weapon );

	return 1;

}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
// Slot button pressed
void CHudAmmo::SlotInput( int iSlot )
{
	// Let the Viewport use it first, for menus
	if ( gViewPort && gViewPort->SlotInput( iSlot ) )
		return;

	gWR.SelectSlot(iSlot, FALSE, 1);
}

void CHudAmmo::UserCmd_Slot1(void)
{
	SlotInput( 0 );
}

void CHudAmmo::UserCmd_Slot2(void)
{
	SlotInput( 1 );
}

void CHudAmmo::UserCmd_Slot3(void)
{
	SlotInput( 2 );
}

void CHudAmmo::UserCmd_Slot4(void)
{
	SlotInput( 3 );
}

void CHudAmmo::UserCmd_Slot5(void)
{
	SlotInput( 4 );
}

void CHudAmmo::UserCmd_Slot6(void)
{
	SlotInput( 5 );
}

void CHudAmmo::UserCmd_Slot7(void)
{
	SlotInput( 6 );
}

void CHudAmmo::UserCmd_Slot8(void)
{
	SlotInput( 7 );
}

void CHudAmmo::UserCmd_Slot9(void)
{
	SlotInput( 8 );
}

void CHudAmmo::UserCmd_Slot10(void)
{
	SlotInput( 9 );
}

void CHudAmmo::UserCmd_Close(void)
{
	if (gWR.m_iSelectedColumn > -1)
	{
//		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gWR.m_iSelectedColumn = -1; // buz
		PlaySound("common/wpn_hudoff.wav", 1);
	}
	else
		ClientCmd("escape");
}


// Selects the next item in the weapon menu
// buz - rewritten
void CHudAmmo::UserCmd_NextWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if (gWR.m_iSelectedColumn == -1)
	{
		if (m_pWeapon)
		{
			gWR.SelectSlot(m_pWeapon->iSlot, FALSE, 1);
		}
		return;
	}

	if (gpActiveSel)
		gpActiveSel = gWR.GetNextActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );

	if (!gpActiveSel)
	{
		for (int i = 1; i <= MAX_WEAPON_SLOTS; i++)
		{
			int check = gWR.m_iSelectedColumn + i;
			if (check >= MAX_WEAPON_SLOTS)
				check -= MAX_WEAPON_SLOTS;

			gpActiveSel = gWR.GetFirstPos( check );
			if (gpActiveSel)
			{
				gWR.m_iSelectedColumn = gpActiveSel->iSlot;
				break;
			}
		}
	}

	if (gpActiveSel)
		PlaySound("common/wpn_moveselect.wav", 1);
}

// Selects the previous item in the menu
// buz - rewritten
void CHudAmmo::UserCmd_PrevWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if (gWR.m_iSelectedColumn == -1)
	{
		if (m_pWeapon)
		{
			gWR.SelectSlot(m_pWeapon->iSlot, FALSE, 1);
		}
		return;
	}

	if (gpActiveSel)
		gpActiveSel = gWR.GetPrevActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );

	if (!gpActiveSel)
	{
		for (int i = 1; i <= MAX_WEAPON_SLOTS; i++)
		{
			int check = gWR.m_iSelectedColumn - i;
			if (check < 0)
				check = MAX_WEAPON_SLOTS + check;

			gpActiveSel = gWR.GetLastPos( check );
			if (gpActiveSel)
			{
				gWR.m_iSelectedColumn = gpActiveSel->iSlot;
				break;
			}
		}
	}

	if (gpActiveSel)
		PlaySound("common/wpn_moveselect.wav", 1);
}



//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------

int CHudAmmo::Draw(float flTime)
{
	if (!FBitSet( gHUD.m_iHideHUDDisplay, ITEM_SUIT ))
		return 1;

	if ( (gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )))
		return 1;

	// Draw Weapon Menu
	DrawWList(flTime);

	// Draw ammo pickup history
	gHR.DrawAmmoHistory( flTime );

	if (!(m_iFlags & HUD_ACTIVE))
		return 0;

	if (!m_pWeapon)
	{
		return 0;
	}

	WEAPON *pw = m_pWeapon; // shorthand

	// SPR_Draw Ammo
	if ((pw->iAmmoType < 0) && (pw->iAmmo2Type < 0))
		return 0;

	// buz: draw crosshair...
	if ( g_iGunMode == 3 )
	{
		SPR_Set(m_pWeapon->hZoomedCrosshair, 255, 255, 255);
		SPR_DrawHoles(0,
			(ScreenWidth - SPR_Width(m_pWeapon->hZoomedCrosshair, 0))/2,
			(ScreenHeight - SPR_Height(m_pWeapon->hZoomedCrosshair, 0))/2,
			&m_pWeapon->rcZoomedCrosshair);
	}

	return 1;
}


//
// Draws the ammo bar on the hud
//
int DrawBar(int x, int y, int width, int height, float f)
{
	int r, g, b;

	if (f < 0)
		f = 0;
	if (f > 1)
		f = 1;

	if (f)
	{
		int w = f * width;

		// Always show at least one pixel if we have ammo.
		if (w <= 0)
			w = 1;
		UnpackRGB(r, g, b, RGB_GREENISH);
		FillRGBA(x, y, w, height, r, g, b, 255);
		x += w;
		width -= w;
	}

	UnpackRGB(r, g, b, gHUD.m_iHUDColor);

	FillRGBA(x, y, width, height, r, g, b, 128);

	return (x + width);
}



void DrawAmmoBar(WEAPON *p, int x, int y, int width, int height)
{
	if ( !p )
		return;
	
	if (p->iAmmoType != -1)
	{
		if (!gWR.CountAmmo(p->iAmmoType))
			return;

		float f = (float)gWR.CountAmmo(p->iAmmoType)/(float)p->iMax1;
		
		x = DrawBar(x, y, width, height, f);


		// Do we have secondary ammo too?

		if (p->iAmmo2Type != -1)
		{
			f = (float)gWR.CountAmmo(p->iAmmo2Type)/(float)p->iMax2;

			x += 5; //!!!

			DrawBar(x, y, width, height, f);
		}
	}
}


// buz: use triapi to draw sprites on screen
void DrawSpriteAsPoly( HSPRITE hspr, wrect_t *rect, wrect_t *screenpos, int mode, float r, float g, float b, float a);

//
// Draw Weapon Menu
//
int CHudAmmo::DrawWList(float flTime)
{
	int x,y,i;
	float a;
	wrect_t screenrect;

//	if ( !gpActiveSel )
	if ( gWR.m_iSelectedColumn == -1 ) // buz
		return 0;

	// buz: correct column sizes
	if (gWR.m_rgColumnSizes[gWR.m_iSelectedColumn] >= 1)
	{
		// dont need correction
		memset(gWR.m_rgColumnSizes, 0, sizeof gWR.m_rgColumnSizes);
		gWR.m_rgColumnSizes[gWR.m_iSelectedColumn] = 1;
	}
	else
	{
		gWR.m_rgColumnSizes[gWR.m_iSelectedColumn] += gHUD.m_flTimeDelta * m_menuSpeed;
		float spaceleft = 1 - gWR.m_rgColumnSizes[gWR.m_iSelectedColumn];
		if (spaceleft < 0)
		{
			memset(gWR.m_rgColumnSizes, 0, sizeof gWR.m_rgColumnSizes);
			gWR.m_rgColumnSizes[gWR.m_iSelectedColumn] = 1;
		}
		else
		{
			float scalefactor = 0;
			for (i = 0; i < MAX_WEAPON_SLOTS; i++)
			{
				if (i != gWR.m_iSelectedColumn)
					scalefactor += gWR.m_rgColumnSizes[i]; // compute, how much space get rest of columns
			}
			scalefactor = spaceleft / scalefactor;
			for (i = 0; i < MAX_WEAPON_SLOTS; i++)
			{
				if (i != gWR.m_iSelectedColumn)
					gWR.m_rgColumnSizes[i] *= scalefactor;
			}
		}
	}

	x = m_menuOfsX;
	for ( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
	//	if ( gWR.m_iSelectedColumn == i )
	//		a = 0.8;
	//	else
	//		a = 0.3;
		a = m_menuMinAlpha + (m_menuAddAlpha * gWR.m_rgColumnSizes[i]);
		
		y = m_menuOfsY;
		screenrect.top = y;
		screenrect.bottom = y + m_menuNumberSizeY;
		screenrect.left = x;
		screenrect.right = x + m_menuNumberSizeX;
		DrawSpriteAsPoly( gHUD.GetSprite(m_HUD_bucket0 + i), &gHUD.GetSpriteRect(m_HUD_bucket0 + i), &screenrect, m_menuRenderMode, 1.0, 1.0, 1.0, m_menuMinAlpha + m_menuAddAlpha);
		y = screenrect.bottom;

		float scale = m_menuScale + ((1-m_menuScale) * gWR.m_rgColumnSizes[i]);
		for ( int pos = 0; pos < MAX_WEAPON_POSITIONS; pos++ )
		{
			if (gWR.rgSlots[i][pos])
			{
				screenrect.top = y;
				screenrect.bottom = y + (m_menuSizeY * scale);
				screenrect.left = x;
				screenrect.right = x + (m_menuSizeX * scale);

				if (gWR.rgSlots[i][pos] == gpActiveSel)
					DrawSpriteAsPoly( gWR.rgSlots[i][pos]->hActive, &gWR.rgSlots[i][pos]->rcActive, &screenrect, m_menuRenderMode, 1.0, 1.0, 1.0, a);
				else
				{
					if ( gWR.HasAmmo(gWR.rgSlots[i][pos]) || (gWR.rgSlots[i][pos]->hNoAmmo == 0))
						DrawSpriteAsPoly( gWR.rgSlots[i][pos]->hInactive, &gWR.rgSlots[i][pos]->rcInactive, &screenrect, m_menuRenderMode, 1.0, 1.0, 1.0, a);
					else
						DrawSpriteAsPoly( gWR.rgSlots[i][pos]->hNoAmmo, &gWR.rgSlots[i][pos]->rcNoAmmo, &screenrect, m_menuRenderMode, 1.0, 1.0, 1.0, a);
				}

				y = screenrect.bottom + m_menuSpaceY;
			}
		}
		x += m_menuSizeX * scale;
		x += m_menuSpaceX;
	}

	return 1;
}


/* =================================
	GetSpriteList

Finds and returns the matching 
sprite name 'psz' and resolution 'iRes'
in the given sprite list 'pList'
iCount is the number of items in the pList
================================= */
client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount)
{
	if (!pList)
		return NULL;

	int i = iCount;
	client_sprite_t *p = pList;

	while(i--)
	{
		if ((!Q_stricmp(psz, p->szName)) && (p->iRes == iRes))
			return p;
		p++;
	}

	return NULL;
}
