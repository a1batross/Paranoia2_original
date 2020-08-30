//=======================================================================
//			Copyright (C) XashXT Group 2015
//		         bullets.h - shared bullet types 
//=======================================================================
#ifndef BULLETS_H
#define BULLETS_H

// bullet types
typedef enum
{
	BULLET_NORMAL = 0,	// single bullet
	BULLET_BUCKSHOT,	// multiple parts
	BULLET_STAB,	// hack for knife
} Bullet;

#endif//BULLETS_H