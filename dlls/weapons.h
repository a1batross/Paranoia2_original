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
#ifndef WEAPONS_H
#define WEAPONS_H

#include <bullets.h>
#include "effects.h"
#include "player.h"

class CBasePlayer;
class CBasePlayerItem;
extern int gmsgWeapPickup;

// Contact Grenade / Timed grenade
class CGrenade : public CBaseMonster
{
public:
	void Spawn( void );

	static CGrenade *ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, float damage = 100.0f );
	static CGrenade *ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static CGrenade *ShootGeneric( CBaseEntity *pOwner, Vector vecStart, Vector vecVelocity, string_t classname );

	void Explode( Vector vecSrc, Vector vecAim );
	void Explode( TraceResult *pTrace, int bitsDamageType );
	void EXPORT Smoke( void );

	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT SlideTouch( CBaseEntity *pOther );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT DangerSoundThink( void );
	void EXPORT PreDetonate( void );
	void EXPORT Detonate( void );
	void EXPORT DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink( void );

	virtual void BounceSound( void );
	virtual int BloodColor( void ) { return DONT_BLEED; }
	virtual void Killed( entvars_t *pevAttacker, int iGib );

	BOOL m_fRegisteredSound;// whether or not this grenade has issued its DANGER sound to the world sound list yet.
};

class CRpgRocket : public CGrenade
{
public:
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	void EXPORT FollowThink( void );
	void EXPORT IgniteThink( void );
	void EXPORT RocketTouch( CBaseEntity *pOther );
	static CRpgRocket *CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CBasePlayerItem *pLauncher );

	int m_iFireTrail; // Wargon: переменная для огненного следа у ракет.
	int m_iTrail;
	float m_flIgniteTime;
	CBasePlayerItem *m_pLauncher;// pointer back to the launcher that fired me. 
};

class CLaserSpot : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
public:
	void Update( CBasePlayer *m_pPlayer );
	void Suspend( float flSuspendTime );
	void EXPORT Revive( void );
	
	static CLaserSpot *CreateSpot( void );
	static CLaserSpot *CreateSpot( const char* spritename );
};

// hardcoded ID's
#define WEAPON_NONE				0
#define WEAPON_PAINKILLER			31
#define WEAPON_CUSTOM_COUNT			WEAPON_PAINKILLER
#define WEAPON_ALLWEAPONS			(~BIT( WEAPON_PAINKILLER ))

#define MAX_WEAPONS				32	// a 32-bit integer limit
#define MAX_AMMO_DESC			256	// various combinations of entities that contain ammo
#define MAX_SHOOTSOUNDS			8	// max of four random shoot sounds
#define MAX_NORMAL_BATTERY			100

// suit definitions
#define BAREHAND_SUIT			0	// just in case
#define MILITARY_SUIT			1	// gordon suit
#define NUM_HANDS				2	// number of hands: barney and gordon

#define PLAYER_HAS_SUIT			( FBitSet( m_pPlayer->m_iHideHUD, ITEM_SUIT ))
#define PLAYER_DRAW_SUIT			( FBitSet( pev->body, MILITARY_SUIT ))

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP			-1

// reload code
#define NOT_IN_RELOAD			0
#define START_RELOAD			1
#define CONTINUE_RELOAD			2	// for shotgun
#define EMPTY_RELOAD			3
#define NORMAL_RELOAD			4

// zoom code
#define NOT_IN_ZOOM				0	// zoom not used
#define NORMAL_ZOOM				1	// default zooming (button one shot)
#define UPDATE_ZOOM				2	// increase zoom (button holding down)
#define RESET_ZOOM				3	// disable zoom

#define ZOOM_MAXIMUM			20
#define ZOOM_DEFAULT			50

// ammo code
#define AMMO_UNKNOWN			0
#define AMMO_PRIMARY			1
#define AMMO_SECONDARY			2

