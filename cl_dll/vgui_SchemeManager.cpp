//=============================================================================
// new Scheme Manager class
// rewritten by BUzer for Half-life: Paranoia modification
// textscheme.txt files are not used
//
// based on original valve's code
//=============================================================================

#include "hud.h"
#include "cl_util.h" // buz
#include "vgui_SchemeManager.h"
#include "VGUI_Font.h"
#include "cvardef.h"

#include <string.h>


typedef struct tgaheader_s
{
	unsigned char	IdLength;
	unsigned char	ColorMap;
	unsigned char	DataType;
	unsigned char	unused[5];
	unsigned short	OriginX;
	unsigned short	OriginY;
	unsigned short	Width;
	unsigned short	Height;
	unsigned char	BPP;
	unsigned char	Description;
} tgaheader_t;

class CConstFont : public vgui::Font
{
public:
	CConstFont(const char* name,void *pFileData,int fileDataLen, int tall,int wide,float rotation,int weight,bool italic,bool underline,bool strikeout,bool symbol) :
	  Font(name, pFileData, fileDataLen, tall, wide, rotation, weight, italic, underline, strikeout, symbol)
	{
		tgaheader_t* head = (tgaheader_t*)pFileData;
		m_iCharWidth = head->Width / 256;
	}

	virtual void getCharABCwide(int ch,int& a,int& b,int& c)
	{
		a = 0;
		b = m_iCharWidth;
		c = 0;
	}

private:
	int	m_iCharWidth;
};



class CWidthFont : public vgui::Font
{
public:
	CWidthFont(const char* name,void *pFileData, int fileDataLen, void *pWidthData, int tall,int wide,float rotation,int weight,bool italic,bool underline,bool strikeout,bool symbol) :
	Font(name, pFileData, fileDataLen, tall, wide, rotation, weight, italic, underline, strikeout, symbol)
	{
		memcpy(widths, pWidthData, sizeof(widths));
	}

	virtual void getCharABCwide(int ch,int& a,int& b,int& c)
	{
		signed char ch2;
		unsigned char *ch3;
		ch2 = ch;
		ch3 = (unsigned char*)&ch2;

		myABC *pABC = &widths[*ch3];
		a = pABC->a;
		b = pABC->b;
		c = pABC->c;
	}

private:
	typedef struct _myABC {
		signed char a;
		unsigned char b;
		signed char c;
	} myABC;

	myABC widths[256];
};


void Scheme_Init()
{
//	g_CV_BitmapFonts = gEngfuncs.pfnRegisterVariable("bitmapfonts", "1", 0);
}



//-----------------------------------------------------------------------------
// Purpose: initializes the scheme manager
//			loading the scheme files for the current resolution
// Input  : xRes - 
//			yRes - dimensions of output window
//-----------------------------------------------------------------------------
CSchemeManager::CSchemeManager( int xRes, int yRes )
{
	// basic setup
	m_iNumSchemes = 0;
}



