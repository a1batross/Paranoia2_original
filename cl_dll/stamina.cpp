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
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "vgui_TeamFortressViewport.h"
#include "vgui_hud.h" 

DECLARE_MESSAGE( m_HudStamina, Stamina )

int CHudStamina :: Init(void)
{
	m_iStam = 100;
	m_fFade = 0;
	m_iFlags = 0;
	
	HOOK_MESSAGE(Stamina);

	gHUD.AddHudElem(this);

	return 1;
}

int CHudStamina :: VidInit( void )
{
	int HUD_suit_empty = gHUD.GetSpriteIndex( "suit_empty" );
	int HUD_suit_full = gHUD.GetSpriteIndex( "suit_full" );

	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = 0;
	return 1;
}

int CHudStamina :: MsgFunc_Stamina( const char *pszName, int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

	if( x != m_iStam )
	{
		m_fFade = FADE_TIME;
		m_iStam = x;
	}
	
	gViewPort->m_pHud2->UpdateStamina(x);

	return 1;
}

int CHudStamina :: Draw( float flTime )
{
	return 1;
}