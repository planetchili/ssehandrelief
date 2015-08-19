#pragma once
#include <d3d9.h>

class Color
{
public:
	union
	{
		D3DCOLOR c;
		struct
		{
			unsigned char b;
			unsigned char g;
			unsigned char r;
			unsigned char x;
		};
	};
public:
	Color( )
	{}
	Color( D3DCOLOR c )
		:
		c( c )
	{}
	Color( unsigned char r,unsigned char g,unsigned char b )
		:
		Color( 255,r,g,b )
	{}
	Color( unsigned char x,unsigned char r,unsigned char g,unsigned char b )
		:
		r( r ),g( g ),b( b ),x( x )
	{}
	Color( unsigned char x,Color c )
		:
		r( c.r ),g( c.g ),b( c.b ),x( x )
	{}
	Color& operator =(Color c)
	{
		this->c = c;
		return *this;
	}
	operator D3DCOLOR() const
	{
		return c;
	}
};

#define BLACK	D3DCOLOR_XRGB( 0,0,0 )
#define WHITE	D3DCOLOR_XRGB( 255,255,255 )
#define GRAY	D3DCOLOR_XRGB( 128,128,128 )
#define RED		D3DCOLOR_XRGB( 255,0,0 )
#define GREEN	D3DCOLOR_XRGB( 0,255,0 )
#define BLUE	D3DCOLOR_XRGB( 0,0,255 )
#define YELLOW	D3DCOLOR_XRGB( 255,255,0 )
#define ORANGE	D3DCOLOR_XRGB( 255,111,0 )
#define BROWN	D3DCOLOR_XRGB( 139,69,19 )
#define PURPLE	D3DCOLOR_XRGB( 127,0,255 )
#define AQUA	D3DCOLOR_XRGB( 0,255,255 )
#define VIOLET	D3DCOLOR_XRGB( 204,0,204 )