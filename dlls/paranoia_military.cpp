/***
*
*	Copyright (c) 2006. All rights reserved.
*	Paranoia military human class, based on valve's hgrunt class
*	Written by BUzer
*
****/


//=========================================================
// Hit groups!	
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"scripted.h" //LRC
#include "rushscript.h" // buz
#include "player.h" // buz
#include "monster_head_controller.h"//MaSTeR

extern DLL_GLOBAL int		g_iSkillLevel;

#define SF_HAS_FLASHLIGHT 4096
#define SF_TOGGLE_FLASHLIGHT 8192
#define SF_FLASHLIGHT_ON 16384
//=========================================================
// monster-specific DEFINE's
//=========================================================
#define MIL_CLIP_SIZE		36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define MIL_VOL			0.35		// volume of grunt sounds
#define MIL_ATTN			ATTN_NORM	// attenutation of grunt sentences
#define MIL_LIMP_HEALTH		20
#define MIL_DMG_HEADSHOT		( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.

// ammunition types
#define MIL_AKONLY		0
#define MIL_GRENADES	1

#define MIL_GUN_GROUP	2
#define MIL_GUN_AK		0
#define MIL_GUN_NONE	1

#define MIL_HEAD_GROUP	0
#define MIL_HEAD_GASMASK	2

#define MIL_GASMASK_GROUP	3

#define MIL_STUFF_GROUP	4

TYPEDESCRIPTION CHeadController::m_SaveData[] = 
{
	DEFINE_FIELD( CHeadController, m_iLightLevel, FIELD_INTEGER ),
	DEFINE_FIELD( CHeadController, m_iBackwardLen, FIELD_INTEGER ),
	DEFINE_FIELD( CHeadController, m_hOwner, FIELD_EHANDLE ),
}; IMPLEMENT_SAVERESTORE( CHeadController, CBaseEntity );

LINK_ENTITY_TO_CLASS( head_flashlight, CFlashlight );
LINK_ENTITY_TO_CLASS( head_controller, CHeadController );

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		MIL_AE_RELOAD		( 2 )
#define		MIL_AE_KICK			( 3 )
#define		MIL_AE_BURST1		( 4 )
#define		MIL_AE_BURST2		( 5 ) 
#define		MIL_AE_BURST3		( 6 ) 
#define		MIL_AE_GREN_TOSS		( 7 )
//#define		MIL_AE_GREN_LAUNCH	( 8 )
#define		MIL_AE_GREN_DROP		( 9 )
#define		MIL_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		MIL_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.
#define		MIL_AE_FLASHLIGHT	( 12)
//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_MIL_SUPPRESS = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_MIL_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_MIL_COVER_AND_RELOAD,
	SCHED_MIL_SWEEP,
	SCHED_MIL_FOUND_ENEMY,
	SCHED_MIL_REPEL,
	SCHED_MIL_REPEL_ATTACK,
	SCHED_MIL_REPEL_LAND,
	SCHED_MIL_WAIT_FACE_ENEMY,
	SCHED_MIL_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_MIL_ELOF_FAIL,
	SCHED_MIL_DUCK_COVER_WAIT, // buz
	SCHED_MIL_FLASHLIGHT, //Toggle flashlight
	SCHED_MIL_WALKBACK_FIRE, //Walk backward and fire
	SCHED_INFECTED_FIRINGWALK,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_MIL_FACE_TOSS_DIR = LAST_TALKMONSTER_TASK + 1,
	TASK_MIL_SPEAK_SENTENCE,
	TASK_MIL_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_MIL_NOFIRE	( bits_COND_SPECIAL1 )

class CMilitary : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	float MaxYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void InitFlashlight( void );
	void InitHeadController( void );
	void ToggleFlashlight ( void );

	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
//	void IdleSound ( void );
	Vector GetGunPosition( void );
	virtual void Shoot ( void );
//	void Shotgun ( void );
//	void PrescheduleThink ( void );
	void GibMonster( void );
	void SpeakSentence( void );
	BOOL NoFriendlyFire(void); // buz
	void TalkInit(); // buz
	void DeclineFollowing(); //buz
	virtual BOOL GetEnemy ( void ); // buz
	void BlockedByPlayer ( CBasePlayer *pBlocker ); // buz
	void TalkAboutDeadFriend( CTalkMonster *pfriend );

	// buz: overriden for grunts to fix model's bugs...
	virtual void SetEyePosition ( void )
	{
		Vector  vecEyePosition;
		void	*pmodel = GET_MODEL_PTR( ENT(pev) );
		GetEyePosition( pmodel, vecEyePosition );
		pev->view_ofs = vecEyePosition;
		if ( pev->view_ofs == g_vecZero )
		{
			pev->view_ofs = Vector(0, 0 ,73);
			// ALERT ( at_aiconsole, "using default view ofs for %s\n", STRING ( pev->classname ) );
		}
	}

	// buz: overriden for soldiers - eye position is differs when monster is crouching
	virtual Vector EyePosition( )
	{
		if (m_Activity == ACT_TWITCH)
			return pev->origin + Vector(0, 0, 36);
		return pev->origin + pev->view_ofs;
	}

	// Wargon: Юзать монстра можно только если он жив. Это нужно чтобы иконка юза не показывалась на мертвых монстрах.
	virtual int	ObjectCaps( void ) { if (pev->deadflag == DEAD_NO) return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE | FCAP_DISTANCE_USE; else return CTalkMonster::ObjectCaps(); }

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

//	int IRelationship ( CBaseEntity *pTarget ); // buz: use talkmoster's relationship

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	//MaSTeR: Flashlight and head controller
	CFlashlight	*pFlashlight;
	CHeadController	*pHeadController;

	Vector		m_vecTossVelocity;

	// Wargon: Если враг взят из данных игрока, то TRUE. Иначе FALSE. (1.1)
	BOOL m_fEnemyFromPlayer;

	BOOL		m_fThrowGrenade;
	BOOL		m_fStanding;
//	BOOL		m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;

	int m_voicePitch;

	int		m_iBrassShell;
//	int		m_iShotgunShell;

	int		m_iSentence;

	// buz
	int		m_iNoGasDamage;

	int		m_iLastFireCheckResult; // buz. 1-only crouch, 2-only standing, 0-any
	static const char *pGruntSentences[];
};

LINK_ENTITY_TO_CLASS( monster_human_military, CMilitary );

TYPEDESCRIPTION	CMilitary::m_SaveData[] = 
{
	DEFINE_FIELD( CMilitary, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CMilitary, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CMilitary, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CMilitary, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMilitary, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMilitary, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CMilitary, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CMilitary, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CMilitary, m_iNoGasDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CMilitary, pFlashlight, FIELD_EHANDLE ),
	DEFINE_FIELD( CMilitary, pHeadController, FIELD_EHANDLE ),
}; IMPLEMENT_SAVERESTORE( CMilitary, CTalkMonster );

const char *CMilitary::pGruntSentences[] = 
{
	"VV_GREN", // grenade scared grunt
	"VV_ALERT", // sees player
	"VV_MONSTER", // sees monster
	"VV_COVER", // running to cover
	"VV_THROW", // about to throw grenade
	"VV_CHARGE",  // running out to get the enemy
	"VV_TAUNT", // say rude things
};

enum
{
	MIL_SENT_NONE = -1,
	MIL_SENT_GREN = 0,
	MIL_SENT_ALERT,
	MIL_SENT_MONSTER,
	MIL_SENT_COVER,
	MIL_SENT_THROW,
	MIL_SENT_CHARGE,
	MIL_SENT_TAUNT,
} MIL_SENTENCE_TYPES;


void CMilitary :: BlockedByPlayer ( CBasePlayer *pBlocker )
{
	if (m_iszSpeakAs)
	{
		char szBuf[32];
		strcpy(szBuf,STRING(m_iszSpeakAs));
		strcat(szBuf,"_BLOCKED");
		PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
	}
	else
	{
		PlaySentence( "VV_BLOCKED", 4, VOL_NORM, ATTN_NORM );
	}
}

void CMilitary :: TalkAboutDeadFriend( CTalkMonster *pfriend )
{
	if (FClassnameIs(pfriend->pev, STRING(pev->classname)))
	{
		if (m_iszSpeakAs)
		{
			char szBuf[32];
			strcpy(szBuf,STRING(m_iszSpeakAs));
			strcat(szBuf,"_TMDOWN");
			PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
		}
		else
		{
			PlaySentence( "VV_TMDOWN", 4, VOL_NORM, ATTN_NORM );
		}
	}
}


//=========================================================
// buz: overriden for allied soldiers - they can get enemy from player's data
//=========================================================
BOOL CMilitary :: GetEnemy ( void )
{
	// Wargon: Если врага нет, то переменная сбрасывается. (1.1)
	if (m_hEnemy == NULL)
	{
		m_fEnemyFromPlayer = FALSE;
	}
	if (!CBaseMonster::GetEnemy())
	{
		// buz: cant get enemy using normal way, try player's data
		if (IsFollowing() && m_hTargetEnt->IsPlayer())
		{
			CBasePlayer *pMyMaster = (CBasePlayer*)((CBaseEntity*)m_hTargetEnt);

			// big bunch of checks..
			if ((pMyMaster->m_hLastEnemy != NULL) &&
				(pMyMaster->m_hLastEnemy->MyMonsterPointer()) &&
				(pMyMaster->m_hLastEnemy != this) &&
				(pMyMaster->m_hLastEnemy->pev->health > 0) &&
				(pMyMaster->m_hLastEnemy->IsAlive()) &&
				!FBitSet(pMyMaster->m_hLastEnemy->pev->spawnflags, SF_MONSTER_PRISONER) &&
				!FBitSet(pMyMaster->m_hLastEnemy->pev->flags, FL_NOTARGET) &&
				(IRelationship( pMyMaster->m_hLastEnemy ) > 0))
			{
				m_hEnemy = pMyMaster->m_hLastEnemy;
				m_vecEnemyLKP = m_hEnemy->pev->origin;

				// Wargon: Враг взят из данных игрока. (1.1)
				m_fEnemyFromPlayer = TRUE;

				if ( m_pSchedule )
				{
					if ( m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY )
					{
						SetConditions(bits_COND_NEW_ENEMY);
					}
				}

//				ALERT(at_console, "get enemy from player!\n");
				return TRUE;
			}
		}
	}

	return FALSE;
//	return CBaseMonster::GetEnemy();
}