#define ITEM_FLAG_SELECTONEMPTY		BIT( 0 )	// can select while ammo is out
#define ITEM_FLAG_NOAUTORELOAD		BIT( 1 )	// manual reload only
#define ITEM_FLAG_NOAUTOSWITCHEMPTY		BIT( 2 )	// don't switch to another gun while ammo is out
#define ITEM_FLAG_LIMITINWORLD		BIT( 3 )	// limit count in world (e.g. explodables)
#define ITEM_FLAG_EXHAUSTIBLE			BIT( 4 )	// A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_NODUPLICATE			BIT( 5 )	// player can't give this weapon again (e.g. knife, crowbar)
#define ITEM_FLAG_USEAUTOAIM			BIT( 6 )	// weapon uses autoaim
#define ITEM_FLAG_ALLOW_FIREMODE		BIT( 7 )	// allow to change firemode
#define ITEM_FLAG_SHOOT_UNDERWATER		BIT( 8 )	// weapon can be shoot underwater (e.g. glock)
#define ITEM_FLAG_IRONSIGHT			BIT( 9 )	// enable client effects: DOF, breathing etc
#define ITEM_FLAG_AUTOFIRE			BIT( 10 )	// hold attack for auto-fire
#define ITEM_FLAG_SCOPE			BIT( 11 )	// using scope instead of IronSight
#define ITEM_FLAG_NODROP			BIT( 12 )	// don't drop this weapon

#define WEAPON_IS_ONTARGET			0x40

typedef enum
{
	ATTACK_NONE = 0,	// no action
	ATTACK_AMMO1,	// fire primary ammo
	ATTACK_AMMO2,	// fire secondary ammo
	ATTACK_LASER_DOT,	// enable laser dot
	ATTACK_ZOOM,	// enable zoom
	ATTACK_FLASHLIGHT,	// enable flashlight
	ATTACK_SWITCHMODE,	// play two beetwen anims and change body
	ATTACK_SWING,	// knife swing
	ATTACK_IRONSIGHT,	// iron sight on and off
	ATTACK_SCOPE	// iron sight & scope
} e_attack;

enum
{
	SPREAD_LINEAR = 0,	// изменение разброса линейно во времени
	SPREAD_QUAD,	// по параболе (вначале узкий, потом резко расширяется)
	SPREAD_CUBE,	// кубическая парабола
	SPREAD_SQRT,	// наоборот (быстро расширяется и плавно переходит в максимальный)
};

typedef struct
{
	RandomRange	range;			// min spread .. max spread
	int		type;			// spread equalize
	float		expand;			// spread expand
} spread_t;

typedef struct
{
	float		maxSpeed;			// clamp at maximum
	float		jumpHeight;		// clamp at maximum
} player_settings_t;

// client feedback
typedef struct
{
	RandomRange	punchangle[3];		// range of x,y,z
	RandomRange	recoil;
} feedback_t;

typedef struct
{
	int		iSlot;			// hud slot
	int		iPosition;		// hud position
	string_t		iViewModel;        		// path to viewmodel
	string_t		iHandModel;		// path to optional hands model
	string_t		iWorldModel;       		// path to worldmodel
	char		szAnimExt[16];		// player anim postfix
	const char	*pszAmmo1;		// ammo 1 type
	int		iMaxAmmo1;		// max ammo 1
	const char	*pszAmmo2;		// ammo 2 type
	int		iMaxAmmo2;		// max ammo 2
	RandomRange	iDefaultAmmo1;		// default primary ammo
	RandomRange	iDefaultAmmo2;		// default secondary ammo
	const char	*pszName;			// unique weapon name
	int		iMaxClip;			// clip size
	int		iId;			// unique weapon ID
	int		iFlags;			// misc flags
	int		iWeight;			// this value used to determine this weapon's importance in autoselection.
	int		attack1;			// attack1 type
	int		attack2;			// attack2 type
	float		fNextAttack1;		// nextattack
	float		fNextAttack2;		// next secondary attack
	Vector		vecThrowOffset;		// throw view offset for melee grenades
	string_t		shootsound1[MAX_SHOOTSOUNDS];	// primary attack sounds
	string_t		shootsound2[MAX_SHOOTSOUNDS];	// secondary attack sounds
	string_t		emptysounds[MAX_SHOOTSOUNDS];	// empty sound
	int		sndcount1;		// primary attack sound count
	int		sndcount2;		// secondary attack sound count
	int		emptysndcount;		// empty sounds count
	string_t		smashDecals[2];		// decal groups for swing weapon (crowbar, knife)
	spread_t		spread1[2];		// customizable spread from Paranoia
	spread_t		spread2[2];		// same but for secondary attack
	feedback_t	feedback1[2];		// client feedback for primary attack (recoil, punch, speed etc)
	feedback_t	feedback2[2];		// same but for secondary attack
	player_settings_t	plr_settings[2];		// player speed and player jumpheight
	float		spreadtime;		// time to return to normal spread
	int		iVolume;			// weapon volume
	int		iFlash;			// weapon flash
	RandomRange	recoil1;			// recoil 1 attack
	RandomRange	recoil2;			// recoil 2 attack
} ItemInfo;

