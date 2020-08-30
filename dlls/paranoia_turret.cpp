/***************************************
*
*	Writen by BUzer for Half-Life: Paranoia modification
*
*	Static machineguns code
*
****************************************/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "weapons.h"
#include "explode.h"
#include "monsters.h"
#include "movewith.h"
#include "animation.h"

#include "player.h"

extern int gmsgSpecTank;


class CFuncMachinegun : public CBaseAnimating
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return FCAP_IMPULSE_USE; }
	BOOL	OnControls( entvars_t *pevTest );
	void	PostFrame( CBaseEntity *pActivator );
	void	UpdateClientData( CBasePlayer *client );
		
	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

//	short	m_usEvent;
	float	m_flPointHeight;
	float	m_flDistUp, m_flDistFwd;
//	float	m_flConeHor, m_flConeVer;
	float	m_flNextAttack;
	float	m_fireRate;
	float	m_flDamage;
//	Vector	savedPlayerAngles;
	int		m_iShouldUpdate;
//	float	m_flLastAnimTime;
	int		m_iAmmo;
};

LINK_ENTITY_TO_CLASS( func_machinegun, CFuncMachinegun );

TYPEDESCRIPTION	CFuncMachinegun::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncMachinegun, m_flPointHeight, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMachinegun, m_flDistUp, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMachinegun, m_flDistFwd, FIELD_FLOAT ),
//	DEFINE_FIELD( CFuncMachinegun, m_flConeHor, FIELD_FLOAT ),
//	DEFINE_FIELD( CFuncMachinegun, m_flConeVer, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMachinegun, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CFuncMachinegun, m_fireRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMachinegun, m_flDamage, FIELD_FLOAT ),
//	DEFINE_FIELD( CFuncMachinegun, savedPlayerAngles, FIELD_VECTOR ),
//	DEFINE_FIELD( CFuncMachinegun, m_flLastAnimTime, FIELD_TIME ),
	DEFINE_FIELD( CFuncMachinegun, m_iAmmo, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncMachinegun, CBaseAnimating );


void CFuncMachinegun::Precache()
{
	PRECACHE_MODEL( (char *)STRING(pev->model) );
	PRECACHE_MODEL ("models/shell.mdl");

	pev->sequence	= 0;
	pev->frame		= 0;
	pev->framerate	= 1;
	ResetSequenceInfo();
	m_fSequenceLoops = TRUE;

	m_iShouldUpdate = 1;
}


