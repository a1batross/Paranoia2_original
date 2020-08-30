#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_paranoiatext.h"
#include "VGUI_LineBorder.h"
#include "VGUI_TextImage.h"
#include "../engine/keydefs.h"
#include "triangleapi.h"
#include "..\game_shared\vgui_LoadTGA.h"
#include "r_studioint.h"
#include "com_model.h"
#include "stringlib.h"
#include "gl_local.h"

extern engine_studio_api_t IEngineStudio;

#define WSIZE_X		XRES(400);
#define WSIZE_Y		YRES(400);

// возвращает 1, если тэг существует, и 0 в случае конца файла
// ptext останавливается на открывающей скобке тэга
int FindNextTag(char* &ptext)
{
	while (*ptext != '<')
	{
		if (*ptext == 0)
			return 0; // end of file

		ptext++;
	}

	// теперь надо убедиться, что тэг имеет закрывающую скобку
	char* hlp = ptext;
	while (*hlp != '>')
	{
		if (*hlp == 0)
			return 0;

		hlp++;
	}

	return 1;
}


// получает указатель на начало названия тэга, и доходит либо до пробела, либо
// до закрывающей скобки
// размер буфера - 32 символа
// возвращает 1, если у тэга есть параметры, и 0 - если нету
int GetTagName(char* &ptext, char* bufTagName)
{
	char* pstart = ptext;
	while(1)
	{
		if (*ptext == '>')
		{
			int length = ptext - pstart;
			if (length > 31) length = 31;

			memcpy(bufTagName, pstart, length);
			bufTagName[length] = 0;

			ptext++;
			return 0;
		}
		else if (*ptext == ' ')
		{
			int length = ptext - pstart;
			if (length > 31) length = 31;

			memcpy(bufTagName, pstart, length);
			bufTagName[length] = 0;

			return 1;
		}

		ptext++;
	}
}


// получает указатель на место сразу за именем тэга, и возвращает его параметры в буферах
// буферы должны быть размером в 32 байта
// предполагает, что конец файла наступить не должен до закрывающей скобки
int GetTagParameter(char* &ptext, char* bufParamName, char* bufParamValue)
{
	// пропускаем начальные пробелы и переносы
	while (*ptext == ' ' || *ptext == '\n')
		ptext++;

	// начинаем чтение названия параметра
	char* start = ptext;
	while (*ptext != '=')
	{
		if (*ptext == '>')
		{
			ptext++;
			return 0; // тэг кончился
		}

		ptext++;
	}

	int paramNameLength = ptext - start;
	if (paramNameLength > 31) paramNameLength = 31; // обрезать по буферу

	memcpy(bufParamName, start, paramNameLength);
	bufParamName[paramNameLength] = 0;

	// теперь читаем его значение
	ptext++; // перепрыгиваем знак равно

	if (*ptext == '\"')
	{
		// аргумент заключен в кавычки
		ptext++;
		start = ptext;
		while (1)
		{
			if (*ptext == '\"')
			{
				int paramValueLength = ptext - start;
				if (paramValueLength > 31) paramValueLength = 31;

				memcpy(bufParamValue, start, paramValueLength);
				bufParamValue[paramValueLength] = 0;

				ptext++;
				return 1;
			}
			else if (*ptext == '>')
			{
				int paramValueLength = ptext - start;
				if (paramValueLength > 31) paramValueLength = 31;

				memcpy(bufParamValue, start, paramValueLength);
				bufParamValue[paramValueLength] = 0;

				return 1;
			}
			ptext++;			
		}	
	}

	start = ptext;
	while(1)
	{
		if (*ptext == '>' || *ptext == ' ' || *ptext == '\n')
		{
			int paramValueLength = ptext - start;
			if (paramValueLength > 31) paramValueLength = 31;

			memcpy(bufParamValue, start, paramValueLength);
			bufParamValue[paramValueLength] = 0;

			return 1;
		}
		ptext++;
	}
}


void ParseColor(char* ptext, int &r, int &g, int &b, int &a)
{
	int iArray[4];
	int current = 0;
	char tmp[8];

	memset(iArray, 0, sizeof(int)*4);
	while (current < 4 && *ptext != 0)
	{
		// search for space or end of string
		char* pstart = ptext;
		while (*ptext != ' ' && *ptext != 0)
			ptext++;

		int length = ptext - pstart;
		if (length > 7) length = 7;

		memcpy(tmp, pstart, length);
		tmp[length] = 0;

		iArray[current] = atoi(tmp);
		current++;

		if (*ptext == ' ') ptext++;
	}

	r = iArray[0];
	g = iArray[1];
	b = iArray[2];
	a = iArray[3];
}






// =====================================================================

