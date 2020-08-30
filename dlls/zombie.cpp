/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"player.h"
// Wargon: Чтобы работали SpawnBlood и AddMultiDamage. (1.1)
#include "weapons.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

class CZombie : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	float MaxYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );

	float m_flNextFlinch;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// Wargon: Особые звуки для потолочника и паука.
	static const char *pCeilingAlertSounds[];
	static const char *pCeilingAttackSounds[];
	static const char *pCeilingPainSounds[];
	static const char *pSpiderAlertSounds[];
	static const char *pSpiderAttackSounds[];
	static const char *pSpiderPainSounds[];

	//Lev: Ещё звуки для толстяка
	static const char *pStrikerAlertSounds[];
	static const char *pStrikerAttackSounds[];
	static const char *pStrikerPainSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual Vector BodyTarget( const Vector &posSrc ) { return Center() + Vector( 0.0f, 0.0f, RANDOM_FLOAT( 1.0, 20.0f )); }; // position to shoot at

	// Wargon: Отдельные множители повреждений по хитгруппам для monster_zombie. (1.1)
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );

//	void RunAI( void );
};

LINK_ENTITY_TO_CLASS( monster_zombie, CZombie );

const char *CZombie::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CZombie::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CZombie::pAttackSounds[] = 
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CZombie::pIdleSounds[] = 
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CZombie::pAlertSounds[] = 
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CZombie::pPainSounds[] = 
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};

// Wargon: Особые звуки для потолочника и паука.
const char *CZombie::pCeilingAlertSounds[] = 
{
	"potolo4nik/zo_alert10.wav",
	"potolo4nik/zo_alert20.wav",
	"potolo4nik/zo_alert30.wav",
};

const char *CZombie::pCeilingAttackSounds[] = 
{
	"potolo4nik/zo_attack1.wav",
	"potolo4nik/zo_attack2.wav",
};

const char *CZombie::pCeilingPainSounds[] = 
{
	"potolo4nik/zo_pain1.wav",
	"potolo4nik/zo_pain2.wav",
};

const char *CZombie::pSpiderAlertSounds[] = 
{
	"spider/zo_alert10.wav",
	"spider/zo_alert20.wav",
	"spider/zo_alert30.wav",
};

const char *CZombie::pSpiderAttackSounds[] = 
{
	"spider/zo_attack1.wav",
	"spider/zo_attack2.wav",
};

const char *CZombie::pSpiderPainSounds[] = 
{
	"spider/zo_pain1.wav",
	"spider/zo_pain2.wav",
};

const char *CZombie::pStrikerAlertSounds[] =
{
	"striker/striker_alert1.wav",
	"striker/striker_alert2.wav",
};

const char *CZombie::pStrikerAttackSounds[] =
{
	"striker/striker_attack1.wav",
	"striker/striker_attack2.wav",
};

const char *CZombie::pStrikerPainSounds[] =
{
	"striker/striker_pain1.wav",
	"striker/striker_pain2.wav",
    "striker/striker_pain3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CZombie :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CZombie :: MaxYawSpeed( void )
{
	float ys;

	ys = 120;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	return ys;
}

int CZombie :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// buz: refuse gas damage
	if (bitsDamageType & DMG_NERVEGAS )
		return 0;

	// Take 30% damage from bullets
	if ( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

// Wargon: Отдельные множители повреждений по хитгруппам для monster_zombie. (1.1)
void CZombie :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if (pev->takedamage)
	{
		if (pev->spawnflags & SF_MONSTER_INVINCIBLE)
		{
			CBaseEntity *pEnt = CBaseEntity::Instance(pevAttacker);
			if (pEnt->IsPlayer())
				return;
			if (pevAttacker->owner)
			{
				pEnt = CBaseEntity::Instance(pevAttacker->owner);
				if (pEnt->IsPlayer())
					return;
			}
		}
		m_LastHitGroup = ptr->iHitgroup;
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		TraceResult btr;
		switch (ptr->iHitgroup)
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			UTIL_TraceLine(ptr->vecEndPos, ptr->vecEndPos + vecDir * 172, ignore_monsters, ENT(pev), &btr);
			UTIL_TraceCustomDecal( &btr, "brains", RANDOM_FLOAT( 0.0f, 360.0f ));
			SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage * 4);
			flDamage *= gSkillData.zomHead;
			break;
		case HITGROUP_CHEST:
			flDamage *= gSkillData.zomChest;
			break;
		case HITGROUP_STOMACH:
			flDamage *= gSkillData.zomStomach;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= gSkillData.zomArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= gSkillData.zomLeg;
			break;
		default:
			break;
		}
		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage * 2);
		AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
	}
}

