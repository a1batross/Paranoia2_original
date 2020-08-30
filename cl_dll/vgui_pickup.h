// ======================================
// written by BUzer for HL: Paranoia modification
// ======================================

#ifndef _VGUIPICKMSG_H
#define _VGUIPICKMSG_H
using namespace vgui;

#include "vgui_shadowtext.h"
#include "VGUI_TextImage.h"

Font* FontFromMessage(const char* &ptext);
void CheckPanel();


#define RIGHTSPACE	(XRES(10))
#define MOVEOFFSET	(XRES(20))

extern int g_ammoAdded; // hack - store amount of added ammo last time in global variable



class CPickupMessage : public ShadowTextPanel
{
public:
    CPickupMessage(int height) : ShadowTextPanel("", 0, 0, ScreenWidth, ScreenHeight)
	{
		setVisible(false);
		setPaintBackgroundEnabled(false);
		m_iHeight = height;
	}

	void Initialize()
	{
		setVisible(false);
	}

	void SetMessage( client_textmessage_t *msg )
	{
		setSize(ScreenWidth, 16);
		const char *text = msg->pMessage;
		Font *pFont = FontFromMessage(text);
		char buf[1024];
		sprintf(buf, text, g_ammoAdded); // text message should contain %d substring
		setFont(pFont);
		setText(buf);

		int tw, th;
		getTextImage()->getTextSizeWrapped( tw, th );
		setSize( tw, th );

		m_starttime = gEngfuncs.GetClientTime();
		m_hold = msg->holdtime;
		m_fadein = msg->fadein;
		m_fadeout = msg->fadeout;
		r1 = msg->r1; g1 = msg->g1; b1 = msg->b1; a1 = 0;
		r2 = msg->r2; g2 = msg->g2; b2 = msg->b2; a2 = 64;

		int xpos = ScreenWidth - RIGHTSPACE - tw - MOVEOFFSET;
		setPos(xpos, m_iHeight);

		setVisible(true);
	}

protected:
	virtual void paint()
	{
		float curtime = gEngfuncs.GetClientTime() - m_starttime;
		if (curtime < 0)
			return;
		
		if(curtime > (m_hold + m_fadein + m_fadeout))
		{
			setVisible(false);
			CheckPanel();
			return;
		}

		if (curtime < m_fadein)
		{
			// fadein stage
			// interpolate color1->color2
			float fraction = curtime / m_fadein;
			int r3, g3, b3, a3;
			r3 = r1 + (byte)((float)(r2 - r1) * fraction);
			g3 = g1 + (byte)((float)(g2 - g1) * fraction);
			b3 = b1 + (byte)((float)(b2 - b1) * fraction);
			a3 = a1 + (byte)((float)(a2 - a1) * fraction);
			setFgColor(r3, g3, b3, a3);

			int wide, tall;
			fraction = 1 - fraction;
			fraction *= fraction;
			getSize(wide, tall);
			int xpos = ScreenWidth - RIGHTSPACE - wide - (int)((float)MOVEOFFSET * fraction);
			setPos(xpos, m_iHeight);
		}
		else if (curtime > (m_fadein + m_hold))
		{
			// fadeout stage
			float fraction = (curtime - m_fadein - m_hold) / m_fadeout;
			int a = a2 + (byte)((float)(255 - a2) * fraction);
			setFgColor(r2, g2, b2, a);

			int wide, tall;
			getSize(wide, tall);
			setPos(ScreenWidth - RIGHTSPACE - wide, m_iHeight);
		}
		else
		{
			setFgColor(r2, g2, b2, a2);

			int wide, tall;
			getSize(wide, tall);
			setPos(ScreenWidth - RIGHTSPACE - wide, m_iHeight);
		}

		ShadowTextPanel::paint();
	}

	float m_starttime;
	float m_hold;
	float m_fadein;
	float m_fadeout;
	int m_iHeight;

	byte	r1, g1, b1, a1;	// start (fadein) color
	byte	r2, g2, b2, a2; // end (hold) color
};

#endif // _VGUIPICKMSG_H