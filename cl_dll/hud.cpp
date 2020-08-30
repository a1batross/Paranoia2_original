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
// hud.cpp
//
// implementation of CHud class
//

//LRC - define to help track what calls are made on changelevel, save/restore, etc
#define ENGINE_DEBUG

#include "hud.h"
#include "cl_util.h"
#include <stringlib.h>
#include <stdio.h>
#include "parsemsg.h"
#include "hud_servers.h"
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_subtitles.h" // buz
#include "vgui_radio.h" // buz
#include "vgui_hud.h" // buz
#include "vgui_tabpanel.h" // buz
#include "studio.h"
#include "demo.h"
#include "demo_api.h"
#include "vgui_scorepanel.h"
#include "gl_local.h" // buz
#include "r_studioint.h"

cvar_t *cl_rollspeed;
cvar_t *cl_rollangle;

class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	virtual void GetPlayerTextColor(int entindex, int color[3])
	{
		color[0] = color[1] = color[2] = 255;

		if( entindex >= 0 && entindex < sizeof(g_PlayerExtraInfo)/sizeof(g_PlayerExtraInfo[0]) )
		{
			int iTeam = g_PlayerExtraInfo[entindex].teamnumber;

			if ( iTeam < 0 )
			{
				iTeam = 0;
			}

			iTeam = iTeam % iNumberOfTeamColors;

			color[0] = iTeamColors[iTeam][0];
			color[1] = iTeamColors[iTeam][1];
			color[2] = iTeamColors[iTeam][2];
		}
	}

	virtual void UpdateCursorState()
	{
		gViewPort->UpdateCursorState();
	}

	virtual int	GetAckIconHeight()
	{
		return ScreenHeight - gHUD.m_iFontHeight*3 - 6;
	}

	virtual bool			CanShowSpeakerLabels()
	{
		if( gViewPort && gViewPort->m_pScoreBoard )
			return !gViewPort->m_pScoreBoard->isVisible();
		else
			return false;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;


extern client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);
extern cvar_t *sensitivity;
cvar_t *cl_lw = NULL;

void ShutdownInput (void);

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_Logo(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_Logo(pszName, iSize, pbuf );
}

//DECLARE_MESSAGE(m_Logo, Logo)
//LRC
int __MsgFunc_HUDColor(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_HUDColor(pszName, iSize, pbuf );
}

//LRC
int __MsgFunc_SetFog(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_SetFog( pszName, iSize, pbuf );
	return 1;
}

//LRC
int __MsgFunc_KeyedDLight(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_KeyedDLight( pszName, iSize, pbuf );
	return 1;
}

//LRC
int __MsgFunc_SetSky(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_SetSky( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf)
{
#ifdef ENGINE_DEBUG
//	CONPRINT("## ResetHUD\n");
#endif
	return gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf );
}

