// ====================================
// Paranoia gamma table viewer
// written by BUzer.
// ====================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_gamma.h"
//#include "..\game_shared\vgui_loadtga.h"
//#include<VGUI_TextImage.h>

#include "com_model.h"

// gamma functions
//void ApplyGammaCorrection ( byte *pData, int count );


CGammaView::CGammaView() : Panel(XRES(620)-256, YRES(400)-256, XRES(10)+256, YRES(10)+256)
{
	setBgColor(0, 0, 0, 100);
	setVisible(false);
	setPaintEnabled(true);
}

CGammaView::~CGammaView()
{

}

void CGammaView::paint()
{	
	int i;
	byte buf[256];
	for (i = 0; i < 256; i++)
		buf[i] = i;

//	ApplyGammaCorrection( buf, 256 );

	for (i = 0; i < 256; i++)
	{
		drawSetColor(255-i, 255-i, 255-i, 0);
		drawFilledRect(0, i, XRES(10), i+1);
	}
	for (i = 0; i < 256; i++)
	{
		drawSetColor(i, i, i, 0);
		drawFilledRect(XRES(10) + i, 256, XRES(10) + i + 1, 256 + YRES(10));
	}

	drawSetColor(255, 255, 255, 0);
	for (i = 0; i < 256; i++)
	{
		drawFilledRect(XRES(10) + i, 256 - buf[i], XRES(10) + i + 1, 256 - buf[i] + 1);
	}
}

void GraphEnable()
{
	if (gViewPort)
		gViewPort->m_pGamma->setVisible(true);
}

void GraphDisable()
{
	if (gViewPort)
		gViewPort->m_pGamma->setVisible(false);
}

void GammaGraphInit()
{
	gEngfuncs.pfnAddCommand ("+gammagraph",GraphEnable);
	gEngfuncs.pfnAddCommand ("-gammagraph",GraphDisable);
}