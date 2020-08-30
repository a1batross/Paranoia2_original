// ======================================
// Paranoia goal panel header file
// written by BUzer.
// ======================================

#ifndef _TABPANEL_H
#define _TABPANEL_H
using namespace vgui;

class CTabPanel : public Panel
{
public:
    CTabPanel();
	~CTabPanel();
	void Initialize();

	void LoadMessage(const char *pszName, int iSize, void *pbuf);
	int Toggle();

protected:
	virtual void paintBackground(); // per-frame calculations and checks

protected:
	Panel		*m_pBkPanel;
	TextPanel	*m_pTextPanel;
	Label		*m_pTitle;
	ImagePanel	*m_pImage;
	BitmapTGA	*m_pBitmap;
	int			m_iTextLoaded;
	float		m_fHideTime;
};

void TabPanelInit();

#endif // _TABPANEL_H