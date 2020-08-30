// ======================================
// written by BUzer for HL: Paranoia modification
// ======================================

#ifndef _TIPS_H
#define _TIPS_H
using namespace vgui;


class CTips : public Panel
{
public:
    CTips();
	~CTips();
	void Show( client_textmessage_t *tempMessage );
	void Initialize();

protected:
	virtual void paintBackground();
	void LoadMsg( client_textmessage_t *tempMessage );

protected:
	BitmapTGA	*m_pBitmap;
	TextPanel	*m_pText;

	float	m_fShowTime;
	float	m_fHideTime;

	client_textmessage_t *nextMsg;
};

#endif // _TIPS_H