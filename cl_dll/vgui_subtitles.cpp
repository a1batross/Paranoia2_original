// ====================================
// Paranoia subtitle system interface
//		written by BUzer
// ====================================

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_subtitles.h"
#include "VGUI_TextImage.h"


cvar_t *time_min;
cvar_t *time_max;
cvar_t *subtitles_enabled;
cvar_t *scroll_speed;
cvar_t *fade_speed;


Font* FontFromMessage(const char* &ptext)
{
	char fontname[64] = "Default Text";
	if (ptext != NULL && ptext[0] != 0)
	{
		if (ptext[0] == '@')
		{
			// get font name
			ptext++;
			ptext = gEngfuncs.COM_ParseFile((char*)ptext, fontname);
			ptext+=2;
		}
	}

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTextScheme = pSchemes->getSchemeHandle( fontname );
	return pSchemes->getFont( hTextScheme );
}

void SubtitleMessageAdd( client_textmessage_t *tempMessage )
{
	if (!subtitles_enabled->value)
		return;

	if (gViewPort && gViewPort->m_pSubtitle)
		gViewPort->m_pSubtitle->AddMessage( tempMessage );
	else
		gEngfuncs.Con_Printf("Subtitle error: CSubtitle or ViewPort is not constructed!\n");
}

void SubtitleInit()
{
	time_min = gEngfuncs.pfnRegisterVariable( "subt_mintime", "0", 0 );
	time_max = gEngfuncs.pfnRegisterVariable( "subt_maxtime", "0", 0 );
	subtitles_enabled = gEngfuncs.pfnRegisterVariable( "subtitles", "0", FCVAR_ARCHIVE );
	scroll_speed = gEngfuncs.pfnRegisterVariable( "subt_scrollspd", "250", 0 );
	fade_speed = gEngfuncs.pfnRegisterVariable( "subt_fadespd", "0.2", 0 );
}


void CSubtitleTextPanel::paintBackground()
{
	float fade = fade_speed->value;
	float time = gEngfuncs.GetClientTime();

	int r, g, b, a;
	getFgColor(r, g, b ,a);

	if (m_fBirthTime && (m_fBirthTime + fade > time))
	{
		float alpha = (time - m_fBirthTime) / fade;
		setFgColor( r, g, b, 255 - (alpha * 255));
	/*	if (bkalpha)
		{
			// draw background
			getBgColor(r, g, b ,a);
			drawSetColor(r, g, b, 255 - (alpha * bkalpha));
			drawFilledRect(0, 0, getWide(), getTall());
		}*/		
		return;
	}

	if (gViewPort && gViewPort->m_pSubtitle) // hm..
	{
		if (gViewPort->m_pSubtitle->m_pCur == this)
		{
			float dietime = gViewPort->m_pSubtitle->m_fCurStartTime + m_fHoldTime;
			if (dietime - fade < time)
			{
				float alpha = (dietime - time) / fade;
				setFgColor( r, g, b, 255 - (alpha * 255));
			/*	if (bkalpha)
				{
					getBgColor(r, g, b ,a);
					setBgColor(r, g, b, 255 - (alpha * bkalpha));
					TextPanel::paintBackground();
				}*/
				return;
			}
		}
	}

	setFgColor( r, g, b, 0 );
}

void CSubtitleTextPanel::paint()
{
	int mr, mg, mb, ma;
	int ix, iy;
	getFgColor(mr, mg, mb, ma);
	getTextImage()->getPos(ix, iy);
	getTextImage()->setPos(ix+1, iy+1);
	getTextImage()->setColor( Color(0, 0, 0, ma) );
	getTextImage()->doPaint(this);
	getTextImage()->setPos(ix, iy);
	getTextImage()->setColor( Color(mr, mg, mb, ma) );
	getTextImage()->doPaint(this);
}


CSubtitle::CSubtitle() : Panel(XRES(10), YRES(10), XRES(330), YRES(240))
{	
	m_pLayer = new Panel;
	m_pLayer->setParent(this);
	m_pLayer->setPaintBackgroundEnabled(false);
	setVisible(false);
	m_pCur = NULL;
	m_pWait = NULL;
	lasttime = 0;
	layerpos = 0;
}

void CSubtitle::Initialize()
{
	m_pLayer->removeAllChildren();
	setVisible(false);
	m_pCur = NULL;
	m_pWait = NULL;
	lasttime = 0;
	layerpos = 0;
}


