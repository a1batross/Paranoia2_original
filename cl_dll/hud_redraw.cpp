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
// hud_redraw.cpp
//
#include <math.h>
#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_hud.h"
#include "triangleapi.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

// buz: for IsHardware check
#include "r_studioint.h"
#include "com_model.h"
extern engine_studio_api_t IEngineStudio;

#include "vgui_TeamFortressViewport.h"
#include "vgui_paranoiatext.h"// buz
#include "gl_local.h"
#include "gl_decals.h"

#define MAX_LOGO_FRAMES 56

// buz: spread value
vec3_t g_vSpread;
int g_iGunMode = 0;

void DrawBlurTest(float frametime); // buz
extern int g_iVisibleMouse;

//float HUD_GetFOV( void ); buz

extern cvar_t *sensitivity;

// Think
void CHud::Think(void)
{
	// buz: calc dead time
	if (gHUD.m_Health.m_iHealth <= 0 && !m_flDeadTime)
	{
		m_flDeadTime = gEngfuncs.GetClientTime();
	}

	// Wargon: Исправление бага эффекта grayscale, остававшегося после загрузки из мертвого состояния.
	if (gHUD.m_Health.m_iHealth > 0)
	{
		m_flDeadTime = 0;
	}

	float targetFOV;
	HUDLIST *pList = m_pHudList;
	static float lasttime = 0;

	while (pList)
	{
		if (pList->p->m_iFlags & HUD_ACTIVE)
			pList->p->Think();
		pList = pList->pNext;
	}

	if( g_iGunMode == 3 )
		targetFOV = 30;
	else if( g_iGunMode == 2 )
		targetFOV = 60;
	else targetFOV = default_fov->value;

	static float lastFixedFov = 0;

	if( m_flFOV < 0 )
	{
		m_flFOV = targetFOV;
		lasttime = gEngfuncs.GetClientTime();
		lastFixedFov = m_flFOV;
	}
	else
	{
		float curtime = gEngfuncs.GetClientTime();
		float mod = targetFOV - m_flFOV;
		if( mod < 0 ) mod *= -1;
		if( mod < 30 ) mod = 30;

		if( g_iGunMode == 3 || lastFixedFov == 30 )
			mod *= 2; // хаками халфа полнится (c)
		mod /= 30;

		if( m_flFOV < targetFOV )
		{
			m_flFOV += (curtime - lasttime) * m_pZoomSpeed->value * mod;
			if( m_flFOV > targetFOV )
			{
				m_flFOV = targetFOV;
				lastFixedFov = m_flFOV;
			}
		}
		else if( m_flFOV > targetFOV )
		{
			m_flFOV -= (curtime - lasttime) * m_pZoomSpeed->value * mod;

			if( m_flFOV < targetFOV )
			{
				m_flFOV = targetFOV;
				lastFixedFov = m_flFOV;
			}
		}
		lasttime = curtime;
	}

	m_iFOV = m_flFOV;

	// Set a new sensitivity
	if ( m_iFOV == default_fov->value )// buz: turn off
	{  
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{  
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * (m_flFOV / (float)90) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}
}

//LRC - fog fading values
extern float g_fFadeDuration;
extern float g_fStartDist;
extern float g_fEndDist;
//extern int g_iFinalStartDist;
extern int g_iFinalEndDist;

void OrthoScope(void); // buz
void OrthoVGUI(void); // buz
extern vec3_t g_CrosshairAngle; // buz

#define CROSS_LENGTH	18.0f

// Redraw
// step through the local data,  placing the appropriate graphics & text as appropriate
// returns 1 if they've changed, 0 otherwise
int CHud :: Redraw( float flTime, int intermission )
{
	m_fOldTime = m_flTime;	// save time of previous redraw
	m_flTime = flTime;
	m_flTimeDelta = (double)m_flTime - m_fOldTime;
	static m_flShotTime = 0;

#if 0
	// g-cont. disabled for users request
	if( g_fRenderInitialized )
		DrawBlurTest( m_flTimeDelta );
#endif
	//LRC - handle fog fading effects. (is this the right place for it?)
	if (g_fFadeDuration)
	{
		// Nicer might be to use some kind of logarithmic fade-in?
		double fFraction = m_flTimeDelta/g_fFadeDuration;
//		g_fStartDist -= (FOG_LIMIT - g_iFinalStartDist)*fFraction;
		g_fEndDist -= (FOG_LIMIT - g_iFinalEndDist)*fFraction;

//		CONPRINT("FogFading: %f - %f, frac %f, time %f, final %d\n", g_fStartDist, g_fEndDist, fFraction, flTime, g_iFinalEndDist);

		// cap it
//		if (g_fStartDist > FOG_LIMIT)				g_fStartDist = FOG_LIMIT;
		if (g_fEndDist   > FOG_LIMIT)				g_fEndDist = FOG_LIMIT;
//		if (g_fStartDist < g_iFinalStartDist)	g_fStartDist = g_iFinalStartDist;
		if (g_fEndDist   < g_iFinalEndDist)		g_fEndDist   = g_iFinalEndDist;
	}
	
	// Clock was reset, reset delta
	if ( m_flTimeDelta < 0 )
		m_flTimeDelta = 0;

	if (g_iGunMode == 3) // buz: special sniper scope
	{
		if (IEngineStudio.IsHardware())
			OrthoScope();
	}

	if (!IEngineStudio.IsHardware() && m_iHeadShieldState != 1 )
		DrawHudString(XRES(10), YRES(350), XRES(600), "Using Head Shield", 180, 180, 180);	

	if( r_stats.debug_surface != NULL )
		gEngfuncs.pfnDrawCharacter( XRES( 320 ), YRES( 240 ), '+', 128, 255, 92 );
	
	OrthoVGUI(); // buz: panels background

	// Bring up the scoreboard during intermission
	if (gViewPort)
	{
		if ( m_iIntermission && !intermission )
		{
			// Have to do this here so the scoreboard goes away
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideScoreBoard();
//			gViewPort->UpdateSpectatorPanel();
		}
		else if ( !m_iIntermission && intermission )
		{
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideVGUIMenu();
			gViewPort->ShowScoreBoard();
//			gViewPort->UpdateSpectatorPanel();

			// Take a screenshot if the client's got the cvar set
			if ( CVAR_GET_FLOAT( "hud_takesshots" ) != 0 )
				m_flShotTime = flTime + 1.0;	// Take a screenshot in a second
		}
	}

	if (m_flShotTime && m_flShotTime < flTime)
	{
		gEngfuncs.pfnClientCmd("snapshot\n");
		m_flShotTime = 0;
	}

	m_iIntermission = intermission;

	// if no redrawing is necessary
	// return 0;
	
	if ( m_pCvarDraw->value )
	{
		HUDLIST *pList = m_pHudList;

		while (pList)
		{
			if ( !intermission )
			{
				if ( (pList->p->m_iFlags & HUD_ACTIVE) && !(m_iHideHUDDisplay & HIDEHUD_ALL) )
					pList->p->Draw(flTime);
			}
			else
			{  // it's an intermission,  so only draw hud elements that are set to draw during intermissions
				if ( pList->p->m_iFlags & HUD_INTERMISSION )
					pList->p->Draw( flTime );
			}

			pList = pList->pNext;
		}
	}

	// buz: draw crosshair
	// Wargon: Прицел рисуется только если hud_draw = 1.
	if(( g_vSpread[0] && g_iGunMode == 1 && m_SpecTank_on == 0 ) && gHUD.m_pCvarDraw->value && cv_crosshair->value )
	{
		if( gViewPort && gViewPort->m_pParanoiaText && gViewPort->m_pParanoiaText->isVisible( ))
			return 1;

		int barsize = XRES(g_iGunMode == 1 ? 9 : 6); 
		int hW = ScreenWidth / 2;
		int hH = ScreenHeight / 2;
		float mod = (1/(tan(M_PI/180*(m_iFOV/2))));
		int dir = ((g_vSpread[0] * hW) / 500) * mod;
	//	gEngfuncs.Con_Printf("mod is %f, %d\n", mod, m_iFOV);

		if (g_CrosshairAngle[0] != 0 || g_CrosshairAngle[1] != 0)
		{
			// adjust for autoaim
			hW -= g_CrosshairAngle[1] * (ScreenWidth / m_iFOV);
			hH -= g_CrosshairAngle[0] * (ScreenWidth / m_iFOV);
		}		

		// g_vSpread[2] - is redish [0..500]
		// gEngfuncs.Con_Printf("received spread: %f\n", g_vSpread[2]);
		int c = 255 - (g_vSpread[2] * 0.5);

		if( cv_crosshair->value > 1.0f )
		{
			// old Paranoia-style crosshair
			FillRGBA(hW - dir - barsize, hH, barsize, 1, 255, c, c, 200);
			FillRGBA(hW + dir, hH, barsize, 1, 255, c, c, 200);
			FillRGBA(hW, hH - dir - barsize, 1, barsize, 255, c, c, 200);
			FillRGBA(hW, hH + dir, 1, barsize, 255, c, c, 200);	
		}
		else
		{
			// new crysis-style crosshair
			float flColor = 255.0f - (g_vSpread[2] * 0.5f);
			flColor = bound( 0.0f, flColor * (1.0f / 255.0f), 1.0f );

			GL_TextureTarget( GL_NONE );

			gEngfuncs.pTriAPI->RenderMode( kRenderTransColor );
			gEngfuncs.pTriAPI->Color4f( 1.0f, flColor, flColor, 0.8f );

			gEngfuncs.pTriAPI->Begin( TRI_LINES );
				gEngfuncs.pTriAPI->Vertex3f( hW, hH - dir, 0.0f );
				gEngfuncs.pTriAPI->Vertex3f( hW, hH - ( CROSS_LENGTH * 1.2f ) - dir, 0.0f );
			gEngfuncs.pTriAPI->End();

			gEngfuncs.pTriAPI->Begin( TRI_LINES );
				gEngfuncs.pTriAPI->Vertex3f( hW + dir, hH + dir, 0.0f );
				gEngfuncs.pTriAPI->Vertex3f( hW + CROSS_LENGTH + dir, hH + CROSS_LENGTH + dir, 0.0f );
			gEngfuncs.pTriAPI->End();

			gEngfuncs.pTriAPI->Begin( TRI_LINES );
				gEngfuncs.pTriAPI->Vertex3f( hW - dir, hH + dir, 0.0f );
				gEngfuncs.pTriAPI->Vertex3f( hW - CROSS_LENGTH - dir, hH + CROSS_LENGTH + dir, 0.0f );
			gEngfuncs.pTriAPI->End();

			GL_TextureTarget( GL_TEXTURE_2D );
		}
	}

	return 1;
}

void ScaleColors( int &r, int &g, int &b, int a )
{
	float x = (float)a / 255;
	r = (int)(r * x);
	g = (int)(g * x);
	b = (int)(b * x);
}

int CHud :: DrawHudString(int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b )
{
	// draw the string until we hit the null character or a newline character
	for ( ; *szIt != 0 && *szIt != '\n'; szIt++ )
	{
		int next = xpos + gHUD.m_scrinfo.charWidths[ *szIt ]; // variable-width fonts look cool
		if ( next > iMaxX )
			return xpos;

		TextMessageDrawChar( xpos, ypos, *szIt, r, g, b );
		xpos = next;		
	}

	return xpos;
}

int CHud :: DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b )
{
	char szString[32];
	sprintf( szString, "%d", iNumber );
	return DrawHudStringReverse( xpos, ypos, iMinX, szString, r, g, b );

}