//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
void CMilitary :: SpeakSentence( void )
{
	if ( m_iSentence == MIL_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

//	if (FOkToSpeak())
//	{
		SENTENCEG_PlayRndSz( ENT(pev), pGruntSentences[ m_iSentence ], MIL_VOL, MIL_ATTN, 0, m_voicePitch);
		JustSpoke();
//	}
}

//=========================================================
//MaSTeR: Инициализация фонаря (создание энтити и проверка спаунфлагов)
//=========================================================
void CMilitary :: InitHeadController( void )
{
	if( !pHeadController )
	{
		pHeadController = GetClassPtr((CHeadController *)NULL );
		pHeadController->Spawn( this );
	}
}

void CMilitary :: InitFlashlight( void )
{
	if ( FBitSet( pev->spawnflags, SF_HAS_FLASHLIGHT ))
	{
		if( !pFlashlight )
		{
			pFlashlight = GetClassPtr((CFlashlight *)NULL );
			pFlashlight->Spawn( pev );
		}

		if( FBitSet( pev->spawnflags, SF_FLASHLIGHT_ON ))
			pFlashlight->On();
	}
}

//=========================================================
//MaSTeR: Переключение фонарика (вкл\выкл)
//=========================================================
void CMilitary :: ToggleFlashlight( void )
{
	// if we suppose to toglle flashlight
	if ( pFlashlight && FBitSet( pev->spawnflags, SF_TOGGLE_FLASHLIGHT ))
	{
		if( pFlashlight->GetState() == STATE_ON )
			pFlashlight->Off();
		else pFlashlight->On();
	}
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CMilitary :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if (GetBodygroup(MIL_GUN_GROUP) != MIL_GUN_NONE && !(pev->spawnflags & SF_MONSTER_NO_WPN_DROP))
	{
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun = DropItem( "weapon_aks", vecGunPos, vecGunAngles );
	
		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	
		if (pev->weapons == MIL_GRENADES )
		{
			pGun = DropItem( "weapon_handgrenade", vecGunPos, vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}

	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CMilitary :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// buz: у грунтов FOkToSpeak очень простой - всего-лишь проверка на Gag.
// но у дружественных игроку монстров эта функция означает возможность
// базарить о всякой фигне - задавать друг другу вопросы, разговаривать о том о сем..
//=========================================================
BOOL CMilitary :: FOkToSpeak( void )
{
	if ( pev->spawnflags & SF_MONSTER_GAG )
		return FALSE;

	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
		return FALSE;

	// if not alive, certainly don't speak
	if ( pev->deadflag != DEAD_NO )
		return FALSE;

	// if player is not in pvs, don't speak
	if (!IsAlive() || FNullEnt(FIND_CLIENT_IN_PVS(edict())))
		return FALSE;

	// don't talk if you're in combat
	if (m_hEnemy != NULL && FVisible( m_hEnemy ))
		return FALSE;
	
	return TRUE;
}

//=========================================================
//=========================================================
void CMilitary :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = MIL_SENT_NONE;
}


//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CMilitary :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CMilitary :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	// Wargon: Расстояние flDist уменьшено с 64 до 48. (1.1)
	if ( flDist <= 48 && flDot >= 0.7	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//==============================================
// buz:
// NoFriendlyFire - basically copied from squad monsters
// true - shoot, false - dont shoot
// =============================================
BOOL CMilitary :: NoFriendlyFire(void)
{
	CPlane	backPlane;
	CPlane  leftPlane;
	CPlane	rightPlane;

	Vector	vecLeftSide;
	Vector	vecRightSide;
	Vector	v_left;

	//!!!BUGBUG - to fix this, the planes must be aligned to where the monster will be firing its gun, not the direction it is facing!!!

	if ( m_hEnemy != NULL )
	{
		UTIL_MakeVectors ( UTIL_VecToAngles( m_hEnemy->Center() - pev->origin ) );
	}
	else
	{
		// if there's no enemy, pretend there's a friendly in the way, so the grunt won't shoot.
		return FALSE;
	}

	// buz: simply check by traceline for other monster_human_military
/*	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + (gpGlobals->v_forward * 1024), dont_ignore_monsters, ENT(pev), &tr);
	if (tr.pHit && (FClassnameIs(tr.pHit, "monster_human_military") ||
					FClassnameIs(tr.pHit, "monster_scientist") ||
					FClassnameIs(tr.pHit, "monster_barney") ||
					FClassnameIs(tr.pHit, "monster_human_alpha") ||
					FClassnameIs(tr.pHit, "monster_alpha_pistol")))
		return FALSE;*/

	vecLeftSide = pev->origin - ( gpGlobals->v_right * ( pev->size.x * 1.5 ) );
	vecRightSide = pev->origin + ( gpGlobals->v_right * ( pev->size.x * 1.5 ) );
	v_left = gpGlobals->v_right * -1;

	leftPlane.InitializePlane ( gpGlobals->v_right, vecLeftSide );
	rightPlane.InitializePlane ( v_left, vecRightSide );
	backPlane.InitializePlane ( gpGlobals->v_forward, pev->origin );

	// search all entities around
	Vector mins = pev->origin - Vector(1024, 1024, 1024);
	Vector maxs = pev->origin + Vector(1024, 1024, 1024);
	CBaseEntity *pList[256];
	int count = UTIL_EntitiesInBox( pList, 256, mins, maxs, FL_MONSTER );
	for ( int i = 0; i < count; i++ )
	{
		CBaseMonster *pMonster = pList[i]->MyMonsterPointer();
		if (pMonster && (pMonster != this) && (IRelationship(pMonster) <= R_NO))
		{
			if ( backPlane.PointInFront  ( pMonster->pev->origin ) &&
				 leftPlane.PointInFront  ( pMonster->pev->origin ) && 
				 rightPlane.PointInFront ( pMonster->pev->origin) )
			{
				// this guy is in the check volume! Don't shoot!
			//	ALERT(at_console, "dont shoot!\n");
				return FALSE;
			}
		}
	}

	// check for player	
	edict_t	*pentPlayer = FIND_CLIENT_IN_PVS( edict() );
	if (!FNullEnt(pentPlayer) && (pentPlayer != m_hEnemy->edict()) &&
		backPlane.PointInFront  ( pentPlayer->v.origin ) &&
		leftPlane.PointInFront  ( pentPlayer->v.origin ) && 
		rightPlane.PointInFront ( pentPlayer->v.origin ) )
	{
		// the player is in the check volume! Don't shoot!
		return FALSE;
	}

	return TRUE;
}



//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CMilitary :: CheckRangeAttack1 ( float flDot, float flDist )
{
/*	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( flDist <= 64 )
		{
			// kick nonclients who are close enough, but don't shoot at them.
			return FALSE;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;*/
	
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 48 ) // Wargon: Расстояние flDist уменьшено с 64 до 48. (1.1)
		{
			// kick nonclients who are close enough, but don't shoot at them.
			return FALSE;
		}

		BOOL savedStanding = m_fStanding;
		m_fStanding = FALSE; // buz: check chrouched fire first
		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);
		if ( tr.flFraction == 1.0 && !pev->gaitsequence) // buz: cant fire crouched when moving
		{
			// buz: we can fire crouched, now check for standing
			m_fStanding = TRUE;
			vecSrc = GetGunPosition();
			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);
			m_fStanding = savedStanding;
			if ( tr.flFraction == 1.0 )
			{
				// ALERT(at_aiconsole, "== mil shoot as wish\n");
				m_iLastFireCheckResult = 0; // shoot as you wish
			}
			else			
			{
				// ALERT(at_aiconsole, "== mil shoot crouched\n");
				m_iLastFireCheckResult = 1; // only chrouched
			}
			return TRUE;
		}
		else
		{
			// buz: cant fire crouching, maybe me or enemy in some kind of cover (or running). Check standing.
			m_fStanding = TRUE;
			vecSrc = GetGunPosition();
			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);
			m_fStanding = savedStanding;
			if ( tr.flFraction == 1.0 )
			{
				// ALERT(at_aiconsole, "== mil shoot standing\n");
				m_iLastFireCheckResult = 2; // buz: standing is our only one choice
				return TRUE;
			}
			else
			{
				// ALERT(at_aiconsole, "== mil cant shoot\n");
				m_iLastFireCheckResult = 0;
				return FALSE; // cant fire
			}
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CMilitary :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (pev->weapons != MIL_GRENADES)
	{
		return FALSE;
	}
	
	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && (m_hEnemy->pev->waterlevel == 0 || m_hEnemy->pev->watertype==CONTENTS_FOG) && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

//	if (FBitSet( pev->weapons, HGRUNT_HANDGRENADE))
//	{
		// find feet
		if (RANDOM_LONG(0,1))
		{
			// magically know where they are
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
/*	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions( bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;
	}*/

	// are any of my squad members near the intended grenade impact area?
/*	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}*/

	// buz: check for allies in target area:
	CBaseEntity *pTarget = NULL;
	while ((pTarget = UTIL_FindEntityInSphere( pTarget, vecTarget, 256 )) != NULL)
	{
		if (FClassnameIs( pTarget->pev, "monster_human_military") || FClassnameIs( pTarget->pev, "player"))
		{
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;		
		}
	}
	
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

		
//	if (FBitSet( pev->weapons, HGRUNT_HANDGRENADE))
//	{
		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
/*	}
	else
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}*/

	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CMilitary :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
//	ALERT(at_console, "mil hitgr: %d\n", ptr->iHitgroup);
	// check for helmet shot
	if ((ptr->iHitgroup == 8) || (ptr->iHitgroup == 1))
	{
		// make sure we're wearing one
/*		if (GetBodygroup( 1 ) == HEAD_GRUNT && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		{
			// absorb damage
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}*/ // buz
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

// buz: basically just like barney
int CMilitary :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// buz: refuse gas damage while wearing gasmask
	if (m_iNoGasDamage && ( bitsDamageType & DMG_NERVEGAS ))
		return 0;

	Forget( bits_MEMORY_INCOVER );

	// Wargon: Союзники не должны восставать против игрока.
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);

/*	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	// LRC - if my reaction to the player has been overridden, don't do this stuff
	if (m_iPlayerReact) return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) )
			{
				// Alright, now I'm pissed!
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf,STRING(m_iszSpeakAs));
					strcat(szBuf,"_MAD");
					PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
				}
				else
				{
					PlaySentence( "VV_MAD", 4, VOL_NORM, ATTN_NORM );
				}

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf,STRING(m_iszSpeakAs));
					strcat(szBuf,"_SHOT");
					PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
				}
				else
				{
					PlaySentence( "VV_SHOT", 4, VOL_NORM, ATTN_NORM );
				}
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			if (m_iszSpeakAs)
			{
				char szBuf[32];
				strcpy(szBuf,STRING(m_iszSpeakAs));
				strcat(szBuf,"_SHOT");
				PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
			}
			else
			{
				PlaySentence( "VV_SHOT", 4, VOL_NORM, ATTN_NORM );
			}
		}
	}

	return ret; */
}

//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CMilitary :: MaxYawSpeed( void )
{
	float ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:	
		ys = 150;		
		break;
	case ACT_RUN:	
		ys = 150;	
		break;
	case ACT_WALK:	
		ys = 180;		
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_RANGE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:	
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	return ys;
}


//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CMilitary :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMilitary :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_PLAYER_ALLY;//CLASS_HUMAN_MILITARY;
}

//=========================================================
//=========================================================
CBaseEntity *CMilitary :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 48); // Wargon: Расстояние уменьшено с 70 до 48. (1.1)

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CMilitary :: GetGunPosition( )
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
	//	return pev->origin + Vector( 0, 0, 48 );
		return pev->origin + Vector( 0, 0, 40 );
	}
}