/*class CMainPanel : public Panel, public CRenderable
{
public:
	CMainPanel(const char* imgname, int x,int y,int wide,int tall) : Panel(x, y, wide, tall)
	{
		m_pBitmap = vgui_LoadTGA(imgname);
		setPaintBackgroundEnabled(false);

		if (!m_pBitmap)
			gEngfuncs.Con_Printf("Cant load image for background: [%s]\n", imgname);

		m_pBorderPanel = NULL;
	}

	~CMainPanel()
	{
		if (m_pBitmap)
			delete m_pBitmap;
	}

	void paint()
	{
		if (m_pBitmap)
		{
			int imgX, imgY;
			int panX, panY;
			m_pBitmap->getSize(imgX, imgY);
			getSize(panX, panY);
			for (int curY = 0; curY < panY; curY += imgY)
			{
				for (int curX = 0; curX < panX; curX += imgX)
				{
					m_pBitmap->setPos(curX, curY);
					m_pBitmap->doPaint(this);
				}
			}
		}
		else
		{
			drawSetColor(0, 0, 0, 100);
			drawFilledRect(0, 0, getWide(), getTall());
		}

		drawSetColor(0, 0, 0, 150);
		drawOutlinedRect(0, 0, getWide() - 1, getTall() - 1);

		drawSetColor(255, 255, 255, 150);
		drawOutlinedRect(1, 1, getWide(), getTall());

		drawSetColor(0, 0, 0, 0);
		drawOutlinedRect(0, 0, getWide(), getTall());

		// sort of hack, to draw frame around the scrollpanel
		if (m_pBorderPanel)
		{
			int x, y, wide, tall;
			m_pBorderPanel->getBounds(x, y, wide, tall);

			drawSetColor(255, 255, 255, 180);
			drawOutlinedRect(x-2, y-2, x+wide+1, y+tall+1);

			drawSetColor(0, 0, 0, 60);
			drawOutlinedRect(x-1, y-1, x+wide+2, y+tall+2);

			drawSetColor(0, 0, 0, 0);
			drawOutlinedRect(x-2, y-2, x+wide+2, y+tall+2);
		}
	}

	void SetDrawBorder(Panel *pan)
	{
		m_pBorderPanel = pan;
	}

private:
	BitmapTGA*	m_pBitmap;
	Panel*		m_pBorderPanel;
};


class CMyButton : public Button, public CRenderable
{
public:
	CMyButton(const char* text, const char* imgname, int x, int y) : Button(text, x, y)
	{
		m_pBitmap = vgui_LoadTGA(imgname);

		if (m_pBitmap)
			m_pBitmap->setPos(0,0);
		else
			gEngfuncs.Con_Printf("Cant load image for button: [%s]\n", imgname);
	}

	~CMyButton()
	{
		if (m_pBitmap)
			delete m_pBitmap;
	}

	void paintBackground()
	{
		if (m_pBitmap)
			m_pBitmap->doPaint(this);

		if (isSelected())
		{
			drawSetColor(0, 0, 0, 200);
			drawFilledRect(0, 0, getWide(), getTall());

			drawSetColor(255, 255, 255, 150);
			drawOutlinedRect(0, 0, getWide()-1, getTall()-1);

			drawSetColor(0, 0, 0, 150);
			drawOutlinedRect(1, 1, getWide(), getTall());
		}
		else
		{
			drawSetColor(0, 0, 0, 150);
			drawOutlinedRect(0, 0, getWide()-1, getTall()-1);

			drawSetColor(255, 255, 255, 150);
			drawOutlinedRect(1, 1, getWide(), getTall());
        }

		drawSetColor(0, 0, 0, 0);
		drawOutlinedRect(0, 0, getWide(), getTall());
	}

	void internalCursorExited()
	{
		setSelected(false);
	}

private:
	BitmapTGA*	m_pBitmap;
};*/

void OrthoQuad(int x1, int y1, int x2, int y2);

//==================================
// CMainPanel - главная панель окна.
//==================================
class CMainPanel : public Panel, public CRenderable
{
public:
	CMainPanel(const char* imgname, int x,int y,int wide,int tall) : Panel(x, y, wide, tall)
	{
		setPaintBackgroundEnabled(false);

		if (!IEngineStudio.IsHardware() || imgname[0] == 0)
		{
			setPaintEnabled(true);
			m_hsprImage = NULL;
			m_hBitmap = 0;
			return;
		}
		else
			setPaintEnabled(false);		

		const char *ext = UTIL_FileExtension( imgname );

		m_hBitmap = 0;

		if(( !Q_stricmp( ext, "dds" ) || !Q_stricmp( ext, "tga" )) && g_fRenderInterfaceValid )
		{
			m_hBitmap = LOAD_TEXTURE( imgname, NULL, 0, TF_IMAGE );

			if (!m_hBitmap)
			{
				gEngfuncs.Con_Printf("ERROR: Cant load image for background: [%s]\n", imgname);
				setPaintEnabled(true);
			}
			return;
		}
		
		m_hsprImage = SPR_Load(imgname);

		if (!m_hsprImage)
		{
			gEngfuncs.Con_Printf("ERROR: Cant load image for background: [%s]\n", imgname);
			setPaintEnabled(true);
			return;
		}
		
		if (SPR_Frames(m_hsprImage) != 4)
		{
			gEngfuncs.Con_Printf("ERROR: Expecting 4 frames in sprite: [%s]\n", imgname);
			m_hsprImage = 0;
			setPaintEnabled(true);
		}
	}

