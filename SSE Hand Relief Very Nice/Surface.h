#pragma once

#include "Colors.h"
#include "Font.h"
#include <gdiplus.h>
#include <string>
#include <assert.h>
#pragma comment( lib,"gdiplus.lib" )

#define DEFAULT_SURFACE_ALIGNMENT 16

class Surface
{
public:
	Surface( unsigned int width,unsigned int height,
		unsigned int byteAlignment = DEFAULT_SURFACE_ALIGNMENT )
		:
		buffer( nullptr ),
		width( width ),
		height( height ),
		pixelPitch( CalculatePixelPitch( width,byteAlignment ) )
	{
		buffer = new Color[height * pixelPitch];
	}
	Surface( Surface&& source )
		:
		buffer( source.buffer ),
		width( source.width ),
		height( source.height ),
		pixelPitch( source.pixelPitch )
	{
		source.buffer = nullptr;
	}
	Surface( Surface& ) = delete;
	Surface& operator=( Surface&& donor )
	{
		width = donor.width;
		height = donor.height;
		pixelPitch = donor.pixelPitch;
		buffer = donor.buffer;
		donor.buffer = nullptr;
		return *this;
	}
	Surface& operator=( const Surface& ) = delete;
	~Surface()
	{
		if( buffer != nullptr )
		{
			delete[] buffer;
			buffer = nullptr;
		}
	}
	inline void Clear()
	{
		memset( buffer,0,height * GetPitch() );
	}
	inline void Present( const unsigned int pitch,BYTE* const buffer ) const
	{
		const unsigned int bytePitch = GetPitch();
		if( pitch == bytePitch )
		{
			memcpy( buffer,this->buffer,height * bytePitch );
		}
		else
		{
			for( unsigned int y = 0; y < height; y++ )
			{
				memcpy( &buffer[pitch * y],&( this->buffer )[pixelPitch * y],sizeof( Color ) * width );
			}
		}
	}
	inline void PutPixel( unsigned int x,unsigned int y,Color c )
	{
		assert( x >= 0 );
		assert( y >= 0 );
		assert( x < width );
		assert( y < height );
		buffer[y * pixelPitch + x] = c;
	}
	inline void PutPixelAlpha( unsigned int x,unsigned int y,Color c )
	{
		assert( x >= 0 );
		assert( y >= 0 );
		assert( x < width );
		assert( y < height );
		// load source pixel
		const Color d = GetPixel( x,y );

		// blend channels
		const unsigned char rsltRed = ( c.r * c.x + d.r * ( 255 - c.x ) ) / 256;
		const unsigned char rsltGreen = ( c.g * c.x + d.g * ( 255 - c.x ) ) / 256;
		const unsigned char rsltBlue = ( c.b * c.x + d.b * ( 255 - c.x ) ) / 256;

		// pack channels back into pixel and fire pixel onto surface
		PutPixel( x,y,{ rsltRed,rsltGreen,rsltBlue } );
	}
	inline Color GetPixel( unsigned int x,unsigned int y ) const
	{
		assert( x >= 0 );
		assert( y >= 0 );
		assert( x < width );
		assert( y < height );
		return buffer[y * pixelPitch + x];
	}
	inline unsigned int GetWidth() const
	{
		return width;
	}
	inline unsigned int GetHeight() const
	{
		return height;
	}
	inline unsigned int GetPitch() const
	{
		return pixelPitch * sizeof( Color );
	}
	inline unsigned int GetPixelPitch() const
	{
		return pixelPitch;
	}
	inline Color* const GetBuffer()
	{
		return buffer;
	}
	inline const Color* const GetBufferConst() const
	{
		return buffer;
	}
	static Surface FromFile( const std::wstring& name,
		unsigned int byteAlignment = DEFAULT_SURFACE_ALIGNMENT )
	{
		Gdiplus::Bitmap bitmap( name.c_str() );
		const unsigned int width = bitmap.GetWidth();
		const unsigned int height = bitmap.GetHeight();
		Surface surf( width,height,byteAlignment );

		for( unsigned int y = 0; y < height; y++ )
		{
			for( unsigned int x = 0; x < width; x++ )
			{
				Gdiplus::Color c;
				bitmap.GetPixel( x,y,&c );
				surf.PutPixel( x,y,c.GetValue() );
			}
		}

		return surf;
	}
	void Save( const std::wstring& filename ) const
	{
		auto GetEncoderClsid = []( const WCHAR* format,CLSID* pClsid ) -> int
		{
			UINT  num = 0;          // number of image encoders
			UINT  size = 0;         // size of the image encoder array in bytes

			Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

			Gdiplus::GetImageEncodersSize( &num,&size );
			if( size == 0 )
				return -1;  // Failure

			pImageCodecInfo = ( Gdiplus::ImageCodecInfo* )( malloc( size ) );
			if( pImageCodecInfo == NULL )
				return -1;  // Failure

			GetImageEncoders( num,size,pImageCodecInfo );

			for( UINT j = 0; j < num; ++j )
			{
				if( wcscmp( pImageCodecInfo[j].MimeType,format ) == 0 )
				{
					*pClsid = pImageCodecInfo[j].Clsid;
					free( pImageCodecInfo );
					return j;  // Success
				}
			}

			free( pImageCodecInfo );
			return -1;  // Failure
		};

		{
			Gdiplus::Bitmap bitmap( width,height,GetPitch(),PixelFormat32bppARGB,(BYTE*)buffer );
			CLSID bmpID;
			GetEncoderClsid( L"image/bmp",&bmpID );
			bitmap.Save( filename.c_str(),&bmpID,NULL );
		}
	}
	void Copy( const Surface& src )
	{
		assert( width == src.width );
		assert( height == src.height );
		if( pixelPitch == src.pixelPitch )
		{
			memcpy( buffer,src.buffer,GetPitch() * height );
		}
		else
		{
			for( unsigned int y = 0; y < height; y++ )
			{
				memcpy( &buffer[pixelPitch * y],&( src.buffer )[pixelPitch * y],sizeof( Color ) * width );
			}
		}
	}
	void Fill( Color c )
	{
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			*i = c;
		}		
	}
	__declspec(noinline) void Fade( unsigned char alpha )
	{
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			const Color src = *i;
			*i = Color {
				(unsigned char)( ( src.r * alpha ) / 256 ),
				(unsigned char)( ( src.g * alpha ) / 256 ),
				(unsigned char)( ( src.b * alpha ) / 256 ) };
		}
	}
	__declspec(noinline)void FadeHalf()
	{
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			const Color src = *i;
			*i = Color {
				(unsigned char)( ( src.r ) / 2 ),
				(unsigned char)( ( src.g ) / 2 ),
				(unsigned char)( ( src.b ) / 2 ) };
		}
	}