typedef struct
{
	const char	*pszName;
	int		iMaxCarry;
	int		iShellIndex;		// precached shell model index
	int		iNumShots;		// > 1 is fraction
	string_t		iMissileClassName;		// it's a projectile ammo (grenades etc)
	float		flDistance;		// typically 2048 for fractions
	float		flPlayerDamage;
	float		flMonsterDamage;
	int		iId;
} AmmoInfo;

// ammo_entity description
typedef struct
{
	string_t		classname;		// to link with real entity
	string_t		ammomodel;		// model to show
	string_t		clipsound;		// sound when touch entity
	AmmoInfo		*type;			// pointer to ammo specification
	RandomRange	count;
} AmmoDesc;

// Items that the player has in their inventory that they can use
class CBasePlayerItem : public CBaseAnimating
{
public:
	// buz: get advance spread vec to send it to client
	// gun dont use advanced spread by default
	virtual Vector GetSpreadVec( void );

	// buz: gun jumping actions
	virtual void PlayerJump( void );
	virtual void PlayerWalk( void );
	virtual void PlayerRun( void );

	// buz: get current weapon mode (for toggleable weapons)
	virtual int GetMode( void )
	{
		if( FBitSet( iFlags(), ITEM_FLAG_SCOPE|ITEM_FLAG_IRONSIGHT ) && !m_fInReload )
		{
			if( FBitSet( iFlags(), ITEM_FLAG_SCOPE ))
				return (m_iIronSight) ? 3 : 1;
			return (m_iIronSight + 1);
		}
		return 0; // 0 means gun dont use mode
	}

	virtual void WeaponToggleMode( void );

	virtual void SetObjectCollisionBox( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	virtual int AddToPlayer( CBasePlayer *pPlayer );	// return TRUE if the item you want the item added to the player inventory
	virtual int AddDuplicate( CBasePlayerItem *pItem ); // return TRUE if you want your duplicate removed from world
	void EXPORT DestroyItem( void );
	void EXPORT DefaultTouch( CBaseEntity *pOther );	// default weapon touch
	void EXPORT KnifeDecal1( void );
	void EXPORT KnifeDecal2( void );
	void Precache( void );
	void Spawn( void );

	// Wargon: Переменные для юзабельности оружий.
	void EXPORT DefaultUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) { DefaultTouch( pActivator ); }
	virtual int ObjectCaps( void ) { return m_iItemCaps | FCAP_ACROSS_TRANSITION | FCAP_USE_ONLY; }

	// generic "shared" ammo handlers
	BOOL AddPrimaryAmmo( int iCount, const char *szName, int iMaxClip, int iMaxCarry, BOOL duplicate );
	BOOL AddSecondaryAmmo( int iCount, const char *szName, int iMaxCarry );

