//=====================================================================================================
//
//					Written by MaSTeR for PARANOIA 2
//
//=====================================================================================================

//Super Officer - an infected officer that spits his infected blood and kicks the player (melee attack)
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"

//Specific defines
#define		OFFICER_RUN_SPEED 100
#define		OFFICER_WALK_SPEED 70
#define		OFFICER_WALK_INJURED_SPEED 50

#define		OFFICER_SPIT_DISTANCE 256
#define		OFFICER_MELEE_ATTACK_MIN_DIST 32
#define		OFFICER_NEXT_SPIT_TIME				  1.5f

int			iOfficerBlood;

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_OFFICER_HOP = LAST_COMMON_SCHEDULE + 1,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_OFFICER_HOP = LAST_COMMON_TASK + 1,
};

//=========================================================
// Oficer's infected blood projectile
//=========================================================
class COfficerBlood : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS( officerblood, COfficerBlood );

TYPEDESCRIPTION	COfficerBlood::m_SaveData[] = 
{
	DEFINE_FIELD( COfficerBlood, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( COfficerBlood, CBaseEntity );

void COfficerBlood::Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "officerblood" );
	
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/bigspit.spr");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
}

void COfficerBlood::Animate( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->frame++ )
	{
		if ( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

void COfficerBlood::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	COfficerBlood *pBlood = GetClassPtr( (COfficerBlood *)NULL );
	pBlood->Spawn();
	
	UTIL_SetOrigin( pBlood, vecStart );
	pBlood->pev->velocity = vecVelocity;
	pBlood->pev->gravity = 1000;
	pBlood->pev->owner = ENT(pevOwner);

	pBlood->SetThink ( Animate );
	pBlood->pev->nextthink = gpGlobals->time + 0.1;
}

void COfficerBlood :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "superofficer/acid1.wav", 1, ATTN_NORM, 0, iPitch );	

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "superofficer/spithit1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "superofficer/spithit2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}

	if ( !pOther->pev->takedamage )
	{

		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_GREEN );
		// make some flecks
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
			WRITE_BYTE( TE_SPRITE_SPRAY );
			WRITE_COORD( tr.vecEndPos.x);	// pos
			WRITE_COORD( tr.vecEndPos.y);	
			WRITE_COORD( tr.vecEndPos.z);	
			WRITE_COORD( tr.vecPlaneNormal.x);	// dir
			WRITE_COORD( tr.vecPlaneNormal.y);	
			WRITE_COORD( tr.vecPlaneNormal.z);	
			WRITE_SHORT( iOfficerBlood );	// model
			WRITE_BYTE ( 5 );			// count
			WRITE_BYTE ( 30 );			// speed
			WRITE_BYTE ( 80 );			// noise ( client will divide by 100 )
		MESSAGE_END();
	}
	else
	{
		pOther->TakeDamage ( pev, pev, gSkillData.superofficerDmgBlood, DMG_GENERIC );
	}

	SetThink ( SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		OFFICER_AE_SPIT		( 1 )
#define		OFFICER_AE_PUNCH	( 2 )
#define		OFFICER_AE_KICK		( 3 )
class CSuperOfficer : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  ISoundMask( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	float MaxYawSpeed( void );
	void IdleSound( void );
	void PainSound( void );
	void DeathSound( void );
	void AlertSound ( void );
	void AttackSound( void );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckHop();
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextSpitTime;
};

LINK_ENTITY_TO_CLASS( monster_superofficer, CSuperOfficer );

TYPEDESCRIPTION	CSuperOfficer::m_SaveData[] = 
{
	DEFINE_FIELD( CSuperOfficer, m_flNextSpitTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CSuperOfficer, CBaseMonster );

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CSuperOfficer :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= OFFICER_SPIT_DISTANCE)
	{
		return FALSE;
	}

	if ( flDist > 64 && flDist <= 128 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime )
	{
		if ( m_hEnemy != NULL )
		{
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 64 )
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + OFFICER_NEXT_SPIT_TIME;
		}

		return TRUE;
	}

	return FALSE;
}

BOOL CSuperOfficer :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 64 && flDot >= 0.7 )
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CSuperOfficer :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if (flDist <= 85 && flDot >= 0.7) 
	{										
		return TRUE;
	}
	return FALSE;
}  

BOOL CSuperOfficer :: CheckHop()
{
	TraceResult tr;
	Vector 	vecHopCheck = ( 0,0, gpGlobals->v_up * 128 );	
	if (m_hEnemy)
	{
		if( fabs(m_hEnemy->pev->origin.z - pev->origin.z) >= 32)
		{
			ALERT(at_console,"checking jump");
			float flDist1;	
			UTIL_TraceLine ( pev->origin, pev->origin + gpGlobals->v_forward * 64, ignore_monsters,ignore_glass, pev->owner, & tr);
			flDist1 = (pev->origin - tr.vecEndPos).Length2D();
			ALERT(at_console,"Dist1 %f \n",flDist1);
			if (flDist1 <= 36)
				return TRUE;
		}
		else
	  return FALSE;
	}
	return FALSE;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CSuperOfficer :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER;
}

