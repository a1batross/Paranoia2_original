// Wargon: AI для крыс. Основан на оригинальном коде monster_cockroach.
// Тараканы и крысы переживут и нас, и наших потомков, и тебя тоже. )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "soundent.h"

#define RAT_IDLE			0
#define RAT_BORED			1
#define RAT_SCARED_BY_ENT	2
#define RAT_SCARED_BY_LIGHT	3
#define RAT_SMELL_FOOD		4
#define RAT_EAT				5

class CRat : public CBaseMonster
{
public:
	void Spawn ( void );
	void Precache ( void );
	float MaxYawSpeed( void );
	void EXPORT MonsterThink ( void );
	void Move ( float flInterval );
	void PickNewDest ( int iCondition );
	void EXPORT Touch ( CBaseEntity *pOther );
	void Killed ( entvars_t *pevAttacker, int iGib );
	float m_flLastLightLevel;
	float m_flNextSmellTime;
	int Classify ( void ) { return m_iClass?m_iClass:CLASS_INSECT; }
	void Look ( int iDistance );
	int ISoundMask ( void ) { return bits_SOUND_CARCASS | bits_SOUND_MEAT; }
	BOOL m_fLightHacked;
	int m_iMode;
};

LINK_ENTITY_TO_CLASS( monster_cockroach, CRat );

void CRat::Touch( CBaseEntity *pOther )
{
	if ( pOther->pev->velocity == g_vecZero || !pOther->IsPlayer() )
		return;
	Vector vecSpot;
	TraceResult tr;
	vecSpot = pev->origin + Vector ( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector ( 0, 0, -24 ), ignore_monsters, ENT( pev ), &tr );
	UTIL_TraceCustomDecal( &tr, "brains", RANDOM_FLOAT( 0.0f, 360.0f ));
	TakeDamage( pOther->pev, pOther->pev, pev->health, DMG_CRUSH );
}

float CRat :: MaxYawSpeed( void )
{
	return 120.0f;
}

void CRat::Spawn()
{
	Precache();
	if ( pev->model )
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
	else
		SET_MODEL( ENT( pev ), "models/krisa.mdl" );
	UTIL_SetSize( pev, Vector( -4, -4, 0 ), Vector( 4, 4, 2 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 1;
	m_flFieldOfView = 0.5;
	m_MonsterState = MONSTERSTATE_NONE;
	MonsterInit();
	SetActivity( ACT_IDLE );
	pev->takedamage = DAMAGE_YES;
	m_fLightHacked = FALSE;
	m_flLastLightLevel = -1;
	m_iMode = RAT_IDLE;
	m_flNextSmellTime = gpGlobals->time;
}

void CRat::Precache()
{
	if ( pev->model )
		PRECACHE_MODEL( (char*)STRING( pev->model ) );
	else
		PRECACHE_MODEL( "models/krisa.mdl" );
	PRECACHE_SOUND( "roach/rch_die.wav" );
	PRECACHE_SOUND( "roach/rch_walk.wav" );
	PRECACHE_SOUND( "roach/rch_smash.wav" );
}

void CRat::Killed( entvars_t *pevAttacker, int iGib )
{
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	if ( RANDOM_LONG( 0, 4 ) == 1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "roach/rch_die.wav", 0.8, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 39 ) );
	else
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "roach/rch_smash.wav", 0.7, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 39 ) );
	CSoundEnt::InsertSound( bits_SOUND_WORLD, pev->origin, 128, 1 );
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner )
		pOwner->DeathNotice( pev );
	SetActivity( ACT_DIESIMPLE );
	if ( pev->health < 0 )
	{
		switch ( RANDOM_LONG( 0, 3 ) )
		{
			case 0:
			{
				SetActivity( ACT_DIEFORWARD );
				break;
			}
			case 1:
			{
				SetActivity( ACT_DIEBACKWARD );
				break;
			}
			case 2:
			{
				SetActivity( ACT_DIE_CHESTSHOT );
				break;
			}
			case 3:
			{
				SetActivity( ACT_DIE_GUTSHOT );
				break;
			}
		}
	}
}

