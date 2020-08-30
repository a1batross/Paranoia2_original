//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <VGUI_Font.h>

#define MAX_SCHEMES 64

// handle to an individual scheme
typedef int SchemeHandle_t;


// Register console variables, etc..
void Scheme_Init();


//-----------------------------------------------------------------------------
// Purpose: Handles the loading of text scheme description from disk
//			supports different font/color/size schemes at different resolutions 
//-----------------------------------------------------------------------------
class CSchemeManager
{
public:
	// initialization
	CSchemeManager( int xRes, int yRes );
	virtual ~CSchemeManager();

	// scheme handling
	SchemeHandle_t getSchemeHandle( const char *schemeName );

	// getting info from schemes
	vgui::Font *getFont( SchemeHandle_t schemeHandle );
	void getFgColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a );
	void getBgColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a );
	void getFgArmedColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a );
	void getBgArmedColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a );
	void getFgMousedownColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a );
	void getBgMousedownColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a );
	void getBorderColor( SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a );

private:

	class CScheme
	{
	public:
		enum { 
			SCHEME_NAME_LENGTH = 32,
		};
		
		// name
		char schemeName[SCHEME_NAME_LENGTH];
		vgui::Font *font;

		// construction/destruction
		CScheme()
		{
			schemeName[0] = 0;
			font = NULL;
		}

		~CScheme()
		{
			delete font;
		}
	};

	CScheme m_Schemes[MAX_SCHEMES];
	int m_iNumSchemes;

	// Resolution we were initted at.
	int		m_xRes;
	CScheme *getSafeScheme( SchemeHandle_t schemeHandle );
	SchemeHandle_t LoadScheme( const char *schemeName );
};