	void Render()
	{
		int x, y;
		getPos(x, y);

		if( m_hBitmap && g_fRenderInterfaceValid )
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );
			GL_BindTexture( GL_TEXTURE0, m_hBitmap );
			OrthoQuad( x, y, x + getWide(), y + getTall() );
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
			return;
		}

		if (!m_hsprImage)
			return;

		const struct model_s *sprmodel = gEngfuncs.GetSpritePointer(m_hsprImage);
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 0);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1 );
		OrthoQuad(x, y, x+getWide()/2, y+getTall()/2);

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 1);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1 );
		OrthoQuad(x+getWide()/2, y, x+getWide(), y+getTall()/2);

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 2);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1 );
		OrthoQuad(x, y+getTall()/2, x+getWide()/2, y+getTall());

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 3);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1 );
		OrthoQuad(x+getWide()/2, y+getTall()/2, x+getWide(), y+getTall());	

		gEngfuncs.pTriAPI->CullFace( TRI_FRONT );
	}

	// in case we didnt loaded texture..
	void paint()
	{	
		drawSetColor(0, 0, 0, 100);
		drawFilledRect(0, 0, getWide(), getTall());

		drawSetColor(255, 255, 2550, 150);
		drawOutlinedRect(0, 0, getWide(), getTall());
	
		if (m_pBorderPanel)
		{
			int x, y, wide, tall;
			m_pBorderPanel->getBounds(x, y, wide, tall);

			drawSetColor(255, 255, 255, 180);
			drawOutlinedRect(x-2, y-2, x+wide+1, y+tall+1);

			drawSetColor(0, 0, 0, 60);
			drawOutlinedRect(x-1, y-1, x+wide+2, y+tall+2);

			drawSetColor(0, 0, 0, 0);
			drawOutlinedRect(x-2, y-2, x+wide+2, y+tall+2);
		}
	}

	void SetDrawBorder(Panel *pan)
	{
		m_pBorderPanel = pan;
	}

	bool IsTgaPanel( void ) { return (m_hBitmap != 0) ? true : false; }

private:
	HSPRITE		m_hsprImage;
	int		m_hBitmap;
	Panel*		m_pBorderPanel;
};

//==================================
// CMyButton - кнопка закрытия окна
//==================================
class CMyButton : public Button, public CRenderable
{
public:
	CMyButton(const char* text, const char* imgname, int x, int y) : Button(text, x, y)
	{
		if (!IEngineStudio.IsHardware() || imgname[0] == 0)
		{
			setPaintBackgroundEnabled(true);
			m_hsprImage = NULL;
			return;
		}
		else
			setPaintBackgroundEnabled(false);
		
		m_hsprImage = SPR_Load(imgname);

		if (!m_hsprImage)
		{
			gEngfuncs.Con_Printf("ERROR: Cant load image for button: [%s]\n", imgname);
			setPaintBackgroundEnabled(true);
			return;
		}
		
		if (SPR_Frames(m_hsprImage) != 2)
		{
			gEngfuncs.Con_Printf("ERROR: Expecting 2 frames in sprite: [%s]\n", imgname);
			m_hsprImage = 0;
			setPaintBackgroundEnabled(true);
		}
	}

	void Render()
	{
		if (!m_hsprImage)
			return;

		int x, y, xparent, yparent;
		getPos(x, y);
		getParent()->getPos(xparent, yparent);
		x += xparent;
		y += yparent;

		int frame = 0;
		if (isSelected()) frame = 1;

		const struct model_s *sprmodel = gEngfuncs.GetSpritePointer(m_hsprImage);
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, frame);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1 );
		OrthoQuad(x, y, x+getWide(), y+getTall());
		gEngfuncs.pTriAPI->CullFace( TRI_FRONT );
	}

	void paintBackground()
	{
		if (isSelected())
		{
			drawSetColor(58, 37, 19, 0);
			drawFilledRect(0, 0, getWide(), getTall());

			drawSetColor(255, 255, 255, 150);
			drawOutlinedRect(0, 0, getWide()-1, getTall()-1);

			drawSetColor(0, 0, 0, 150);
			drawOutlinedRect(1, 1, getWide(), getTall());
		}
		else
		{
			drawSetColor(98, 60, 30, 0);
			drawFilledRect(0, 0, getWide(), getTall());

			drawSetColor(0, 0, 0, 150);
			drawOutlinedRect(0, 0, getWide()-1, getTall()-1);

			drawSetColor(255, 255, 255, 150);
			drawOutlinedRect(1, 1, getWide(), getTall());
        }

		drawSetColor(0, 0, 0, 0);
		drawOutlinedRect(0, 0, getWide(), getTall());
	}

	void internalCursorExited()
	{
		setSelected(false);
	}

private:
	HSPRITE		m_hsprImage;
};


//==================================
// CMySlider - ползунок прокрутки в стиле паранойи
//==================================
class CMySlider : public Slider
{
public:
	CMySlider(int x,int y,int wide,int tall,bool vertical) : Slider(x,y,wide,tall,vertical){};
	
	void paintBackground( void )
	{
		int wide,tall,nobx,noby;
		getPaintSize(wide,tall);
		getNobPos(nobx,noby);

		//background
		drawSetColor(98, 60, 30, 150);
		drawFilledRect( 0,0,wide,tall );

		// nob
		drawSetColor(98, 60, 30, 0);
		drawFilledRect( 0,nobx,wide,noby );

		drawSetColor(0, 0, 0, 150);
		drawOutlinedRect( 0,nobx,wide-1,noby-1 );

		drawSetColor(255, 255, 255, 150);
		drawOutlinedRect( 1,nobx+1,wide,noby );

		drawSetColor(0, 0, 0, 0);
		drawOutlinedRect( 0,nobx,wide,noby );
	}
};


