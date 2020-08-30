#ifndef _PARANOIATEXT_H
#define _PARANOIATEXT_H
using namespace vgui;

//#include "..\game_shared\vgui_checkbutton2.h"
#include "..\game_shared\vgui_loadtga.h"
#include "VGUI_ScrollPanel.h"

class CMyScrollPanel;
class CMainPanel;

class CRenderable
{
public:
	virtual void Render() = 0;
};


class CParanoiaTextPanel : public Panel, public ActionSignal//, public ICheckButton2Handler
{
public:
	CParanoiaTextPanel(char* filename);
	void actionPerformed(Panel* panel);
	int KeyInput(int down, int keynum, const char *pszCurrentBinding);
	void paint();
	void ResetBackground();
//	void StateChanged(CCheckButton2 *pButton); // будет использовано для переключения режима отрисовки мира
	char	m_loadedFileName[128];

	void Render( void )
	{
		for (int i=0; i < m_iRenderElms; ++i)
			m_pRenderList[i]->Render();
	}

private:
	void BuildErrorPanel(const char* errorString);
	void CloseWindow(void);

	void AddToRenderList(CRenderable* pnew);

	CMyScrollPanel* m_pScrollPanel;
	CMainPanel*		panel;
	int		m_iMaxScrollValue;
	float	m_flStartTime;

	CRenderable*	m_pRenderList[4];
	int	m_iRenderElms;
};


#endif