void CSubtitle::AddMessage( client_textmessage_t *msg )
{
	float time = gEngfuncs.GetClientTime();
	float holdtime = msg->holdtime;	
	if (time_min->value && (holdtime < time_min->value)) holdtime = time_min->value;
	if (time_max->value && (holdtime > time_max->value)) holdtime = time_max->value;

//	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
//	SchemeHandle_t hTextScheme = pSchemes->getSchemeHandle( "Default Text" );
//	Font *pFont = pSchemes->getFont( hTextScheme );

	const char *pText = msg->pMessage;
	client_textmessage_t *postMsg = NULL;
	if (pText[0] == '$')
	{
		// get postMsg
		char postMsgName[64];
		pText = gEngfuncs.COM_ParseFile((char*)pText, postMsgName);
		postMsg = TextMessageGet( &postMsgName[1] );
		pText+=2;
		if (!postMsg)
			gEngfuncs.Con_Printf("WARNING: post-message %s not found in titles.txt!\n", postMsgName);
	}

	Font *pFont = FontFromMessage(pText);

	int tw, th;
	CSubtitleTextPanel *text = new CSubtitleTextPanel(pText, 0, 0, getWide(), 64 );
	text->setParent( m_pLayer );
	text->setFont(pFont);
	text->msgAfterDeath = postMsg;
//	text->setPaintBackgroundEnabled(false);
//	text->setBgColor(msg->r2, msg->g2, msg->b2, 255);
//	text->bkalpha = msg->fadeout;
//	gEngfuncs.Con_Printf("added bkalpha: %d\n", text->bkalpha);
	text->setFgColor(msg->r1, msg->g1, msg->b1, 0);
	text->getTextImage()->getTextSizeWrapped( tw, th );
	text->getTextImage()->setPos(0, 5);
	th += 10;
	text->setSize( getWide(), th );
	text->m_fHoldTime = holdtime;

	if (!isVisible())
	{
		layerpos = getTall()-th;
		m_pLayer->setBounds(0, layerpos, getWide(), th);
		text->m_fBirthTime = time;
		m_fCurStartTime = time;
		m_pCur = text;
		m_pWait = NULL;
		text->setVisible(true);
		setVisible(true);
	}
	else
	{
		text->setVisible(false);
		text->m_fBirthTime = 0;

		if (!m_pWait)
		{
			int lt = m_pLayer->getTall();
			m_pLayer->setSize(getWide(), lt+th);
			text->setPos(0, lt);
			m_pWait = text;
		}
	}
}

void CSubtitle::paintBackground()
{	
	float curtime = gEngfuncs.GetClientTime();
	if (lasttime == 0) lasttime = curtime;
	float deltatime = curtime - lasttime;

	if (!subtitles_enabled->value)
	{
		Initialize();
		return;
	}

	if (m_pCur)
	{
		if (m_fCurStartTime + m_pCur->m_fHoldTime <= curtime)
		{
			m_fCurStartTime = curtime;
			client_textmessage_t *newmsg = m_pCur->msgAfterDeath;
			m_pLayer->removeChild(m_pCur);
			m_pCur = NULL;

			if (newmsg)
				AddMessage(newmsg);

			if (m_pLayer->getChildCount() == 0)
			{
				m_pWait = NULL;
				setVisible(false);
				return;
			}

			// find oldest child to start fading int
			float mintime = 99999;
			for (int i = 0; i < m_pLayer->getChildCount(); i++)
			{
				CSubtitleTextPanel *chld = (CSubtitleTextPanel*)m_pLayer->getChild(i);
				if (chld->isVisible() && chld->m_fBirthTime < mintime)
				{
					m_pCur = chld;
					mintime = chld->m_fBirthTime;
				}
			}

			if (!m_pCur)
			{
				// get cur from waiting queue
				m_pCur = m_pWait;
				if (m_pCur)
				{
					layerpos = getTall() - m_pCur->getTall();
					m_pLayer->setBounds(0, layerpos, getWide(), m_pCur->getTall());
					m_pCur->setPos(0,0);
					m_pCur->m_fBirthTime = curtime;					
					m_pCur->setVisible(true);

					m_pWait = NULL;
					for (i = 0; i < m_pLayer->getChildCount(); i++)
					{
						if (!m_pLayer->getChild(i)->isVisible())
						{
							m_pWait = (CSubtitleTextPanel*)m_pLayer->getChild(i);
							int lt = m_pLayer->getTall();
							m_pLayer->setSize(getWide(), lt + m_pWait->getTall());
							m_pWait->setPos(0, lt);
							break;
						}
					}					
				}
				else
				{
					gEngfuncs.Con_Printf("ERROR: subtitles - has childs but no lists!\n");
					setVisible(false);
					return;
				}
			}
		}
	}

	if (m_pWait)
	{
		float rest = layerpos + m_pLayer->getTall() - getTall();
		float spd = rest > 40 ? 0.5 : (rest / 40 * 0.5); spd += 0.5;	
		layerpos -= (deltatime * scroll_speed->value * spd);
	//	gEngfuncs.Con_Printf("layerpos: %f, spd: %f, deltatime: %f, cvarspeed: %f\n", layerpos, spd, deltatime, scroll_speed->value);
		if (layerpos + m_pLayer->getTall() <= getTall())
		{
			layerpos = getTall() - m_pLayer->getTall();
			m_pWait->setVisible(true);
			m_pWait->m_fBirthTime = curtime;

			// find next waiting message
			m_pWait = NULL;
			for (int i = 0; i < m_pLayer->getChildCount(); i++)
			{
				if (!m_pLayer->getChild(i)->isVisible())
				{
					m_pWait = (CSubtitleTextPanel*)m_pLayer->getChild(i);
					int lt = m_pLayer->getTall();
					m_pLayer->setSize(getWide(), lt + m_pWait->getTall());
					m_pWait->setPos(0, lt);
					break;
				}
			}
		}
		m_pLayer->setPos(0, layerpos);
	}

	lasttime = curtime;
//	Panel::paintBackground();
}