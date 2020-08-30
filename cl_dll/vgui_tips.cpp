// ====================================
// written by BUzer for HL: Paranoia modification
// ====================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_tips.h"
#include "vgui_shadowtext.h"
#include "..\game_shared\vgui_loadtga.h"
#include "getfont.h"

#define TIPS_APPEAR_TIME		0.15
#define TIPS_DISAPPEAR_TIME		1

#define TIPS_BASE_HEIGHT	(YRES(390))
#define TIPS_XSTEP			(XRES(20))

int errorshows[10];

void ShowTip( client_textmessage_t *tempMessage )
{
	if (gViewPort && gViewPort->m_pTips)
		gViewPort->m_pTips->Show( tempMessage );
	else
		gEngfuncs.Con_Printf("Tips error: m_pTips or ViewPort is not constructed!\n");
}


Font* FontFromMessage(const char* &ptext);

void CTips::Initialize()
{
	setVisible(false);
	m_fShowTime = 0;
	m_fHideTime = 0;

	if (m_pBitmap) delete m_pBitmap;
	m_pBitmap = NULL;
}

CTips::CTips() : Panel(0, 0, ScreenWidth, ScreenHeight)
{
	m_pBitmap = NULL;
	m_pText = new ShadowTextPanel("", 0, 0, (ScreenWidth*3)/2, ScreenHeight);
	m_pText->setParent(this);
	m_pText->setPaintBackgroundEnabled(false);
	Initialize();

	for (int i = 0; i < 10; i++)
		errorshows[i] = 0;
}


CTips::~CTips()
{
	if (m_pBitmap) delete m_pBitmap;
	m_pBitmap = NULL;
}

void CTips::Show( client_textmessage_t *tempMessage )
{
	if (isVisible())
	{
		nextMsg = tempMessage;
		m_fHideTime = gEngfuncs.GetClientTime() + TIPS_APPEAR_TIME; // use fast disappear when someone waits
		return;
	}

	nextMsg = NULL;
	setVisible(true);
	LoadMsg(tempMessage);	
}

// it assumes that
// 20 <= (msg->effect) < 30
void CTips::LoadMsg( client_textmessage_t *msg )
{
	float curtime = gEngfuncs.GetClientTime();
//	gEngfuncs.Con_Printf("loaded\n");

	m_fShowTime = curtime;
	m_fHideTime = curtime + msg->holdtime;
	const char *pText = msg->pMessage;
	Font *pFont = FontFromMessage(pText);
	m_pText->setFont(pFont);
	m_pText->setText(pText);
	m_pText->setFgColor(msg->r1, msg->g1, msg->b1, 0);

	if (m_pBitmap) delete m_pBitmap;
	m_pBitmap = NULL;

	// load image
	//   (damn, instead of copying this block of code everywhere,
	//    i really need to create a single function to load images this way...)
	int iconnum = msg->effect - 20;

	static int resArray[] =
	{
		320, 400, 512, 640, 800,
		1024, 1152, 1280, 1600
	};

	//find current resolution
	int resArrayIndex = 0;
	int i = 0;
	while ((resArray[i] <= ScreenWidth) && (i < 9))
	{
		resArrayIndex = i;
		i++;
	}

	while(m_pBitmap == NULL && resArrayIndex >= 0)
	{
		char imgName[64];
		sprintf(imgName, "gfx/vgui/icon%d_%d.tga", iconnum, resArray[resArrayIndex]);
		m_pBitmap = vgui_LoadTGA(imgName);
		resArrayIndex--;
	}
	
	int iconWide = 0, iconTall = 0;
	if (!m_pBitmap)
	{
		if (!errorshows[iconnum]) // dont show error message for same icon wice
		{
			gEngfuncs.Con_Printf("Could not load icon gfx/vgui/icon%d_...\n", iconnum);
			errorshows[iconnum] = 1;
		}
	}
	else
	{
		m_pBitmap->getSize( iconWide, iconTall );
		m_pBitmap->setPos( TIPS_XSTEP, TIPS_BASE_HEIGHT - (iconTall / 2));
		iconWide += XRES(7);
	}
	
	m_pText->setPos( TIPS_XSTEP + iconWide, TIPS_BASE_HEIGHT - (pFont->getTall() / 2));	
}


void CTips::paintBackground()
{
	float curtime = gEngfuncs.GetClientTime();

	if (curtime > m_fHideTime)
	{
		if (nextMsg)
		{
			LoadMsg(nextMsg);
			nextMsg = NULL;
		}
		else
		{
			// put away from screen
			Initialize();
			return;
		}
	}

	int mr, mg, mb, ma;
	m_pText->getFgColor(mr, mg, mb, ma);

	if (curtime < m_fShowTime + TIPS_APPEAR_TIME)
	{
		float alpha = (curtime - m_fShowTime) / TIPS_APPEAR_TIME;
		m_pText->setFgColor( mr, mg, mb, (1 - alpha) * 255 );
		if (m_pBitmap) m_pBitmap->setColor( Color(255, 255, 255, (1 - alpha) * 255) );
	}
	else if (nextMsg && (curtime > m_fHideTime - TIPS_APPEAR_TIME))
	{
		float alpha = (m_fHideTime - curtime) / TIPS_APPEAR_TIME;
		m_pText->setFgColor( mr, mg, mb, (1 - alpha) * 255 );
		if (m_pBitmap) m_pBitmap->setColor( Color(255, 255, 255, (1 - alpha) * 255) );
	}
	else if (!nextMsg && (curtime > m_fHideTime - TIPS_DISAPPEAR_TIME))
	{
		float alpha = (m_fHideTime - curtime) / TIPS_DISAPPEAR_TIME;
		m_pText->setFgColor( mr, mg, mb, (1 - alpha) * 255 );
		if (m_pBitmap) m_pBitmap->setColor( Color(255, 255, 255, (1 - alpha) * 255) );
	}
	else
	{
		m_pText->setFgColor( mr, mg, mb, 0 );
		if (m_pBitmap) m_pBitmap->setColor( Color(255, 255, 255, 0) );
	}

	if (m_pBitmap)
		m_pBitmap->doPaint(this);

//	Panel::paintBackground();
}



// неважно, куда-бы это засунуть..

#include "vgui_screenmsg.h"

void VGuiAddScreenMessage( client_textmessage_t *msg )
{
	if (gViewPort && gViewPort->m_pScreenMsg)
	{
		gViewPort->m_pScreenMsg->SetMessage( msg );
	}
	else
		gEngfuncs.Con_Printf("Screenmessage error: ViewPort is not constructed!\n");
}

// Wargon: «асуну и € это сюда. )
void VGuiAddScrollingMessage( client_textmessage_t *msg )
{
	if (gViewPort && gViewPort->m_pScrollingMsg)
		gViewPort->m_pScrollingMsg->SetMessage(msg);
	else
		gEngfuncs.Con_Printf("Scrollingmessage error: ViewPort is not constructed!\n");
}