	void EXPORT FallThink ( void );// when an item is first spawned, this think is run to determine when the object has hit the ground.
	void EXPORT Materialize( void );// make a weapon visible and tangible
	void EXPORT AttemptToMaterialize( void );  // the weapon desires to become visible and tangible, if the game rules allow for it
	CBaseEntity* Respawn ( void );// copy a weapon
	void FallInit( void );
	void CheckRespawn( void );
	virtual int GetItemInfo( ItemInfo *p );
	virtual BOOL CanDeploy( void );
	virtual BOOL CanHolster( void );			// can this weapon be put away right now?
	virtual BOOL IsUseable( void );
	virtual BOOL HasAmmo( void );
	virtual BOOL WaitForHolster( void ) { return m_fWaitForHolster; }
	virtual BOOL AllowToDrop( void ) { return !FBitSet( iFlags(), ITEM_FLAG_NODROP ); }

	virtual void ItemPreFrame( void ) { return; }		// called each frame by the player PreThink
	virtual void ItemPostFrame( void );			// called each frame by the player PostThink

	virtual int ExtractAmmo( CBasePlayerItem *pWeapon, BOOL duplicate );		// Return TRUE if you can add ammo to yourself when picked up
	virtual int ExtractClipAmmo( CBasePlayerItem *pWeapon );			// Return TRUE if you can add ammo to yourself when picked up

	virtual int AddWeapon( void ) { ExtractAmmo( this, FALSE ); return TRUE; };	// Return TRUE if you want to add yourself to the player

	virtual void PlayClientFire( const Vector &vecDir, float spread, int iAnim, int shellidx, string_t shootsnd, int cShots = 1 );
	virtual void SendWeaponAnim( int iAnim, float framerate = 1.0f );

	virtual void Drop( void );
	virtual void Kill( void );
	virtual void AttachToPlayer ( CBasePlayer *pPlayer );

	virtual int PrimaryAmmoIndex( void );
	virtual int SecondaryAmmoIndex( void );

	virtual int UpdateClientData( CBasePlayer *pPlayer );
	virtual void UpdateItemInfo( void ) {};	// updates HUD state

	Vector GetConeVectorForDegree( int degree );
	Vector CalcSpreadVec( const spread_t *info, float &spread );
	void DoEqualizeSpread( int type, float &spread );
	float ExpandSpread( float expandPower );
	float CalcSpread( void );

	void SetDefaultParams( ItemInfo *II );

	int ParseWeaponFile( ItemInfo *II, const char *filename );	// parse weapon_*.txt
	int ParseWeaponData( ItemInfo *II, char *file );	     	// parse WeaponData {}
	int ParsePrimaryAttack( ItemInfo *II, char *pfile );  	// parse PrimaryAttack {}
	int ParseSecondaryAttack( ItemInfo *II, char *pfile );  	// parse SeconadryAttack {}
	int ParseSoundData( ItemInfo *II, char *pfile );  	// parse SoundData {}
	char *ParseViewPunch( char *pfile, feedback_t *pFeed );
	int ParseItemFlags( char *pfile );
	// HudData will be parsed on the client side	
          virtual void GenerateID( void );			// generate unique ID number for each weapon
          virtual bool FindWeaponID( void );

	int GetAnimation( Activity activity );
	int SetAnimation( Activity activity, float fps = 1.0f );
	int SetAnimation( char *name, float fps = 1.0f ); 
	Activity GetIronSightActivity( Activity act );
	int UseAmmo( const char *ammo, int count );
	int GetAmmoType( const char *ammo );			// incredible stupid way...

	inline int IsEmptyReload( void ) { return m_iStepReload == EMPTY_RELOAD ? TRUE : FALSE; }
	int ShootGeneric( const char *ammo, int primary, int cShots = 1 );
	int ThrowGeneric( const char *ammo, int primary, int cShots = 1 );
	int GetCurrentAttack( const char *ammo, int primary );
	int PlayCurrentAttack( int action, int primary );
	void PlayAttackSound( int primary );
	void ApplyPlayerSettings( bool bReset );
	void SetPlayerEffects( void );
	float AutoAimDelta( int primary );

	float SetNextAttack( float delay ) { return m_pPlayer->m_flNextAttack = gpGlobals->time + delay; }
	float SetNextIdle( float delay ) { return m_flTimeWeaponIdle = gpGlobals->time + delay; }
	float SequenceDuration( void ) { return CBaseAnimating :: SequenceDuration( m_pPlayer->pev->weaponanim ); }
          