SchemeHandle_t CSchemeManager::LoadScheme( const char *schemeName )
{
	int fontFileLength = -1;
	char fontFilename[512];
	void *pFontData = NULL;

	if (m_iNumSchemes == MAX_SCHEMES)
	{
		ALERT( at_error, "ERROR: too many schemes!\n" );
		return 0;
	}

	strcpy( m_Schemes[m_iNumSchemes].schemeName, schemeName );

	static int resArray[] =
	{
		320, 400, 512, 640, 800,
		1024, 1152, 1280, 1600
	};

	int resArrayIndex = 0;
	int i2 = 0;
	while ((resArray[i2] <= ScreenWidth) && (i2 < 9))
	{
		resArrayIndex = i2;
		i2++;
	}

	while(pFontData == NULL && resArrayIndex >= 0)
	{
		sprintf(fontFilename, "gfx\\vgui\\fonts\\%d_%s.tga", resArray[resArrayIndex], schemeName);
		pFontData = gEngfuncs.COM_LoadFile( fontFilename, 5, &fontFileLength );
	//	gEngfuncs.Con_Printf("=== trying to load: %s\n", imgName);
		resArrayIndex--;
	}
			
	if(!pFontData)
	{
		ALERT( at_aiconsole, "Failed to load bitmap font for scheme \"%s\"\n", schemeName);
		m_Schemes[m_iNumSchemes].font = new vgui::Font("Arial", 20, 0, 0, 700, false, false, false, false);
	}
	else
	{
		int wdataFileLength = -1;
		void *pWidthData = NULL;

		fontFilename[strlen(fontFilename) - 4] = 0;
		strcat(fontFilename, ".chw");
		pWidthData = gEngfuncs.COM_LoadFile( fontFilename, 5, &wdataFileLength );
		fontFilename[strlen(fontFilename) - 4] = 0;
		strcat(fontFilename, ".tga");
		if (pWidthData && wdataFileLength == 768)
		{
			ALERT( at_aiconsole, "Loading bitmap font \"%s\" using width info file\n", fontFilename );
			m_Schemes[m_iNumSchemes].font = new CWidthFont("Arial", pFontData, fontFileLength, pWidthData,
				20,	0, 0, 700, false, false, false, false);			
		}
		else
		{
			if (pWidthData && wdataFileLength != 768)
				ALERT( at_error, "ERROR: Bogus width info file for font \"%s\"\n", fontFilename);
			else
				ALERT( at_aiconsole, "Loading bitmap font \"%s\"\n", fontFilename );

			m_Schemes[m_iNumSchemes].font = new CConstFont("Arial", pFontData, fontFileLength,
				20,	0, 0, 700, false, false, false, false);
		}

		if (pWidthData)
			gEngfuncs.COM_FreeFile(pWidthData);
	}

	m_iNumSchemes++;
	return (m_iNumSchemes - 1);
}

//-----------------------------------------------------------------------------
// Purpose: frees all the memory used by the scheme manager
//-----------------------------------------------------------------------------
CSchemeManager::~CSchemeManager()
{
	m_iNumSchemes = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a scheme in the list, by name
// Input  : char *schemeName - string name of the scheme
// Output : SchemeHandle_t handle to the scheme
//-----------------------------------------------------------------------------
SchemeHandle_t CSchemeManager::getSchemeHandle( const char *schemeName )
{
	// iterate through the list
	for ( int i = 0; i < m_iNumSchemes; i++ )
	{
		if ( !stricmp(schemeName, m_Schemes[i].schemeName) )
			return i;
	}

	return LoadScheme( schemeName );
}

//-----------------------------------------------------------------------------
// Purpose: always returns a valid scheme handle
// Input  : schemeHandle - 
// Output : CScheme
//-----------------------------------------------------------------------------
CSchemeManager::CScheme *CSchemeManager::getSafeScheme( SchemeHandle_t schemeHandle )
{
	if ( schemeHandle < m_iNumSchemes )
		return &m_Schemes[schemeHandle];

	return m_Schemes;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the schemes pointer to a font
// Input  : schemeHandle - 
// Output : vgui::Font
//-----------------------------------------------------------------------------
vgui::Font *CSchemeManager::getFont( SchemeHandle_t schemeHandle )
{
	return getSafeScheme( schemeHandle )->font;
}


// buz: stubs
void CSchemeManager::getFgColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a )
{
	r = 255; g = 255; b = 255; a = 0;
}

void CSchemeManager::getBgColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a )
{
	r = 0; g = 0; b = 0; a = 0;
}

void CSchemeManager::getFgArmedColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a )
{
	r = 255; g = 255; b = 255; a = 0;
}

void CSchemeManager::getBgArmedColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a )
{
	r = 0; g = 0; b = 0; a = 0;
}

void CSchemeManager::getFgMousedownColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a )
{
	r = 255; g = 255; b = 255; a = 0;
}

void CSchemeManager::getBgMousedownColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a )
{
	r = 255; g = 255; b = 255; a = 0;
}

void CSchemeManager::getBorderColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a )
{
	r = 255; g = 255; b = 255; a = 0;
}



