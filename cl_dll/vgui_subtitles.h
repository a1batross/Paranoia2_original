// ======================================
// Paranoia subtitle system header file
//		written by BUzer
// ======================================

#ifndef _SUBTITLES_H
#define _SUBTITLES_H
using namespace vgui;

class CSubtitleTextPanel : public TextPanel
{
public:
	CSubtitleTextPanel(const char* text,int x,int y,int wide,int tall) : TextPanel(text, x, y, wide, tall)
	{
		m_fBirthTime = 0;
		m_fHoldTime = 0;
		msgAfterDeath = NULL;
//		bkalpha = 0;
	}

	virtual void paintBackground();
	virtual void paint();

	float m_fBirthTime;
	float m_fHoldTime;
//	int bkalpha;
	client_textmessage_t *msgAfterDeath;
};


class CSubtitle : public Panel
{
public:
    CSubtitle();
	void AddMessage( client_textmessage_t *tempMessage );
	void Initialize();

protected:
	virtual void paintBackground();

//protected:
public: // hacks..
	Panel *m_pLayer;
	CSubtitleTextPanel *m_pCur;
	CSubtitleTextPanel *m_pWait;

	float layerpos; // float version of layer's y-coordinate
	float lasttime;
	float m_fCurStartTime;
};

void SubtitleMessageAdd( client_textmessage_t *tempMessage );
void SubtitleInit();

#endif // _SUBTITLES_H