	static ItemInfo ItemInfoArray[MAX_WEAPONS];
	static int m_iGlobalID;				// unique ID for each weapon

	// weapon routines
	void ZoomUpdate( void );
	void ZoomReset( void );

	CBasePlayer	*m_pPlayer;
	CBasePlayerItem	*m_pNext;
	CLaserSpot	*m_pSpot;				// LTD spot (don't save this, becase spot have FCAP_DONT_SAVE flag)
	int		m_iId;				// WEAPON_???
	int		m_iItemCaps;
	TraceResult	m_trHit;				// for knife
	BOOL		m_fWaitForHolster;			// weapon waiting for holster
	string_t		m_iHandModel;			// in case it present
	
	// virtual methods for ItemInfo acess
	int		iItemPosition( void )	{ return ItemInfoArray[m_iId].iPosition; }
	int		iItemSlot( void )		{ return ItemInfoArray[m_iId].iSlot + 1; }
	int		iViewModel( void )		{ return ItemInfoArray[m_iId].iViewModel; }
	int		iHandModel( void )		{ return ItemInfoArray[m_iId].iHandModel; }
	int		iWorldModel( void )		{ return ItemInfoArray[m_iId].iWorldModel; }
	int		iDefaultAmmo1( void )	{ return (int)ItemInfoArray[m_iId].iDefaultAmmo1.Random(); }
	int		iDefaultAmmo2( void )	{ return (int)ItemInfoArray[m_iId].iDefaultAmmo2.Random(); }
	int		iMaxAmmo1( void )		{ return ItemInfoArray[m_iId].iMaxAmmo1; }
	int		iMaxAmmo2( void )		{ return ItemInfoArray[m_iId].iMaxAmmo2; }
	int		iMaxClip( void )		{ return ItemInfoArray[m_iId].iMaxClip; }
	int		iWeight( void )		{ return ItemInfoArray[m_iId].iWeight; }
	int		iFlags( void )		{ return ItemInfoArray[m_iId].iFlags; }
	int		iAttack1( void )		{ return ItemInfoArray[m_iId].attack1; }
	int		sndcnt1( void )		{ return ItemInfoArray[m_iId].sndcount1; }
	int		sndcnt2( void )		{ return ItemInfoArray[m_iId].sndcount2; }
	int		emptycnt( void )		{ return ItemInfoArray[m_iId].emptysndcount; }
	string_t		ShootSnd1( void )		{ return ItemInfoArray[m_iId].shootsound1[RANDOM_LONG( 0, sndcnt1( ) - 1)]; }
	string_t		ShootSnd2( void )		{ return ItemInfoArray[m_iId].shootsound2[RANDOM_LONG( 0, sndcnt2( ) - 1)]; }
	string_t		EmptySnd( void )		{ return ItemInfoArray[m_iId].emptysounds[RANDOM_LONG( 0, emptycnt() - 1)]; }
	int		iAttack2( void )		{ return ItemInfoArray[m_iId].attack2; }
	char		*szAnimExt( void )		{ return ItemInfoArray[m_iId].szAnimExt; }
	float		fNextAttack1( void )	{ return ItemInfoArray[m_iId].fNextAttack1; }
	float		fNextAttack2( void )	{ return ItemInfoArray[m_iId].fNextAttack2; }
	float		fRecoil1( void )		{ return ItemInfoArray[m_iId].recoil1.Random();	}
	float		fRecoil2( void )		{ return ItemInfoArray[m_iId].recoil2.Random(); }
	const char	*pszAmmo1( void )		{ return ItemInfoArray[m_iId].pszAmmo1; }
	const char	*pszName( void )		{ return ItemInfoArray[m_iId].pszName; }
	const char	*pszAmmo2( void )		{ return ItemInfoArray[m_iId].pszAmmo2; }
	const spread_t	*pSpread1( void )		{ return &ItemInfoArray[m_iId].spread1[m_iIronSight]; }
	const spread_t	*pSpread2( void )		{ return &ItemInfoArray[m_iId].spread2[m_iIronSight]; }
	const feedback_t	*pFeedback1( void )		{ return &ItemInfoArray[m_iId].feedback1[m_iIronSight]; }
	const feedback_t	*pFeedback2( void )		{ return &ItemInfoArray[m_iId].feedback2[m_iIronSight]; }
	const char	*pszDecalName( int type )	{ return STRING( ItemInfoArray[m_iId].smashDecals[type] ); }
	float		ClientMaxSpeed( void )	{ return ItemInfoArray[m_iId].plr_settings[m_iIronSight].maxSpeed; }
	float		ClientJumpHeight( void )	{ return ItemInfoArray[m_iId].plr_settings[m_iIronSight].jumpHeight; }
	Vector		vecThrowOffset( void )	{ return ItemInfoArray[m_iId].vecThrowOffset; }
	float		fSpreadTime( void )		{ return ItemInfoArray[m_iId].spreadtime; }
	int		iVolume( void )		{ return ItemInfoArray[m_iId].iVolume; }
	int		iFlash( void )		{ return ItemInfoArray[m_iId].iFlash; }

