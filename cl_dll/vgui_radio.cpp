// ====================================
// Paranoia radio icon
// written by BUzer, based on valve's code
// ====================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_radio.h"
#include "..\game_shared\vgui_loadtga.h"
#include "getfont.h"

#define RADIO_FADE_TIME		0.25


int MsgShowRadioIcon(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	char *title = READ_STRING();
	float time = READ_COORD();
	int r = READ_BYTE();
	int g = READ_BYTE();
	int b = READ_BYTE();
	int a = READ_BYTE();
	
	if (gViewPort && gViewPort->m_pRadio)
		gViewPort->m_pRadio->Show(title, time, r, g, b, a);

	return 1;
}

void RadioIconInit()
{
	gEngfuncs.pfnHookUserMsg("RadioIcon", MsgShowRadioIcon);
}

void CRadioIcon::Initialize()
{
	setVisible(false);
	m_fShowTime = 0;
	m_fHideTime = 0;
	next_a = 0;
//	gEngfuncs.Con_Printf("--- initialize called!\n");
}

CRadioIcon::CRadioIcon() : Panel(0, 0, 10, 10)
{
	if( m_pSpeakerBitmap = vgui_LoadTGANoInvertAlpha("gfx/vgui/speaker4.tga" ) )
		m_pSpeakerBitmap->setColor( Color(255,255,255,1) );
	else
		gEngfuncs.Con_Printf("Cannot load gfx/vgui/speaker4.tga!\n");

//	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
//	SchemeHandle_t hTextScheme = pSchemes->getSchemeHandle( "Default Text" );
//	Font *pFont = pSchemes->getFont( hTextScheme );

	m_pIcon = new ImagePanel(m_pSpeakerBitmap);
	m_pIcon->setParent(this);
	m_pLabel = new Label("");
	m_pLabel->setParent(this);
//	m_pLabel->setFont(pFont);
	m_pLabel->setPaintBackgroundEnabled(false);
//	m_pIcon->setPaintBackgroundEnabled(false);
	Initialize();
}


CRadioIcon::~CRadioIcon()
{
	delete m_pSpeakerBitmap;
}


void CRadioIcon::Reposition()
{
	int y = ScreenHeight / 2;
	
	int iconWide = 8, iconTall = 8;
	if( m_pSpeakerBitmap )
	{
		m_pSpeakerBitmap->getSize( iconWide, iconTall );
	//	gEngfuncs.Con_Printf("speaker sizes %d, %d\n", iconWide, iconTall);
	}
	
	int textWide, textTall;
	m_pLabel->getContentSize( textWide, textTall );

	// Don't let it stretch too far across their screen.
	if( textWide > (ScreenWidth*2)/3 )
		textWide = (ScreenWidth*2)/3;

	// Setup the background label to fit everything in.
	int border = 2;
	int bgWide = textWide + iconWide + border*3;
	int bgTall = max( textTall, iconTall ) + border*2;
	setBounds( ScreenWidth - bgWide - 8, y, bgWide, bgTall );

	// Put the text at the left.
	m_pLabel->setBounds( border, (bgTall - textTall) / 2, textWide, textTall );

	// Put the icon at the right.
	int iconLeft = border + textWide + border;
	int iconTop = (bgTall - iconTall) / 2;
	m_pIcon->setBounds( iconLeft, iconTop, iconWide, iconTall );
}


void CRadioIcon::Show(const char *title, float time, int _r, int _g, int _b, int _a)
{
	float curtime = gEngfuncs.GetClientTime();

	if (curtime > m_fHideTime)
	{
		setVisible(true);
		m_fShowTime = curtime;
		m_fHideTime = curtime + time;
		char *pText = CHudTextMessage::BufferedLocaliseTextString(title);
		Font *pFont = FontFromMessage(pText);
		m_pLabel->setFont(pFont);
		m_pLabel->setText(pText);
		r = _r; g = _g; b = _b; a = _a;
		next_a = 0;
		Reposition();
	}
	else if (curtime > (m_fHideTime - RADIO_FADE_TIME))
	{
		// current title is about to go away, just queue new
		strcpy(nextTitle, title);
		nextTime = time;
		next_r = _r; next_g = _g; next_b = _b; next_a = _a;
	}
	else
	{
		// hide current and queue new
		m_fHideTime = curtime + RADIO_FADE_TIME;
		strcpy(nextTitle, title);
		nextTime = time;
		next_r = _r; next_g = _g; next_b = _b; next_a = _a;
	}
}


void CRadioIcon::paintBackground()
{
	float curtime = gEngfuncs.GetClientTime();

	if (curtime > m_fHideTime)
	{
		if (next_a)
		{
			// next title queued
			m_fShowTime = curtime;
			m_fHideTime = curtime + nextTime;
			char *pText = CHudTextMessage::BufferedLocaliseTextString(nextTitle);
			Font *pFont = FontFromMessage(pText);
			m_pLabel->setFont(pFont);
			m_pLabel->setText(pText);			
			r = next_r; g = next_g; b = next_b; a = next_a;
			next_a = 0;
			Reposition();
		}
		else
		{
			// put away from screen
			setVisible(false);
			return;
		}
	}

	if (curtime < m_fShowTime + RADIO_FADE_TIME)
	{
		float alpha = (curtime - m_fShowTime) / RADIO_FADE_TIME;
		setBgColor( r, g, b, 255 - ((float)a * alpha) );
		m_pLabel->setFgColor( 255, 255, 255, (1 - alpha) * 255 );
		m_pSpeakerBitmap->setColor( Color(255, 255, 255, (1 - alpha) * 255) );
	}
	else if (curtime > m_fHideTime - RADIO_FADE_TIME)
	{
		float alpha = (m_fHideTime - curtime) / RADIO_FADE_TIME;
		setBgColor( r, g, b, 255 - ((float)a * alpha) );
		m_pLabel->setFgColor( 255, 255, 255, (1 - alpha) * 255 );
		m_pSpeakerBitmap->setColor( Color(255, 255, 255, (1 - alpha) * 255) );
	}
	else
	{
		setBgColor( r, g, b, 255 - a );
		m_pLabel->setFgColor( 255, 255, 255, 0 );
		m_pSpeakerBitmap->setColor( Color(255, 255, 255, 0) );
	}

	Panel::paintBackground();
}