//==================================
// CMyScrollButton - кнопка прокрутки
//==================================
class CMyScrollbutton : public Button
{
public:
	CMyScrollbutton(int up, int x, int y) : Button("", x, y, 16, 16)
	{
		if (up)
			setImage(vgui_LoadTGA("gfx/vgui/arrowup.tga"));
		else
			setImage(vgui_LoadTGA("gfx/vgui/arrowdown.tga")); 

		setPaintEnabled(true);
		setPaintBackgroundEnabled(true);
	}

	void paintBackground()
	{
		if (isSelected())
		{
			drawSetColor(58, 37, 19, 0);
			drawFilledRect(0, 0, getWide(), getTall());

			drawSetColor(255, 255, 255, 150);
			drawOutlinedRect(0, 0, getWide()-1, getTall()-1);

			drawSetColor(0, 0, 0, 150);
			drawOutlinedRect(1, 1, getWide(), getTall());
		}
		else
		{
			drawSetColor(98, 60, 30, 0);
			drawFilledRect(0, 0, getWide(), getTall());

			drawSetColor(0, 0, 0, 150);
			drawOutlinedRect(0, 0, getWide()-1, getTall()-1);

			drawSetColor(255, 255, 255, 150);
			drawOutlinedRect(1, 1, getWide(), getTall());
        }

		drawSetColor(0, 0, 0, 0);
		drawOutlinedRect(0, 0, getWide(), getTall());
	}

	void internalCursorExited()
	{
		setSelected(false);
	}
};


//==================================
// CMyScrollPanel - прокручиваемая панель
//==================================
class CMyScrollPanel : public ScrollPanel, public CRenderable
{
public:
	CMyScrollPanel(const char* imgname, int x,int y,int wide,int tall) : ScrollPanel(x, y, wide, tall)
	{
		ScrollBar *pScrollBar = getVerticalScrollBar();
		pScrollBar->setButton( new CMyScrollbutton( 1, 0,0 ), 0 );
		pScrollBar->setButton( new CMyScrollbutton( 0, 0,0 ), 1 );
		pScrollBar->setSlider( new CMySlider(0,wide-1,wide,(tall-(wide*2))+2,true) ); 
		pScrollBar->setPaintBorderEnabled(false);
		pScrollBar->setPaintBackgroundEnabled(false);
		pScrollBar->setPaintEnabled(false);
			
		setPaintBackgroundEnabled(false);
		setPaintEnabled(false);
		
		if (!IEngineStudio.IsHardware() || imgname[0] == 0)
		{			
			m_hsprImage = NULL;
		//	setPaintBackgroundEnabled(true);
			return;
		}
				
		m_hsprImage = SPR_Load(imgname);

		if (!m_hsprImage)
		{
			gEngfuncs.Con_Printf("ERROR: Cant load image for scrollpanel: [%s]\n", imgname);
		}
	}

	void Render()
	{
		if (!m_hsprImage)
			return;

		int x, y, xparent, yparent;
		getPos(x, y);
		getParent()->getPos(xparent, yparent);
		x += xparent;
		y += yparent;

		const struct model_s *sprmodel = gEngfuncs.GetSpritePointer(m_hsprImage);
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 0);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1 );
		OrthoQuad(x, y, x+getWide(), y+getTall());
		gEngfuncs.pTriAPI->CullFace( TRI_FRONT );
	}

/*	void	paint()
	{
		drawSetColor(255, 255, 255, 180);
		drawOutlinedRect(0, 0, getWide(), getTall());
	}*/

private:
	HSPRITE		m_hsprImage;
};




// ==============================================================


void CParanoiaTextPanel::BuildErrorPanel(const char* errorString)
{
		CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
		SchemeHandle_t hTextScheme = pSchemes->getSchemeHandle( "Default Text" );
		Font *pNormalFont = pSchemes->getFont( hTextScheme );

		Panel* panel = new Panel(XRES(120), YRES(180), XRES(400), YRES(120));
		panel->setParent(this);
		panel->setBgColor(0, 0, 0, 100);
		panel->setBorder(new LineBorder);

		int butX, butY;
		Button* button = new Button("     OK     ", 0, 0);
		button->setParent(panel);
		button->addActionSignal(this);
		button->getSize(butX, butY);
		butX = (panel->getWide() - butX) / 2;
		butY = panel->getTall() - butY - YRES(10);
		button->setPos( butX, butY );

		int labelpos = (butY - (pNormalFont->getTall() + YRES(8))) / 2;
		Label* label = new Label("", XRES(10), labelpos, panel->getWide() - XRES(20), pNormalFont->getTall() + YRES(8));
		label->setParent(panel);
		label->setFont(pNormalFont);
		label->setPaintBackgroundEnabled(false);
		label->setFgColor(255, 255, 255, 0);
		label->setContentAlignment( Label::a_center );
		label->setText( errorString );

		return;
}