void CRat::MonsterThink( void )
{
	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		SetNextThink( RANDOM_FLOAT( 1, 1.5 ) );
	else
		SetNextThink( 0.1 );
	float flInterval = StudioFrameAdvance();
	if ( pev->health > 0 )
	{
		if ( !m_fLightHacked )
		{
			SetNextThink( 1 );
			m_fLightHacked = TRUE;
			return;
		}
		else if ( m_flLastLightLevel < 0 )
			m_flLastLightLevel = GETENTITYILLUM( ENT( pev ) );
		switch ( m_iMode )
		{
		case RAT_IDLE:
		case RAT_EAT:
			{
				if ( RANDOM_LONG( 0, 2 ) == 1 )
				{
					Look( 150 );
					if ( HasConditions( bits_COND_SEE_FEAR ) )
					{
						Eat( 30 + ( RANDOM_LONG( 0, 14 ) ) );
						PickNewDest( RAT_SCARED_BY_ENT );
						SetActivity( ACT_WALK );
					}
					else if ( RANDOM_LONG( 0, 128 ) == 1 )
					{
						PickNewDest( RAT_BORED );
						SetActivity( ACT_WALK );
						if ( m_iMode == RAT_EAT )
							Eat( 30 + ( RANDOM_LONG ( 0, 14 ) ) );
					}
				}
				if ( m_iMode == RAT_IDLE )
				{
					if ( FShouldEat() )
						Listen();
					if ( GETENTITYILLUM( ENT( pev ) ) > m_flLastLightLevel )
					{
						PickNewDest( RAT_SCARED_BY_LIGHT );
						SetActivity( ACT_WALK );
					}
					else if ( HasConditions( bits_COND_SMELL_FOOD ) )
					{
						CSound *pSound;
						pSound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );
						if ( pSound && abs( pSound->m_vecOrigin.z - pev->origin.z ) <= 3 )
						{
							PickNewDest( RAT_SMELL_FOOD );
							SetActivity( ACT_WALK );
						}
					}
				}
				break;
			}
		case RAT_SCARED_BY_LIGHT:
			{
				if ( GETENTITYILLUM( ENT( pev ) ) <= m_flLastLightLevel )
				{
					m_flLastLightLevel = GETENTITYILLUM( ENT( pev ) );
					SetActivity( ACT_IDLE );
				}
				break;
			}
		}
	}
	if ( m_flGroundSpeed != 0 )
		Move( flInterval );
}

void CRat::PickNewDest( int iCondition )
{
	Vector vecNewDir;
	Vector vecDest;
	float flDist;
	m_iMode = iCondition;
	if ( m_iMode == RAT_SMELL_FOOD )
	{
		CSound *pSound;
		pSound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );
		if ( pSound )
		{
			m_Route[ 0 ].vecLocation.x = pSound->m_vecOrigin.x + ( 3 - RANDOM_LONG( 0, 5 ) );
			m_Route[ 0 ].vecLocation.y = pSound->m_vecOrigin.y + ( 3 - RANDOM_LONG( 0, 5 ) );
			m_Route[ 0 ].vecLocation.z = pSound->m_vecOrigin.z;
			m_Route[ 0 ].iType = bits_MF_TO_LOCATION;
			m_movementGoal = RouteClassify( m_Route[ 0 ].iType );
			return;
		}
	}
	do
	{
		vecNewDir.x = RANDOM_FLOAT( -1, 1 );
		vecNewDir.y = RANDOM_FLOAT( -1, 1 );
		flDist = 256 + ( RANDOM_LONG( 0, 255 ) );
		vecDest = pev->origin + vecNewDir * flDist;
	} while ( ( vecDest - pev->origin ).Length2D() < 128 );
	m_Route[ 0 ].vecLocation.x = vecDest.x;
	m_Route[ 0 ].vecLocation.y = vecDest.y;
	m_Route[ 0 ].vecLocation.z = pev->origin.z;
	m_Route[ 0 ].iType = bits_MF_TO_LOCATION;
	m_movementGoal = RouteClassify( m_Route[ 0 ].iType );
	if ( RANDOM_LONG( 0, 8 ) == 1 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "roach/rch_walk.wav", 1, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 39 ) );
}

void CRat::Move( float flInterval )
{
	float flWaypointDist;
	Vector vecApex;
	flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D();
	SetIdealYawToTargetAndUpdate( m_Route[ m_iRouteIndex ].vecLocation, AI_KEEP_YAW_SPEED );
	UTIL_MakeVectors( pev->angles );
	if ( !WALK_MOVE( ENT( pev ), pev->ideal_yaw, 4, WALKMOVE_NORMAL ) )
		PickNewDest( m_iMode );
	WALK_MOVE( ENT( pev ), pev->ideal_yaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );
	if ( flWaypointDist <= m_flGroundSpeed * flInterval )
	{
		SetActivity( ACT_IDLE );
		m_flLastLightLevel = GETENTITYILLUM( ENT( pev ) );
		if ( m_iMode == RAT_SMELL_FOOD )
			m_iMode = RAT_EAT;
		else
			m_iMode = RAT_IDLE;
	}
	if ( RANDOM_LONG(0, 128) == 1 && m_iMode != RAT_SCARED_BY_LIGHT && m_iMode != RAT_SMELL_FOOD )
		PickNewDest( FALSE );
}

void CRat::Look( int iDistance )
{
	CBaseEntity *pSightEnt = NULL;
	CBaseEntity *pPreviousEnt;
	int iSighted = 0;
	ClearConditions( bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR );
	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		return;
	m_pLink = NULL;
	pPreviousEnt = this;
	while ( ( pSightEnt = UTIL_FindEntityInSphere( pSightEnt, pev->origin, iDistance ) ) != NULL )
	{
		if ( pSightEnt->IsPlayer() || FBitSet( pSightEnt->pev->flags, FL_MONSTER ) )
		{
			if ( !FBitSet( pSightEnt->pev->flags, FL_NOTARGET ) && pSightEnt->pev->health > 0 )
			{
				pPreviousEnt->m_pLink = pSightEnt;
				pSightEnt->m_pLink = NULL;
				pPreviousEnt = pSightEnt;
				switch ( IRelationship( pSightEnt ) )
				{
				case R_FR:
					iSighted |= bits_COND_SEE_FEAR;
					break;
				case R_NO:
					break;
				default:
					break;
				}
			}
		}
	}
	SetConditions( iSighted );
}