	int		m_iWeaponAutoFire;		// 0 - semi-auto, 1 - full auto

	int		m_iDefaultAmmo1;		// how much ammo you get when you pick up this weapon as placed by a level designer.
	int		m_iDefaultAmmo2;		// how much ammo you get when you pick up this weapon as placed by a level designer.

	float		m_flNextPrimaryAttack;	// soonest time ItemPostFrame will call PrimaryAttack
	float		m_flNextSecondaryAttack;	// soonest time ItemPostFrame will call SecondaryAttack
	float		m_flTimeWeaponIdle;		// soonest time ItemPostFrame will call WeaponIdle
	int		m_iPrimaryAmmoType;		// "primary" ammo index into players m_rgAmmo[]
	int		m_iSecondaryAmmoType;	// "secondary" ammo index into players m_rgAmmo[]
	int		m_fFireOnEmpty;		// True when the gun is empty and the player is still holding down the attack key(s)
	float		m_flTimeUpdate;		// special time for additional effects
	int 		m_iStepReload;		// reload state (e.g. for shotgun)
	int		m_iIronSight;		// iron sight is enabled
	float		m_flSpreadTime;		// time to return from full spread
	float		m_flLastShotTime;		// time from last shot
	float		m_flLastSpreadPower;	// [0..1] range value
	int		m_cActiveRockets;		// stuff for rpg with LTD
	int		m_iClip;			// number of shots left in the primary weapon clip, -1 it not used
	int		m_iSpot;			// enable laser dot
          int		m_iZoom;			// zoom current level
	int		m_iBody;			// viewmodel body
	int		m_iSkin;			// viewmodel skin

	int		m_fInReload;		// Are we in the middle of a reload;
	int		m_iPlayEmptySound;		// trigger to playing empty sound once
	int		m_iClientAnim;		// used to resend anim on save\restore
	int		m_iClientClip;		// the last version of m_iClip sent to hud dll
	int		m_iClientWeaponState;	// the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int		m_iClientSkin;		// the last version of m_iSkin sent to hud dll
	int		m_iClientBody;		// the last version of m_iBody sent to hud dll
          float		m_flHoldTime;		// button holdtime
 
	// this methods can be overloaded
	virtual void RetireWeapon( void );
	virtual BOOL ShouldWeaponIdle( void ) { return FALSE; };
	virtual BOOL PlayEmptySound( void );
	virtual void ResetEmptySound( void );

	// default methods
	BOOL DefaultDeploy( Activity sequence );
	BOOL DefaultHolster( Activity sequence, bool force = false );
	BOOL DefaultReload( Activity sequence );
	BOOL DefaultSwing( int primary );
	void DefaultIdle( void );

