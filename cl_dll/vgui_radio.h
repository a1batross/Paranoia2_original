// ======================================
// Paranoia radio icon header file
// written by BUzer, based on valve's code
// ======================================

#ifndef _RADIO_H
#define _RADIO_H
using namespace vgui;

class CRadioIcon : public Panel
{
public:
    CRadioIcon();
	~CRadioIcon();
	void Show(const char *title, float time, int r, int g, int b, int a);
	void Initialize();

protected:
	virtual void paintBackground();
	void	Reposition();

protected:
	Image		*m_pSpeakerBitmap;
	Label		*m_pLabel;
	ImagePanel	*m_pIcon;

	float	m_fShowTime;
	float	m_fHideTime;
	int		r, g, b, a;

	char	nextTitle[256];
	float	nextTime;
	int next_r, next_g, next_b, next_a; // next_a shows - is next message present
};

void RadioIconInit();

#endif // _RADIO_H