int __MsgFunc_InitHUD(const char *pszName, int iSize, void *pbuf)
{
#ifdef ENGINE_DEBUG
//	CONPRINT("## InitHUD\n");
#endif
	gHUD.MsgFunc_InitHUD( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_ViewMode(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_ViewMode( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_SetFOV(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_SetFOV( pszName, iSize, pbuf );
}

int __MsgFunc_Concuss(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_Concuss( pszName, iSize, pbuf );
}

int __MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_GameMode( pszName, iSize, pbuf );
}

int __MsgFunc_WaterSplash( const char *pszName, int iSize, void *pbuf )
{
    gHUD.MsgFunc_WaterSplash( pszName, iSize, pbuf );
    return 1;
}

int __MsgFunc_NewExplode( const char *pszName, int iSize, void *pbuf )
{
    gHUD.MsgFunc_NewExplode( pszName, iSize, pbuf );
    return 1;
}

// buz
int __MsgFunc_GasMask(const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_GasMask( pszName, iSize, pbuf );
}

// buz
int __MsgFunc_SpecTank(const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_SpecTank( pszName, iSize, pbuf );
}

int __MsgFunc_HeadShield(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_HeadShield( pszName, iSize, pbuf );
}

int __MsgFunc_Particle(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_Particle( pszName, iSize, pbuf );
}

int __MsgFunc_DelParticle(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_DelParticle( pszName, iSize, pbuf );
}

int __MsgFunc_WeaponAnim(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_WeaponAnim( pszName, iSize, pbuf );
}

int __MsgFunc_WeaponBody(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_WeaponBody( pszName, iSize, pbuf );
}

int __MsgFunc_WeaponSkin(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_WeaponSkin( pszName, iSize, pbuf );
}

int __MsgFunc_SkyMarker(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_SkyMarker( pszName, iSize, pbuf );
}

int __MsgFunc_WorldMarker(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_WorldMarker( pszName, iSize, pbuf );
}

int __MsgFunc_CustomDecal(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_CustomDecal( pszName, iSize, pbuf );
}

int __MsgFunc_StudioDecal(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_StudioDecal( pszName, iSize, pbuf );
}

int __MsgFunc_PartEffect(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_PartEffect( pszName, iSize, pbuf );
}

int __MsgFunc_LevelTime(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_LevelTime( pszName, iSize, pbuf );
}

int __MsgFunc_BlurEffect(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_BlurEffect( pszName, iSize, pbuf );
	return 1;
}

// TFFree Command Menu
void __CmdFunc_OpenCommandMenu(void)
{
	if ( gViewPort )
	{
		gViewPort->ShowCommandMenu( gViewPort->m_StandardMenu );
	}
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial(void)
{
	if ( gViewPort )
	{
		gViewPort->InputPlayerSpecial();
	}
}

void __CmdFunc_CloseCommandMenu(void)
{
	if ( gViewPort )
	{
		gViewPort->InputSignalHideCommandMenu();
	}
}

void __CmdFunc_ForceCloseCommandMenu( void )
{
	if ( gViewPort )
	{
		gViewPort->HideCommandMenu();
	}
}

void __CmdFunc_ToggleServerBrowser( void )
{
	if ( gViewPort )
	{
		gViewPort->ToggleServerBrowser();
	}
}

// TFFree Command Menu Message Handlers
int __MsgFunc_ValClass(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ValClass( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamNames(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamNames( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_Feign(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Feign( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_Detpack(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Detpack( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_VGUIMenu(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_VGUIMenu( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_MOTD(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_MOTD( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_BuildSt(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_BuildSt( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_RandomPC(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_RandomPC( pszName, iSize, pbuf );
	return 0;
}
 
int __MsgFunc_ServerName(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ServerName( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_ScoreInfo(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ScoreInfo( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamScore(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamScore( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamInfo(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamInfo( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_Spectator(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Spectator( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_AllowSpec(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_AllowSpec( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_MusicFade(const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_MusicFade( pszName, iSize, pbuf );
}

// buz
int __MsgFunc_TextWindow(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ShowTextWindow( pszName, iSize, pbuf );
	return 0;
}

// buz
int __MsgFunc_RainData(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_RainData( pszName, iSize, pbuf );
}

int MsgCustomDlight(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	vec3_t pos;
	pos.x = READ_COORD();
	pos.y = READ_COORD();
	pos.z = READ_COORD();
	float radius = (float)READ_BYTE() * 10;
	float life = (float)READ_BYTE() / 10;
	float decay = (float)READ_BYTE() * 10;

	CDynLight *dl = CL_AllocDlight (0);

	R_SetupLightParams( dl, pos, g_vecZero, radius, 0.0f, LIGHT_OMNI );

	dl->color = Vector( 0.7f, 0.6f, 0.5f );
	dl->die = GET_CLIENT_TIME() + life;
	dl->decay = decay;

	return 1;
}

// debug command
// usage: makelight [R G B] [radius] [life]\n" );
void MakeLight( void )
{
	CDynLight *dl = CL_AllocDlight( 0 );

	if( CMD_ARGC() >= 4 )
	{
		dl->color[0] = Q_atof(CMD_ARGV( 1 ));
		dl->color[1] = Q_atof(CMD_ARGV( 2 ));
		dl->color[2] = Q_atof(CMD_ARGV( 3 ));
	}
	else
	{
		dl->color = Vector( 0.7f, 0.6f, 0.5f );
	}

	float radius;

	if( CMD_ARGC() >= 5 )
		radius = Q_atof(CMD_ARGV( 4 ));
	else radius = 128.0f;

	R_SetupLightParams( dl, GetVieworg(), g_vecZero, radius, 0.0f, LIGHT_OMNI );

	if( CMD_ARGC() >= 6 )
		dl->die = GET_CLIENT_TIME() + Q_atof(CMD_ARGV( 5 ));
	else dl->die = GET_CLIENT_TIME() + 30.0f;
}

void GammaGraphInit();

void CanUseInit( void ); // Wargon: »конка юза.

// This is called every time the DLL is loaded
void CHud :: Init( void )
{
	static cl_entity_t	head_shield;

	GammaGraphInit();
	RadioIconInit(); // buz
	Hud2Init(); // buz
	SubtitleInit(); // buz
	TabPanelInit(); // buz

	CanUseInit(); // Wargon: »конка юза.

	// pointer to headshield entity
	m_pHeadShieldEnt = &head_shield;

	HOOK_MESSAGE( Logo );
	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( GameMode );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( ViewMode );
	HOOK_MESSAGE( SetFOV );
	HOOK_MESSAGE( Concuss );
	HOOK_MESSAGE( HUDColor ); //LRC
	HOOK_MESSAGE( SetFog ); //LRC
	HOOK_MESSAGE( KeyedDLight ); //LRC
	HOOK_MESSAGE( SetSky ); //LRC
	HOOK_MESSAGE( GasMask ); //buz
	HOOK_MESSAGE( SpecTank ); //buz
	HOOK_MESSAGE( TextWindow ); // buz
	HOOK_MESSAGE( RainData );// buz
	HOOK_MESSAGE( Particle );// buz
	HOOK_MESSAGE( DelParticle );// buz
	gEngfuncs.pfnHookUserMsg( "mydlight", MsgCustomDlight );

	// TFFree CommandMenu
	HOOK_COMMAND( "+commandmenu", OpenCommandMenu );
	HOOK_COMMAND( "-commandmenu", CloseCommandMenu );
	HOOK_COMMAND( "ForceCloseCommandMenu", ForceCloseCommandMenu );
	HOOK_COMMAND( "special", InputPlayerSpecial );
	HOOK_COMMAND( "togglebrowser", ToggleServerBrowser );
	gEngfuncs.pfnAddCommand( "makelight", MakeLight );

	HOOK_MESSAGE( ValClass );
	HOOK_MESSAGE( TeamNames );
	HOOK_MESSAGE( Feign );
	HOOK_MESSAGE( Detpack );
	HOOK_MESSAGE( MOTD );
	HOOK_MESSAGE( BuildSt );
	HOOK_MESSAGE( RandomPC );
	HOOK_MESSAGE( ServerName );
	HOOK_MESSAGE( ScoreInfo );
	HOOK_MESSAGE( TeamScore );
	HOOK_MESSAGE( TeamInfo );

	HOOK_MESSAGE( Spectator );
	HOOK_MESSAGE( AllowSpec );
	HOOK_MESSAGE( WaterSplash );
	HOOK_MESSAGE( NewExplode );
	HOOK_MESSAGE( HeadShield );

	HOOK_MESSAGE( WeaponAnim );
	HOOK_MESSAGE( WeaponBody );
	HOOK_MESSAGE( WeaponSkin );

	HOOK_MESSAGE( SkyMarker );
	HOOK_MESSAGE( WorldMarker );

	HOOK_MESSAGE( StudioDecal );
	HOOK_MESSAGE( CustomDecal );

	HOOK_MESSAGE( PartEffect );
	HOOK_MESSAGE( LevelTime );

	HOOK_MESSAGE( BlurEffect );

	// VGUI Menus
	HOOK_MESSAGE( VGUIMenu );

	m_pZoomSpeed = gEngfuncs.pfnRegisterVariable( "cl_zoomspeed","100", 0 );
	CVAR_REGISTER( "hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );		// controls whether or not to suicide immediately on TF class switch
	CVAR_REGISTER( "hud_takesshots", "0", FCVAR_ARCHIVE );// controls whether or not to automatically take screenshots at the end of a round

	HOOK_MESSAGE( MusicFade );

	m_iLogo = 0;
	m_iFOV = 90; // buz - make 90, not 0
	m_iHUDColor = 0x00FFA000; //255,160,0 -- LRC

	CVAR_REGISTER( "zoom_sensitivity_ratio", "1.2", 0 );
	default_fov = CVAR_REGISTER( "default_fov", "75", FCVAR_ARCHIVE );// buz: turn off
	m_pCvarStealMouse = CVAR_REGISTER( "hud_capturemouse", "1", FCVAR_ARCHIVE );
	m_pCvarDraw = CVAR_REGISTER( "hud_draw", "1", FCVAR_ARCHIVE );
	cl_lw = gEngfuncs.pfnGetCvarPointer( "cl_lw" );

	cl_rollangle = gEngfuncs.pfnRegisterVariable ( "cl_rollangle", "0.65", FCVAR_CLIENTDLL|FCVAR_ARCHIVE );
	cl_rollspeed = gEngfuncs.pfnRegisterVariable ( "cl_rollspeed", "300", FCVAR_CLIENTDLL|FCVAR_ARCHIVE );

	m_pSpriteList = NULL;

	// Clear any old HUD list
	if ( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_Ammo.Init();
	m_HudStamina.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_AmmoSecondary.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	m_Lensflare.Init();    
	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, (vgui::Panel**)&gViewPort);

	m_Menu.Init();
	
	ServersInit();

	MsgFunc_ResetHUD(0, 0, NULL );
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud :: ~CHud()
{
	delete [] m_rghSprites;
	delete [] m_rgrcRects;
	delete [] m_rgszSpriteNames;

	if ( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	ServersShutdown();
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rghSprites[] array
// returns 0 if sprite not found
int CHud :: GetSpriteIndex( const char *SpriteName )
{
	// look through the loaded sprite name list for SpriteName
	for ( int i = 0; i < m_iSpriteCount; i++ )
	{
		if ( strncmp( SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH ) == 0 )
			return i;
	}

	return -1; // invalid sprite
}

void CHud :: VidInit( void )
{
	m_flDeadTime = 0; // buz
	m_SpecTank_on = 0; // buz

	memset( m_pHeadShieldEnt, 0, sizeof(cl_entity_t));
	m_pHeadShieldEnt->modelhandle = INVALID_HANDLE;
	m_pHeadShieldEnt->curstate.framerate = 1.0f;
	m_iHeadShieldState = SHIELD_OFF;
	m_flHeadShieldSwitchTime = 0.0f;
	CVAR_SET_FLOAT( "hud_draw", 1.0f );

	m_flFOV = -1; // buz

	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	// ----------
	// Load Sprites
	// ---------
//	m_hsprFont = LoadSprite("sprites/%d_font.spr");

	m_hsprLogo = 0;	
	m_hsprCursor = 0;

	if (ScreenWidth < 640)
		m_iRes = 320;
	else
		m_iRes = 640;

	// Only load this once
	if ( !m_pSpriteList )
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList("scripts/weapons/hud.txt", &m_iSpriteCountAllRes);

		if (m_pSpriteList)
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t *p = m_pSpriteList;
			for ( int j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if ( p->iRes == m_iRes )
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
 			m_rghSprites = new HSPRITE[m_iSpriteCount];
			m_rgrcRects = new wrect_t[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for ( j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if ( p->iRes == m_iRes )
				{
					char sz[256];
					sprintf(sz, "sprites/%s.spr", p->szSprite);
					m_rghSprites[index] = SPR_Load(sz);
					m_rgrcRects[index] = p->rc;
					strncpy( &m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH );

					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t *p = m_pSpriteList;
		int index = 0;
		for ( int j = 0; j < m_iSpriteCountAllRes; j++ )
		{
			if ( p->iRes == m_iRes )
			{
				char sz[256];
				sprintf( sz, "sprites/%s.spr", p->szSprite );
				m_rghSprites[index] = SPR_Load(sz);
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex( "number_0" );

	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;

	m_Ammo.VidInit();
	m_HudStamina.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
	m_Geiger.VidInit();
	m_Train.VidInit();
	m_Battery.VidInit();
	m_Flash.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();   
	m_Lensflare.VidInit(); 	
	GetClientVoiceMgr()->VidInit();
}

int CHud::MsgFunc_Logo(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	// update Train data
	m_iLogo = READ_BYTE();

	return 1;
}

//LRC
int CHud::MsgFunc_HUDColor(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	m_iHUDColor = READ_LONG();

	return 1;
}

//float g_lastFOV = 0.0; buz

/*
=================
HUD_IsGame

=================
*/
int HUD_IsGame( const char *game )
{
	const char *gamedir;
	char gd[ 1024 ];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if ( gamedir && gamedir[0] )
	{
		COM_FileBase( gamedir, gd );
		if ( !stricmp( gd, game ) )
			return 1;
	}
	return 0;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
#if 0 // buz
float HUD_GetFOV( void )
{
	if ( gEngfuncs.pDemoAPI->IsRecording() )
	{
		// Write it
		int i = 0;
		unsigned char buf[ 100 ];

		// Active
		*( float * )&buf[ i ] = g_lastFOV;
		i += sizeof( float );

		Demo_WriteBuffer( TYPE_ZOOM, i, buf );
	}

	if ( gEngfuncs.pDemoAPI->IsPlayingback() )
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}
#endif

int CHud::MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int newfov = READ_BYTE();

	return 1;
}

void CHud::AddHudElem(CHudBase *phudelem)
{
	HUDLIST *pdl, *ptemp;

//phudelem->Think();

	if (!phudelem)
		return;

	pdl = (HUDLIST *)malloc(sizeof(HUDLIST));
	if (!pdl)
		return;

	memset(pdl, 0, sizeof(HUDLIST));
	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while (ptemp->pNext)
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity( void )
{
	return m_flMouseSensitivity;
}