//=========================================================
// RunTask
//=========================================================
void CSuperOfficer :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_OFFICER_HOP:
		{
		{
			SetActivity(ACT_HOP);
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin (this, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors ( pev->angles );

			Vector vecJumpDir;
			if (m_hEnemy != NULL)
			{
				float gravity = g_psv_gravity->value;
				if (gravity <= 1)
					gravity = 1;

				// How fast does the headcrab need to travel to reach that height given gravity?
				float height = (m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
				if (height < 8)
					height = 8;
				float speed = sqrt( 2 * gravity * height );
				float time = speed / gravity;

				// Scale the sideways velocity to get there at the right time
				vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
				vecJumpDir = vecJumpDir * ( 1.0 / time );

				// Speed to offset gravity at the desired height
				vecJumpDir.z = speed;

				// Don't jump too far/fast
				float distance = vecJumpDir.Length();
				
				if (distance > 650)
				{
					vecJumpDir = vecJumpDir * ( 650.0 / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
			}

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2;
		}
		break;
		}
	case TASK_MELEE_ATTACK1:
		{
			if ( m_fSequenceFinished )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	case TASK_MELEE_ATTACK2:
		{
			if ( m_fSequenceFinished )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CBaseMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSuperOfficer :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// IdleSound 
//=========================================================
void CSuperOfficer :: IdleSound ( void )
{
	switch ( RANDOM_LONG(0,4) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/idle1.wav", 1, ATTN_NORM );	
		break;
	case 1:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/idle2.wav", 1, ATTN_NORM );	
		break;
	case 2:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/idle3.wav", 1, ATTN_NORM );	
		break;
	case 3:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/idle4.wav", 1, ATTN_NORM );	
		break;
	case 4:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/idle5.wav", 1, ATTN_NORM );	
		break;
	}
}

//=========================================================
// PainSound 
//=========================================================
void CSuperOfficer :: PainSound ( void )
{
	int iPitch = RANDOM_LONG( 85, 120 );

	switch ( RANDOM_LONG(0,3) )
	{
	case 0:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "superofficer/pain1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "superofficer/pain2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 2:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "superofficer/pain3.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 3:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "superofficer/pain4.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CSuperOfficer :: AlertSound ( void )
{
	int iPitch = RANDOM_LONG( 140, 160 );

	switch ( RANDOM_LONG ( 0, 1  ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "superofficer/alert1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "superofficer/alert2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}
}

//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CSuperOfficer :: MaxYawSpeed( void )
{
	float ys;

	ys = 0;

	switch ( m_Activity )
	{
	case	ACT_WALK:			ys = 90;	break;
	case	ACT_RUN:			ys = 90;	break;
	case	ACT_IDLE:			ys = 90;	break;
	case	ACT_RANGE_ATTACK1:	ys = 90;	break;
	default:
		ys = 90;
		break;
	}

	return ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSuperOfficer :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case OFFICER_AE_SPIT:
		{
			Vector	vecSpitOffset;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecSpitOffset = ( 0,0, gpGlobals->v_up * 64 );		
			vecSpitOffset = ( pev->origin + vecSpitOffset );
			vecSpitDir = ( ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs ) - vecSpitOffset ).Normalize();

			vecSpitDir.x += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.y += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.z += RANDOM_FLOAT( -0.05, 0 );


			// do stuff for this event.
			AttackSound();

			// spew the spittle temporary ents.
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpitOffset );
				WRITE_BYTE( TE_SPRITE_SPRAY );
				WRITE_COORD( vecSpitOffset.x);	// pos
				WRITE_COORD( vecSpitOffset.y);	
				WRITE_COORD( vecSpitOffset.z);	
				WRITE_COORD( vecSpitDir.x);	// dir
				WRITE_COORD( vecSpitDir.y);	
				WRITE_COORD( vecSpitDir.z);	
				WRITE_SHORT( iOfficerBlood );	// model
				WRITE_BYTE ( 15 );			// count
				WRITE_BYTE ( 210 );			// speed
				WRITE_BYTE ( 25 );			// noise ( client will divide by 100 )
			MESSAGE_END();

			COfficerBlood::Shoot( pev, vecSpitOffset, vecSpitDir * 900 );
		}
		break;

		case OFFICER_AE_PUNCH:
		{
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, RANDOM_LONG(8,13), DMG_SLASH );
			
			if ( pHurt )
			{
				//pHurt->pev->punchangle.z = -15;
				//pHurt->pev->punchangle.x = -45;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}
		}
		break;
		case OFFICER_AE_KICK:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, 10, DMG_CLUB | DMG_ALWAYSGIB );
			if ( pHurt ) 
			{
				pHurt->pev->punchangle.z = -20;
				pHurt->pev->punchangle.x = 20;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CSuperOfficer :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/monsters/monster_superofficer.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.superofficerHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flNextSpitTime = gpGlobals->time;

		MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSuperOfficer :: Precache()
{
	PRECACHE_MODEL("models/monsters/monster_superofficer.mdl");
	
	PRECACHE_MODEL("sprites/bigspit.spr");// spit projectile.
	
	iOfficerBlood = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("superofficer/attack1.wav");
	PRECACHE_SOUND("superofficer/attack2.wav");
	
	PRECACHE_SOUND("superofficer/attackgrowl.wav");
	PRECACHE_SOUND("superofficer/attackgrowl2.wav");
	PRECACHE_SOUND("superofficer/attackgrowl3.wav");

	PRECACHE_SOUND("superofficer/die1.wav");
	PRECACHE_SOUND("superofficer/die2.wav");
	PRECACHE_SOUND("superofficer/die3.wav");
	
	PRECACHE_SOUND("superofficer/idle1.wav");
	PRECACHE_SOUND("superofficer/idle2.wav");
	PRECACHE_SOUND("superofficer/idle3.wav");
	PRECACHE_SOUND("superofficer/idle4.wav");
	PRECACHE_SOUND("superofficer/idle5.wav");
	
	PRECACHE_SOUND("superofficer/pain1.wav");
	PRECACHE_SOUND("superofficer/pain2.wav");
	PRECACHE_SOUND("superofficer/pain3.wav");
	PRECACHE_SOUND("superofficer/pain4.wav");

	PRECACHE_SOUND("superofficer/acid1.wav");

	PRECACHE_SOUND("superofficer/spithit1.wav");
	PRECACHE_SOUND("superofficer/spithit2.wav");

}	

//=========================================================
// DeathSound
//=========================================================
void CSuperOfficer :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/die1.wav", 1, ATTN_NORM );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/die2.wav", 1, ATTN_NORM );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/die3.wav", 1, ATTN_NORM );	
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CSuperOfficer :: AttackSound ( void )
{
	switch ( RANDOM_LONG(0,1) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "superofficer/attack1.wav", 1, ATTN_NORM );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "superofficer/attack2.wav", 1, ATTN_NORM );	
		break;
	}
}


//========================================================
// AI Schedules Specific to this monster
//=========================================================

//Hop
Task_t  tlOfficerHop[] = 
{
    { TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_OFFICER_HOP,			(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
};
Schedule_t  slOfficerHop[] = 
{
	{
		tlOfficerHop,
		ARRAYSIZE( tlOfficerHop ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_TASK_FAILED,
		0,
		"Officer Hop"
	
	},
};

// primary range attack
Task_t	tlOfficerRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slOfficerRangeAttack1[] =
{
	{ 
		tlOfficerRangeAttack1,
		ARRAYSIZE ( tlOfficerRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"Officer Range Attack1"
	},
};



// Chase enemy schedule
Task_t tlOfficerChaseEnemy1[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1	},
	{ TASK_GET_PATH_TO_TARGET,	(float)0					},
	{ TASK_RUN_PATH,			(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0					},
};

Schedule_t slOfficerChaseEnemy[] =
{
	{ 
		tlOfficerChaseEnemy1,
		ARRAYSIZE ( tlOfficerChaseEnemy1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_TASK_FAILED		|
		bits_COND_HEAR_SOUND,
		0,
		"Officer Chase Enemy"
	},
};

DEFINE_CUSTOM_SCHEDULES( CSuperOfficer ) 
{
	slOfficerRangeAttack1,
	slOfficerChaseEnemy,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSuperOfficer, CBaseMonster );

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CSuperOfficer :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType ( SCHED_WAKE_ANGRY );
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) && !HasConditions( bits_COND_CAN_MELEE_ATTACK2 ))
			{
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) && !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ))
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
			}
			
			return GetScheduleOfType ( SCHED_CHASE_ENEMY );

			break;
		}
	}

	return CBaseMonster :: GetSchedule();
}


//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CSuperOfficer :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_OFFICER_HOP:
		return &slOfficerHop[0];
		break;
	case SCHED_RANGE_ATTACK1:
		return &slOfficerRangeAttack1[ 0 ];
		break;
	case SCHED_CHASE_ENEMY:
		return &slOfficerChaseEnemy[ 0 ];
		break;
	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CSuperOfficer :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	
	case TASK_MELEE_ATTACK1:
		{
		SetActivity(ACT_MELEE_ATTACK1);
		break;
		}
			
	case TASK_MELEE_ATTACK2:
		{
			SetActivity(ACT_MELEE_ATTACK2);
			switch ( RANDOM_LONG ( 0, 2 ) )
			{
			case 0:	
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/attackgrowl.wav", 1, ATTN_NORM );		
				break;
			case 1:	
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/attackgrowl2.wav", 1, ATTN_NORM );	
				break;
			case 2:	
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "superofficer/attackgrowl3.wav", 1, ATTN_NORM );	
				break;
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			if ( BuildRoute ( m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy ) )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			else
			{
				ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
	default:
		{
			CBaseMonster :: StartTask ( pTask );
			break;
		}
	}
}
