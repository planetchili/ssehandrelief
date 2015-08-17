/****************************************************************************************** 
 *	Chili DirectX Framework Version 14.03.22											  *	
 *	D3DGraphics.cpp																		  *
 *	Copyright 2014 PlanetChili.net <http://www.planetchili.net>							  *
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
#include "D3DGraphics.h"
#include "ChiliMath.h"
#include <assert.h>
#include <functional>
#pragma comment( lib,"d3d9.lib" )

D3DGraphics::D3DGraphics( HWND hWnd )
	:
pDirect3D( NULL ),
pDevice( NULL ),
pBackBuffer( NULL ),
sysBuffer( screenWidth,screenHeight )
{
	HRESULT result;
	
	pDirect3D = Direct3DCreate9( D3D_SDK_VERSION );
	assert( pDirect3D != NULL );

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp,sizeof( d3dpp ) );
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	d3dpp.Windowed = TRUE;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

    result = pDirect3D->CreateDevice( D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE,&d3dpp,&pDevice );
	assert( !FAILED( result ) );

	result = pDevice->GetBackBuffer( 0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer );
	assert( !FAILED( result ) );
}

D3DGraphics::~D3DGraphics()
{
	if( pDevice )
	{
		pDevice->Release();
		pDevice = NULL;
	}
	if( pDirect3D )
	{
		pDirect3D->Release();
		pDirect3D = NULL;
	}
	if( pBackBuffer )
	{
		pBackBuffer->Release();
		pBackBuffer = NULL;
	}
}

void D3DGraphics::EndFrame()
{
	HRESULT result;
	D3DLOCKED_RECT backRect;

	result = pBackBuffer->LockRect( &backRect,NULL,NULL );
	assert( !FAILED( result ) );

	sysBuffer.Present( backRect.Pitch,(BYTE*)backRect.pBits );

	result = pBackBuffer->UnlockRect();
	assert( !FAILED( result ) );

	result = pDevice->Present( NULL,NULL,NULL,NULL );
	assert( !FAILED( result ) );
}

void D3DGraphics::DrawRectangle( int left,int right,int top,int bottom,Color c )
{
	for( int x = left; x < right; x++ )
	{
		for( int y = top; y < bottom; y++ )
		{
			PutPixel( x,y,c );
		}
	}
}

void D3DGraphics::BltAlpha( int xStartDst,int yStartDst,int xStartSrc,int yStartSrc,
	int width,int height,const Surface& src )
{
	for( int ySrc = yStartSrc,yDst = yStartDst; ySrc < height; ySrc++,yDst++ )
	{
		for( int xSrc = xStartSrc,xDst = xStartDst; xSrc < width; xSrc++,xDst++ )
		{
			sysBuffer.PutPixelAlpha( xDst,yDst,src.GetPixel( xSrc,ySrc ) );
		}
	}
}

void D3DGraphics::Fill( Color c )
{
	sysBuffer.Fill( c );
}

void D3DGraphics::Fade( unsigned char alpha )
{
	//for( unsigned int y = 0; y < sysBuffer.GetHeight(); y++ )
	//{
	//	for( unsigned int x = 0; x < sysBuffer.GetWidth(); x++ )
	//	{
	//		const Color src = sysBuffer.GetPixel( x,y );
	//		sysBuffer.PutPixel( x,y,Color { 
	//			(unsigned char)( ( src.r * alpha ) / 256 ),
	//			(unsigned char)( ( src.g * alpha ) / 256 ),
	//			(unsigned char)( ( src.b * alpha ) / 256 ) } );
	//	}
	//}
	sysBuffer.FadeHalf();
}

void D3DGraphics::FadeHalf()
{
	for( unsigned int y = 0; y < sysBuffer.GetHeight(); y++ )
	{
		for( unsigned int x = 0; x < sysBuffer.GetWidth(); x++ )
		{
			const Color src = sysBuffer.GetPixel( x,y );
			sysBuffer.PutPixel( x,y,Color {
				(unsigned char)( ( src.r / 2 ) ),
				(unsigned char)( ( src.g / 2 ) ),
				(unsigned char)( ( src.b / 2 ) ) } );
		}
	}
}

void D3DGraphics::Tint( Color c )
{
	for( unsigned int y = 0; y < sysBuffer.GetHeight(); y++ )
	{
		for( unsigned int x = 0; x < sysBuffer.GetWidth(); x++ )
		{
			sysBuffer.PutPixelAlpha( x,y,c );
		}
	}
}

void D3DGraphics::TintHalf( Color c )
{
	for( unsigned int y = 0; y < sysBuffer.GetHeight(); y++ )
	{
		for( unsigned int x = 0; x < sysBuffer.GetWidth(); x++ )
		{
			const Color src = sysBuffer.GetPixel( x,y );
			sysBuffer.PutPixel( x,y,Color {
				(unsigned char)( ( src.r / 2 + c.r / 2 ) ),
				(unsigned char)( ( src.g / 2 + c.g / 2 ) ),
				(unsigned char)( ( src.b / 2 + c.b / 2 ) ) } );
		}
	}
}