	virtual void PrimaryAttack( void )   // do "+ATTACK"
	{
		if( !m_iWeaponAutoFire && !FBitSet( m_pPlayer->m_afButtonPressed, IN_ATTACK ))
			return; // no effect for hold button

		int iResult = PlayCurrentAttack( iAttack1(), true );

		if( iResult == 1 )
		{
			const feedback_t *pFB = pFeedback1();
			m_pPlayer->ViewPunch( pFB->punchangle[0].Random(), pFB->punchangle[1].Random(), pFB->punchangle[2].Random() );
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * fRecoil1();
                             
			PrimaryPostAttack(); // run post effects

			float flNextAttack = fNextAttack1();
			if( flNextAttack == -1.0f )
				flNextAttack = SequenceDuration();

			if( m_flNextPrimaryAttack < UTIL_WeaponTimeBase( ))
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flNextAttack + 0.02f;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flNextAttack;
			if( HasAmmo( )) m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10.0f, 15.0f );
		}
		else if( iResult == 0 )
		{
			float flNextAttack = 0.1f;

			if( GetAnimation( ACT_VM_SHOOT_EMPTY ) != -1 )
			{
				SetAnimation( ACT_VM_SHOOT_EMPTY );
				flNextAttack = SequenceDuration();
			}

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flNextAttack;
			PlayEmptySound();
		}

		m_iStepReload = NOT_IN_RELOAD; // reset reload
	}

	virtual void SecondaryAttack( void ) // do "+ATTACK2"
	{
		if( !FBitSet( m_pPlayer->m_afButtonPressed, IN_ATTACK2 ))
			return; // no effect for hold button

		int iResult = PlayCurrentAttack( iAttack2(), false );

		if( iResult == 1 )
		{
			const feedback_t *pFB = pFeedback2();
			m_pPlayer->ViewPunch( pFB->punchangle[0].Random(), pFB->punchangle[1].Random(), pFB->punchangle[2].Random() );
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * fRecoil2();

			SecondaryPostAttack(); // run post effects

			float flNextAttack = fNextAttack2();
			if( flNextAttack == -1.0f )
				flNextAttack = SequenceDuration();

			if( m_flNextSecondaryAttack < UTIL_WeaponTimeBase() )
				m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flNextAttack + 0.02f;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flNextAttack;
			if( HasAmmo( )) m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10.0f, 15.0f );
		}
		else if( iResult == 0 )
		{
			float flNextAttack = 0.1f;

			if( GetAnimation( ACT_VM_SHOOT_EMPTY ) != -1 )
			{
				SetAnimation( ACT_VM_SHOOT_EMPTY );
				flNextAttack = SequenceDuration();
			}

			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flNextAttack;
			PlayEmptySound();
		}

		m_iStepReload = NOT_IN_RELOAD; // reset reload
	}

	virtual void Reload( void ){ DefaultReload( ACT_VM_RELOAD ); } 	// do "+RELOAD"
	virtual void PrimaryPostAttack( void ) {}
	virtual void SecondaryPostAttack( void ) {}
	virtual void PostReload( void ) {}
	virtual void PostIdle( void ) {}				// calling every frame
	virtual void WeaponIdle( void ){ DefaultIdle(); }			// called when no buttons pressed
	virtual void Deploy( void );					// deploy function
	virtual void Holster( bool force = false );			// holster function
};

class CBasePlayerAmmo : public CBaseEntity
{
public:
	void Precache( void );
	void Spawn( void );

	// Wargon: Переменные для юзабельности патронов.
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	void EXPORT DefaultUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) { DefaultTouch( pActivator ); }
	virtual int ObjectCaps( void ) { return m_iAmmoCaps | FCAP_ACROSS_TRANSITION | FCAP_USE_ONLY; }
	virtual BOOL IsGenericAmmo( void ) { return ( FStrEq( STRING( pev->classname ), "ammo_generic" ) && pev->netname != iStringNull ); }
	void KeyValue( KeyValueData *pkvd );
	BOOL InitGenericAmmo( void );

	RandomRange	m_rAmmoCount;
	BOOL		m_bCustomAmmo;	// it's a virtual entity
	string_t		m_iAmmoType;	// just store name of ammo so we can find them again
	int		m_iAmmoCaps;

	static AmmoInfo AmmoInfoArray[MAX_AMMO_SLOTS];
	static AmmoDesc AmmoDescArray[MAX_AMMO_DESC];

	void EXPORT DefaultTouch( CBaseEntity *pOther ); // default weapon touch
	virtual BOOL AddAmmo( CBaseEntity *pOther );

	CBaseEntity* Respawn( void );
	void EXPORT Materialize( void );
};