//=========================================================
// Shoot
//=========================================================
void CMilitary :: Shoot ( void )
{
//	if (m_hEnemy == NULL && m_pCine == NULL) //LRC - scripts may fire when you have no enemy
//	{
//		return;
//	}

	UTIL_MakeVectors ( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if (m_cAmmoLoaded > 0)
	{
		Vector vecBrassPos, vecBrassDir;
		GetAttachment(3, vecBrassPos, vecBrassDir);
		Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass( vecBrassPos, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 
		FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_7DEGREES, 2048.0f, BULLET_NORMAL, gSkillData.monDmgAK ); // shoot +-5 degrees

		pev->effects |= EF_MUZZLEFLASH;
	
		m_cAmmoLoaded--;// take away a bullet!
	//	ALERT(at_console, "mil ammo has %d\n", m_cAmmoLoaded);
	}

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMilitary :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case MIL_AE_FLASHLIGHT:
			{
				ToggleFlashlight();
				break;
			}
		case MIL_AE_DROP_GUN:
			{
				if (pev->spawnflags & SF_MONSTER_NO_WPN_DROP) break; //LRC

				Vector	vecGunPos;
				Vector	vecGunAngles;

				GetAttachment( 0, vecGunPos, vecGunAngles );

				// switch to body group with no gun.
				SetBodygroup( MIL_GUN_GROUP, MIL_GUN_NONE );

				// now spawn a gun.
				DropItem( "weapon_ak74", vecGunPos, vecGunAngles );

				if (pev->weapons == MIL_GRENADES)
				{
					DropItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles );
				}

				break;
			}
		case MIL_AE_RELOAD:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "military/mil_reload.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case MIL_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
			//LRC - a bit of a hack. Ideally the grunts would work out in advance whether it's ok to throw.
			if (m_pCine)
			{
				Vector vecToss = g_vecZero;
				if (m_hTargetEnt != NULL && m_pCine->PreciseAttack())
				{
					vecToss = VecCheckToss( pev, GetGunPosition(), m_hTargetEnt->pev->origin, 0.5 );
				}
				if (vecToss == g_vecZero)
				{
					vecToss = (gpGlobals->v_forward*0.5+gpGlobals->v_up*0.5).Normalize()*gSkillData.hgruntGrenadeSpeed;
				}
				CGrenade::ShootTimed( pev, GetGunPosition(), vecToss, 3.5 );
			}
			else
				CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;

		// buz: no grenade laucher
/*		case HGRUNT_AE_GREN_LAUNCH:
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
			//LRC: firing due to a script?
			if (m_pCine)
			{
				Vector vecToss;
				if (m_hTargetEnt != NULL && m_pCine->PreciseAttack())
					vecToss = VecCheckThrow( pev, GetGunPosition(), m_hTargetEnt->pev->origin, gSkillData.hgruntGrenadeSpeed, 0.5 );
				else
				{
					// just shoot diagonally up+forwards
					UTIL_MakeVectors(pev->angles);
					vecToss = (gpGlobals->v_forward*0.5 + gpGlobals->v_up*0.5).Normalize() * gSkillData.hgruntGrenadeSpeed;
				}
				CGrenade::ShootContact( pev, GetGunPosition(), vecToss );
			}
			else
			CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
			m_fThrowGrenade = FALSE;
			if (g_iSkillLevel == SKILL_HARD)
				m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );// wait a random amount of time before shooting again
			else
				m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		}
		break;*/

		case MIL_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
		break;

		case MIL_AE_BURST1:
		{
		//	ALERT(at_console, "*-------- mil burst 1\n");
			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if (m_cAmmoLoaded > 0)
			{
				if ( RANDOM_LONG(0,1) )
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "military/mil_mgun1.wav", 1, ATTN_NORM );
				}
				else
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "military/mil_mgun2.wav", 1, ATTN_NORM );
				}
			}
			else
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/dryfire1.wav", 1, ATTN_NORM );
			}

			Shoot();
		
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case MIL_AE_BURST2:
		case MIL_AE_BURST3:
			Shoot();
			break;

		case MIL_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				// buz: move only if it is a monster!
				if (pHurt->MyMonsterPointer())
				{
					UTIL_MakeVectors( pev->angles );
					pHurt->pev->punchangle.x = 15;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				}
				pHurt->TakeDamage( pev, pev, gSkillData.MilDmgKick, DMG_CLUB );
			}
		}
		break;

		case MIL_AE_CAUGHT_ENEMY:
		{
		//	if ( FOkToSpeak() )
		//	{
				SENTENCEG_PlayRndSz(ENT(pev), "VV_ALERT", MIL_VOL, MIL_ATTN, 0, m_voicePitch);
				 JustSpoke();
		//	}
		}

		default:
			CTalkMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CMilitary :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/soldier.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->health == 0)
		pev->health			= gSkillData.milHealth;
	m_flFieldOfView		= VIEW_FIELD_FULL;//0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= MIL_SENT_NONE;

	m_afCapability		= bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

//	m_fEnemyEluded		= FALSE;
//	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_cClipSize		= MIL_CLIP_SIZE;
	m_cAmmoLoaded = m_cClipSize;

	// buz: pev->body is head number
	int head = pev->body;
	pev->body = 0;
	SetBodygroup( MIL_HEAD_GROUP, head );

	if (head == MIL_HEAD_GASMASK)
	{
		SetBodygroup( MIL_GASMASK_GROUP, 1 );
		m_iNoGasDamage = 1;
	}
	else
	{
		SetBodygroup( MIL_GASMASK_GROUP, 0 );
		m_iNoGasDamage = 0;
	}

	// buz: pev->effects is additional stuff number
	SetBodygroup( MIL_STUFF_GROUP, pev->effects );

	MonsterInit();
	InitFlashlight();
	InitHeadController();

	SetUse(&CMilitary :: FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMilitary :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/soldier.mdl");

	PRECACHE_SOUND( "weapons/dryfire1.wav" ); //LRC

	PRECACHE_SOUND( "military/mil_mgun1.wav" );
	PRECACHE_SOUND( "military/mil_mgun2.wav" );
	
	PRECACHE_SOUND( "military/mil_die1.wav" );
	PRECACHE_SOUND( "military/mil_die2.wav" );
	PRECACHE_SOUND( "military/mil_die3.wav" );

	PRECACHE_SOUND( "military/mil_pain1.wav" );
	PRECACHE_SOUND( "military/mil_pain2.wav" );
	PRECACHE_SOUND( "military/mil_pain3.wav" );
	PRECACHE_SOUND( "military/mil_pain4.wav" );
	PRECACHE_SOUND( "military/mil_pain5.wav" );

	PRECACHE_SOUND( "military/mil_reload.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL ("models/ak74_shell.mdl");// brass shell

	TalkInit();
	CTalkMonster::Precache();
}	


// talk init
void CMilitary :: TalkInit()
{
	CTalkMonster::TalkInit();

	// military human speech group names (group names are in sentences.txt)

	if (!m_iszSpeakAs)
	{
		m_szGrp[TLK_ANSWER]		=	"VV_ANSWER";
		m_szGrp[TLK_QUESTION]	=	"VV_QUESTION";
		m_szGrp[TLK_IDLE]		=	"VV_IDLE";
		m_szGrp[TLK_STARE]		=	"VV_STARE";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER) //LRC
			m_szGrp[TLK_USE]	=	"VV_PFOLLOW";
		else
			m_szGrp[TLK_USE] =	"VV_OK";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_UNUSE] = "VV_PWAIT";
		else
			m_szGrp[TLK_UNUSE] = "VV_WAIT";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_DECLINE] =	"VV_POK";
		else
			m_szGrp[TLK_DECLINE] =	"VV_NOTOK";
		m_szGrp[TLK_STOP] =		"VV_STOP";

		m_szGrp[TLK_NOSHOOT] =	"VV_SCARED";
		m_szGrp[TLK_HELLO] =	"VV_HELLO";

		m_szGrp[TLK_PLHURT1] =	"!VV_CUREA";
		m_szGrp[TLK_PLHURT2] =	"!VV_CUREB"; 
		m_szGrp[TLK_PLHURT3] =	"!VV_CUREC";

		m_szGrp[TLK_PHELLO] =	NULL;	//"BA_PHELLO";		// UNDONE
		m_szGrp[TLK_PIDLE] =	NULL;	//"BA_PIDLE";			// UNDONE
		m_szGrp[TLK_PQUESTION] = "VV_PQUEST";		// UNDONE

		m_szGrp[TLK_SMELL] =	"VV_SMELL";
	
		m_szGrp[TLK_WOUND] =	"VV_WOUND";
		m_szGrp[TLK_MORTAL] =	"VV_MORTAL";
	}
}


void CMilitary::DeclineFollowing( void )
{
	PlaySentence( m_szGrp[TLK_DECLINE], 2, VOL_NORM, ATTN_NORM ); //LRC
}



//=========================================================
// start task
//=========================================================
void CMilitary :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_MIL_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_MIL_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_MIL_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CTalkMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_MIL_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CTalkMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default: 
		CTalkMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CMilitary :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_MIL_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			SetIdealYawToTargetAndUpdate( pev->origin + m_vecTossVelocity * 64, AI_KEEP_YAW_SPEED );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CTalkMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void CMilitary :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "military/mil_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "military/mil_pain4.wav", 1, ATTN_NORM );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "military/mil_pain5.wav", 1, ATTN_NORM );	
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "military/mil_pain1.wav", 1, ATTN_NORM );	
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "military/mil_pain2.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CMilitary :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "military/mil_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "military/mil_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "military/mil_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
/*
extern Schedule_t slMilFail[];
extern Schedule_t slMilCombatFail[];
extern Schedule_t slMilVictoryDance[];
extern Schedule_t slMilEstablishLineOfFire[];
extern Schedule_t slMilFoundEnemy[];
extern Schedule_t slMilCombatFace[];
extern Schedule_t slMilSignalSuppress[];
extern Schedule_t slMilSuppress[];
extern Schedule_t slMilWaitInCover[];
extern Schedule_t slMilTakeCover[];
extern Schedule_t slMilGrenadeCover[];
extern Schedule_t slMilTossGrenadeCover[];
extern Schedule_t slMilTakeCoverFromBestSound[];
extern Schedule_t slMilHideReload[];
extern Schedule_t slMilSweep[];
extern Schedule_t slMilRangeAttack1A[];
extern Schedule_t slMilRangeAttack1B[];
extern Schedule_t slMilRangeAttack2[];
extern Schedule_t slMilRepel[];
extern Schedule_t slMilRepelAttack[];
extern Schedule_t slMilRepelLand[];*/

// ============================= base grunt schedules ================

