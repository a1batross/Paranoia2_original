// ======================================
// written by BUzer for HL: Paranoia modification
// ======================================

#ifndef _VGUIMSG_H
#define _VGUIMSG_H
using namespace vgui;

#include "vgui_shadowtext.h"

Font* FontFromMessage(const char* &ptext);


class CScreenMessage : public ShadowTextPanel
{
public:
    CScreenMessage() : ShadowTextPanel("", 0, 0, ScreenWidth, ScreenHeight)
	{
		setVisible(false);
		setPaintBackgroundEnabled(false);
	}

	void Initialize()
	{
		setVisible(false);
	}

	void SetMessage( client_textmessage_t *msg )
	{
		const char *text = msg->pMessage;
		Font *pFont = FontFromMessage(text);
		setFont(pFont);
		setText(text);
		setFgColor(msg->r1, msg->g1, msg->b1, msg->a1);

		m_starttime = gEngfuncs.GetClientTime();
		m_hold = msg->holdtime;
		m_fadein = msg->fadein;
		m_fadeout = msg->fadeout;

		// Wargon: Если координаты заданы неправильно, то текст просто центрируется.
		if (msg->x < 0 || msg->x > 1 || msg->y < 0 || msg->y > 1)
		{
			// gEngfuncs.Con_Printf("Error: invalid message coordinates!\n");
			// return;
			int tw, th;
			getTextImage()->getTextSizeWrapped(tw, th);
			setSize(tw, th);
			setPos((ScreenWidth - tw) * 0.5, (ScreenHeight - th) * 0.5);
		}
		else
		{
			int x = msg->x * ScreenWidth;
			int y = msg->y * ScreenHeight;
			setPos(x, y);
		}

		setVisible(true);
	}

protected:
	virtual void paint()
	{
		int mr, mg, mb, ma;
		getFgColor(mr, mg, mb, ma);

		float curtime = gEngfuncs.GetClientTime() - m_starttime;
		if (curtime < 0)
			return;
		
		if (curtime > (m_hold + m_fadein + m_fadeout))
		{
			setVisible(false);
			return;
		}

		if (curtime < m_fadein)
		{
			float alpha = curtime / m_fadein;
			setFgColor( mr, mg, mb, (1 - alpha) * 255 );
		}
		else if (curtime > (m_fadein + m_hold))
		{
			float alpha = (curtime - m_fadein - m_hold) / m_fadeout;
			setFgColor( mr, mg, mb, (alpha) * 255 );
		}
		else
		{
			setFgColor( mr, mg, mb, 0 );
		}

		ShadowTextPanel::paint();
	}

	float m_starttime;
	float m_hold;
	float m_fadein;
	float m_fadeout;
};

// Wargon: Код скроллящегося снизу вверх текста. Использован $effect 6.
class CScrollingMessage : public ShadowTextPanel
{
public:
	CScrollingMessage() : ShadowTextPanel("", 0, 0, ScreenWidth, ScreenHeight)
	{
		setVisible(false);
		setPaintBackgroundEnabled(false);
	}

	void Initialize()
	{
		setVisible(false);
	}

	void SetMessage( client_textmessage_t *msg )
	{
		int w, h;
		const char *text = msg->pMessage;
		Font *pFont = FontFromMessage(text);
		setSize(ScreenWidth, ScreenHeight);
		setFont(pFont);
		setText(text);
		setFgColor(msg->r1, msg->g1, msg->b1, msg->a1);
		getTextImage()->getTextSizeWrapped(w, h);
		setSize(w, h);
		m_starttime = gEngfuncs.GetClientTime();
		m_hold = msg->holdtime;
		m_speed = ((ScreenHeight + h) / m_hold) * 0.02;
		m_delay = 0;
		if (msg->x < 0 || msg->x > 1)
		{
			setPos((ScreenWidth - w) * 0.5, ScreenHeight - 1);
		}
		else
		{
			setPos(msg->x * ScreenWidth, ScreenHeight - 1);
		}
		setVisible(true);
	}

protected:
	virtual void paint()
	{
		float curtime = gEngfuncs.GetClientTime() - m_starttime;
		if (curtime < 0)
			return;

		m_delay += m_speed;

		if (curtime > m_hold)
		{
			setVisible(false);
			return;
		}
		else if (m_delay >= 1)
		{
			int x, y;
			getPos(x, y);
			setPos(x, y - 1);
			m_delay = 0;
		}

		ShadowTextPanel::paint();
	}

	float m_starttime;
	float m_hold;
	float m_speed;
	float m_delay;
};

#endif // _VGUIMSG_H