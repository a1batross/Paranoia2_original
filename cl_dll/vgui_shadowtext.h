
#ifndef _SHADOWTEXT_H
#define _SHADOWTEXT_H

#include "VGUI_TextImage.h"

class ShadowTextPanel : public TextPanel
{
public:
	ShadowTextPanel(const char* text,int x,int y,int wide,int tall) : TextPanel(text, x, y, wide, tall)
	{
	}

	virtual void paint()
	{
		int mr, mg, mb, ma;
		int ix, iy;
		getFgColor(mr, mg, mb, ma);
		getTextImage()->getPos(ix, iy);
		getTextImage()->setPos(ix+1, iy+1);
		getTextImage()->setColor( Color(0, 0, 0, ma) );
		getTextImage()->doPaint(this);
		getTextImage()->setPos(ix, iy);
		getTextImage()->setColor( Color(mr, mg, mb, ma) );
		getTextImage()->doPaint(this);
	}
};

#endif