// Wargon: Добавлены особые звуки в Alert, Attack и Pain для потолочника и паука.
void CZombie :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);
	if ( FStrEq( STRING(pev->model), "models/zombie_c.mdl" ) )
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pCeilingAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pCeilingAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
	else if ( FStrEq( STRING(pev->model), "models/spider.mdl" ) )
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pSpiderAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pSpiderAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
    else if ( FStrEq( STRING(pev->model), "models/zombie_striker.mdl" ) )
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pStrikerAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pSpiderAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
	else
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CZombie :: AttackSound( void )
{
	if ( FStrEq( STRING(pev->model), "models/zombie_c.mdl" ) )
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pCeilingAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pCeilingAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
	else if ( FStrEq( STRING(pev->model), "models/spider.mdl" ) )
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pSpiderAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pSpiderAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
    else if ( FStrEq( STRING(pev->model), "models/zombie_striker.mdl" ) )
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pStrikerAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pSpiderAlertSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
	else
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CZombie :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);
	if (RANDOM_LONG(0,5) < 2)
	{
		if ( FStrEq( STRING(pev->model), "models/zombie_c.mdl" ) )
			EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pCeilingPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pCeilingPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
		else if ( FStrEq( STRING(pev->model), "models/spider.mdl" ) )
			EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pSpiderPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pSpiderPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
        else if ( FStrEq( STRING(pev->model), "models/zombie_striker.mdl" ) )
		    EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pStrikerPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pSpiderAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
		else
			EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
	}
}

void CZombie :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

// buz: update player's geiger counter
/*void CZombie :: RunAI()
{
	if (!m_pPlayer2)
	{
		edict_t *pent = g_engfuncs.pfnPEntityOfEntIndex( 1 );
		if (!FNullEnt(pent))
		{
			m_pPlayer2 = GetClassPtr( (CBasePlayer *)VARS(pent));
		}
		else
			ALERT(at_console, "ERROR: zombie cant find the player entity!!\n");
	}
	else
	{
		Vector vecSpot1 = (pev->absmin + pev->absmax) * 0.5;
		Vector vecSpot2 = (m_pPlayer2->pev->absmin + m_pPlayer2->pev->absmax) * 0.5;
		Vector vecRange = vecSpot1 - vecSpot2;
		float flRange = vecRange.Length();

		if (m_pPlayer2->m_flgeigerRange >= flRange)
			m_pPlayer2->m_flgeigerRange = flRange;
	}

	CBaseMonster::RunAI();
}*/


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case ZOMBIE_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case ZOMBIE_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash left!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case ZOMBIE_AE_ATTACK_BOTH:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgBothSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CZombie :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/zombie.mdl");

	// Wargon: Особые размеры для потолочника и паука.
	if ( FStrEq( STRING(pev->model), "models/zombie_c.mdl" ) )
		UTIL_SetSize( pev, Vector(-16, -16, 0), Vector(16, 16, 128) );
	else if ( FStrEq( STRING(pev->model), "models/spider.mdl" ) )
		UTIL_SetSize( pev, Vector(-32, -32, 0), Vector(32, 32, 64) );
	else
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if ( pev->health == 0.0f || pev->health == 999.0f )
		pev->health			= gSkillData.zombieHealth;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

//	pev->renderfx = 49; // buz: hack for studiorenderer to draw red eyes

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie :: Precache() // Wargon: Особые звуки для потолочника и паука.
{
	int i;

	if ( FStrEq( STRING(pev->model), "models/zombie_c.mdl" ) )
	{
		PRECACHE_MODEL("models/zombie_c.mdl");

		for ( i = 0; i < ARRAYSIZE( pCeilingAlertSounds ); i++ )
			PRECACHE_SOUND((char *)pCeilingAlertSounds[i]);

		for ( i = 0; i < ARRAYSIZE( pCeilingAttackSounds ); i++ )
			PRECACHE_SOUND((char *)pCeilingAttackSounds[i]);

		for ( i = 0; i < ARRAYSIZE( pCeilingPainSounds ); i++ )
			PRECACHE_SOUND((char *)pCeilingPainSounds[i]);
	}
	else if ( FStrEq( STRING(pev->model), "models/spider.mdl" ) )
	{
		PRECACHE_MODEL("models/spider.mdl");

		for ( i = 0; i < ARRAYSIZE( pSpiderAlertSounds ); i++ )
			PRECACHE_SOUND((char *)pSpiderAlertSounds[i]);

		for ( i = 0; i < ARRAYSIZE( pSpiderAttackSounds ); i++ )
			PRECACHE_SOUND((char *)pSpiderAttackSounds[i]);

		for ( i = 0; i < ARRAYSIZE( pSpiderPainSounds ); i++ )
			PRECACHE_SOUND((char *)pSpiderPainSounds[i]);
	}
	else if ( FStrEq( STRING(pev->model), "models/zombie_striker.mdl" ) )
	{
		PRECACHE_MODEL("models/zombie_striker.mdl");

		for ( i = 0; i < ARRAYSIZE( pStrikerAlertSounds ); i++ )
			PRECACHE_SOUND((char *)pStrikerAlertSounds[i]);

		for ( i = 0; i < ARRAYSIZE( pStrikerAttackSounds ); i++ )
			PRECACHE_SOUND((char *)pStrikerAttackSounds[i]);

		for ( i = 0; i < ARRAYSIZE( pStrikerPainSounds ); i++ )
			PRECACHE_SOUND((char *)pStrikerPainSounds[i]);
	}
	else
	{
		if (pev->model)
			PRECACHE_MODEL((char*)STRING(pev->model));
		else
			PRECACHE_MODEL("models/zombie.mdl");

		for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
			PRECACHE_SOUND((char *)pAlertSounds[i]);

		for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
			PRECACHE_SOUND((char *)pAttackSounds[i]);

		for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
			PRECACHE_SOUND((char *)pPainSounds[i]);
	}

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

//	PRECACHE_MODEL("models/zombie_eyes.mdl"); // buz
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CZombie::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif			
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
	
}
