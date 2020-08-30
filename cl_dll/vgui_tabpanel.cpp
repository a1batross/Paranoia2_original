// ====================================
// Paranoia goal panel
// written by BUzer.
// ====================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_tabpanel.h"
#include "..\game_shared\vgui_loadtga.h"
#include<VGUI_TextImage.h>
#include<VGUI_LineBorder.h>
#include "getfont.h"

#define TP_FL_ENABLE	1
#define TP_FL_TITLE		2
#define TP_FL_IMAGE		4
#define TP_FL_POPUP		8

#define XSTEP XRES(5)
#define YSTEP YRES(5)

cvar_t *hidetime;

int MsgShowTabPanel(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort && gViewPort->m_pTabPanel)
		gViewPort->m_pTabPanel->LoadMessage(pszName, iSize, pbuf);

	return 1;
}

void TabPanelInit()
{
	gEngfuncs.pfnHookUserMsg("TabPanel", MsgShowTabPanel);
	hidetime = gEngfuncs.pfnRegisterVariable( "gpanel_hidetime", "20", 0 );
}


void CTabPanel::LoadMessage(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int flags = READ_BYTE();
	if (!flags)
	{
		Initialize();
		return;
	}

	m_pTitle->setVisible(false);
	m_pImage->setVisible(false);
	m_pImage->setImage(NULL);
	if (m_pBitmap) delete m_pBitmap;
	m_pBitmap = NULL;

	int popup = flags & TP_FL_POPUP;
	static char dst_buffer[4096];
	CHudTextMessage::LocaliseTextString( READ_STRING(), dst_buffer, 4096 );
	char *pText = dst_buffer;
	Font *pFont = FontFromMessage(pText);
	m_pTextPanel->setText(pText);
	m_pTextPanel->setFont( pFont );

	int ypos = YSTEP;
	if (flags & TP_FL_TITLE)
	{
		char *pt = READ_STRING();
	//	if (*pt == '#') pt++;
	//	client_textmessage_t *clmsg = TextMessageGet( pt );
		CHudTextMessage::LocaliseTextString(pt, dst_buffer, 4096 );
		pText = dst_buffer;
	//	pText = clmsg->pMessage;
		int ln = strlen(pText);
		if (pText[ln-1] == 13) pText[ln-1] = 0;
	/*	gEngfuncs.Con_Printf("SET TEXT ---[%s]--- %d\n", pText, ln);		
		for (int i = 0; i < ln; i++)
			gEngfuncs.Con_Printf("%d, ",pText[i]);
		gEngfuncs.Con_Printf("\n");*/

		pFont = FontFromMessage(pText);
		m_pTitle->setFont( pFont );
		m_pTitle->setText(pText);		
		m_pTitle->setVisible(true);
	//	m_pTitle->setContentAlignment( Label::a_west );
		int textWide, textTall;
		m_pTitle->getContentSize( textWide, textTall );
		m_pTitle->setSize(textWide, textTall);
	//	gEngfuncs.Con_Printf("%d, %d\n", textWide, textTall);
		ypos = ypos + textTall + YSTEP;
	}
	m_pBkPanel->setBounds(XSTEP, ypos, getWide() - (XSTEP*2), 16);

	if (flags & TP_FL_IMAGE)
	{
		static int resArray[] =
		{
			320, 400, 512, 640, 800,
			1024, 1152, 1280, 1600
		};

		// try to load image directly
		const char* imgname = READ_STRING();
		m_pBitmap = vgui_LoadTGA(imgname);
		if (!m_pBitmap)
		{
			//resolution based image. Should contain %d substring
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
				sprintf(imgName, imgname, resArray[resArrayIndex]);
				m_pBitmap = vgui_LoadTGA(imgName);
				resArrayIndex--;
			}
		}

		if (!m_pBitmap)
		{
			// still no image
			gEngfuncs.Con_Printf("Could not load image [%s]\n", imgname);
		}
	}

	if (m_pBitmap)
	{
		m_pBitmap->setColor(Color(255, 255, 255, 0));
		m_pImage->setImage(m_pBitmap);
		m_pImage->setFgColor(255, 255, 255, 0);
		m_pImage->setBgColor(255, 255, 255, 0);
		m_pImage->setVisible(true);
		
		int imgx, imgy;
		m_pBitmap->getSize(imgx, imgy);
		m_pTextPanel->setBounds((XSTEP*2)+imgx, YSTEP, m_pBkPanel->getWide()-((XSTEP*3)+imgx), 16 );
		m_pImage->setSize(imgx, imgy);

		int tw, th;
		m_pTextPanel->getTextImage()->getTextSizeWrapped( tw, th );
		m_pTextPanel->setSize( tw, th );

		int ymax = (imgy > th) ? imgy : th;
		m_pBkPanel->setSize( m_pBkPanel->getWide(), ymax+YSTEP*2);
	}
	else
	{
		m_pTextPanel->setBounds(XSTEP, YSTEP, m_pBkPanel->getWide()-(XSTEP*2), 16 );

		int tw, th;
		m_pTextPanel->getTextImage()->getTextSizeWrapped( tw, th );
		m_pTextPanel->setSize( tw, th );
		m_pBkPanel->setSize( m_pBkPanel->getWide(), th+YSTEP*2);
	}

	setSize( getWide(), ypos + m_pBkPanel->getTall() + YSTEP );

	m_iTextLoaded = 1;
	if (hidetime->value)
		m_fHideTime = gEngfuncs.GetClientTime() + hidetime->value;
	else
		m_fHideTime = 0;

	if (popup)
	{
		setVisible(true);
		PlaySound("common/gpanel_popup.wav", 1);
	}
	// non-popup messages are sent after initialization
}

