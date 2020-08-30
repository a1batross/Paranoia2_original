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
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
#include "vgui_schememanager.h"

#include "pm_shared.h"
#include "pm_defs.h"

#include <string.h>
#include "hud_servers.h"
#include "vgui_int.h"

int developer_level;
int g_iXashEngineBuildNumber;
BOOL g_fRenderInitialized = FALSE;
BOOL g_fRenderInterfaceValid = FALSE;
BOOL g_fXashEngine = FALSE;
cl_enginefunc_t gEngfuncs;
render_api_t gRenderfuncs;
CHud gHUD;
TeamFortressViewport *gViewPort = NULL;

void InitInput( void );
void ShutdownInput( void );
void EV_HookEvents( void );
void IN_Commands( void );

/*
========================== 
    Initialize

Called when the DLL is first loaded.
==========================
*/
extern "C" 
{
int	DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion );
int	DLLEXPORT HUD_VidInit( void );
void	DLLEXPORT HUD_Init( void );
void	DLLEXPORT HUD_Shutdown( void );
int	DLLEXPORT HUD_Redraw( float flTime, int intermission );
int	DLLEXPORT HUD_UpdateClientData( client_data_t *cdata, float flTime );
void	DLLEXPORT HUD_Reset ( void );
void	DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server );
void	DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove );
char	DLLEXPORT HUD_PlayerMoveTexture( char *name );
int	DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
void	DLLEXPORT HUD_PostRunCmd( local_state_t *from, local_state_t *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int seed );
int	DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void	DLLEXPORT HUD_Frame( double time );
void	DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking);
void	DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf );
void	DLLEXPORT HUD_ClipMoveToEntity( physent_t *pe, const float *start, float *mins, float *maxs, const float *end, pmtrace_t *tr );
}

/*
================================
HUD_GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;

	switch ( hullnumber )
	{
	case 0:				// Normal player
		mins = Vector(-16, -16, -36);
		maxs = Vector(16, 16, 36);
		iret = 1;
		break;
	case 1:				// Crouched player
		mins = Vector(-16, -16, -18 );
		maxs = Vector(16, 16, 18 );
		iret = 1;
		break;
	case 2:				// Point based hull
		mins = Vector( 0, 0, 0 );
		maxs = Vector( 0, 0, 0 );
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
HUD_ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

void DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove )
{
	PM_Init( ppmove );
}

char DLLEXPORT HUD_PlayerMoveTexture( char *name )
{
	return (char)0;
}

void DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server )
{
	PM_Move( ppmove, server );
}

int DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion )
{
	gEngfuncs = *pEnginefuncs;

	if( iVersion != CLDLL_INTERFACE_VERSION )
		return 0;

	memcpy( &gEngfuncs, pEnginefuncs, sizeof( cl_enginefunc_t ));

	// get developer level
	developer_level = (int)CVAR_GET_FLOAT( "developer" );

	if( CVAR_GET_POINTER( "host_clientloaded" ) != NULL )
		g_fXashEngine = TRUE;

	g_iXashEngineBuildNumber = (int)CVAR_GET_FLOAT( "build" ); // 0 for old builds or GoldSrc
	if( g_iXashEngineBuildNumber <= 0 )
		g_iXashEngineBuildNumber = (int)CVAR_GET_FLOAT( "buildnum" );

	EV_HookEvents();

	return 1;
}


/*
==========================
	HUD_VidInit

Called when the game initializes
and whenever the vid_mode is changed
so the HUD can reinitialize itself.
==========================
*/
int DLLEXPORT HUD_VidInit( void )
{
	gHUD.VidInit();

	VGui_Startup();

	if( g_fXashEngine && g_fRenderInitialized )
		R_VidInit();

	return 1;
}

/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/
void DLLEXPORT HUD_Init( void )
{
	InitInput();

	if( g_fXashEngine && g_fRenderInitialized )
		GL_Init();

	gHUD.Init();
	Scheme_Init();
}

void DLLEXPORT HUD_Shutdown( void )
{
	ShutdownInput();

	if( g_fXashEngine && g_fRenderInitialized )
		GL_Shutdown();
}

/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/
int DLLEXPORT HUD_Redraw( float time, int intermission )
{    
	return gHUD.Redraw( time, intermission );
}


/*
==========================
	HUD_UpdateClientData

called every time shared client
dll/engine data gets changed,
and gives the cdll a chance
to modify the data.

returns 1 if anything has been changed, 0 otherwise.
==========================
*/
int DLLEXPORT HUD_UpdateClientData(client_data_t *pcldata, float flTime )
{
	IN_Commands();

	return gHUD.UpdateClientData(pcldata, flTime );
}

/*
==========================
	HUD_Reset

Called at start and end of demos to restore to "non"HUD state.
==========================
*/
void DLLEXPORT HUD_Reset( void )
{
	gHUD.VidInit();
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/
void DLLEXPORT HUD_Frame( double time )
{
	ServersThink( time );

	GetClientVoiceMgr()->Frame(time);
}


/*
==========================
HUD_VoiceStatus

Called when a player starts or stops talking.
==========================
*/
void DLLEXPORT HUD_VoiceStatus( int entindex, qboolean bTalking )
{
	GetClientVoiceMgr()->UpdateSpeakerStatus( entindex, bTalking );
}

/*
==========================
HUD_DirectorEvent

Called when a director event message was received
==========================
*/
void DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf )
{
	 gHUD.m_Spectator.DirectorMessage( iSize, pbuf );
}

void DLLEXPORT HUD_PostRunCmd( local_state_t *from, local_state_t *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int seed )
{
	to->client.fov = 0;//g_lastFOV; buz
}

/*
==========================
HUD_ClipMoveToEntity

This called only for non-local clients (multiplayer)
==========================
*/
void DLLEXPORT HUD_ClipMoveToEntity( physent_t *pe, const float *start, float *mins, float *maxs, const float *end, pmtrace_t *tr )
{
	// convert physent_t to cl_entity_t
	cl_entity_t *pTouch = gEngfuncs.GetEntityByIndex( pe->info );
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