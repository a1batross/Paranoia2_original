#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"
#include "vgui_TeamFortressViewport.h"
#include "triangleapi.h"
#include "ref_params.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pm_movevars.h"
#include "gl_local.h"

int CHudLensflare :: Init( void )
{
	m_iFlags |= HUD_ACTIVE;

	m_pCvarDraw = CVAR_REGISTER( "cl_lensflare", "1", FCVAR_ARCHIVE );

	gHUD.AddHudElem( this );

	return 1;
}

int CHudLensflare :: VidInit( void )
{
	text[0] = SPR_Load("sprites/lens/lens1.spr");
	red[0] = green[0] = blue[0] = 1.0;
	scale[0] = 45;
	multi[0] = -0.45;

	text[1] = SPR_Load("sprites/lens/lens2.spr");
	red[1] = green[0] = blue[0] = 1.0;
	scale[1] = 25;
	multi[1] = 0.2;

	text[2] = SPR_Load("sprites/lens/glow1.spr");
	red[2] = 132/255;
	green[2] = 1.0;
	blue[2] = 153/255;
	scale[2] = 35;
	multi[2] = 0.3;

	text[3] = SPR_Load("sprites/lens/glow2.spr");
	red[3] = 1.0;
	green[3] = 164/255;
	blue[3] = 164/255;
	scale[3] = 40;
	multi[3] = 0.46;

	text[4] = SPR_Load("sprites/lens/lens3.spr");
	red[4] = 1.0;
	green[4] = 164/255;
	blue[4] = 164/255;
	scale[4] = 52;
	multi[4] = 0.5;

	text[5] = SPR_Load("sprites/lens/lens2.spr");
	red[5] = green[5] = blue[5] = 1.0;
	scale[5] = 31;
	multi[5] = 0.54;

	text[6] = SPR_Load("sprites/lens/lens2.spr");
	red[6] = 0.6;
	green[6] = 1.0;
	blue[6] = 0.6;
	scale[6] = 26;
	multi[6] = 0.64;

	text[7] = SPR_Load("sprites/lens/glow1.spr");
	red[7] = 0.5;
	green[7] = 1.0;
	blue[7] = 0.5;
	scale[7] = 20;
	multi[7] = 0.77;

	text[8] = SPR_Load("sprites/lens/lens2.spr");

	text[9] = SPR_Load("sprites/lens/lens1.spr");

	flPlayerBlend = 0.0;
	flPlayerBlend2 = 0.0;

	return 1;
}