int CTabPanel::Toggle()
{
	if (m_iTextLoaded)
	{
		m_fHideTime = 0;
		if (isVisible())
		{
			PlaySound("common/gpanel_close.wav", 1);
			setVisible(false);
		}
		else
		{
			PlaySound("common/gpanel_open.wav", 1);
			setVisible(true);
		}
	}
	return 0;
}

void CTabPanel::Initialize()
{
	setVisible(false);
	m_iTextLoaded = 0;
	m_fHideTime = 0;
	m_pTitle->setVisible(false);
	m_pImage->setVisible(false);
	m_pImage->setImage(NULL);
	if (m_pBitmap)
		delete m_pBitmap;

	m_pBitmap = NULL;
}

CTabPanel::CTabPanel() : Panel(XRES(160), YRES(10), XRES(320), 16)
{
//	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
//	SchemeHandle_t hTextScheme = pSchemes->getSchemeHandle( "Default Text" );
//	Font *pFont = pSchemes->getFont( hTextScheme );

	setBgColor(0, 0, 0, 100);
//	setBorder(new LineBorder(1, Color(176, 110, 40, 0)));

	m_pTitle = new Label("", XSTEP, YSTEP);
	m_pTitle->setParent(this);
	m_pBkPanel = new Panel;
	m_pBkPanel->setParent(this);
	m_pTextPanel = new TextPanel("", 10, 10, 10, 10);
	m_pTextPanel->setParent(m_pBkPanel);
	m_pImage = new ImagePanel(NULL);
	m_pImage->setParent(m_pBkPanel);
	m_pImage->setPos(XSTEP, YSTEP);
	m_pBitmap = NULL;	
	
	m_pTitle->setPaintBackgroundEnabled(false);
	m_pTextPanel->setPaintBackgroundEnabled(false);
//	m_pImage->setPaintBackgroundEnabled(false);

	m_pTitle->setFgColor(255, 255, 255, 0);
	m_pBkPanel->setBgColor(50, 80, 90, 100);
	m_pTextPanel->setFgColor(255, 255, 255, 0);

	Initialize();
}

CTabPanel::~CTabPanel()
{
	if (m_pBitmap)
		delete m_pBitmap;

	m_pBitmap = NULL;
}

void CTabPanel::paintBackground()
{
	float curtime = gEngfuncs.GetClientTime();
	if (m_fHideTime && curtime > m_fHideTime)
	{
		m_fHideTime = 0;
		setVisible(false);
	}

	Panel::paintBackground();
}