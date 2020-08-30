// ======================================
// Paranoia vgui hud header file
// written by BUzer.
// ======================================

#ifndef _VGUIHUD_H
#define _VGUIHUD_H
using namespace vgui;

#define NUM_WEAPON_ICONS		256 // ammo + miscellaneous items

class ImageHolder;

class CHud2 : public Panel
{
public:
	CHud2();
	~CHud2();
	void Initialize();
	
	void UpdateHealth (int newHealth);
	void UpdateArmor (int newArmor);
	void UpdateStamina(int newStamina);

//	void Think();
	virtual void solve();

protected:
	virtual void paintBackground(); // per-frame calculations and checks
	virtual void paint();

protected:
	// Wargon: »конка юза.
	CImageLabel	*m_pUsageIcon;
	float		m_fUsageUpdateTime;

	// painkiller icon
	CImageLabel	*m_pMedkitsIcon;
	Label		*m_pMedkitsCount;
	int		m_pMedkitsOldNum;
	float		m_fMedkitUpdateTime;

	// gasmask & headshield
	CImageLabel	*m_pShieldIcon;
	CImageLabel	*m_pGasMaskIcon;

	// health and armor bars
	ImageHolder	*m_pBitmapHealthFull;
	ImageHolder	*m_pBitmapHealthEmpty;
	ImageHolder	*m_pBitmapHealthFlash;

	ImageHolder	*m_pBitmapArmorFull;
	ImageHolder	*m_pBitmapArmorEmpty;
	ImageHolder	*m_pBitmapArmorFlash;

	ImageHolder *m_pBitmapStaminaFull;
	ImageHolder *m_pBitmapStaminaEmpty;
	ImageHolder *m_pBitmapStaminaFlash;

	ImageHolder	*m_pHealthIcon;
	ImageHolder	*m_pArmorIcon;
	ImageHolder *m_pStaminaIcon;

	int		m_iHealthBarWidth, m_iHealthBarHeight;
	int		m_iHealthBarXpos, m_iHealthBarYpos;
	int		m_iArmorBarWidth, m_iArmorBarHeight;
	int		m_iArmorBarXpos, m_iArmorBarYpos;
	int m_iStaminaBarWidth, m_iStaminaBarHeight;
	int m_iStaminaBarXpos, m_iStaminaBarYpos;

	BitmapTGA		*m_pWeaponIconsArray[NUM_WEAPON_ICONS];
	BitmapTGA		*FindAmmoImageForWeapon(const char *wpn);

	Font		*m_pFontDigits;

	char		*m_pWeaponNames[NUM_WEAPON_ICONS];
	int		m_iNumWeaponNames;

	int health, armor, stamina;
	int oldhealth, oldarmor, oldstamina;
	float m_fHealthUpdateTime, m_fArmorUpdateTime, m_fStaminaUpdateTime;
};

void Hud2Init();

#endif // _VGUIHUD_H