Task_t	tlMilFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slMilFail[] =
{
	{
		tlMilFail,
		ARRAYSIZE ( tlMilFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t	tlMilCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slMilCombatFail[] =
{
	{
		tlMilCombatFail,
		ARRAYSIZE ( tlMilCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Grunt Combat Fail"
	},
};
//=========================================================
//Move back and fire
//=========================================================
Task_t  tlMilWalkBackFire[] =
{
	{TASK_FACE_ENEMY,		0		},
	{TASK_SET_ACTIVITY,		(float)ACT_WALKBACK_FIRE},
};

Schedule_t slMilWalkBackFire[]=
{
	{
		tlMilWalkBackFire,
		ARRAYSIZE ( tlMilWalkBackFire ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Walk backward and fire"
	},
};

//=========================================================
// MaSTeR: toggle flashlight
//=========================================================
Task_t  tlMilToggleFlashlight[] =
{
	{TASK_STOP_MOVING,			0				},
	{TASK_PLAY_SEQUENCE,			(float)ACT_FLASHLIGHT},

};
Schedule_t slMilToggleFlashlight[] = 
{
	{
		tlMilToggleFlashlight,
		ARRAYSIZE ( tlMilToggleFlashlight ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Toggle Flashlight"
	},
};

//=========================================================
// MaSTeR: walk and fire
//=========================================================
Task_t tlInfFiringWalk [] =
{
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_SET_ACTIVITY,					(float)ACT_FIRINGWALK	},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slInfFiringWalk[] =
{
	{ 
		tlInfFiringWalk,
		ARRAYSIZE ( tlInfFiringWalk ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_MELEE_ATTACK1	|		
		bits_SOUND_DANGER,
		0,
		"Infected Soldier Walk and Fire"
	},
};
//=========================================================
// Victory dance!
//=========================================================
Task_t	tlMilVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slMilVictoryDance[] =
{
	{ 
		tlMilVictoryDance,
		ARRAYSIZE ( tlMilVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlMilEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_MIL_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_MIL_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slMilEstablishLineOfFire[] =
{
	{ 
		tlMilEstablishLineOfFire,
		ARRAYSIZE ( tlMilEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlMilFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slMilFoundEnemy[] =
{
	{ 
		tlMilFoundEnemy,
		ARRAYSIZE ( tlMilFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlMilCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_MIL_SWEEP	},
};

Schedule_t	slMilCombatFace[] =
{
	{ 
		tlMilCombatFace1,
		ARRAYSIZE ( tlMilCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlMilSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_MIL_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_MIL_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_MIL_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_MIL_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_MIL_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slMilSignalSuppress[] =
{
	{ 
		tlMilSignalSuppress,
		ARRAYSIZE ( tlMilSignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_MIL_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlMilSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_MIL_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_MIL_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_MIL_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_MIL_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_MIL_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slMilSuppress[] =
{
	{ 
		tlMilSuppress,
		ARRAYSIZE ( tlMilSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_MIL_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlMilWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slMilWaitInCover[] =
{
	{ 
		tlMilWaitInCover,
		ARRAYSIZE ( tlMilWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"GruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlMilTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_MIL_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_MIL_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_MIL_WAIT_FACE_ENEMY	},
};

Schedule_t	slMilTakeCover[] =
{
	{ 
		tlMilTakeCover1,
		ARRAYSIZE ( tlMilTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlMilGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_MIL_WAIT_FACE_ENEMY	},
};

Schedule_t	slMilGrenadeCover[] =
{
	{ 
		tlMilGrenadeCover1,
		ARRAYSIZE ( tlMilGrenadeCover1 ), 
		0,
		0,
		"GrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlMilTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slMilTossGrenadeCover[] =
{
	{ 
		tlMilTossGrenadeCover1,
		ARRAYSIZE ( tlMilTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlMilTakeCoverFromBestSound[] =
{
// Wargon: Теперь союзники не ищут укрытия от врагов. (1.1)
//	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
// Wargon: Теперь союзники не ищут укрытия от врагов. (1.1)
//	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slMilTakeCoverFromBestSound[] =
{
	{ 
		tlMilTakeCoverFromBestSound,
		ARRAYSIZE ( tlMilTakeCoverFromBestSound ), 
		0,
		0,
		"GruntTakeCoverFromBestSound"
	},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlMilHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slMilHideReload[] = 
{
	{
		tlMilHideReload,
		ARRAYSIZE ( tlMilHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlMilSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slMilSweep[] =
{
	{ 
		tlMilSweep,
		ARRAYSIZE ( tlMilSweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Grunt Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlMilRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
//	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH }, buz
	{ TASK_FACE_ENEMY,			(float)0		}, // buz
	{ TASK_MIL_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MIL_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MIL_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MIL_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slMilRangeAttack1A[] =
{
	{ 
		tlMilRangeAttack1A,
		ARRAYSIZE ( tlMilRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_MIL_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlMilRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
//	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  }, buz
	{ TASK_FACE_ENEMY,			(float)0		}, // buz
	{ TASK_MIL_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MIL_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MIL_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MIL_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slMilRangeAttack1B[] =
{
	{ 
		tlMilRangeAttack1B,
		ARRAYSIZE ( tlMilRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_LIGHT_DAMAGE		| // buz: interruptable by light damage
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_MIL_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlMilRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_MIL_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_MIL_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slMilRangeAttack2[] =
{
	{ 
		tlMilRangeAttack2,
		ARRAYSIZE ( tlMilRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlMilRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slMilRepel[] =
{
	{ 
		tlMilRepel,
		ARRAYSIZE ( tlMilRepel ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlMilRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slMilRepelAttack[] =
{
	{ 
		tlMilRepelAttack,
		ARRAYSIZE ( tlMilRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlMilRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slMilRepelLand[] =
{
	{ 
		tlMilRepelLand,
		ARRAYSIZE ( tlMilRepelLand ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel Land"
	},
};



// ======================= end base grunt schedules =================


Task_t	tlMilFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 256 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slMilFollow[] =
{
	{
		tlMilFollow,
		ARRAYSIZE ( tlMilFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlMilFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slMilFaceTarget[] =
{
	{
		tlMilFaceTarget,
		ARRAYSIZE ( tlMilFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

//=========================================================
// buz: duck and wait couple seconds behind the barrel, crate, etc..
//=========================================================
Task_t	tlMilDuckAndCoverWait[] =
{
	{ TASK_STOP_MOVING,				(float)0			},	
	{ TASK_MIL_SPEAK_SENTENCE,		(float)0			},
	{ TASK_SET_ACTIVITY,			(float)ACT_TWITCH	},
	{ TASK_WAIT,					(float)3			}, // randomize a bit?
};

Schedule_t	slMilDuckAndCoverWait[] =
{
	{ 
		tlMilDuckAndCoverWait,
		ARRAYSIZE ( tlMilDuckAndCoverWait ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
	//	bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_CROUCH_NOT_SAFE	| // buz: terminate, if crouching is not safe more
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"DuckAndCover!"
	},
};

DEFINE_CUSTOM_SCHEDULES( CMilitary )
{
	slMilFail,
	slMilCombatFail,
	slMilVictoryDance,
	slMilEstablishLineOfFire,
	slMilFoundEnemy,
	slMilCombatFace,
	slMilSignalSuppress,
	slMilSuppress,
	slMilWaitInCover,
	slMilTakeCover,
	slMilGrenadeCover,
	slMilTossGrenadeCover,
	slMilTakeCoverFromBestSound,
	slMilHideReload,
	slMilSweep,
	slMilRangeAttack1A,
	slMilRangeAttack1B,
	slMilRangeAttack2,
	slMilRepel,
	slMilRepelAttack,
	slMilRepelLand,
	slMilFollow,
	slMilFaceTarget,
	slMilDuckAndCoverWait,
	slMilToggleFlashlight,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMilitary, CTalkMonster );

//=========================================================
// SetActivity 
//=========================================================
void CMilitary :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_WALKBACK_FIRE:
		iSequence = LookupSequence("walkback");
		break;
	case ACT_FIRINGWALK:
		iSequence = LookupSequence("firingwalk");
		break;
	case ACT_DIESIMPLE:
		iSequence = LookupSequence("die-simple");
		break;
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if ( m_fStanding )
		{
			// get aimable sequence
//			ALERT(at_console, "MIL STANDING\n");
			iSequence = LookupSequence( "standing_mp5" );
		}
		else
		{
//			ALERT(at_console, "MIL CROUCHING\n");
			// get crouching shoot
			iSequence = LookupSequence( "crouching_mp5" );
		}

		break;
	case ACT_RANGE_ATTACK2:
		// get toss anim
		iSequence = LookupSequence( "throwgrenade" );
	
		break;
	case ACT_RUN:
		if ( pev->health <= MIL_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			// buz: get combat movement animation in combat state
		//	if (m_MonsterState == MONSTERSTATE_COMBAT || m_MonsterState == MONSTERSTATE_ALERT)
			if (m_iUseAlertAnims)
			{
				iSequence = LookupSequence("combat_run_primary");
				if (iSequence != -1)
					break;
			}

			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= MIL_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			// buz: get combat movement animation in combat state
		//	if (m_MonsterState == MONSTERSTATE_COMBAT || m_MonsterState == MONSTERSTATE_ALERT)
			if (m_iUseAlertAnims)
			{
				iSequence = LookupSequence("combat_walk_primary");
				if (iSequence != -1)
					break;
			}

			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		RecalculateYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_debug, "%s has no sequence for act:%s\n", STRING(pev->classname), GetNameForActivity( NewActivity ));
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CMilitary :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = MIL_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_MIL_REPEL_LAND );
		}
		else
		{
			// repel down a rope, 
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_MIL_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_MIL_REPEL );
		}
	}

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				
				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
			//	if (FOkToSpeak())
			//	{
					SENTENCEG_PlayRndSz( ENT(pev), "VV_GREN", MIL_VOL, MIL_ATTN, 0, m_voicePitch);
					JustSpoke();
			//	}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				SetIdealYawToTargetAndUpdate( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if ( pFlashlight && pHeadController && FBitSet( pev->spawnflags, SF_TOGGLE_FLASHLIGHT ))
		{
			if (( pHeadController->ShouldUseLights() && pFlashlight->GetState() != STATE_ON ) || ( !pHeadController->ShouldUseLights() && pFlashlight->GetState() != STATE_OFF ))
			{
				return GetScheduleOfType(SCHED_MIL_FLASHLIGHT);
			}
		}
		// buz: перезарядиться, если врага нет и магазин полупуст
		if (m_cAmmoLoaded < m_cClipSize / 2)
		{
			return GetScheduleOfType ( SCHED_RELOAD );
		}

		if (!FStringNull(m_hRushEntity) && (gpGlobals->time > m_flRushNextTime) && (m_flRushNextTime != -1))
		{
			CBaseEntity *pRushEntity = UTIL_FindEntityByTargetname( NULL, STRING( m_hRushEntity ) );
			if (pRushEntity)
			{
				CStartRush* pRush = (CStartRush*)pRushEntity;
				m_hTargetEnt = pRush->GetDestinationEntity();

				return GetScheduleOfType( SCHED_RUSH_TARGET );
			}
			else
				// rush entity not found on this map.
				//   try again after next changelevel
				m_flRushNextTime = -1;
		}

		// Behavior for following the player
		if ( IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}

		// If I'm already close enough to my target
			if ( TargetDistance() <= 128 )
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
			}

			return GetScheduleOfType( SCHED_TARGET_FACE );	// Just face and follow.
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
			return GetScheduleOfType( SCHED_MOVE_AWAY );

		// try to say something about smells
		TrySmellTalk();
		break;

	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			// Wargon: Кроме случаев, когда враг взят из данных игрока. (1.1)
			if ( HasConditions( bits_COND_ENEMY_DEAD ) && !m_fEnemyFromPlayer )
			{
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf,STRING(m_iszSpeakAs));
					strcat(szBuf,"_KILL");
					PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
				}
				else
				{
					PlaySentence( "VV_KILL", 4, VOL_NORM, ATTN_NORM );
				}

				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{			
			//	if (FOkToSpeak())// && RANDOM_LONG(0,1))
			//	{
					if (m_hEnemy != NULL) // buz: rewritten
					{
						// american spy
						if (m_hEnemy->Classify() == CLASS_HUMAN_MILITARY)
							SENTENCEG_PlayRndSz( ENT(pev), "VV_AMER", MIL_VOL, MIL_ATTN, 0, m_voicePitch);

						// monster
						else if ((m_hEnemy->Classify() == CLASS_ALIEN_MILITARY) || 
								(m_hEnemy->Classify() == CLASS_ALIEN_MONSTER) || 
								(m_hEnemy->Classify() == CLASS_ALIEN_PREY) || 
								(m_hEnemy->Classify() == CLASS_ALIEN_PREDATOR))
							SENTENCEG_PlayRndSz( ENT(pev), "VV_MONST", MIL_VOL, MIL_ATTN, 0, m_voicePitch);
					}
					JustSpoke();
			//	}
				
				if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_MIL_SUPPRESS );
				}
				else
				{
					return GetScheduleOfType ( SCHED_MIL_ESTABLISH_LINE_OF_FIRE );
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) ) // buz: reload here, if safe
					return GetScheduleOfType ( SCHED_RELOAD );
				else
					return GetScheduleOfType ( SCHED_MIL_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
			/*	// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = MIL_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}*/

				int iPercent;

				// buz: 90% to duck and cover, if can
				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) && m_hEnemy != NULL )
				{
					iPercent = RANDOM_LONG(0,99);
					if (iPercent <= 90)
						return GetScheduleOfType( SCHED_MIL_DUCK_COVER_WAIT ); // wait some time in cover
				}

				// buz: now 50% to try normal way of taking cover
				iPercent = RANDOM_LONG(0,99);
				if ( iPercent <= 50 && m_hEnemy != NULL )
				{
					//!!!KELLY - this grunt was hit and is going to run to cover.
				//	if (FOkToSpeak()) // && RANDOM_LONG(0,1))
				//	{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = MIL_SENT_COVER;
						//JustSpoke();
				//	}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				if( pHeadController && pHeadController->GetBackTrace() < 32.0f )
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
				else
					return GetScheduleOfType ( SCHED_MIL_WALKBACK_FIRE ); 
			}
// can grenade launch

			else if (( pev->weapons == MIL_GRENADES ) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ))
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
				//	if (FOkToSpeak())
				//	{
						SENTENCEG_PlayRndSz( ENT(pev), "VV_THROW", MIL_VOL, MIL_ATTN, 0, m_voicePitch);
						JustSpoke();
				//	}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				
				//!!!KELLY - grunt cannot see the enemy and has just decided to 
				// charge the enemy's position. 
			//	if (FOkToSpeak())// && RANDOM_LONG(0,1))
			//	{
					//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					m_iSentence = MIL_SENT_CHARGE;
					//JustSpoke();
			//	}

				return GetScheduleOfType( SCHED_MIL_ESTABLISH_LINE_OF_FIRE );
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MIL_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CTalkMonster :: GetSchedule();
}


extern Schedule_t	slIdleStand[];
//=========================================================
//=========================================================
Schedule_t* CMilitary :: GetScheduleOfType ( int Type ) 
{
	Schedule_t *psched;
	
	switch	( Type )
	{
	case SCHED_TARGET_CHASE:
		return slMilFollow;

	case SCHED_TARGET_FACE:
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slMilFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_MIL_FLASHLIGHT:
		{
			return &slMilToggleFlashlight[ 0 ];
		}
		break;

	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
		//	if ( RANDOM_LONG(0,1) ) - buz - this is bad trick for player allied monsters..
		//	{
		//		return &slMilGrenadeCover[ 0 ];
		//	}
		//	else
		//	{
		// Wargon: Теперь союзники не ищут укрытия от врагов. (1.1)
		//		return &slMilTakeCover[ 0 ];
		//	}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slMilTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_MIL_DUCK_COVER_WAIT: // buz
		{
			return &slMilDuckAndCoverWait[ 0 ]; 
		}
	case SCHED_MIL_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_MIL_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_MIL_ESTABLISH_LINE_OF_FIRE:
		{
			return &slMilEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
		//	if (RANDOM_LONG(0,9) == 0)
		//		m_fStanding = RANDOM_LONG(0,1);

		/*	m_fStanding = RANDOM_LONG(0,1); // buz

			if (m_fStanding)
				return &slMilRangeAttack1B[ 0 ];
			else
				return &slMilRangeAttack1A[ 0 ];*/


			// buz: use CheckRangedAttack1's recommendations
			switch (m_iLastFireCheckResult)
			{
			case 0: // fire any
			default:
				if (RANDOM_LONG(0,5) == 0)
					m_fStanding = RANDOM_LONG(0,1);
				break;
			case 1: // only crouched
			//	ALERT(at_console, "MIL SET CROUCHING\n");
				m_fStanding = FALSE;
				break;
			case 2: // only standing
			//	ALERT(at_console, "MIL SET STANDING\n");
				m_fStanding = TRUE;
				break;
			}

			// buz: 1B is interruptable - use it if grunt can fast take cover when hit
			if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) ) 
				return &slMilRangeAttack1B[ 0 ];
			else
				return &slMilRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slMilRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slMilCombatFace[ 0 ];
		}
	case SCHED_MIL_WAIT_FACE_ENEMY:
		{
			return &slMilWaitInCover[ 0 ];
		}
	case SCHED_MIL_WALKBACK_FIRE:
		{
			return &slMilWalkBackFire[ 0 ];
		}
	case SCHED_MIL_SWEEP:
		{
			return &slMilSweep[ 0 ];
		}
	case SCHED_MIL_COVER_AND_RELOAD:
		{
			return &slMilHideReload[ 0 ];
		}
	case SCHED_MIL_FOUND_ENEMY:
		{
			return &slMilFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			// buz
			if (RANDOM_LONG(0,4) == 0)
				return &slMilVictoryDance[ 0 ];
			else
				return &slMilFail[ 0 ];
		}
	case SCHED_MIL_SUPPRESS:
		{
			// buz: use CheckRangedAttack1's recommendations
			switch (m_iLastFireCheckResult)
			{
			case 0: // fire any
			default:
				if (RANDOM_LONG(0,5) == 0)
					m_fStanding = RANDOM_LONG(0,1);
				break;
			case 1: // only crouched
//				ALERT(at_console, "MIL SET CROUCHING\n");
				m_fStanding = FALSE;
				break;
			case 2: // only standing
//				ALERT(at_console, "MIL SET STANDING\n");
				m_fStanding = TRUE;
				break;
			}

			return &slMilSuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slMilCombatFail[ 0 ];
			}

			return &slMilFail[ 0 ];
		}
	case SCHED_MIL_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slMilRepel[ 0 ];
		}
	case SCHED_MIL_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slMilRepelAttack[ 0 ];
		}
	case SCHED_MIL_REPEL_LAND:
		{
			return &slMilRepelLand[ 0 ];
		}
	default:
		{
			return CTalkMonster :: GetScheduleOfType ( Type );
		}
	}
}


//=========================================================
// CHGruntRepel - when triggered, spawns a monster_human_grunt
// repelling down a line.
//=========================================================

class CMilitaryRepel : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;	// Don't save, precache
};

LINK_ENTITY_TO_CLASS( monster_military_repel, CMilitaryRepel );

void CMilitaryRepel::Spawn( void )
{
	Precache( );
	pev->solid = SOLID_NOT;

	SetUse(&CMilitaryRepel:: RepelUse );
}

void CMilitaryRepel::Precache( void )
{
	UTIL_PrecacheOther( "monster_human_military" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CMilitaryRepel::RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP) 
		return NULL;
	*/

	CBaseEntity *pEntity = Create( "monster_human_military", pev->origin, pev->angles );
	CBaseMonster *pGrunt = pEntity->MyMonsterPointer( );
	pGrunt->pev->movetype = MOVETYPE_FLY;
	pGrunt->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pGrunt->SetActivity( ACT_GLIDE );
	// UNDONE: position?
	pGrunt->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector(0,0,112), pGrunt->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink(&CBeam:: SUB_Remove );
	pBeam->SetNextThink( -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5 );

	UTIL_Remove( this );
}



//=========================================================
// DEAD HGRUNT PROP
//=========================================================
class CDeadMilitary : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );
	float MaxYawSpeed( void ) { return 8.0f; }
	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadMilitary::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadMilitary::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_military_dead, CDeadMilitary );

//=========================================================
// ********** DeadHGrunt SPAWN **********
//=========================================================
void CDeadMilitary :: Spawn( void )
{
	int oldBody;

	PRECACHE_MODEL("models/soldier.mdl");
	SET_MODEL(ENT(pev), "models/soldier.mdl");

	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_debug, "Dead hgrunt with bad pose\n" );
	}

	// Corpses have less health
	pev->health			= 20;

	oldBody = pev->body;
	pev->body = 0;

	MonsterInitDead();
}



#define SPETSNAZ_HEAD_GROUP	1
#define SPETSNAZ_GUN_GROUP	2
#define SPETSNAZ_HEAD_SHIELD	2

#define SPETSNAZ_WEAPON_AKS		0
#define SPETSNAZ_WEAPON_ASVAL	1
#define SPETSNAZ_WEAPON_GROZA	2
#define SPETSNAZ_WEAPON_EMPTY	3

#define SF_SPETSNAZ_RADIO	8

/***************************************
	Spetsnaz class
***************************************/

class CSpetsnaz : public CMilitary
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void DeathSound( void );
	void PainSound( void );
	void Shoot ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void RunAI ( void ); // only to check radio sound
	void BlockedByPlayer ( CBasePlayer *pBlocker );
	void TalkAboutDeadFriend( CTalkMonster *pfriend );

	void TalkInit(); // buz
	void SpeakSentence( void );

	void KeyValue( KeyValueData *pkvd );
	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType(int Type);
	// имеет шлем
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	// другие сентенсы
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void GibMonster( void );

	int m_iHasHeadShield; // ignores headshots
	int m_iHasGrenades;
	float m_fNextRadioNoise; // 0 - no noise
	static const char *pSpetsnazSentences[];

	// Wargon: При смерти спецназовца имитируется действие энтити player_loadsaved. (1.1)
	void RunTask( Task_t *pTask );
	void EXPORT MonsterDeadThink( void );
};

const char *CSpetsnaz::pSpetsnazSentences[] = 
{
	"AL_GREN", // grenade scared grunt
	"AL_ALERT", // sees player
	"AL_MONSTER", // sees monster
	"AL_COVER", // running to cover
	"AL_THROW", // about to throw grenade
	"AL_CHARGE",  // running out to get the enemy
	"AL_TAUNT", // say rude things
};

enum
{
	AL_SENT_NONE = -1,
	AL_SENT_GREN = 0,
	AL_SENT_ALERT,
	AL_SENT_MONSTER,
	AL_SENT_COVER,
	AL_SENT_THROW,
	AL_SENT_CHARGE,
	AL_SENT_TAUNT,
} AL_SENTENCE_TYPES;

TYPEDESCRIPTION	CSpetsnaz::m_SaveData[] = 
{
	DEFINE_FIELD( CSpetsnaz, m_iHasHeadShield, FIELD_INTEGER),
	DEFINE_FIELD( CSpetsnaz, m_iHasGrenades, FIELD_INTEGER),
};

LINK_ENTITY_TO_CLASS( monster_human_alpha, CSpetsnaz );
IMPLEMENT_SAVERESTORE(CSpetsnaz,CMilitary);


void CSpetsnaz :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iHasGrenades"))
	{
		m_iHasGrenades = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CMilitary::KeyValue( pkvd );
}

void CSpetsnaz :: BlockedByPlayer ( CBasePlayer *pBlocker )
{
	if (m_iszSpeakAs)
	{
		char szBuf[32];
		strcpy(szBuf,STRING(m_iszSpeakAs));
		strcat(szBuf,"_BLOCKED");
		PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
	}
	else
	{
		PlaySentence( "AL_BLOCKED", 4, VOL_NORM, ATTN_NORM );
	}
}

void CSpetsnaz :: TalkAboutDeadFriend( CTalkMonster *pfriend )
{
	if (FClassnameIs(pfriend->pev, "monster_human_alpha") ||
		FClassnameIs(pfriend->pev, "monster_alpha_pistol"))
	{
		if (m_iszSpeakAs)
		{
			char szBuf[32];
			strcpy(szBuf,STRING(m_iszSpeakAs));
			strcat(szBuf,"_TMDOWN");
			PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
		}
		else
		{
			PlaySentence( "AL_TMDOWN", 4, VOL_NORM, ATTN_NORM );
		}
	}
}

//=========================================================
// Spawn
//=========================================================
void CSpetsnaz :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/soldier_alpha.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	// Wargon: Хелсы спецназовцев, заданные в параметрах энтити, игнорируются. (1.1)
	// if (pev->health == 0)
	pev->health			= gSkillData.alphaHealth;
	m_flFieldOfView		= VIEW_FIELD_FULL;//0.2;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= MIL_SENT_NONE;
	m_afCapability		= bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_HackedGunPos = Vector ( 0, 0, 55 );

	// buz: pev->body is head number
	int head = pev->body;
	pev->body = 0;
	SetBodygroup( SPETSNAZ_HEAD_GROUP, head );
	m_iHasHeadShield = (head == SPETSNAZ_HEAD_SHIELD) ? 1 : 0;
	m_iNoGasDamage = 0;

	switch (pev->weapons)
	{
	case SPETSNAZ_WEAPON_AKS:
	default:
		SetBodygroup( SPETSNAZ_GUN_GROUP, SPETSNAZ_WEAPON_AKS );
	//	ALERT(at_console, "********* AKS\n");
		m_cClipSize	= 36;
		break;
	case SPETSNAZ_WEAPON_GROZA:
		SetBodygroup( SPETSNAZ_GUN_GROUP, SPETSNAZ_WEAPON_GROZA );
	//	ALERT(at_console, "********* GROZA\n");
		m_cClipSize	= 36;
		break;
	case SPETSNAZ_WEAPON_ASVAL:
		SetBodygroup( SPETSNAZ_GUN_GROUP, SPETSNAZ_WEAPON_ASVAL );
	//	ALERT(at_console, "********* ASVAL\n");
		m_cClipSize	= 24;
		break;
	}

	//MaSTeR: Flashlight and head controller
	m_cAmmoLoaded = m_cClipSize;
//	ALERT(at_console, "** spec start ammo %d\n", m_cAmmoLoaded);
	MonsterInit();
	InitFlashlight();
	InitHeadController();
	SetUse(&CSpetsnaz :: FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSpetsnaz :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/soldier_alpha.mdl");

	PRECACHE_SOUND( "weapons/dryfire1.wav" ); //LRC

	switch (pev->weapons)
	{
	case SPETSNAZ_WEAPON_AKS:
		PRECACHE_SOUND ("weapons/aks_fire1.wav");
		PRECACHE_SOUND ("weapons/aks_fire2.wav");
		PRECACHE_SOUND ("weapons/aks_fire3.wav");
		m_iBrassShell = PRECACHE_MODEL ("models/aks_shell.mdl");
		break;

	case SPETSNAZ_WEAPON_GROZA:
		PRECACHE_SOUND ("weapons/groza_fire1.wav");
		PRECACHE_SOUND ("weapons/groza_fire2.wav");
		PRECACHE_SOUND ("weapons/groza_fire3.wav");
		m_iBrassShell = PRECACHE_MODEL ("models/groza_shell.mdl");
		break;

	case SPETSNAZ_WEAPON_ASVAL:
		PRECACHE_SOUND ("weapons/val_fire1.wav");
		PRECACHE_SOUND ("weapons/val_fire2.wav");
		PRECACHE_SOUND ("weapons/val_fire3.wav");
		m_iBrassShell = PRECACHE_MODEL ("models/val_shell.mdl");
		break;
	}	
	
	PRECACHE_SOUND( "alpha/alpha_die1.wav" );
	PRECACHE_SOUND( "alpha/alpha_die2.wav" );
	PRECACHE_SOUND( "alpha/alpha_die3.wav" );

	PRECACHE_SOUND( "alpha/alpha_pain1.wav" );
	PRECACHE_SOUND( "alpha/alpha_pain2.wav" );
	PRECACHE_SOUND( "alpha/alpha_pain3.wav" );
	PRECACHE_SOUND( "alpha/alpha_pain4.wav" );
	PRECACHE_SOUND( "alpha/alpha_pain5.wav" );

	PRECACHE_SOUND( "alpha/alpha_reload.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

	if (pev->spawnflags & SF_SPETSNAZ_RADIO)
	{
		m_fNextRadioNoise = gpGlobals->time + RANDOM_FLOAT( 20, 60 );	
	}

	TalkInit();
	CTalkMonster::Precache();
}	


// talk init
void CSpetsnaz :: TalkInit()
{
	CTalkMonster::TalkInit();

	if (!m_iszSpeakAs)
	{
		m_szGrp[TLK_ANSWER]		=	"AL_ANSWER";
		m_szGrp[TLK_QUESTION]	=	"AL_QUESTION";
		m_szGrp[TLK_IDLE]		=	"AL_IDLE";
		m_szGrp[TLK_STARE]		=	"AL_STARE";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER) //LRC
			m_szGrp[TLK_USE]	=	"AL_PFOLLOW";
		else
			m_szGrp[TLK_USE] =	"AL_OK";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_UNUSE] = "AL_PWAIT";
		else
			m_szGrp[TLK_UNUSE] = "AL_WAIT";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_DECLINE] =	"AL_POK";
		else
			m_szGrp[TLK_DECLINE] =	"AL_NOTOK";
		m_szGrp[TLK_STOP] =		"AL_STOP";

		m_szGrp[TLK_NOSHOOT] =	"AL_SCARED";
		m_szGrp[TLK_HELLO] =	"AL_HELLO";

		m_szGrp[TLK_PLHURT1] =	"!AL_CUREA";
		m_szGrp[TLK_PLHURT2] =	"!AL_CUREB"; 
		m_szGrp[TLK_PLHURT3] =	"!AL_CUREC";

		m_szGrp[TLK_PHELLO] =	NULL;
		m_szGrp[TLK_PIDLE] =	NULL;
		m_szGrp[TLK_PQUESTION] = "AL_PQUEST";

		m_szGrp[TLK_SMELL] =	"AL_SMELL";
	
		m_szGrp[TLK_WOUND] =	"AL_WOUND";
		m_szGrp[TLK_MORTAL] =	"AL_MORTAL";
	}
}



//=========================================================
// PainSound
//=========================================================
void CSpetsnaz :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "alpha/alpha_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "alpha/alpha_pain4.wav", 1, ATTN_NORM );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "alpha/alpha_pain5.wav", 1, ATTN_NORM );	
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "alpha/alpha_pain1.wav", 1, ATTN_NORM );	
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "alpha/alpha_pain2.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CSpetsnaz :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "alpha/alpha_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "alpha/alpha_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "alpha/alpha_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// Shoot
//=========================================================
void CSpetsnaz :: Shoot ( void )
{
//	if (m_hEnemy == NULL && m_pCine == NULL) //LRC - scripts may fire when you have no enemy
//	{
//		return;
//	}

	UTIL_MakeVectors ( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if (m_cAmmoLoaded > 0)
	{
		switch (pev->weapons)
		{
		case SPETSNAZ_WEAPON_AKS:
			switch(RANDOM_LONG(0,2))
			{
			case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/aks_fire1.wav", 1, ATTN_NORM ); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/aks_fire2.wav", 1, ATTN_NORM ); break;
			case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/aks_fire3.wav", 1, ATTN_NORM ); break;
			}
			break;

		case SPETSNAZ_WEAPON_GROZA:
			switch(RANDOM_LONG(0,2))
			{
			case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/groza_fire1.wav", 1, ATTN_NORM ); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/groza_fire2.wav", 1, ATTN_NORM ); break;
			case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/groza_fire3.wav", 1, ATTN_NORM ); break;
			}
			break;

		case SPETSNAZ_WEAPON_ASVAL:
			switch(RANDOM_LONG(0,2))
			{
			case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/val_fire1.wav", 1, ATTN_NORM ); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/val_fire2.wav", 1, ATTN_NORM ); break;
			case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/val_fire3.wav", 1, ATTN_NORM ); break;
			}
			break;
		}

		Vector vecBrassPos, vecBrassDir;
		GetAttachment(3, vecBrassPos, vecBrassDir);
		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( vecBrassPos, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 

		switch (pev->weapons)
		{
		case SPETSNAZ_WEAPON_AKS:
			FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_7DEGREES, 2048.0f, BULLET_NORMAL, gSkillData.monDmgAK );
			break;
		case SPETSNAZ_WEAPON_GROZA:
			FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048.0f, BULLET_NORMAL, gSkillData.monDmgGroza );
			break;
		case SPETSNAZ_WEAPON_ASVAL:
			FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048.0f, BULLET_NORMAL, gSkillData.monDmgAsval );
			break;
		default:
			ALERT(at_error, "ERROR: trying to fire without a gun!\n");
		}		

		pev->effects |= EF_MUZZLEFLASH;
	
		m_cAmmoLoaded--;// take away a bullet!
	//	ALERT(at_console, "spcnazz ammo has %d\n", m_cAmmoLoaded);
	}
	else
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/dryfire1.wav", 1, ATTN_NORM );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}



//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CSpetsnaz :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
//	ALERT(at_console, "specnaz hitgr: %d\n", ptr->iHitgroup);
	// check for helmet shot
	if ((ptr->iHitgroup == 8) || (ptr->iHitgroup == 1))
	{
		// make sure we're wearing one
		if (m_iHasHeadShield && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		{
			// absorb damage
			flDamage -= 40;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CSpetsnaz :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	// Wargon: Союзники не должны восставать против игрока. И исключена возможность гибания спецназовцев - этого требует автозагрузка при смерти. (1.1)
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType | DMG_NEVERGIB);

/*	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	// LRC - if my reaction to the player has been overridden, don't do this stuff
	if (m_iPlayerReact) return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) )
			{
				// Alright, now I'm pissed!
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf,STRING(m_iszSpeakAs));
					strcat(szBuf,"_MAD");
					PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
				}
				else
				{
					PlaySentence( "AL_MAD", 4, VOL_NORM, ATTN_NORM );
				}

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf,STRING(m_iszSpeakAs));
					strcat(szBuf,"_SHOT");
					PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
				}
				else
				{
					PlaySentence( "AL_SHOT", 4, VOL_NORM, ATTN_NORM );
				}
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			if (m_iszSpeakAs)
			{
				char szBuf[32];
				strcpy(szBuf,STRING(m_iszSpeakAs));
				strcat(szBuf,"_SHOT");
				PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
			}
			else
			{
				PlaySentence( "AL_SHOT", 4, VOL_NORM, ATTN_NORM );
			}
		}
	}
	return ret; */
}



//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CSpetsnaz :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (!m_iHasGrenades)
		return FALSE;
	
	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && (m_hEnemy->pev->waterlevel == 0 || m_hEnemy->pev->watertype==CONTENTS_FOG) && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

		// find feet
		if (RANDOM_LONG(0,1))
		{
			// magically know where they are
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}

	// buz: check for allies in target area:
	CBaseEntity *pTarget = NULL;
	while ((pTarget = UTIL_FindEntityInSphere( pTarget, vecTarget, 256 )) != NULL)
	{
		if (FClassnameIs( pTarget->pev, "monster_human_alpha") || FClassnameIs( pTarget->pev, "player"))
		{
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}
	
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}

	return m_fThrowGrenade;
}



void CSpetsnaz :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if (GetBodygroup(SPETSNAZ_GUN_GROUP) != SPETSNAZ_WEAPON_EMPTY && !(pev->spawnflags & SF_MONSTER_NO_WPN_DROP))
	{
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun = NULL;
		switch (pev->weapons)
		{
		case SPETSNAZ_WEAPON_AKS:
			pGun = DropItem( "weapon_aks", vecGunPos, vecGunAngles );
			break;
		case SPETSNAZ_WEAPON_GROZA:
			pGun = DropItem( "weapon_groza", vecGunPos, vecGunAngles );
			break;
		case SPETSNAZ_WEAPON_ASVAL:
			pGun = DropItem( "weapon_val", vecGunPos, vecGunAngles );
			break;
		}

		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		
		if (m_iHasGrenades)
		{
			pGun = DropItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}
	CBaseMonster :: GibMonster();
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSpetsnaz :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case MIL_AE_DROP_GUN:
			{
				if (pev->spawnflags & SF_MONSTER_NO_WPN_DROP) break; //LRC

				Vector	vecGunPos;
				Vector	vecGunAngles;
				GetAttachment( 0, vecGunPos, vecGunAngles );
				// switch to body group with no gun.
				SetBodygroup( SPETSNAZ_GUN_GROUP, SPETSNAZ_WEAPON_EMPTY );

				switch (pev->weapons)
				{
				case SPETSNAZ_WEAPON_AKS:
					DropItem( "weapon_aks", vecGunPos, vecGunAngles );
					break;
				case SPETSNAZ_WEAPON_GROZA:
					DropItem( "weapon_groza", vecGunPos, vecGunAngles );
					break;
				case SPETSNAZ_WEAPON_ASVAL:
					DropItem( "weapon_val", vecGunPos, vecGunAngles );
					break;
				}
				
				if (m_iHasGrenades)
					DropItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles );

				break;
			}

		case MIL_AE_RELOAD:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "alpha/alpha_reload.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			pev->health = gSkillData.alphaHealth; // Wargon: Пополнение хелсов при перезарядке. (1.1)
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case MIL_AE_BURST1:
		{
			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			// buz: sound moved to Shoot
		//	ALERT(at_console, "*-------- spec burst 1\n");
			Shoot();
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case MIL_AE_CAUGHT_ENEMY:
		{
		//	if ( FOkToSpeak() )
		//	{
				SENTENCEG_PlayRndSz(ENT(pev), "AL_ALERT", MIL_VOL, MIL_ATTN, 0, m_voicePitch);
				 JustSpoke();
		//	}
		}

		default:
			CMilitary::HandleAnimEvent( pEvent );
			break;
	}
}




//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CSpetsnaz :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = MIL_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_MIL_REPEL_LAND );
		}
		else
		{
			// repel down a rope, 
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_MIL_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_MIL_REPEL );
		}
	}

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				
				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
			//	if (FOkToSpeak())
			//	{
					SENTENCEG_PlayRndSz( ENT(pev), "AL_GREN", MIL_VOL, MIL_ATTN, 0, m_voicePitch);
					JustSpoke();
			//	}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				SetIdealYawToTargetAndUpdate( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if ( pFlashlight && pHeadController && FBitSet( pev->spawnflags, SF_TOGGLE_FLASHLIGHT ))
		{
			if (( pHeadController->ShouldUseLights() && pFlashlight->GetState() != STATE_ON ) || ( !pHeadController->ShouldUseLights() && pFlashlight->GetState() != STATE_OFF ))
			{
				return GetScheduleOfType(SCHED_MIL_FLASHLIGHT);
			}
		}
		// buz: перезарядиться, если врага нет и магазин полупуст
		if (m_cAmmoLoaded < m_cClipSize / 2)
		{
			return GetScheduleOfType ( SCHED_RELOAD );
		}

		if (!FStringNull(m_hRushEntity) && (gpGlobals->time > m_flRushNextTime) && (m_flRushNextTime != -1))
		{
			CBaseEntity *pRushEntity = UTIL_FindEntityByTargetname( NULL, STRING( m_hRushEntity ) );
			if (pRushEntity)
			{
				CStartRush* pRush = (CStartRush*)pRushEntity;
				m_hTargetEnt = pRush->GetDestinationEntity();

				return GetScheduleOfType( SCHED_RUSH_TARGET );
			}
			else
				// rush entity not found on this map.
				//   try again after next changelevel
				m_flRushNextTime = -1;
		}

		// Behavior for following the player
		if ( IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}

		// If I'm already close enough to my target
			if ( TargetDistance() <= 128 )
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
			}

			return GetScheduleOfType( SCHED_TARGET_FACE );	// Just face and follow.
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
			return GetScheduleOfType( SCHED_MOVE_AWAY );

		// try to say something about smells
		TrySmellTalk();
		break;

	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			// Wargon: Кроме случаев, когда враг взят из данных игрока. (1.1)
			if ( HasConditions( bits_COND_ENEMY_DEAD ) && !m_fEnemyFromPlayer )
			{
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf,STRING(m_iszSpeakAs));
					strcat(szBuf,"_KILL");
					PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
				}
				else
				{
					PlaySentence( "AL_KILL", 4, VOL_NORM, ATTN_NORM );
				}

				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{			
			//	if (FOkToSpeak())// && RANDOM_LONG(0,1))
			//	{
					if (m_hEnemy != NULL) // buz: rewritten
					{
						// american spy
						if (m_hEnemy->Classify() == CLASS_HUMAN_MILITARY)
							SENTENCEG_PlayRndSz( ENT(pev), "AL_AMER", MIL_VOL, MIL_ATTN, 0, m_voicePitch);

						// monster
						else if ((m_hEnemy->Classify() == CLASS_ALIEN_MILITARY) || 
								(m_hEnemy->Classify() == CLASS_ALIEN_MONSTER) || 
								(m_hEnemy->Classify() == CLASS_ALIEN_PREY) || 
								(m_hEnemy->Classify() == CLASS_ALIEN_PREDATOR))
							SENTENCEG_PlayRndSz( ENT(pev), "AL_MONST", MIL_VOL, MIL_ATTN, 0, m_voicePitch);
					}
					JustSpoke();
			//	}
				
				if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_MIL_SUPPRESS );
				}
				else
				{
					return GetScheduleOfType ( SCHED_MIL_ESTABLISH_LINE_OF_FIRE );
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				// Wargon: Спецназовец часто бегает в укрытие через всю карту. Нам это не нужно. (1.1)
//				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) ) // buz: reload here, if safe
					return GetScheduleOfType ( SCHED_RELOAD );
//				else
//					return GetScheduleOfType ( SCHED_MIL_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
			/*	// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this grunt was hit and is going to run to cover.
				//	if (FOkToSpeak()) // && RANDOM_LONG(0,1))
				//	{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = AL_SENT_COVER;
						//JustSpoke();
				//	}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}*/

				int iPercent;

				// buz: 90% to duck and cover, if can
				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) && m_hEnemy != NULL )
				{
					iPercent = RANDOM_LONG(0,99);
					if (iPercent <= 90)
						return GetScheduleOfType( SCHED_MIL_DUCK_COVER_WAIT ); // wait some time in cover
				}

				// buz: now 50% to try normal way of taking cover
				iPercent = RANDOM_LONG(0,99);
				if ( iPercent <= 50 && m_hEnemy != NULL )
				{
					//!!!KELLY - this grunt was hit and is going to run to cover.
				//	if (FOkToSpeak()) // && RANDOM_LONG(0,1))
				//	{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = MIL_SENT_COVER;
						//JustSpoke();
				//	}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				if( pHeadController && pHeadController->GetBackTrace() < 32.0f )
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
				else
				{
					//ALERT(at_console, "BACK! \n");
					return GetScheduleOfType ( SCHED_MIL_WALKBACK_FIRE );
				}
			}
// can grenade launch

			else if ((m_iHasGrenades) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ))
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
				//	if (FOkToSpeak())
				//	{
						SENTENCEG_PlayRndSz( ENT(pev), "AL_THROW", MIL_VOL, MIL_ATTN, 0, m_voicePitch);
						JustSpoke();
				//	}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				
				//!!!KELLY - grunt cannot see the enemy and has just decided to 
				// charge the enemy's position. 
			//	if (FOkToSpeak())// && RANDOM_LONG(0,1))
			//	{
					//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					m_iSentence = AL_SENT_CHARGE;
					//JustSpoke();
			//	}

				return GetScheduleOfType( SCHED_MIL_ESTABLISH_LINE_OF_FIRE );
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MIL_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CTalkMonster :: GetSchedule();
}

Schedule_t *CSpetsnaz :: GetScheduleOfType(int Type)
{

	return CMilitary::GetScheduleOfType(Type);

}


void CSpetsnaz :: SpeakSentence( void )
{
	if ( m_iSentence == AL_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

//	if (FOkToSpeak())
//	{
		SENTENCEG_PlayRndSz( ENT(pev), pSpetsnazSentences[ m_iSentence ], MIL_VOL, MIL_ATTN, 0, m_voicePitch);
		JustSpoke();
//	}
}


BOOL CSpetsnaz :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (pev->weapons == SPETSNAZ_WEAPON_EMPTY)
		return false;

	return CMilitary :: CheckRangeAttack1 ( flDot, flDist );
}


void CSpetsnaz :: RunAI ( void )
{
//	ALERT(at_console, "*** time %f\n", gpGlobals->time);
	if (m_fNextRadioNoise && (m_fNextRadioNoise < gpGlobals->time))
	{
		m_fNextRadioNoise = gpGlobals->time + RANDOM_FLOAT( 20, 60 );
		SENTENCEG_PlayRndSz( ENT(pev), "AL_RADIONOISE", 0.2, ATTN_STATIC, 0, m_voicePitch, CHAN_BODY);
	}
	CMilitary::RunAI();
}

// Wargon: Дефолтный TASK_DIE оверрайден для имитации действия энтити player_loadsaved. (1.1)
void CSpetsnaz :: RunTask ( Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_DIE:
		{
			if (m_fSequenceFinished && pev->frame >= 255)
			{
				pev->deadflag = DEAD_DEAD;
				StopAnimation();
				if (!BBoxFlat())
					UTIL_SetSize(pev, Vector(-4, -4, 0), Vector(4, 4, 1));
				else
					UTIL_SetSize(pev, Vector(pev->mins.x, pev->mins.y, pev->mins.z), Vector(pev->maxs.x, pev->maxs.y, pev->mins.z + 1));
				if (ShouldFadeOnDeath())
					SUB_StartFadeOut();
				else
					CSoundEnt::InsertSound(bits_SOUND_CARCASS, pev->origin, 384, 30);
				if (!gpGlobals->deathmatch)
				{
					CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(1));
					if (pPlayer)
					{
						ClearBits( pPlayer->m_iHideHUD, ITEM_SUIT );
						if (pPlayer->m_iGasMaskOn)
							pPlayer->ToggleGasMask();
						ClearBits( pPlayer->m_iHideHUD, ITEM_GASMASK );
						if (pPlayer->m_iHeadShieldOn)
							pPlayer->ToggleHeadShield();
						ClearBits( pPlayer->m_iHideHUD, ITEM_HEADSHIELD );
					}
					UTIL_ScreenFadeAll(Vector(0, 0, 0), 1, 6, 255, 0x0001); // Wargon: FFADE_OUT. (1.1)
					UTIL_ShowMessageAll("#ALPHA_DIED");
					SetNextThink(5);
					SetThink(&CSpetsnaz::MonsterDeadThink);
				}
				else
					SetThink(NULL);
			}
			break;
		}
	default:
		{
			CMilitary::RunTask(pTask);
			break;
		}
	}
}

// Wargon: Автозагрузка срабатывает с задержкой после смерти спецназовца. (1.1)
void CSpetsnaz :: MonsterDeadThink ( void )
{
	SERVER_COMMAND("reload\n");
}


/***************************************
	Spetsnaz with aps class
***************************************/

#define SPETSNAZ2_HEAD_GROUP	1
#define SPETSNAZ2_HEAD_SHIELD	1

#define SPETSNAZ2_GUN_GROUP		2
#define SPETSNAZ2_WEAPON_APS	0
#define SPETSNAZ2_WEAPON_EMPTY	1

class CSpetsnazAPS : public CSpetsnaz
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void Shoot ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void SetActivity ( Activity NewActivity );
	void GibMonster( void );
};

LINK_ENTITY_TO_CLASS( monster_alpha_pistol, CSpetsnazAPS );

void CSpetsnazAPS :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/soldier_alpha_pistol.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	// Wargon: Хелсы спецназовцев, заданные в параметрах энтити, игнорируются. (1.1)
	// if (pev->health == 0)
	pev->health			= gSkillData.alphaHealth;
	m_flFieldOfView		= VIEW_FIELD_FULL; //0.2;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= MIL_SENT_NONE;
	m_afCapability		= bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_cClipSize	= 12;
	m_cAmmoLoaded = m_cClipSize;

	// buz: pev->body is head number
	int head = pev->body;
	pev->body = 0;
	SetBodygroup( SPETSNAZ2_HEAD_GROUP, head );
	m_iHasHeadShield = (head == SPETSNAZ2_HEAD_SHIELD) ? 1 : 0;
	m_iNoGasDamage = 0;

	MonsterInit();
	SetUse(&CSpetsnazAPS :: FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSpetsnazAPS :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/soldier_alpha_pistol.mdl");

	PRECACHE_SOUND( "weapons/dryfire1.wav" ); //LRC

	PRECACHE_SOUND ("weapons/aps_fire.wav");
	
	PRECACHE_SOUND( "alpha/alpha_die1.wav" );
	PRECACHE_SOUND( "alpha/alpha_die2.wav" );
	PRECACHE_SOUND( "alpha/alpha_die3.wav" );

	PRECACHE_SOUND( "alpha/alpha_pain1.wav" );
	PRECACHE_SOUND( "alpha/alpha_pain2.wav" );
	PRECACHE_SOUND( "alpha/alpha_pain3.wav" );
	PRECACHE_SOUND( "alpha/alpha_pain4.wav" );
	PRECACHE_SOUND( "alpha/alpha_pain5.wav" );

	PRECACHE_SOUND( "alpha/alpha_reload_aps.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_iBrassShell = PRECACHE_MODEL ("models/aps_shell.mdl");

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

	if (pev->spawnflags & SF_SPETSNAZ_RADIO)
	{
		m_fNextRadioNoise = gpGlobals->time + RANDOM_FLOAT( 20, 60 );	
	}

	TalkInit();
	CTalkMonster::Precache();
}

BOOL CSpetsnazAPS :: CheckRangeAttack1 ( float flDot, float flDist )
{
	return CMilitary :: CheckRangeAttack1 ( flDot, flDist );
}



void CSpetsnazAPS :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if (GetBodygroup(SPETSNAZ2_GUN_GROUP) != SPETSNAZ2_WEAPON_EMPTY && !(pev->spawnflags & SF_MONSTER_NO_WPN_DROP))
	{
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun = DropItem( "weapon_aps", vecGunPos, vecGunAngles );
		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		
		if (m_iHasGrenades)
		{
			pGun = DropItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}

	CBaseMonster :: GibMonster();
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSpetsnazAPS :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case MIL_AE_DROP_GUN:
			{
				if (pev->spawnflags & SF_MONSTER_NO_WPN_DROP) break; //LRC

				Vector	vecGunPos;
				Vector	vecGunAngles;
				GetAttachment( 0, vecGunPos, vecGunAngles );
				// switch to body group with no gun.
				SetBodygroup( SPETSNAZ2_GUN_GROUP, SPETSNAZ2_WEAPON_EMPTY );
				DropItem( "weapon_aps", vecGunPos, vecGunAngles );
							
				if (m_iHasGrenades)
					DropItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles );

				break;
			}

		case MIL_AE_RELOAD:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "alpha/alpha_reload_aps.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		default:
			CSpetsnaz::HandleAnimEvent( pEvent );
			break;
	}
}


//=========================================================
// Shoot
//=========================================================
void CSpetsnazAPS :: Shoot ( void )
{
//	if (m_hEnemy == NULL && m_pCine == NULL) //LRC - scripts may fire when you have no enemy
//	{
//		return;
//	}

	UTIL_MakeVectors ( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if (m_cAmmoLoaded > 0)
	{
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/aps_fire.wav", 1, ATTN_NORM );			

		Vector vecBrassPos, vecBrassDir;
		GetAttachment(3, vecBrassPos, vecBrassDir);
		Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass( vecBrassPos, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 
		FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048.0f, BULLET_NORMAL, gSkillData.monDmg9MM );
	
		pev->effects |= EF_MUZZLEFLASH;
	
		m_cAmmoLoaded--;// take away a bullet!
	}
	else
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/dryfire1.wav", 1, ATTN_NORM );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}


void CSpetsnazAPS :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		//iSequence = LookupActivity ( NewActivity );
		if ( m_fStanding )
		{
			// get aimable sequence
			iSequence = LookupSequence( "standing" );
		}
		else
		{
			// get crouching shoot
			iSequence = LookupSequence( "crouching" );
		}

		break;
	case ACT_RANGE_ATTACK2:
		// get toss anim
		iSequence = LookupSequence( "throwgrenade" );
	
		break;
	case ACT_RUN:
		if ( pev->health <= MIL_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= MIL_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		RecalculateYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_debug, "%s has no sequence for act:%s\n", STRING(pev->classname), GetNameForActivity( NewActivity ));
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

#define INFECTED_WEAPON_AKS		0
#define INFECTED_WEAPON_ASVAL	1
#define INFECTED_WEAPON_GROZA	2
#define INFECTED_WEAPON_EMPTY	3

/***************************************
     	Infected soldier class
****************************************/

class CSoldierInfected : public CMilitary
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void DeathSound( void );
	void PainSound( void );
	void Shoot ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	int Classify(void);

	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType(int Type);
	void GibMonster( void );

};

LINK_ENTITY_TO_CLASS( monster_soldier_infected, CSoldierInfected );

void CSoldierInfected::Spawn( )
{
	Precache( );
	CMilitary::Spawn();
	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/monsters/monster_soldiershooter.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	//pev->health			= gSkillData.infSoldierHealth;
	m_flFieldOfView		= VIEW_FIELD_FULL; //0.2;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextPainTime	= gpGlobals->time;
	m_afCapability		= bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_HackedGunPos      = Vector ( 0, 0, 55 );

	m_cClipSize	= 30;
	m_cAmmoLoaded = m_cClipSize;

	// buz: pev->body is head number
	int head = pev->body;
	pev->body = 0;

	MonsterInit();
	InitFlashlight();
	InitHeadController();
}

void CSoldierInfected :: Precache( )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/monsters/monster_soldiershooter.mdl");

	PRECACHE_SOUND( "weapons/dryfire1.wav" ); //LRC

	PRECACHE_SOUND ("weapons/aps_fire.wav");
	
	PRECACHE_SOUND( "InfectedSoldier/die1.wav" );
	PRECACHE_SOUND( "InfectedSoldier/die2.wav" );
	PRECACHE_SOUND( "InfectedSoldier/die3.wav" );

	PRECACHE_SOUND( "InfectedSoldier/pain1.wav" );
	PRECACHE_SOUND( "InfectedSoldier/pain2.wav" );
	PRECACHE_SOUND( "InfectedSoldier/pain3.wav" );
	PRECACHE_SOUND( "InfectedSoldier/pain4.wav" );
	PRECACHE_SOUND( "InfectedSoldier/pain5.wav" );

	PRECACHE_SOUND( "alpha/alpha_reload_aps.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_iBrassShell = PRECACHE_MODEL ("models/aps_shell.mdl");

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

} 
void CSoldierInfected::HandleAnimEvent( MonsterEvent_t *pEvent )
{
Vector	vecShootDir;
Vector	vecShootOrigin;

	switch( pEvent->event )
	{

		case MIL_AE_DROP_GUN:
			{
				if (pev->spawnflags & SF_MONSTER_NO_WPN_DROP) break; //LRC

				Vector	vecGunPos;
				Vector	vecGunAngles;

				GetAttachment( 0, vecGunPos, vecGunAngles );

				// switch to body group with no gun.
				SetBodygroup( MIL_GUN_GROUP, MIL_GUN_NONE );

				// now spawn a gun.
				DropItem( "weapon_ak74", vecGunPos, vecGunAngles );

				if (pev->weapons == MIL_GRENADES)
				{
					DropItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles );
				}

				break;
			}
		case MIL_AE_RELOAD:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "military/mil_reload.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;


		case MIL_AE_BURST1:
		{
		//	ALERT(at_console, "*-------- mil burst 1\n");
			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if (m_cAmmoLoaded > 0)
			{
				if ( RANDOM_LONG(0,1) )
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "military/mil_mgun1.wav", 1, ATTN_NORM );
				}
				else
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "military/mil_mgun2.wav", 1, ATTN_NORM );
				}
			}
			else
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/dryfire1.wav", 1, ATTN_NORM );
			}

			Shoot();
		
		}
		break;

		case MIL_AE_BURST2:
		case MIL_AE_BURST3:
			Shoot();
			break;

		case MIL_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				// buz: move only if it is a monster!
				if (pHurt->MyMonsterPointer())
				{
					UTIL_MakeVectors( pev->angles );
					pHurt->pev->punchangle.x = 15;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				}
				pHurt->TakeDamage( pev, pev, gSkillData.MilDmgKick, DMG_CLUB );
			}
		}
		break;
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CSoldierInfected :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = MIL_SENT_NONE;

	if (m_cAmmoLoaded < m_cClipSize / 2)
	{
		return GetScheduleOfType ( SCHED_RELOAD );
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:


	case MONSTERSTATE_COMBAT:
		{
			
// no ammo
	if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				// Wargon: Спецназовец часто бегает в укрытие через всю карту. Нам это не нужно. (1.1)
//				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) ) // buz: reload here, if safe
					return GetScheduleOfType ( SCHED_RELOAD );
//				else
//					return GetScheduleOfType ( SCHED_MIL_COVER_AND_RELOAD );
			}
			

// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				return GetScheduleOfType( SCHED_MIL_ESTABLISH_LINE_OF_FIRE );
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_INFECTED_FIRINGWALK );
			}
		}
	}
	
	// no special cases here, call the base class
	return CBaseMonster :: GetSchedule();
}

Schedule_t* CSoldierInfected :: GetScheduleOfType ( int Type ) 
{
	
	switch	( Type )
	{
	case SCHED_INFECTED_FIRINGWALK:
		{
		return slInfFiringWalk;
		}
		break;
	default:
		{
			return CMilitary :: GetScheduleOfType ( Type );
		}
	}
}
//=========================================================
// PainSound
//=========================================================
void CSoldierInfected :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "InfectedSoldier/pain1.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "InfectedSoldier/pain2.wav", 1, ATTN_NORM );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "InfectedSoldier/pain3.wav", 1, ATTN_NORM );	
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "InfectedSoldier/pain4.wav", 1, ATTN_NORM );	
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "InfectedSoldier/pain5.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CSoldierInfected :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "InfectedSoldier/die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "InfectedSoldier/die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "InfectedSoldier/die3.wav", 1, ATTN_IDLE );	
		break;
	}
}
void CSoldierInfected::Shoot ( void )
{
	UTIL_MakeVectors ( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if (m_cAmmoLoaded > 0)
	{
		switch (pev->weapons)
		{
		case INFECTED_WEAPON_AKS:
			switch(RANDOM_LONG(0,2))
			{
			case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/aks_fire1.wav", 1, ATTN_NORM ); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/aks_fire2.wav", 1, ATTN_NORM ); break;
			case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/aks_fire3.wav", 1, ATTN_NORM ); break;
			}
			break;

		case INFECTED_WEAPON_GROZA:
			switch(RANDOM_LONG(0,2))
			{
			case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/groza_fire1.wav", 1, ATTN_NORM ); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/groza_fire2.wav", 1, ATTN_NORM ); break;
			case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/groza_fire3.wav", 1, ATTN_NORM ); break;
			}
			break;

		case INFECTED_WEAPON_ASVAL:
			switch(RANDOM_LONG(0,2))
			{
			case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/val_fire1.wav", 1, ATTN_NORM ); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/val_fire2.wav", 1, ATTN_NORM ); break;
			case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/val_fire3.wav", 1, ATTN_NORM ); break;
			}
			break;
		}

		Vector vecBrassPos, vecBrassDir;
		GetAttachment(3, vecBrassPos, vecBrassDir);
		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( vecBrassPos, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 

		switch (pev->weapons)
		{
		case SPETSNAZ_WEAPON_AKS:
			FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_7DEGREES, 2048, BULLET_NORMAL, gSkillData.monDmgAK );
			break;
		case SPETSNAZ_WEAPON_GROZA:
			FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048, BULLET_NORMAL, gSkillData.monDmgGroza );
			break;
		case SPETSNAZ_WEAPON_ASVAL:
			FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048, BULLET_NORMAL, gSkillData.monDmgAsval );
			break;
		default:
			ALERT(at_error, "ERROR: trying to fire without a gun!\n");
		}		

		pev->effects |= EF_MUZZLEFLASH;
	
		m_cAmmoLoaded--;// take away a bullet!
	//	ALERT(at_console, "spcnazz ammo has %d\n", m_cAmmoLoaded);
	}
	else
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/dryfire1.wav", 1, ATTN_NORM );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );

}

BOOL CSoldierInfected :: CheckRangeAttack1 ( float flDot, float flDist )
{
	return FALSE;
}

void CSoldierInfected :: GibMonster( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if (GetBodygroup(SPETSNAZ_GUN_GROUP) != SPETSNAZ_WEAPON_EMPTY && !(pev->spawnflags & SF_MONSTER_NO_WPN_DROP))
	{
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun = NULL;
		switch (pev->weapons)
		{
		case INFECTED_WEAPON_AKS:
			pGun = DropItem( "weapon_aks", vecGunPos, vecGunAngles );
			break;
		case INFECTED_WEAPON_GROZA:
			pGun = DropItem( "weapon_groza", vecGunPos, vecGunAngles );
			break;
		case INFECTED_WEAPON_ASVAL:
			pGun = DropItem( "weapon_val", vecGunPos, vecGunAngles );
			break;
		}

		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		

	}
	CBaseMonster :: GibMonster();
}

BOOL CSoldierInfected :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

int	CSoldierInfected :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_PREY;
}