extern DLL_GLOBAL short	g_sModelIndexLaser;// holds the index for the laser beam
extern DLL_GLOBAL const char	*g_pModelNameLaser;
extern DLL_GLOBAL short	g_sModelIndexLaserDot;// holds the index for the laser beam dot
extern DLL_GLOBAL short	g_sModelIndexFireball;// holds the index for the fireball
extern DLL_GLOBAL short	g_sModelIndexSmoke;// holds the index for the smoke cloud
extern DLL_GLOBAL short	g_sModelIndexWExplosion;// holds the index for the underwater explosion
extern DLL_GLOBAL short	g_sModelIndexBubbles;// holds the index for the bubbles model
extern DLL_GLOBAL short	g_sModelIndexBloodDrop;// holds the sprite index for blood drops
extern DLL_GLOBAL short	g_sModelIndexBloodSpray;// holds the sprite index for blood spray (bigger)
extern DLL_GLOBAL short	g_sModelIndexWaterSplash;
extern DLL_GLOBAL short	g_sModelIndexSmokeTrail;
extern DLL_GLOBAL short	g_sModelIndexNull;

extern void ClearMultiDamage( void );
extern void ApplyMultiDamage( entvars_t* pevInflictor, entvars_t* pevAttacker );
extern void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType );

extern void DecalGunshot( TraceResult *pTrace, int iBulletType, const Vector &vecSrc, bool fromPlayer = false );
extern void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern const char *DamageDecal( CBaseEntity *pEntity, int bitsDamageType );
extern void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType );
extern void UTIL_InitAmmoDescription( const char *filename );
extern void UTIL_InitWeaponDescription( const char *pattern );
extern void AddAmmoNameToAmmoRegistry( const char *szAmmoname );
extern AmmoInfo *UTIL_FindAmmoType( const char *szAmmoname );

typedef struct 
{
	CBaseEntity	*pEntity;
	float		amount;
	int		type;
} MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;

#define LOUD_GUN_VOLUME		1000
#define NORMAL_GUN_VOLUME		600
#define QUIET_GUN_VOLUME		200
#define NO_GUN_VOLUME		0

#define BRIGHT_GUN_FLASH		512
#define NORMAL_GUN_FLASH		256
#define DIM_GUN_FLASH		128
#define NO_GUN_FLASH		0

#define KNIFE_BODYHIT_VOLUME		128
#define KNIFE_WALLHIT_VOLUME		512

#define BIG_EXPLOSION_VOLUME	2048
#define NORMAL_EXPLOSION_VOLUME	1024
#define SMALL_EXPLOSION_VOLUME	512

#define WEAPON_ACTIVITY_VOLUME	64

#define VECTOR_CONE_1DEGREES	Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES	Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES	Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES	Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES	Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES	Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES	Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES	Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES	Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES	Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES	Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES	Vector( 0.17365, 0.17365, 0.17365 )

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo. 
//=========================================================
class CWeaponBox : public CBaseAnimating
{
	void Precache( void );
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	BOOL IsEmpty( void );
	int GiveAmmo( int iCount, const char *szName );
	int MaxAmmoCarry( int iszName );
	void SetObjectCollisionBox( void );
public:
	void EXPORT Kill( void );

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BOOL HasWeapon( CBasePlayerItem *pCheckItem );
	BOOL PackWeapon( CBasePlayerItem *pWeapon );
	BOOL PackAmmo( int iszName, int iCount );
	
	CBasePlayerItem	*m_rgpPlayerItems[MAX_ITEM_TYPES];// one slot for each 

	int m_rgiszAmmo[MAX_AMMO_SLOTS];// ammo names
	int m_rgAmmo[MAX_AMMO_SLOTS];// ammo quantities

	int m_cAmmoTypes;// how many ammo types packed into this box (if packed by a level designer)
};

#endif // WEAPONS_H