// draws a string from right to left (right-aligned)
int CHud :: DrawHudStringReverse( int xpos, int ypos, int iMinX, char *szString, int r, int g, int b )
{
	// find the end of the string
	for ( char *szIt = szString; *szIt != 0; szIt++ )
	{ // we should count the length?		
	}

	// iterate throug the string in reverse
	for ( szIt--;  szIt != (szString-1);  szIt-- )	
	{
		int next = xpos - gHUD.m_scrinfo.charWidths[ *szIt ]; // variable-width fonts look cool
		if ( next < iMinX )
			return xpos;
		xpos = next;

		TextMessageDrawChar( xpos, ypos, *szIt, r, g, b );
	}

	return xpos;
}

int CHud :: DrawHudNumber( int x, int y, int iFlags, int iNumber, int r, int g, int b)
{
	int iWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;
	int k;

	if (iNumber > 0)
	{
		// SPR_Draw 100's
		if (iNumber >= 100)
		{
			 k = iNumber/100;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if (iFlags & (DHN_3DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw 10's
		if (iNumber >= 10)
		{
			k = (iNumber % 100)/10;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if (iFlags & (DHN_3DIGITS | DHN_2DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b );
		SPR_DrawAdditive(0,  x, y, &GetSpriteRect(m_HUD_number_0 + k));
		x += iWidth;
	} 
	else if (iFlags & DHN_DRAWZERO) 
	{
		SPR_Set(GetSprite(m_HUD_number_0), r, g, b );

		// SPR_Draw 100's
		if (iFlags & (DHN_3DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		if (iFlags & (DHN_3DIGITS | DHN_2DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		
		SPR_DrawAdditive( 0,  x, y, &GetSpriteRect(m_HUD_number_0));
		x += iWidth;
	}

	return x;
}


int CHud::GetNumWidth( int iNumber, int iFlags )
{
	if (iFlags & (DHN_3DIGITS))
		return 3;

	if (iFlags & (DHN_2DIGITS))
		return 2;

	if (iNumber <= 0)
	{
		if (iFlags & (DHN_DRAWZERO))
			return 1;
		else
			return 0;
	}

	if (iNumber < 10)
		return 1;

	if (iNumber < 100)
		return 2;

	return 3;

}	

// buz
void DrawBlurTest( float frametime )
{
	static int blurstate = false;

	if( gHUD.m_flBlurAmount > 0.0f && frametime )
	{
		if( !g_fRenderInitialized )
		{
			ALERT( at_error, "GL effects are not allowed\n" );
			gHUD.m_flBlurAmount = 0.0f;
			return;
		}

		GLint val_r, val_g, val_b;
		pglGetIntegerv( GL_ACCUM_RED_BITS, &val_r );
		pglGetIntegerv( GL_ACCUM_GREEN_BITS, &val_g );
		pglGetIntegerv( GL_ACCUM_BLUE_BITS, &val_b );

		if( !val_r || !val_g || !val_b )
		{
			ALERT( at_error, "Accumulation buffer is not present\n" );
			gHUD.m_flBlurAmount = 0.0f;
			return;
		}

		if( !blurstate )
		{
			// load entire screen first time
			pglAccum( GL_LOAD, 1.0f );
		}
		else
		{
			float blur = ( 51.0f - ( gHUD.m_flBlurAmount * 50.0f ));
			blur = bound( 0.0f, blur * frametime, 1.0f );
			pglReadBuffer( GL_BACK );
			pglAccum( GL_MULT, 1.0f - blur ); // scale contents of accumulation buffer
			pglAccum( GL_ACCUM, blur ); // add screen contents
			pglAccum( GL_RETURN, 1 ); // read result back
		}

		blurstate = true;
	}
	else
	{
		blurstate = false;
	}
}