
#ifndef _GAMMAVIEW_H
#define _GAMMAVIEW_H
using namespace vgui;

class CGammaView : public Panel
{
public:
	CGammaView();
	~CGammaView();
//	void Initialize();

protected:
	virtual void paint();
};

#endif // _GAMMAVIEW_H