CParanoiaTextPanel::CParanoiaTextPanel(char* filename) : Panel(0, 0, ScreenWidth, ScreenHeight)
{
	strcpy(m_loadedFileName, filename); // remember file name
	m_iRenderElms = 0;
	panel = NULL;
	
	setVisible(true);
	setPaintBackgroundEnabled(false);
	gViewPort->UpdateCursorState();

	int fileLength;
	char* pFile = (char*)gEngfuncs.COM_LoadFile( filename, 5, &fileLength );
	if (!pFile)
	{
		char buf[128];
		sprintf(buf, "Unable to load file %s", filename);
		BuildErrorPanel(buf);
		return;
	}


	char* ptext = pFile;
	char tagName[32];

	if (!FindNextTag(ptext))
	{
		char buf[128];
		sprintf(buf, "%s - empty file", filename);
		BuildErrorPanel(buf);
		return;
	}

	
	int size_x = WSIZE_X;
	int size_y = WSIZE_Y;
	int upperBound = YRES(10);
	int lowerBound;

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTextScheme = pSchemes->getSchemeHandle( "Default Text" );
//	SchemeHandle_t hTextScheme = pSchemes->getSchemeHandle( "Title Font" );
	Font *pDefaultFont = pSchemes->getFont( hTextScheme );

	ptext++;
	int hasParams = GetTagName(ptext, tagName);

	char panelImage[32];
	char buttonImage[32];
	char scrollImage[32];
	panelImage[0] = 0;
	buttonImage[0] = 0;
	scrollImage[0] = 0;
	int butclr_r = 255, butclr_g = 255, butclr_b = 255, butclr_a = 0;
	int scroll_x = 0, scroll_y = 0, scroll_wide = 0, scroll_tall = 0;

	// parse HEAD section
	if(!strcmp(tagName, "HEAD"))
	{
		if (hasParams)
		{
			char paramName[32];
			char paramValue[32];

			while( GetTagParameter(ptext, paramName, paramValue) )
			{
				if (!strcmp(paramName, "xsize"))
					size_x = XRES(atoi(paramValue));
				else if (!strcmp(paramName, "ysize"))
					size_y = YRES(atoi(paramValue));
				else if (!strcmp(paramName, "defaultfont"))
				{
					hTextScheme = pSchemes->getSchemeHandle( paramValue );
					pDefaultFont = pSchemes->getFont( hTextScheme );
				}
				else if (!strcmp(paramName, "background"))
					strcpy(panelImage, paramValue);
				else if (!strcmp(paramName, "imgscroll"))
					strcpy(scrollImage, paramValue);
				else if (!strcmp(paramName, "imgbutton"))
					strcpy(buttonImage, paramValue);
				else if (!strcmp(paramName, "buttoncolor"))
					ParseColor(paramValue, butclr_r, butclr_g, butclr_b, butclr_a);
				else if (!strcmp(paramName, "scrollpos"))
					ParseColor(paramValue, scroll_x, scroll_y, scroll_wide, scroll_tall);
				else
					gEngfuncs.Con_Printf("File %s - unknown HEAD parameter: [%s]\n", filename, paramName);
			}
		}

		if (!FindNextTag(ptext))
		{
			char buf[128];
			sprintf(buf, "%s - got nothing, except HEAD", filename);
			BuildErrorPanel(buf);
			return;
		}

		ptext++;
		hasParams = GetTagName(ptext, tagName);
	}

	// create main panel
	panel = new CMainPanel(panelImage, (getWide()-size_x)/2, (getTall()-size_y)/2, size_x, size_y);
	panel->setParent(this);
	AddToRenderList(panel);
//	panel->setBgColor(0, 0, 0, 100);
//	panel->setBorder(new LineBorder);
	ResetBackground();

	// create closing button
	int butX, butY;
	CMyButton* button = new CMyButton("", buttonImage, 0, 0);
	button->setParent(panel);
	button->setFont(pDefaultFont);
	button->setText("   Close   ");
	AddToRenderList(button);
	button->addActionSignal(this);	
	button->getSize(butX, butY);
	button->setFgColor(butclr_r, butclr_g, butclr_b, butclr_a);
	butX = panel->getWide() - butX - XRES(10);
	butY = panel->getTall() - butY - YRES(10);
	button->setPos( butX, butY );
	lowerBound = butY - YRES(10);

/*	CCheckButton2* pSwitch = new CCheckButton2();
	pSwitch->setParent(panel);
	pSwitch->SetImages("gfx/vgui/checked.tga", "gfx/vgui/unchecked.tga");
	pSwitch->SetText("Dont draw world");
	pSwitch->setPos(XRES(10), butY);
	pSwitch->SetCheckboxLeft(true);
	pSwitch->SetChecked(g_DontDrawWorld ? true : false);
	pSwitch->SetHandler(this);*/

	// parse TITLE section
	if(!strcmp(tagName, "TITLE"))
	{
		Label* pTitle = new Label("", XRES(10), upperBound, panel->getWide() - XRES(20), pDefaultFont->getTall()+YRES(8));
		pTitle->setParent(panel);
		pTitle->setPaintBackgroundEnabled(false);
		pTitle->setFont(pDefaultFont); // default font
		pTitle->setFgColor(255, 255, 255, 0); // default color
		pTitle->setContentAlignment( Label::a_center ); // default alignment

		if (hasParams)
		{
			char paramName[32];
			char paramValue[32];

			while( GetTagParameter(ptext, paramName, paramValue) )
			{
				if (!strcmp(paramName, "font"))
				{
					SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle(paramValue);
					Font* pTitleFont = pSchemes->getFont( hTitleScheme );
					pTitle->setFont(pTitleFont);
					pTitle->setSize(panel->getWide() - XRES(20), pTitleFont->getTall()+YRES(8));
				}					
				else if (!strcmp(paramName, "color"))
				{
					int r, g, b, a;
					ParseColor(paramValue, r, g, b, a);
					pTitle->setFgColor(r, g, b, a);
				}					
				else if (!strcmp(paramName, "align"))
				{
					if (!strcmp(paramValue, "left"))
						pTitle->setContentAlignment( Label::a_west );
					else if (!strcmp(paramValue, "right"))
						pTitle->setContentAlignment( Label::a_east );
					else if (!strcmp(paramValue, "center"))
						pTitle->setContentAlignment( Label::a_center );
					else
						gEngfuncs.Con_Printf("File %s - unknown align value: [%s]\n", filename, paramValue);
				}
				else
					gEngfuncs.Con_Printf("File %s - unknown TITLE parameter: [%s]\n", filename, paramName);
			}
		}

		upperBound = YRES(20) + pTitle->getTall();

		char* titleStart = ptext;
		int haveNextTag = FindNextTag(ptext);
		int titleLength = ptext - titleStart + 1;
		pTitle->setText(titleLength, titleStart);

		if (!haveNextTag)
			return;

		ptext++;
		hasParams = GetTagName(ptext, tagName);
	}


	// scroll panel begins
	if (!scroll_wide)
	{
		scroll_wide = size_x * 0.8;
		scroll_tall = size_y * 0.6;
		scroll_x = (size_x - scroll_wide) / 2;
		scroll_y = (size_y - scroll_tall) / 2;
	}
	else
	{
		scroll_wide = XRES(scroll_wide);
		scroll_tall = YRES(scroll_tall);
		scroll_x = XRES(scroll_x);
		scroll_y = YRES(scroll_y);
	}

	button->setPos( scroll_x, scroll_y + scroll_tall + YRES(15) );
//	m_pScrollPanel = new CMyScrollPanel(scrollImage, XRES(40), upperBound, panel->getWide()-XRES(80), lowerBound - upperBound );
	m_pScrollPanel = new CMyScrollPanel(scrollImage, scroll_x, scroll_y, scroll_wide, scroll_tall );
	m_pScrollPanel->setParent(panel);
	AddToRenderList(m_pScrollPanel);
	panel->SetDrawBorder(m_pScrollPanel);
//	pScrollPanel->setBorder( new LineBorder(Color(0,0,0,0)) );
	m_pScrollPanel->setScrollBarAutoVisible(true, true);
	m_pScrollPanel->setScrollBarVisible(false, false);
	m_pScrollPanel->validate();

	// including panel
	int panelsize = YRES(5);
	Panel* pDocument = new Panel(0, 0, m_pScrollPanel->getClientClip()->getWide(), 64);
	pDocument->setParent(m_pScrollPanel->getClient());
	pDocument->setPaintBackgroundEnabled(false);
//	pDocument->setBgColor(0, 0, 0, 100);

	// reading document elements
	while(1)
	{
		// parse text field
		if(!strcmp(tagName, "TEXT"))
		{
			TextPanel* pTextPanel = new TextPanel("", XRES(5), panelsize, pDocument->getWide() - XRES(10), 64);
			pTextPanel->setParent(pDocument);
			pTextPanel->setPaintBackgroundEnabled(false);
			pTextPanel->setFont(pDefaultFont); // default font
			pTextPanel->setFgColor(255, 255, 255, 0); // default color
			int dspace = 0;

			bool bgColorSet = false;

			if (hasParams)
			{
				char paramName[32];
				char paramValue[32];

				while( GetTagParameter(ptext, paramName, paramValue) )
				{
					if (!strcmp(paramName, "font"))
					{
						SchemeHandle_t hTextScheme = pSchemes->getSchemeHandle(paramValue);
						Font* pTextFont = pSchemes->getFont( hTextScheme );
						pTextPanel->setFont(pTextFont);
					}
					else if (!strcmp(paramName, "color"))
					{
						int r, g, b, a;
						ParseColor(paramValue, r, g, b, a);
						pTextPanel->setFgColor(r, g, b, a);
					}
					else if (!strcmp(paramName, "bgcolor"))
					{
						int r, g, b, a;
						ParseColor(paramValue, r, g, b, a);
						pTextPanel->setPaintBackgroundEnabled(true);
						pTextPanel->setBgColor(r, g, b, a);
						bgColorSet = true;
					}
					else if (!strcmp(paramName, "lspace"))
					{
						int tmp_posx, tmp_posy, tmp_sizex, tmp_sizey;
						pTextPanel->getBounds(tmp_posx, tmp_posy, tmp_sizex, tmp_sizey);
						int ofs = XRES(atoi(paramValue));
						pTextPanel->setBounds(tmp_posx + ofs, tmp_posy, tmp_sizex - ofs, tmp_sizey);
					}
					else if (!strcmp(paramName, "rspace"))
					{
						int tmp_sizex, tmp_sizey;
						pTextPanel->getSize(tmp_sizex, tmp_sizey);
						int ofs = XRES(atoi(paramValue));
						pTextPanel->setSize(tmp_sizex - ofs, tmp_sizey);
					}
					else if (!strcmp(paramName, "dspace"))
					{
						dspace = YRES(atoi(paramValue));
					}
					else if (!strcmp(paramName, "uspace"))
					{
						int x, y;
						pTextPanel->getPos(x, y);
						int uspace = YRES(atoi(paramValue));
						y += uspace;
						panelsize += uspace;
						pTextPanel->setPos(x, y);
					}
					else
						gEngfuncs.Con_Printf("File %s - unknown TEXT parameter: [%s]\n", filename, paramName);
				}
			}

			char* textStart = ptext;
			int haveNextTag = FindNextTag(ptext);
			int textLength = ptext - textStart + 1;
			pTextPanel->getTextImage()->setText(textLength, textStart);
			pTextPanel->getTextImage()->setSize( pTextPanel->getWide(), pTextPanel->getTall() );

			int wrappedX, wrappedY;
			pTextPanel->getTextImage()->getTextSizeWrapped(wrappedX, wrappedY);

			if (bgColorSet)
			{
				int x, y, realY, unused;
				pTextPanel->setSize( m_pScrollPanel->getClientClip()->getWide() , wrappedY );
				pTextPanel->getTextImage()->setSize( wrappedX , wrappedY );
				pTextPanel->getPos(x, realY);
				pTextPanel->getTextImage()->getPos(unused, y);
				pTextPanel->getTextImage()->setPos(x, y);
				pTextPanel->setPos(0, realY);
			}
			else
				pTextPanel->setSize( wrappedX , wrappedY );

			panelsize += (wrappedY + dspace);

			if (!haveNextTag)
				break; // no more tags

			ptext++;
				hasParams = GetTagName(ptext, tagName);
		}

		// parse image parameters
		else if(!strcmp(tagName, "IMG"))
		{
			if (hasParams)
			{
				char paramName[32];
				char paramValue[32];
				BitmapTGA* pImage = NULL;
				int uspace = 0;
				int dspace = 0;
				int align = 0; // 0 - center, 1 - left, 2 - right

				while( GetTagParameter(ptext, paramName, paramValue) )
				{
					if (!strcmp(paramName, "src"))
					{
						static int resArray[] =
						{
							320, 400, 512, 640, 800,
							1024, 1152, 1280, 1600
						};

						if (pImage)
						{
							gEngfuncs.Con_Printf("File %s - ignoring [src] parameter - already has an image\n", filename);
							continue;
						}

						// try to load image directly
						pImage = vgui_LoadTGA(paramValue);

						if (!pImage)
						{
							//resolution based image.
							// should contain %d substring
							int resArrayIndex = 0;
							int i = 0;
							while ((resArray[i] <= ScreenWidth) && (i < 9))
							{
								resArrayIndex = i;
								i++;
							}

							while(pImage == NULL && resArrayIndex >= 0)
							{
								char imgName[64];
								sprintf(imgName, paramValue, resArray[resArrayIndex]);
							//	gEngfuncs.Con_Printf("=== trying to load: %s\n", imgName);
								pImage = vgui_LoadTGA(imgName);
								resArrayIndex--;
							}
						}

						if (!pImage)
						{
							// still no image
							gEngfuncs.Con_Printf("File %s - image not loaded: [%s]\n", filename, paramValue);
						}
					}
					else if (!strcmp(paramName, "align"))
					{
						if (!strcmp(paramValue, "left"))
							align = 1;
						else if (!strcmp(paramValue, "right"))
							align = 2;
						else if (!strcmp(paramValue, "center"))
							align = 3;
						else
							gEngfuncs.Con_Printf("File %s - unknown align value: [%s]\n", filename, paramValue);
					}
					else if (!strcmp(paramName, "uspace"))
						uspace = YRES(atoi(paramValue));
					else if (!strcmp(paramName, "dspace"))
						dspace = YRES(atoi(paramValue));
					else
						gEngfuncs.Con_Printf("File %s - unknown IMG parameter: [%s]\n", filename, paramName);
				}

				// create image panel
				if (pImage)
				{
					int tmp_x, tmp_y;
					pImage->getSize(tmp_x, tmp_y);

					switch (align)
					{
					case 0: tmp_x = (pDocument->getWide() - tmp_x) / 2; break;
					case 1: default: tmp_x = 0; break;
					case 2: tmp_x = pDocument->getWide() - tmp_x; break;
					}

					Label *pImg = new Label("", tmp_x, panelsize + uspace);
					pImg->setParent(pDocument);
					pImg->setImage(pImage);
					pImg->setPaintBackgroundEnabled(false);

					panelsize = panelsize + uspace + tmp_y + dspace;
				}
			}
			else
			{
				gEngfuncs.Con_Printf("File %s - IMG with no parameters\n", filename);
			}

			if (!FindNextTag(ptext))
				break;

			ptext++;
			hasParams = GetTagName(ptext, tagName);
		}

		// unknown tag
		else
		{
			gEngfuncs.Con_Printf("File %s - unknown tag: [%s]\n", filename, tagName);

			if (!FindNextTag(ptext))
				break; // no more tags

			ptext++;
				hasParams = GetTagName(ptext, tagName);
		}
	}

	gEngfuncs.COM_FreeFile(pFile);

	// document is ready, panelsize now contains the height of panel
	panelsize += YRES(5);
	int doc_x = pDocument->getWide();
	pDocument->setSize(doc_x, panelsize);
	m_pScrollPanel->validate();

	m_iMaxScrollValue = panelsize - m_pScrollPanel->getClientClip()->getTall();
}