int CHudLensflare :: DrawFlare( const Vector &forward, const Vector &lightdir, const Vector &lightorg )
{
 	flPlayerBlend = max( DotProduct( forward, lightdir ) - 0.85, 0.0 ) * 6.8;
 	if( flPlayerBlend > 1.0 ) flPlayerBlend = 1.0;

	flPlayerBlend4 = max( DotProduct( forward, lightdir ) - 0.90, 0.0 ) * 6.6;
	if( flPlayerBlend4 > 1.0 ) flPlayerBlend4 = 1.0;

	flPlayerBlend6 = max( DotProduct( forward, lightdir ) - 0.80, 0.0 ) * 6.7;
	if( flPlayerBlend6 > 1.0 ) flPlayerBlend6 = 1.0;

	flPlayerBlend2 = flPlayerBlend6 * 140.0 ;
	flPlayerBlend3 = flPlayerBlend * 190.0 ;
	flPlayerBlend5 = flPlayerBlend4 * 222.0 ;

	Vector normal, point, screen;

	if( cv_renderer->value ) R_WorldToScreen( lightorg, screen );
	else gEngfuncs.pTriAPI->WorldToScreen( (float *)&lightorg, screen );

	Suncoordx = XPROJECT( screen[0] );
	Suncoordy = YPROJECT( screen[1] ); 

	Screenmx = ScreenWidth / 2;
	Screenmy = ScreenHeight / 2;

	Sundistx = Screenmx - Suncoordx;
	Sundisty = Screenmy - Suncoordy;

	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(SPR_Load("sprites/lens/lensflare2.spr")) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f( 0.97f, 0.6f, 0.02f, 1.0f );
	gEngfuncs.pTriAPI->Brightness( 0.3f );
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx + 190, Suncoordy + 190, 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx + 190, Suncoordy - 190, 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx - 190, Suncoordy - 190, 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx - 190, Suncoordy + 190, 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(SPR_Load("sprites/lens/glow2.spr")) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(1.0, 1.0 , 1.0, flPlayerBlend3/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend3/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx + 160, Suncoordy + 160, 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx + 160, Suncoordy - 160, 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx - 160, Suncoordy - 160, 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx - 160, Suncoordy + 160, 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(SPR_Load("sprites/lens/glow3.spr")) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(1.0, 1.0 , 1.0, flPlayerBlend5/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend5/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(0, 0, 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(0, ScreenHeight, 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(ScreenWidth, ScreenHeight, 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(ScreenWidth, 0, 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	int i = 1;
	Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
	Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	i++;
	Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
	Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	i++;
	Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
	Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	i++;
	Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
	Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	i++;
	Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
	Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	i++;
	Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
	Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	i++;
	Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
	Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	i++;
	int scale1 = 32;
	int Lensx1,Lensy1 = 0;
	Lensx1 = (Suncoordx + (Sundistx * 0.88));
	Lensy1 = (Suncoordy + (Sundisty * 0.88));
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(0.9, 0.9 , 0.9, flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 + scale1, Lensy1 + scale1, 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 + scale1, Lensy1 - scale1, 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 - scale1, Lensy1 - scale1, 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 - scale1, Lensy1 + scale1, 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	i++;
	scale1 = 140;
	Lensx1 = (Suncoordx + (Sundistx * 1.1));
	Lensy1 = (Suncoordy + (Sundisty * 1.1));
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f(0.9, 0.9 , 0.9, flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Brightness(flPlayerBlend2/255.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 + scale1, Lensy1 + scale1, 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 + scale1, Lensy1 - scale1, 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 - scale1, Lensy1 - scale1, 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 - scale1, Lensy1 + scale1, 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	gEngfuncs.pTriAPI->CullFace( TRI_FRONT );

	return 1;
}

int CHudLensflare :: Draw( float flTime )
{  
	Vector forward, sundir, suntarget;
	pmtrace_t ptr;

	if( m_pCvarDraw->value <= 0.0f )
		return 0;

	forward = g_pViewParams->forward;

	// draw flares for dynlights
	for( int i = 0; i < MAX_DLIGHTS; i++ )
	{
		CDynLight *pl = &tr.dlights[i];

		if( pl->die < flTime || !pl->radius || !FBitSet( pl->flags, DLF_LENSFLARE ))
			continue;

		if( pl->type == LIGHT_SPOT )
			sundir = pl->frustum.GetPlane( FRUSTUM_FAR )->normal;
		else sundir = ( pl->origin - g_pViewParams->vieworg ).Normalize();

		suntarget = pl->origin;

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( g_pViewParams->vieworg, suntarget, PM_GLASS_IGNORE, -1, &ptr );

		if( DotProduct( forward, sundir ) >= 0.68f && !pl->frustum.CullSphere( GetVieworg(), 72.0f ) && ptr.fraction == 1.0f )
		{		
			DrawFlare( forward, sundir, suntarget );
		}
	}

	if( CVAR_TO_BOOL( v_sunshafts ))
		return 1;	// don't mixing sunshafts and lensflares because this looks ugly

	if( !tr.fogEnabled )
	{
		Vector skyVec = tr.sky_normal;

		if( skyVec == g_vecZero )
			return 1;	// no light_environment on a map

		sundir = -skyVec.Normalize();
		suntarget = tr.cached_vieworigin + sundir * 32768.0f;	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( tr.cached_vieworigin, suntarget, PM_GLASS_IGNORE, -1, &ptr );

		if( DotProduct( forward, sundir ) >= 0.68f && R_SkyIsVisible() && gEngfuncs.PM_PointContents( ptr.endpos, null ) == CONTENTS_SKY )
		{		
			DrawFlare( forward, sundir, suntarget );
		}
	}

	return 1;
}