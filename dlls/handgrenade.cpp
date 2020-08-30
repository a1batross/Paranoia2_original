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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"

class CHandGrenade : public CBasePlayerItem
{
public:
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void PrimaryAttack( void );
	void Deploy( void );
	BOOL CanHolster( void );
	void WeaponIdle( void );

	float m_flStartThrow;
	float m_flReleaseThrow;
};

TYPEDESCRIPTION CHandGrenade :: m_SaveData[] = 
{
	DEFINE_FIELD( CHandGrenade, m_flStartThrow, FIELD_TIME ),
	DEFINE_FIELD( CHandGrenade, m_flReleaseThrow, FIELD_TIME ),
}; IMPLEMENT_SAVERESTORE( CHandGrenade, CBasePlayerItem );

LINK_ENTITY_TO_CLASS( weapon_handgrenade, CHandGrenade );

void CHandGrenade :: Deploy( void )
{
	m_flReleaseThrow = -1.0f;
	DefaultDeploy( ACT_VM_DEPLOY );
}

BOOL CHandGrenade :: CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0.0f );
}

void CHandGrenade :: PrimaryAttack( void )
{
	if( !m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0.0f;
		SetAnimation( ACT_VM_START_CHARGE );
	}
}

void CHandGrenade :: WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase( ))
		return;

	if ( m_flStartThrow )
	{
		if ( m_flReleaseThrow > 0.0f )
		{
			// buz: уже кинули, теперь надо достать новую или переключить оружие
			m_flStartThrow = 0.0f;
			m_flReleaseThrow = -1.0f;

			if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
			{
				SetAnimation( ACT_VM_DEPLOY );
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10.0f, 15.0f );
			}
			else
			{
				RetireWeapon();
			}
			return;
		}

		AmmoInfo *pInfo = UTIL_FindAmmoType( pszAmmo1() );

		float flDamage = gSkillData.plrDmgHandGrenade;
		float flDistance = 500.0f;

		if( pInfo != NULL )
		{
			// overwrite default values
			flDamage = pInfo->flPlayerDamage;
			flDistance = pInfo->flDistance;
		}		

		m_flReleaseThrow = gpGlobals->time;
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if ( angThrow.x < 0 )
			angThrow.x = -10.0f + angThrow.x * ( ( 90.0f - 10.0f ) / 90.0f );
		else angThrow.x = -10.0f + angThrow.x * ( ( 90.0f + 10.0f ) / 90.0f );

		float flVel = ( 90.0f - angThrow.x ) * 4.0f;
		if ( flVel > flDistance )
			flVel = flDistance;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * vecThrowOffset().x
		+ gpGlobals->v_right * vecThrowOffset().y + gpGlobals->v_up * vecThrowOffset().z;
		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0f;
		if( time < 0.0f ) time = 0.0f;
		float framerate = 1.0f;

		CGrenade :: ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time, flDamage );

		// use single anim with various framerate instead of three anims
		if ( flVel < 500 )
			framerate = 0.8f;
		else if ( flVel < 1000 )
			framerate = 1.2f;
		else framerate = 1.8f;

		SetAnimation( ACT_VM_MELEE_ATTACK, framerate );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		float flNextAttack = fNextAttack1();
		if( flNextAttack == -1.0f )
			flNextAttack = SequenceDuration();

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flNextAttack;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + SequenceDuration() + 0.2f;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		SetAnimation( ACT_VM_IDLE );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10.0f, 15.0f );
	}
}