private:
	static unsigned int CalculatePixelPitch( unsigned int width,unsigned int byteAlignment )
	{
		assert( byteAlignment % 4 == 0 );
		const unsigned int pixelAlignment = byteAlignment / sizeof( Color );
		return width + ( pixelAlignment - width % pixelAlignment ) % pixelAlignment;
	}
protected:
	Color* buffer;
	unsigned int width;
	unsigned int height;
	unsigned int pixelPitch;
};

class TextSurface : public Surface
{
public:
	TextSurface( unsigned int width,unsigned int height )
		:
		Surface( width,height ),
		bitmap( width,height,GetPitch(),
		PixelFormat32bppRGB,(byte*)buffer ),
		g( &bitmap )
	{
		g.SetSmoothingMode( Gdiplus::SmoothingModeAntiAlias );
	}
	void DrawString( const std::wstring& string,Vec2 pt,const Font& font,Color c )
	{
		Gdiplus::Color textColor( c.r,c.g,c.b );
		Gdiplus::SolidBrush textBrush( textColor );
		g.DrawString( string.c_str(),-1,font,
			Gdiplus::PointF( pt.x,pt.y ),&textBrush );
	}
	void DrawString( const std::wstring& string,const RectF& rect,const Font& font,
		Color c = WHITE,Font::Alignment a = Font::Center )
	{
		Gdiplus::StringFormat format;
		switch( a )
		{
		case Font::Left:
			format.SetAlignment( Gdiplus::StringAlignmentNear );
			break;
		case Font::Right:
			format.SetAlignment( Gdiplus::StringAlignmentFar );
			break;
		case Font::Center:
		default:
			format.SetAlignment( Gdiplus::StringAlignmentCenter );
			break;
		}
		Gdiplus::Color textColor( c.r,c.g,c.b );
		Gdiplus::SolidBrush textBrush( textColor );
		g.DrawString( string.c_str(),-1,font,
			Gdiplus::RectF( rect.left,rect.top,rect.GetWidth(),rect.GetHeight() ),
			&format,
			&textBrush );
	}
	TextSurface( const TextSurface& ) = delete;
	TextSurface( TextSurface&& ) = delete;
	TextSurface& operator=( const TextSurface& ) = delete;
	TextSurface& operator=( TextSurface&& ) = delete;
private:
	Gdiplus::Bitmap	bitmap;
	Gdiplus::Graphics g;
};