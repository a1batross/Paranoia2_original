#ifndef MONSTER_FLASHLIGHT_H
#define MONSTER_FLASHLIGHT_H

//Flashlight
class CFlashlight : public CPointEntity
{
public:
	void Spawn( entvars_t *pevOwner )
	{
		pev->classname = MAKE_STRING( "head_flashlight" );
		pev->owner = ENT( pevOwner );
		pev->solid = SOLID_NOT;
		pev->skin = ENTINDEX( ENT( pevOwner ));
		pev->body = 3;	//TESTTEST
		pev->aiment = ENT(pevOwner);
		pev->movetype = MOVETYPE_FOLLOW;
		Precache();

		pev->renderfx = 71;	// dynamic light

		SET_MODEL(ENT(pev),"sprites/null.spr"); // should be visible to send to client
		UTIL_SetOrigin( this, pevOwner->origin );

		pev->rendercolor.x = pev->rendercolor.y = pev->rendercolor.z = 255;
		pev->renderamt = 90.0f; // (already divided by 8);
		pev->rendermode = 0; // number of light texture
		pev->scale = 90.0f;

		// spotlight
		// create second PVS point
		pev->enemy = Create( "info_target", pev->origin, g_vecZero, edict() )->edict();
		SET_MODEL( pev->enemy, "sprites/null.spr" ); // allow to collect visibility info
		UTIL_SetSize ( VARS( pev->enemy ), Vector ( -8, -8, -8 ), Vector ( 8, 8, 8 ) );

		// to prevent disapperaing from PVS (renderamt is premultiplied by 0.125)
		UTIL_SetSize( pev, Vector( -pev->renderamt, -pev->renderamt, -pev->renderamt ), Vector( pev->renderamt, pev->renderamt, pev->renderamt ));

		pev->effects |= EF_NODRAW;
	}

	void Precache()
	{
		PRECACHE_MODEL("sprites/null.spr");
	}

	STATE GetState( void )
	{
		if (pev->effects & EF_NODRAW)
			return STATE_OFF;
		else
			return STATE_ON;
	}
	
	void On()
	{
		pev->effects &= ~EF_NODRAW;

		SetThink( PVSThink );
		SetNextThink( 0.1f );
	}			

	void Off()
	{
		pev->effects |= EF_NODRAW;
		DontThink();
	}

	void UpdatePVSPoint( void )
	{
		TraceResult tr;
		Vector forward;

		UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );
		Vector vecSrc = pev->origin + forward * 8.0f;
		Vector vecEnd = vecSrc + forward * (pev->renderamt * 8.0f);
		UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, edict(), &tr );

		// this is our second PVS point
		CBaseEntity *pVisHelper = CBaseEntity::Instance( pev->enemy );
		if( pVisHelper ) UTIL_SetOrigin( pVisHelper, tr.vecEndPos + tr.vecPlaneNormal * 8.0f ); 
	}

	void EXPORT PVSThink( void )
	{
		UpdatePVSPoint();

		// static light should be updated in case
		// moving platform under them
		SetNextThink( m_pMoveWith ? 0.01f : 0.1f );
	}
};

#define DEFAULT_SAIR_HEIGTH 17

//MaSTeR: Head controller for monsters 
class CHeadController : public CBaseEntity
{
public:
	void Spawn( CBaseEntity *pOwner )
	{
		pev->classname = MAKE_STRING( "head_controller" );
		m_iLightLevel = 1000; // Because if m_iLightLevel = 0, monster can turn on the flashlight, even if world is bright.
		m_iBackwardLen = 0;
		pev->origin = pOwner->pev->origin + Vector ( 0 , 0 , 32 );
		pev->angles = pOwner->pev->angles;
		m_hOwner = pOwner;
		SetNextThink( 0.1 );
	}
	
	BOOL ShouldUseLights( void )
	{
		if( m_iLightLevel < 10 )
			return TRUE;
		return FALSE;
	}

	void Think( void )
	{
		if( m_hOwner == NULL )
		{
			ALERT( at_error, "HeadController: onwer is lost!\n" );
			return;
		}

		MeasureLightLevel();
		TraceBack();
		SetNextThink( 0.5 );
	}

	int GetLightLevel( void ) 
	{
		return m_iLightLevel;
	}

	void MeasureLightLevel( void )
	{
		TraceResult tr;
		pev->origin = m_hOwner->pev->origin + Vector( 0.0f, 0.0f, 32 );
		pev->angles = m_hOwner->pev->angles;
		UTIL_MakeVectors( pev->angles );
		UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * 128.0f, ignore_monsters,ignore_glass, m_hOwner->edict(), &tr );
		m_iLightLevel = Illumination();
	}

	void TraceBack( void )
	{
		TraceResult tr;
		pev->origin = m_hOwner->pev->origin + Vector( 0, 0, DEFAULT_SAIR_HEIGTH );
		pev->angles = m_hOwner->pev->angles;
		UTIL_MakeVectors( pev->angles );
		UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * -512.0f, ignore_monsters, dont_ignore_glass, pev->owner, &tr );
		m_iBackwardLen = (pev->origin - tr.vecEndPos).Length2D();
	}

	int GetBackTrace( void )
	{
		return m_iBackwardLen;
	}

	virtual int Save( CSave &save ); 
	virtual int Restore( CRestore &restore );
	
	static TYPEDESCRIPTION m_SaveData[];

	int m_iLightLevel;
	int m_iBackwardLen;
	EHANDLE m_hOwner;
};
#endif