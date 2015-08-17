/****************************************************************************************** 
 *	Chili DirectX Framework Version 14.03.22											  *	
 *	D3DGraphics.h																		  *
 *	Copyright 2014 PlanetChili <http://www.planetchili.net>								  *
 *																						  *
 *	This file is part of The Chili DirectX Framework.									  *
 *																						  *
 *	The Chili DirectX Framework is free software: you can redistribute it and/or modify	  *
 *	it under the terms of the GNU General Public License as published by				  *
 *	the Free Software Foundation, either version 3 of the License, or					  *
 *	(at your option) any later version.													  *
 *																						  *
 *	The Chili DirectX Framework is distributed in the hope that it will be useful,		  *
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of						  *
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the						  *
 *	GNU General Public License for more details.										  *
 *																						  *
 *	You should have received a copy of the GNU General Public License					  *
 *	along with The Chili DirectX Framework.  If not, see <http://www.gnu.org/licenses/>.  *
 ******************************************************************************************/
#pragma once

#include <d3d9.h>
#include "GdiPlusManager.h"
#include "Vec2.h"
#include "Rect.h"
#include "Colors.h"
#include "Surface.h"

class D3DGraphics
{
public:
	D3DGraphics( HWND hWnd );
	~D3DGraphics();
	inline void PutPixel( int x,int y,Color c )
	{
		sysBuffer.PutPixel( x,y,c );
	}
	inline void PutPixelAlpha( int x,int y,Color c )
	{
		sysBuffer.PutPixelAlpha( x,y,c );
	}
	inline Color GetPixel( int x,int y ) const
	{
		return sysBuffer.GetPixel( x,y );
	}
	template<typename T>
	inline void DrawRectangle( _Rect<T> rect,Color c )
	{
		DrawRectangle( (int)rect.left,(int)rect.right,(int)rect.top,(int)rect.bottom,c );
	}
	template<typename T>
	inline void DrawRectangle( _Vec2<T> topLeft,_Vec2<T> bottomRight,Color c )
	{
		DrawRectangle( (int)topLeft.x,(int)bottomRight.x,(int)topLeft.y,(int)bottomRight.y,c );
	}
	void DrawRectangle( int left,int right,int top,int bottom,Color c );
	template<typename T,typename S>
	inline void BltAlpha( _Vec2<T> dstVec,_Rect<S> srcRect,const Surface& src )
	{
		BltAlpha( (int)dstVec.x,(int)dstVec.y,(int)srcRect.left,(int)srcRect.top,
			srcRect.GetWidth(),srcRect.GetHeight(),src );
	}
	void BltAlpha( int xDst,int yDst,int xSrc,int ySrc,int width,int height,const Surface& src );
	inline void DrawString( const std::wstring& string,Vec2 pt,const Font& font,Color c = WHITE )
	{
		sysBuffer.DrawString( string,pt,font,c );
	}
	inline void DrawString( const std::wstring& string,const RectF &rect,const Font& font,Color c = WHITE,Font::Alignment a = Font::Center )
	{
		sysBuffer.DrawString( string,rect,font,c,a );
	}
	inline void CopySurface( const Surface& src )
	{
		sysBuffer.Copy( src );
	}
	inline void BeginFrame()
	{
		sysBuffer.Clear();
	}
	void EndFrame();
	void Fill( Color c );
	void Fade( unsigned char alpha );
	void FadeHalf();
	void Tint( Color c );
	void TintHalf( Color c );
public:
	static const unsigned int	screenWidth =	1280;
	static const unsigned int	screenHeight =	720;
private:
	GdiPlusManager		gdiManager;
	IDirect3D9*			pDirect3D;
	IDirect3DDevice9*	pDevice;
	IDirect3DSurface9*	pBackBuffer;
	TextSurface			sysBuffer;
};