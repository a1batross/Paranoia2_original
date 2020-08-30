//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined( IN_DEFSH )
#define IN_DEFSH
#pragma once

// up / down
#define	PITCH	0
// left / right
#define	YAW		1
// fall over
#define	ROLL	2 

#define DLLEXPORT __declspec( dllexport )

void V_StartPitchDrift( void );
void V_StopPitchDrift( void );

#endif