void CParanoiaTextPanel::paint()
{
	if (panel && !panel->IsTgaPanel( ))
	{
		float curtime = gEngfuncs.GetClientTime();
		float delta = (curtime - m_flStartTime) / 0.5;
		if (delta > 1) delta = 1;

		int x, y, wide, tall;
		panel->getBounds(x, y, wide, tall);
		drawSetColor(0, 0, 0, 255 - (int)(delta * 120));

		drawFilledRect(0, 0, getWide(), y);
		drawFilledRect(0, y+tall, getWide(), getTall());
		drawFilledRect(0, y, x, y+tall);
		drawFilledRect(x+wide, y, getWide(), y+tall);
	}
}


void CParanoiaTextPanel::actionPerformed(Panel* panel)
{
	CloseWindow();
}


// return 0 to hook key
// return 1 to allow key
int CParanoiaTextPanel::KeyInput(int down, int keynum, const char *pszCurrentBinding)
{
	if (!down)
		return 1; // dont disable releasing of the keys

	switch (keynum)
	{
	// close window
	case K_ENTER:
	case K_ESCAPE:
	{
		CloseWindow();
		return 0;
	}

	// mouse and arrows key scroll
	case K_MWHEELUP:
	case K_UPARROW:
	case K_KP_UPARROW:
	{
		int hor, ver;
		m_pScrollPanel->getScrollValue(hor, ver);
		ver -= YRES(30);
		if (ver < 0) ver = 0;
		m_pScrollPanel->setScrollValue(hor, ver);
		return 0;
	}

	case K_MWHEELDOWN:
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
	{
		int hor, ver;
		m_pScrollPanel->getScrollValue(hor, ver);
		ver += YRES(30);
		if (ver > m_iMaxScrollValue) ver = m_iMaxScrollValue;
		m_pScrollPanel->setScrollValue(hor, ver);
		return 0;
	}

	// keyboard page listing
	case K_HOME:
	case K_KP_HOME:
	{
		int hor, ver;
		m_pScrollPanel->getScrollValue(hor, ver);
		m_pScrollPanel->setScrollValue(hor, 0);
		return 0;
	}

	case K_END:
	case K_KP_END:
	{
		int hor, ver;
		m_pScrollPanel->getScrollValue(hor, ver);
		m_pScrollPanel->setScrollValue(hor, m_iMaxScrollValue);
		return 0;
	}

	case K_PGDN:
	case K_KP_PGDN:
	{
		int hor, ver;
		m_pScrollPanel->getScrollValue(hor, ver);
		ver += m_pScrollPanel->getClientClip()->getTall();
		if (ver > m_iMaxScrollValue) ver = m_iMaxScrollValue;
		m_pScrollPanel->setScrollValue(hor, ver);
		return 0;
	}

	case K_PGUP:
	case K_KP_PGUP:
	{
		int hor, ver;
		m_pScrollPanel->getScrollValue(hor, ver);
		ver -= m_pScrollPanel->getClientClip()->getTall();
		if (ver < 0) ver = 0;
		m_pScrollPanel->setScrollValue(hor, ver);
		return 0;
	}

	// Wargon: Консоль, функциональные клавиши и кнопки мыши пропускаются.
	case 96:
	case 126:
	case K_F1:
	case K_F2:
	case K_F3:
	case K_F4:
	case K_F5:
	case K_F6:
	case K_F7:
	case K_F8:
	case K_F9:
	case K_F10:
	case K_F11:
	case K_F12:
	case K_MOUSE1:
	case K_MOUSE2:
	case K_MOUSE3:
	case K_MOUSE4:
	case K_MOUSE5:
		return 1;
	}

	// Wargon: Все остальные клавиши блокируются.
	return 0;
}


void CParanoiaTextPanel::CloseWindow()
{
	setVisible(false);
	gViewPort->UpdateCursorState();
}


void CParanoiaTextPanel::ResetBackground()
{
	m_flStartTime = gEngfuncs.GetClientTime();
}

void CParanoiaTextPanel::AddToRenderList(CRenderable* pnew)
{
	if (m_iRenderElms < 4)
	{
		m_pRenderList[m_iRenderElms] = pnew;
		m_iRenderElms++;
	}
	else
		gEngfuncs.Con_Printf("ERROR: too many renderable objects!\n");
}


void OrthoVGUI(void)
{
	if (gViewPort && gViewPort->m_pParanoiaText && gViewPort->m_pParanoiaText->isVisible())
		gViewPort->m_pParanoiaText->Render();
}



//void CParanoiaTextPanel::StateChanged(CCheckButton2 *pButton)
//{
//	g_DontDrawWorld = !g_DontDrawWorld;
//}