void CFuncMachinegun::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "baseheight"))
	{
		m_flPointHeight = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "distfwd"))
	{
		m_flDistFwd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "distup"))
	{
		m_flDistUp = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
/*	else if (FStrEq(pkvd->szKeyName, "conehor"))
	{
		m_flConeHor = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "conever"))
	{
		m_flConeVer = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}*/
	else if (FStrEq(pkvd->szKeyName, "firerate"))
	{
		m_fireRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damage"))
	{
		m_flDamage = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "ammo"))
	{
		m_iAmmo = atoi(pkvd->szValue);
		if (m_iAmmo > 999) m_iAmmo = 999;
		if (m_iAmmo < -1) m_iAmmo = -1;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue( pkvd );
}


void CFuncMachinegun::Spawn()
{
	Precache();

	pev->movetype	= MOVETYPE_NONE;
	pev->solid		= SOLID_BBOX;
	pev->angles.x	= 0; // remove pitch
//	m_flLastAnimTime = gpGlobals->time;
	pev->renderfx = 51; // хак для рендерера, чтобы он не интерполировал контроллеры

	SET_MODEL( ENT(pev), STRING(pev->model) );
	UTIL_SetSize( pev, Vector(-25,-25,0), Vector(25,25,60));

	ResetSequenceInfo();
	m_fSequenceLoops = TRUE;

	InitBoneControllers();
}
	

BOOL CFuncMachinegun::OnControls( entvars_t *pevTest )
{
	Vector vecToPlayer = pevTest->origin - pev->origin;
	vecToPlayer.z = 0;

	if ( vecToPlayer.Length() > 50 )
		return FALSE;

	UTIL_MakeVectors(pev->angles);
	vecToPlayer = vecToPlayer.Normalize();

	if (DotProduct(vecToPlayer, gpGlobals->v_forward) > -0.7)
		return FALSE;
	
	return TRUE;
}


void CFuncMachinegun::PostFrame( CBaseEntity *pActivator )
{
	Vector plAngles = pActivator->pev->angles;
	while (plAngles.y < 0) plAngles.y += 360;

	float yawAngle = plAngles.y - pev->angles.y;
	float pitchAngle = pActivator->pev->angles.x * -3;
	SetBoneController( 0, yawAngle );
	SetBoneController( 1, pitchAngle );

	StudioFrameAdvance(); // ^%&*(#%$%^@# !!!!

	// return to idle after fire anim
	if (m_fSequenceFinished)
	{
		pev->sequence	= 0;
		pev->frame		= 0;
		ResetSequenceInfo();
//		ALERT(at_console, "return to idle\n");
		m_fSequenceLoops = TRUE;
	}

	if ( gpGlobals->time < m_flNextAttack )
		return;

//	ALERT(at_console, "current ammo: %d\n", m_iAmmo);

	if ( pActivator->pev->button & IN_ATTACK && (m_iAmmo > 0 || m_iAmmo == -1))
	{
	// fire
		Vector vecForward, vecSrc, vecAngles;
		vecAngles = pActivator->pev->angles;
		vecAngles.x = vecAngles.x * -3; // invert anf scale pitch
		UTIL_MakeVectorsPrivate( vecAngles, vecForward, NULL, NULL );
		GetAttachment(0, vecSrc, vecAngles);

		pev->sequence = 1; // sounds, muzzleflashes, and shells will go by anim event
		pev->frame = 0;
		ResetSequenceInfo();
		m_fSequenceLoops = FALSE;

		if( !m_flDamage ) m_flDamage = gSkillData.monDmg12MM;
		FireBullets( 1, vecSrc, vecForward, Vector( 0, 0, 0 ), 4096, BULLET_NORMAL, m_flDamage, pActivator->pev );
		if (m_iAmmo > 0) m_iAmmo--;

		// update ammo counter
		MESSAGE_BEGIN( MSG_ONE, gmsgSpecTank, NULL, pActivator->pev );
			WRITE_BYTE( 2 ); // ammo update
			WRITE_LONG(m_iAmmo);
		MESSAGE_END();
		
		// HACKHACK -- make some noise (that the AI can hear)
		if ( pActivator->IsPlayer() )
			((CBasePlayer *)pActivator)->m_iWeaponVolume = LOUD_GUN_VOLUME;

		m_flNextAttack = gpGlobals->time + ( 1.0f / m_fireRate );		
	}

//	ALERT(at_console, "current sequence: %d\n", pev->sequence);
}


void CFuncMachinegun::UpdateClientData( CBasePlayer *client )
{
	if (m_iShouldUpdate)
	{
		Vector vecPoint = pev->origin + Vector(0, 0, m_flPointHeight);
		float coneHor = GetControllerBound( 0 );
		float coneVer = GetControllerBound( 1 );

		MESSAGE_BEGIN( MSG_ONE, gmsgSpecTank, NULL, client->pev );
			WRITE_BYTE( 1 ); // tank is on
			WRITE_COORD(vecPoint.x);
			WRITE_COORD(vecPoint.y);
			WRITE_COORD(vecPoint.z);
			WRITE_COORD(pev->angles.y); // write default yaw
			WRITE_COORD(coneHor); // write cone
			WRITE_COORD(coneVer); //
			WRITE_COORD(m_flDistFwd);
			WRITE_COORD(m_flDistUp);
			WRITE_LONG(m_iAmmo);
		MESSAGE_END();

		m_iShouldUpdate = 0;
	}
}

void CFuncMachinegun::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!pActivator->IsPlayer())
	{
		ALERT(at_console, "non player activating func_machinegun!\n");
		return;
	}
	
	if (useType == USE_SET && value == 2)	// called by playerthink, update position
	{
		PostFrame( pActivator );
	}
	else if (useType == USE_SET && value == 3)	// update client data
	{
		UpdateClientData( (CBasePlayer*)pActivator );
	}
	else if (useType == USE_OFF) // turn off
	{
		CBasePlayer *player = (CBasePlayer *)pActivator;
	//	player->pev->angles = savedPlayerAngles;
		MESSAGE_BEGIN( MSG_ONE, gmsgSpecTank, NULL, player->pev );
			WRITE_BYTE( 0 ); // tank is off
		MESSAGE_END();

		pev->sequence	= 0;
		pev->frame		= 0;
		ResetSequenceInfo();
		m_fSequenceLoops = TRUE;

		if ( player->m_pActiveItem )
			player->m_pActiveItem->Deploy();

		player->m_iHideHUD &= ~HIDEHUD_WEAPONS;
	}
	else // turn on
	{
		// is player in valid zone
		if ( OnControls(pActivator->pev) )
		{
			CBasePlayer *player = (CBasePlayer *)pActivator;
			player->m_pSpecTank = this;
			m_iShouldUpdate = 1;
		//	savedPlayerAngles = player->pev->angles;
		//	player->pev->angles = pev->angles;
			player->m_iHideHUD |= HIDEHUD_WEAPONS;

			if ( player->m_pActiveItem )
			{
				player->m_pActiveItem->Holster( true );
				player->pev->weaponmodel = 0;
				player->pev->viewmodel = 0; 
			}
		}
	//	else
	//		ALERT(at_console, "player is not in valid zone\n");
	}
}
