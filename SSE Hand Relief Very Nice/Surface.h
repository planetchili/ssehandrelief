#pragma once

#include "Colors.h"
#include "Font.h"
#include <gdiplus.h>
#include <string>
#include <assert.h>
#include <immintrin.h>
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
		buffer = (Color*)_aligned_malloc( sizeof(Color) * height * pixelPitch,32 );
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
	Surface( const Surface& src )
		:
		buffer( nullptr ),
		width( src.width ),
		height( src.height ),
		pixelPitch( src.pixelPitch )
	{
		buffer = (Color*)_aligned_malloc( sizeof( Color ) * height * pixelPitch,32 );
		Copy( src );
	}
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
			_aligned_free( buffer );
			buffer = nullptr;
		}
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
		// load destination pixel
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
	void PremultiplyAlpha()
	{
		for( unsigned int y = 0; y < height; y++ )
		{
			for( unsigned int x = 0; x < width; x++ )
			{
				// load destination pixel
				const Color d = GetPixel( x,y );

				// scale channels
				const unsigned char rsltRed = ( d.r *  d.x ) / 256;
				const unsigned char rsltGreen = ( d.g * d.x ) / 256;
				const unsigned char rsltBlue = ( d.b * d.x ) / 256;

				// pack channels back into pixel and fire pixel onto surface
				PutPixel( x,y,{ (unsigned char)(255 - d.x),rsltRed,rsltGreen,rsltBlue } );
			}
		}
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
	void Clear()
	{
		memset( buffer,0,height * GetPitch() );
	}
	//////////////////////////////////
	// Bench Functions (straight 'C')
	void FillSlow( Color c )
	{
		const unsigned int height = GetHeight();
		const unsigned int width = GetWidth();
		for( unsigned int y = 0; y < height; y++ )
		{
			for( unsigned int x = 0; x < width; x++ )
			{
				buffer[y * pixelPitch + x] = c;
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

	void Fade( unsigned char alpha )
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
	void FadeShift( unsigned char a )
	{
		const unsigned int alpha = a;
		const unsigned int mask = 0xFF;
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			const Color src = *i;
			const unsigned int r = ( ( ( src >> 16 ) & mask ) * a ) / 256;
			const unsigned int g = ( ( ( src >> 8  ) & mask ) * a ) / 256;
			const unsigned int b = ( (   src         & mask ) * a ) / 256;
			*i = ( r << 16 ) | ( g << 8 ) | b;
		}
	}
	void FadeHalf()
	{
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			const Color src = *i;
			*i = Color {
				(unsigned char)( src.r / 2 ),
				(unsigned char)( src.g / 2 ),
				(unsigned char)( src.b / 2 ) };
		}
	}
	void FadeHalfPacked()
	{
		const unsigned int shiftMask = 0x007F7F7F;
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			// divide source channels by 2 using shiftMask to kill boundary crossing bits
			*i = ( *i >> 1 ) & shiftMask;
		}
	}
	void FadeHalfShift()
	{
		const unsigned int mask = 0x7F;
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			const Color src = *i;
			const unsigned int r = ( src >> 17 ) & mask;
			const unsigned int g = ( src >> 9 )  & mask;
			const unsigned int b = ( src >> 1 )  & mask;
			*i = ( r << 16 ) | ( g << 8 ) | b;
		}
	}

	void Tint( Color c )
	{
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			// load destination pixel
			const Color d = *i;

			// blend channels
			const unsigned char rsltRed = ( c.r * c.x + d.r * ( 255 - c.x ) ) / 256;
			const unsigned char rsltGreen = ( c.g * c.x + d.g * ( 255 - c.x ) ) / 256;
			const unsigned char rsltBlue = ( c.b * c.x + d.b * ( 255 - c.x ) ) / 256;

			// pack channels back into pixel and fire pixel onto surface
			*i = { rsltRed,rsltGreen,rsltBlue };
		}
	}
	void TintShift( Color c )
	{
		const unsigned int mask = 0xFF;
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			// load destination pixel
			const Color src = *i;

			// unpack tint components
			const unsigned int a =   c >> 24;
			const unsigned int r = ( c >> 16 ) & mask;
			const unsigned int g = ( c >> 8  ) & mask;
			const unsigned int b =   c         & mask;

			// unpack source components and blend channels
			const unsigned int rsltRed =   ( ( ( src >> 16 ) & mask ) *  ( 255 - a ) + r * a ) >> 8;
			const unsigned int rsltGreen = ( ( ( src >> 8  ) & mask ) *  ( 255 - a ) + g * a ) >> 8;
			const unsigned int rsltBlue =  (   ( src         & mask ) *  ( 255 - a ) + b * a ) >> 8;

			// pack channels back into pixel and fire pixel onto surface
			*i = ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue;
		}
	}
	void TintPrecomputed( Color c )
	{
		// unpack and premultiply tint channels
		const unsigned int rPrecomp = c.r * c.x;
		const unsigned int gPrecomp = c.g * c.x;
		const unsigned int bPrecomp = c.b * c.x;
		const unsigned int caPrecomp = 255 - c.x;
		const unsigned int mask = 0xFF;
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			// load destination pixel
			const Color src = *i;

			// unpack source components and blend channels
			const unsigned int rsltRed =   ( ( ( src >> 16 ) & mask ) *  caPrecomp + rPrecomp ) >> 8;
			const unsigned int rsltGreen = ( ( ( src >> 8  ) & mask ) *  caPrecomp + gPrecomp ) >> 8;
			const unsigned int rsltBlue =  (   ( src         & mask ) *  caPrecomp + bPrecomp ) >> 8;

			// pack channels back into pixel and fire pixel onto surface
			*i = ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue;
		}
	}
	void TintPrecomputedPacked( Color c )
	{
		// unpack and premultiply tint channels
		const unsigned int rPrecomp = ( c.r * c.x ) >> 8;
		const unsigned int gPrecomp = ( c.g * c.x ) >> 8;
		const unsigned int bPrecomp = ( c.b * c.x ) >> 8;
		// pack premultiplied tint channels
		const Color cPrecomp = Color( rPrecomp,gPrecomp,bPrecomp );
		const unsigned int caPrecomp = 255 - c.x;
		const unsigned int mask = 0xFF;
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			// load destination pixel
			const Color src = *i;

			// unpack source components and multiply
			const unsigned int rsltRed =   ( ( ( src >> 16 ) & mask ) * caPrecomp ) >> 8;
			const unsigned int rsltGreen = ( ( ( src >> 8  ) & mask ) * caPrecomp ) >> 8;
			const unsigned int rsltBlue =  (   ( src         & mask ) * caPrecomp ) >> 8;

			// pack channels back into pixel, add premultiplied tint, and fire pixel onto surface
			*i = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + cPrecomp;
		}
	}
	void TintHalfPacked( Color c )
	{
		const unsigned int shiftMask = 0x007F7F7F;
		const Color preComp = ( c >> 1 ) & shiftMask;
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
		{
			// divide destination channels by 2 and add predivided tint (no carry will happen)
			*i = ( ( *i >> 1 ) & shiftMask ) + preComp;
		}
	}
	void Blend( const Surface& s,unsigned char alpha )
	{
		const unsigned int mask = 0xFF;
		const unsigned int a = alpha;
		const Color* j = s.GetBufferConst();
		for( Color* i = buffer,*end = &buffer[pixelPitch * height]; 
			i < end; i++,j++ )
		{
			// load source and destination pixels
			const Color d = *i;
			const Color s = *j;
			
			// unpack source components
			const unsigned int r = ( s >> 16 ) & mask;
			const unsigned int g = ( s >> 8  ) & mask;
			const unsigned int b =   s         & mask;

			// unpack source components and blend channels
			const unsigned int rsltRed =   ( ( ( d >> 16 ) & mask ) *  ( 255 - a ) + r * a ) >> 8;
			const unsigned int rsltGreen = ( ( ( d >> 8  ) & mask ) *  ( 255 - a ) + g * a ) >> 8;
			const unsigned int rsltBlue =  (   ( d         & mask ) *  ( 255 - a ) + b * a ) >> 8;

			// pack channels back into pixel and fire pixel onto surface
			*i = ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue;
		}
	}
	void BlendHalfPacked( const Surface& s )
	{
		const unsigned int shiftMask = 0x007F7F7F;
		const Color* j = s.GetBufferConst();
		for( Color* i = buffer,*end = &buffer[pixelPitch * height];
			i < end; i++,j++ )
		{
			// divide source and destination channels by 2 and sum (no carry will happen)
			*i = ( ( *i >> 1 ) & shiftMask ) + ( ( *j >> 1 ) & shiftMask );
		}
	}
	void BlendAlpha( const Surface& s )
	{
		const unsigned int mask = 0xFF;
		const Color* j = s.GetBufferConst();
		for( Color* i = buffer,*end = &buffer[pixelPitch * height];
			i < end; i++,j++ )
		{
			// load (premultiplied) source and destination pixels
			const Color d = *i;
			const Color s = *j;

			// unpack source components
			const unsigned int a =   s >> 24;
			const unsigned int r = ( s >> 16 ) & mask;
			const unsigned int g = ( s >> 8  ) & mask;
			const unsigned int b =   s         & mask;

			// unpack source components and blend channels
			const unsigned int rsltRed =   ( ( ( d >> 16 ) & mask ) *  ( 255 - a ) + r * a ) >> 8;
			const unsigned int rsltGreen = ( ( ( d >> 8  ) & mask ) *  ( 255 - a ) + g * a ) >> 8;
			const unsigned int rsltBlue =  (   ( d         & mask ) *  ( 255 - a ) + b * a ) >> 8;

			// pack channels back into pixel and fire pixel onto surface
			*i = ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue;
		}
	}
	void BlendAlphaPremultipliedPacked( const Surface& s )
	{
		const unsigned int mask = 0xFF;
		const Color* j = s.GetBufferConst();
		for( Color* i = buffer,*end = &buffer[pixelPitch * height];
			i < end; i++,j++ )
		{
			// load (premultiplied) source and destination pixels
			const Color d = *i;
			const Color s = *j;

			// extract alpha complement
			const unsigned int ca = s >> 24;

			// unpack source components and blend channels
			const unsigned int rsltRed =   ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
			const unsigned int rsltGreen = ( ( ( d >> 8  ) & mask ) * ca ) >> 8;
			const unsigned int rsltBlue =  (   ( d         & mask ) * ca ) >> 8;

			// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
			*i = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
		}
	}

	void DrawRect( const RectI& rect,Color c )
	{
		for( unsigned int y = unsigned int( rect.top ); y < unsigned int( rect.bottom ); y++ )
		{
			for( Color* i = &buffer[y * width + unsigned int( rect.left )],*end = i + rect.GetWidth();
				i < end; i++ )
			{
				*i = c;
			}
		}
	}
	void DrawRectBlendPrecomputedPacked( const RectI& rect,Color c )
	{
		// unpack and premultiply tint channels
		const unsigned int rPrecomp = ( c.r * c.x ) >> 8;
		const unsigned int gPrecomp = ( c.g * c.x ) >> 8;
		const unsigned int bPrecomp = ( c.b * c.x ) >> 8;
		// pack premultiplied tint channels
		const Color cPrecomp = Color( rPrecomp,gPrecomp,bPrecomp );
		const unsigned int caPrecomp = 255 - c.x;
		const unsigned int mask = 0xFF;
		for( unsigned int y = unsigned int( rect.top ); y < unsigned int( rect.bottom ); y++ )
		{
			for( Color* i = &buffer[y * width + unsigned int( rect.left )],*end = i + rect.GetWidth();
				i < end; i++ )
			{
				// load destination pixel
				const Color d = *i;

				// unpack source components and multiply
				const unsigned int rsltRed =   ( ( ( d >> 16 ) & mask ) * caPrecomp ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8  ) & mask ) * caPrecomp ) >> 8;
				const unsigned int rsltBlue =  ( (   d         & mask ) * caPrecomp ) >> 8;

				// pack channels back into pixel, add premultiplied tint, and fire pixel onto surface
				*i = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + cPrecomp;
			}
		}
	}
	void DrawRectBlendHalfPacked( const RectI& rect,Color c )
	{
		const unsigned int shiftMask = 0x007F7F7F;
		const Color preComp = ( c >> 1 ) & shiftMask;
		for( unsigned int y = unsigned int( rect.top ); y < unsigned int( rect.bottom ); y++ )
		{
			for( Color* i = &buffer[y * width + unsigned int( rect.left )],*end = i + rect.GetWidth();
				i < end; i++ )
			{
				*i = ( ( *i >> 1 ) & shiftMask ) + preComp;
			}
		}
	}
	void Blt( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		for( int yDst = dstPt.y, 
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			const Color* j = &src.GetBufferConst()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth();
				i < iEnd; i++,j++ )
			{
				*i = *j;
			}
		}
	}
	void BltBlend( Vei2 dstPt,const RectI& srcRect,const Surface& src,unsigned char alpha )
	{
		const unsigned int mask = 0xFF;
		const unsigned int a = alpha;
		for( int yDst = dstPt.y,
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			const Color* j = &src.GetBufferConst()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth();
				i < iEnd; i++,j++ )
			{
				// load source and destination pixels
				const Color d = *i;
				const Color s = *j;

				// unpack source components
				const unsigned int r = ( s >> 16 ) & mask;
				const unsigned int g = ( s >> 8  ) & mask;
				const unsigned int b =   s         & mask;

				// unpack source components and blend channels
				const unsigned int rsltRed =   ( ( ( d >> 16 ) & mask ) * ( 255 - a ) + r * a ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8  ) & mask ) * ( 255 - a ) + g * a ) >> 8;
				const unsigned int rsltBlue =  (   ( d         & mask ) * ( 255 - a ) + b * a ) >> 8;

				// pack channels back into pixel and fire pixel onto surface
				*i = ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue;
			}
		}
	}
	void BltBlendHalfPacked( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		const unsigned int shiftMask = 0x007F7F7F;
		for( int yDst = dstPt.y,
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			const Color* j = &src.GetBufferConst()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth();
				i < iEnd; i++,j++ )
			{
				// divide source and destination channels by 2 and sum (no carry will happen)
				*i = ( ( *i >> 1 ) & shiftMask ) + ( ( *j >> 1 ) & shiftMask );
			}
		}
	}
	void BltAlpha( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		const unsigned int mask = 0xFF;
		for( int yDst = dstPt.y,
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			const Color* j = &src.GetBufferConst()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth();
				i < iEnd; i++,j++ )
			{
				// load source and destination pixels
				const Color d = *i;
				const Color s = *j;

				// unpack source components
				const unsigned int a =   s >> 24;
				const unsigned int r = ( s >> 16 ) & mask;
				const unsigned int g = ( s >> 8  ) & mask;
				const unsigned int b =   s         & mask;

				// unpack source components and blend channels
				const unsigned int rsltRed =   ( ( ( d >> 16 ) & mask ) *  ( 255 - a ) + r * a ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8  ) & mask ) *  ( 255 - a ) + g * a ) >> 8;
				const unsigned int rsltBlue =  (   ( d         & mask ) *  ( 255 - a ) + b * a ) >> 8;

				// pack channels back into pixel and fire pixel onto surface
				*i = ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue;
			}
		}
	}
	void BltAlphaPremultipliedPacked( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		const unsigned int mask = 0xFF;
		for( int yDst = dstPt.y,
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			const Color* j = &src.GetBufferConst()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth();
				i < iEnd; i++,j++ )
			{
				// load (premultiplied) source and destination pixels
				const Color d = *i;
				const Color s = *j;

				// unpack alpha complement
				const unsigned int ca = s >> 24;

				// unpack source components and blend channels
				const unsigned int rsltRed =   ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8  ) & mask ) * ca ) >> 8;
				const unsigned int rsltBlue =  ( (   d         & mask ) * ca ) >> 8;

				// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
				*i = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
			}
		}
	}
	void BltKey( Vei2 dstPt,const RectI& srcRect,const Surface& src,Color key )
	{
		for( int yDst = dstPt.y,
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			const Color* j = &src.GetBufferConst()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth();
				i < iEnd; i++,j++ )
			{
				*i = ( *j == key ? *i : *j );
			}
		}
	}
	//////////////////////////////////
	// Bench Functions (SSE)
	void ClearSSE()
	{
		__m128i zero = _mm_setzero_si128();
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++ )
		{
			_mm_store_si128( i,zero );
		}
	}
	void FillSSE( Color c )
	{
		const __m128i color = _mm_set1_epi32( c );
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++ )
		{
			_mm_store_si128( i,color );
		}
	}

	void FadeSSE( unsigned char a )
	{
		const __m128i alpha = _mm_set1_epi16( a );
		const __m128i zero = _mm_setzero_si128();
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++ )
		{
			const __m128i srcPixels = _mm_load_si128( i );
			const __m128i srcLo16 = _mm_unpacklo_epi8( srcPixels,zero );
			const __m128i srcHi16 = _mm_unpackhi_epi8( srcPixels,zero );
			__m128i rsltLo16 = _mm_mullo_epi16( srcLo16,alpha );
			__m128i rsltHi16 = _mm_mullo_epi16( srcHi16,alpha );
			rsltLo16 = _mm_srli_epi16( rsltLo16,8 );
			rsltHi16 = _mm_srli_epi16( rsltHi16,8 );
			_mm_store_si128( i,_mm_packus_epi16( rsltLo16,rsltHi16 ) );
		}
	}
	void FadeHalfSSE()
	{
		const __m128i zero = _mm_setzero_si128();
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++ )
		{
			const __m128i srcPixels = _mm_load_si128( i );
			const __m128i srcLo16 = _mm_unpacklo_epi8( srcPixels,zero );
			const __m128i srcHi16 = _mm_unpackhi_epi8( srcPixels,zero );
			const __m128i rsltLo16 = _mm_srli_epi16( srcLo16,1 );
			const __m128i rsltHi16 = _mm_srli_epi16( srcHi16,1 );
			_mm_store_si128( i,_mm_packus_epi16( rsltLo16,rsltHi16 ) );
		}
	}
	void FadeHalfPackedSSE()
	{
		const __m128i shiftMask = _mm_set1_epi8( 0x7F );
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++ )
		{
			const __m128i srcPixels = _mm_load_si128( i );
			__m128i rslt = _mm_srli_epi16( srcPixels,1 );
			rslt = _mm_and_si128( rslt,shiftMask );
			_mm_store_si128( i,rslt );
		}
	}
	void FadeHalfAvgSSE()
	{
		const __m128i zero = _mm_setzero_si128();
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++ )
		{
			const __m128i srcPixels = _mm_load_si128( i );
			_mm_store_si128( i,_mm_avg_epu8( srcPixels,zero ) );
		}
	}

	void TintSSE( Color c )
	{
		const __m128i zero = _mm_setzero_si128();
		const __m128i ones = _mm_set1_epi16( 0x00FF );
		const __m128i color = _mm_set1_epi32( c );
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++ )
		{
			const __m128i dstPixels = _mm_load_si128( i );
			const __m128i dstLo16 = _mm_unpacklo_epi8( dstPixels,zero );
			const __m128i dstHi16 = _mm_unpackhi_epi8( dstPixels,zero );
			const __m128i color16 = _mm_unpacklo_epi8( color,zero );
			__m128i alpha = _mm_shufflelo_epi16( color16,_MM_SHUFFLE( 3,3,3,3 ) );
			alpha = _mm_shufflehi_epi16( alpha,_MM_SHUFFLE( 3,3,3,3 ) );
			const __m128i calpha = _mm_sub_epi16( ones,alpha );
			const __m128i mdstLo16 = _mm_mullo_epi16( dstLo16,calpha );
			const __m128i mdstHi16 = _mm_mullo_epi16( dstHi16,calpha );
			const __m128i mcolor16 = _mm_mullo_epi16( color16,alpha );
			__m128i rsltLo16 = _mm_add_epi16( mdstLo16,mcolor16 );
			__m128i rsltHi16 = _mm_add_epi16( mdstHi16,mcolor16 );
			rsltLo16 = _mm_srli_epi16( rsltLo16,8 );
			rsltHi16 = _mm_srli_epi16( rsltHi16,8 );
			_mm_store_si128( i,_mm_packus_epi16( rsltLo16,rsltHi16 ) );
		}
	}
	void TintPrecomputedSSE( Color c )
	{
		const __m128i zero = _mm_setzero_si128();
		const __m128i alpha = _mm_set1_epi16( c.x );
		const __m128i calpha = _mm_sub_epi16( _mm_set1_epi16( 0x00FF ),alpha );
		__m128i preColor = _mm_set1_epi32( c );
		preColor = _mm_unpacklo_epi8( preColor,zero );
		preColor = _mm_mullo_epi16( preColor,alpha );
		preColor = _mm_srli_epi16( preColor,8 );
		preColor = _mm_packus_epi16( preColor,preColor );

		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++ )
		{
			const __m128i dstPixels = _mm_load_si128( i );
			const __m128i dstLo16 = _mm_unpacklo_epi8( dstPixels,zero );
			const __m128i dstHi16 = _mm_unpackhi_epi8( dstPixels,zero );
			const __m128i mdstLo16 = _mm_mullo_epi16( dstLo16,calpha );
			const __m128i mdstHi16 = _mm_mullo_epi16( dstHi16,calpha );
			const __m128i rsltLo16 = _mm_srli_epi16( mdstLo16,8 );
			const __m128i rsltHi16 = _mm_srli_epi16( mdstHi16,8 );
			const __m128i rsltDst = _mm_packus_epi16( rsltLo16,rsltHi16 );
			_mm_store_si128( i,_mm_add_epi8( rsltDst,preColor ) );
		}
	}
	void TintHalfAvgSSE( Color c )
	{
		const __m128i color = _mm_set1_epi32( c );
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++ )
		{
			const __m128i dstPixels = _mm_load_si128( i );
			const __m128i rslt = _mm_avg_epu8( dstPixels,color );
			_mm_store_si128( i,rslt );
		}
	}
	void BlendSSE( const Surface& s,unsigned char a )
	{
		const __m128i alpha = _mm_set1_epi16( a );
		const __m128i calpha = _mm_set1_epi16( 255u - a );
		const __m128i zero = _mm_setzero_si128();
		const __m128i* j = reinterpret_cast<const __m128i*>( s.GetBufferConst() );
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++,j++ )
		{
			const __m128i srcPixels = _mm_load_si128( j );
			const __m128i dstPixels = _mm_load_si128( i );
			const __m128i srcLo16 = _mm_unpacklo_epi8( srcPixels,zero );
			const __m128i srcHi16 = _mm_unpackhi_epi8( srcPixels,zero );
			const __m128i dstLo16 = _mm_unpacklo_epi8( dstPixels,zero );
			const __m128i dstHi16 = _mm_unpackhi_epi8( dstPixels,zero );
			__m128i rsltSrcLo16 = _mm_mullo_epi16( srcLo16,alpha );
			__m128i rsltSrcHi16 = _mm_mullo_epi16( srcHi16,alpha );
			__m128i rsltDstLo16 = _mm_mullo_epi16( dstLo16,calpha );
			__m128i rsltDstHi16 = _mm_mullo_epi16( dstHi16,calpha );
			rsltSrcLo16 = _mm_srli_epi16( rsltSrcLo16,8 );
			rsltSrcHi16 = _mm_srli_epi16( rsltSrcHi16,8 );
			rsltDstLo16 = _mm_srli_epi16( rsltDstLo16,8 );
			rsltDstHi16 = _mm_srli_epi16( rsltDstHi16,8 );
			const __m128i rsltLo16 = _mm_adds_epi16( rsltSrcLo16,rsltDstLo16 );
			const __m128i rsltHi16 = _mm_adds_epi16( rsltSrcHi16,rsltDstHi16 );
			_mm_store_si128( i,_mm_packus_epi16( rsltLo16,rsltHi16 ) );
		}
	}
	void BlendSSEHi( const Surface& s,unsigned char a )
	{
		const __m128i alpha = _mm_set1_epi16( a << 8 );
		const __m128i calpha = _mm_set1_epi16( (255u - a) << 8 );
		const __m128i zero = _mm_setzero_si128();
		const __m128i* j = reinterpret_cast<const __m128i*>( s.GetBufferConst() );
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
		i < end; i++,j++ )
		{
			const __m128i srcPixels = _mm_load_si128( j );
			const __m128i dstPixels = _mm_load_si128( i );
			const __m128i srcLo16 = _mm_unpacklo_epi8( srcPixels,zero );
			const __m128i srcHi16 = _mm_unpackhi_epi8( srcPixels,zero );
			const __m128i dstLo16 = _mm_unpacklo_epi8( dstPixels,zero );
			const __m128i dstHi16 = _mm_unpackhi_epi8( dstPixels,zero );
			const __m128i rsltSrcLo16 = _mm_mulhi_epu16( srcLo16,alpha  );
			const __m128i rsltSrcHi16 = _mm_mulhi_epu16( srcHi16,alpha  );
			const __m128i rsltDstLo16 = _mm_mulhi_epu16( dstLo16,calpha );
			const __m128i rsltDstHi16 = _mm_mulhi_epu16( dstHi16,calpha );
			const __m128i rsltLo16 = _mm_add_epi16( rsltSrcLo16,rsltDstLo16 );
			const __m128i rsltHi16 = _mm_add_epi16( rsltSrcHi16,rsltDstHi16 );
			_mm_store_si128( i,_mm_packus_epi16( rsltLo16,rsltHi16 ) );
		}
	}
	void BlendAVXHi( const Surface& s,unsigned char a )
	{
		const __m256i alpha = _mm256_set1_epi16( a << 8 );
		const __m256i calpha = _mm256_set1_epi16( ( 255u - a ) << 8 );
		const __m256i zero = _mm256_setzero_si256();
		const __m256i* j = reinterpret_cast<const __m256i*>( s.GetBufferConst() );
		for( __m256i* i = reinterpret_cast<__m256i*>( buffer ),
			*end = reinterpret_cast<__m256i*>( &buffer[pixelPitch * height] );
		i < end; i++,j++ )
		{
			const __m256i srcPixels = _mm256_load_si256( j );
			const __m256i dstPixels = _mm256_load_si256( i );
			const __m256i srcLo16 = _mm256_unpacklo_epi8( srcPixels,zero );
			const __m256i srcHi16 = _mm256_unpackhi_epi8( srcPixels,zero );
			const __m256i dstLo16 = _mm256_unpacklo_epi8( dstPixels,zero );
			const __m256i dstHi16 = _mm256_unpackhi_epi8( dstPixels,zero );
			const __m256i rsltSrcLo16 = _mm256_mulhi_epu16( srcLo16,alpha );
			const __m256i rsltSrcHi16 = _mm256_mulhi_epu16( srcHi16,alpha );
			const __m256i rsltDstLo16 = _mm256_mulhi_epu16( dstLo16,calpha );
			const __m256i rsltDstHi16 = _mm256_mulhi_epu16( dstHi16,calpha );
			const __m256i rsltLo16 = _mm256_add_epi16( rsltSrcLo16,rsltDstLo16 );
			const __m256i rsltHi16 = _mm256_add_epi16( rsltSrcHi16,rsltDstHi16 );
			_mm256_store_si256( i,_mm256_packus_epi16( rsltLo16,rsltHi16 ) );
		}
	}
	void BlendHalfAvgSSE( const Surface& s )
	{
		const __m128i* j = reinterpret_cast<const __m128i*>( s.GetBufferConst() );
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
			i < end; i++,j++ )
		{
			const __m128i srcPixels = _mm_load_si128( j );
			const __m128i dstPixels = _mm_load_si128( i );
			const __m128i rslt = _mm_avg_epu8( dstPixels,srcPixels );
			_mm_store_si128( i,rslt );
		}
	}
	void BlendAlphaSSEHi( const Surface& s )
	{
		const __m128i zero = _mm_setzero_si128();
		const __m128i ones = _mm_set1_epi16( 255u );
		const __m128i* j = reinterpret_cast<const __m128i*>( s.GetBufferConst() );
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
		i < end; i++,j++ )
		{
			const __m128i srcPixels = _mm_load_si128( j );
			const __m128i dstPixels = _mm_load_si128( i );

			__m128i srcLo16 = _mm_unpacklo_epi8( srcPixels,zero );
			__m128i srcHi16 = _mm_unpackhi_epi8( srcPixels,zero );
			const __m128i dstLo16 = _mm_unpacklo_epi8( zero,dstPixels );
			const __m128i dstHi16 = _mm_unpackhi_epi8( zero,dstPixels );

			const __m128i alphaLo = _mm_shufflehi_epi16( _mm_shufflelo_epi16(
				srcLo16,_MM_SHUFFLE( 3,3,3,3 ) ),_MM_SHUFFLE( 3,3,3,3 ) );
			const __m128i alphaHi = _mm_shufflehi_epi16( _mm_shufflelo_epi16(
				srcHi16,_MM_SHUFFLE( 3,3,3,3 ) ),_MM_SHUFFLE( 3,3,3,3 ) );
			const __m128i calphaLo = _mm_sub_epi16( ones,alphaLo );
			const __m128i calphaHi = _mm_sub_epi16( ones,alphaHi );

			srcLo16 = _mm_slli_epi16( srcLo16,8 );
			srcHi16 = _mm_slli_epi16( srcHi16,8 );

			const __m128i rsltSrcLo16 = _mm_mulhi_epu16( srcLo16,alphaLo );
			const __m128i rsltSrcHi16 = _mm_mulhi_epu16( srcHi16,alphaHi );
			const __m128i rsltDstLo16 = _mm_mulhi_epu16( dstLo16,calphaLo );
			const __m128i rsltDstHi16 = _mm_mulhi_epu16( dstHi16,calphaHi );

			const __m128i rsltLo16 = _mm_add_epi16( rsltSrcLo16,rsltDstLo16 );
			const __m128i rsltHi16 = _mm_add_epi16( rsltSrcHi16,rsltDstHi16 );

			_mm_store_si128( i,_mm_packus_epi16( rsltLo16,rsltHi16 ) );
		}
	}
	void BlendAlphaPremultipliedSSEHi( const Surface& s )
	{
		const __m128i zero = _mm_setzero_si128();
		const __m128i* j = reinterpret_cast<const __m128i*>( s.GetBufferConst() );
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
		i < end; i++,j++ )
		{
			const __m128i srcPixels = _mm_load_si128( j );
			const __m128i dstPixels = _mm_load_si128( i );
			const __m128i dstLo16 = _mm_unpacklo_epi8( zero,dstPixels );
			const __m128i dstHi16 = _mm_unpackhi_epi8( zero,dstPixels );
			const __m128i srcLo16 = _mm_unpacklo_epi8( srcPixels,zero );
			const __m128i srcHi16 = _mm_unpackhi_epi8( srcPixels,zero );
			const __m128i calphaLo = _mm_shufflehi_epi16( _mm_shufflelo_epi16( 
				srcLo16,_MM_SHUFFLE( 3,3,3,3 ) ),_MM_SHUFFLE( 3,3,3,3 ) );
			const __m128i calphaHi = _mm_shufflehi_epi16( _mm_shufflelo_epi16(
				srcHi16,_MM_SHUFFLE( 3,3,3,3 ) ),_MM_SHUFFLE( 3,3,3,3 ) );
			const __m128i rsltDstLo16 = _mm_mulhi_epu16( dstLo16,calphaLo );
			const __m128i rsltDstHi16 = _mm_mulhi_epu16( dstHi16,calphaHi );
			const __m128i rsltDst = _mm_packus_epi16( rsltDstLo16,rsltDstHi16 );
			const __m128i rslt = _mm_add_epi8( rsltDst,srcPixels );
			_mm_store_si128( i,rslt );
		}
	}
	void BlendAlphaPremultipliedSSSE3Hi( const Surface& s )
	{
		const __m128i zero = _mm_setzero_si128();
		const __m128i alphaShuffleLo = _mm_set_epi8(
			128u,128u,128u,7u,128u,7u,128u,7u,
			128u,128u,128u,3u,128u,3u,128u,3u );
		const __m128i alphaShuffleHi = _mm_set_epi8(
			128u,128u,128u,15u,128u,15u,128u,15u,
			128u,128u,128u,11u,128u,11u,128u,11u );
		const __m128i* j = reinterpret_cast<const __m128i*>( s.GetBufferConst() );
		for( __m128i* i = reinterpret_cast<__m128i*>( buffer ),
			*end = reinterpret_cast<__m128i*>( &buffer[pixelPitch * height] );
		i < end; i++,j++ )
		{
			const __m128i srcPixels = _mm_load_si128( j );
			const __m128i dstPixels = _mm_load_si128( i );
			const __m128i dstLo16 = _mm_unpacklo_epi8( zero,dstPixels );
			const __m128i dstHi16 = _mm_unpackhi_epi8( zero,dstPixels );
			const __m128i calphaLo = _mm_shuffle_epi8( srcPixels,alphaShuffleLo );
			const __m128i calphaHi = _mm_shuffle_epi8( srcPixels,alphaShuffleHi );
			const __m128i rsltDstLo16 = _mm_mulhi_epu16( dstLo16,calphaLo );
			const __m128i rsltDstHi16 = _mm_mulhi_epu16( dstHi16,calphaHi );
			const __m128i rsltDst = _mm_packus_epi16( rsltDstLo16,rsltDstHi16 );
			const __m128i rslt = _mm_add_epi8( rsltDst,srcPixels );
			_mm_store_si128( i,rslt );
		}
	}

	void DrawRectSSEBad( const RectI& rect,Color c )
	{
		const __m128i c128 = _mm_set1_epi32( c );
		for( int y = rect.top; y < rect.bottom; y++ )
		{
			for( __m128i* pCur = reinterpret_cast<__m128i*>( &buffer[rect.left + pixelPitch * y] ),
				*pEnd = reinterpret_cast<__m128i*>( &buffer[rect.right + pixelPitch * y] );
				pCur < pEnd; pCur++ )
			{
				_mm_store_si128( pCur,c128 );
			}
		}
	}
	void DrawRectSSE( const RectI& rect,Color c )
	{
		const __m128i c128 = _mm_set1_epi32( c );
		// round up to nearest multiple of 4
		const unsigned int xStartLoop = ( rect.left + 3 ) & ~0x03;
		// round down to nearest multiple of 4
		const unsigned int xEndLoop = rect.right & ~0x03;
		for( int y = rect.top; y < rect.bottom; y++ )
		{
			Color* const pEndPeel = &buffer[xStartLoop + pixelPitch * y];
			Color* const pEndLoop = &buffer[xEndLoop + pixelPitch * y];
			for( Color* pCur = &buffer[rect.left + pixelPitch * y];	pCur < pEndPeel; pCur++ )
			{
				*pCur = c;
			}
			for( __m128i* pCur = reinterpret_cast<__m128i*>( pEndPeel ); 
				pCur < reinterpret_cast<__m128i*>( pEndLoop ); pCur++ )
			{
				_mm_store_si128( pCur,c128 );
			}
			for( Color* pCur = pEndLoop,*pEnd = &buffer[rect.right + pixelPitch * y]; pCur < pEnd; pCur++ )
			{
				*pCur = c;
			}
		}
	}
	void BltSSESrcDstAligned( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );
		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y; 
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const __m128i* pSrc = reinterpret_cast<const __m128i*>(
				&src.GetBufferConst()[srcRect.left + srcPitch * ySrc] );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd = pSrc + srcRect.GetWidth() / 4;
			__m128i* pDst = reinterpret_cast<__m128i*>(
				&GetBuffer()[dstPt.x + dstPitch * yDst] );
			for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				_mm_store_si128( pDst,_mm_load_si128( pSrc ) );
			}
		}
	}
	void BltSSESrcAligned( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		const int alignment = dstPt.x % 4;
		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );
		switch( alignment )
		{
		case 0:
			for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
				ySrc < ySrcEnd; ySrc++,yDst++ )
			{
				const __m128i* pSrc = reinterpret_cast<const __m128i*>(
					&src.GetBufferConst()[srcRect.left + srcPitch * ySrc] );
				// divide by 4 because were working with 128-bit pointer arithmetic
				const __m128i* const pSrcEnd = pSrc + srcRect.GetWidth() / 4;
				__m128i* pDst = reinterpret_cast<__m128i*>(
					&GetBuffer()[dstPt.x + dstPitch * yDst] );
				for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
				{
					_mm_store_si128( pDst,_mm_load_si128( pSrc ) );
				}
			}
			break;
		case 1:
			for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
				ySrc < ySrcEnd; ySrc++,yDst++ )
			{
				const __m128i* pSrc = reinterpret_cast<const __m128i*>(
					&src.GetBufferConst()[srcRect.left + srcPitch * ySrc] );
				// divide by 4 because were working with 128-bit pointer arithmetic
				const __m128i* const pSrcEnd = pSrc + srcRect.GetWidth() / 4;
				// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
				__m128i* pDst = reinterpret_cast<__m128i*>(
					&GetBuffer()[( dstPt.x & ~0x03 ) + dstPitch * yDst] );

				// prime the pump with destination pixels that will not be overwritten (pre-shift)
				__m128i prevSrc = _mm_slli_si128( _mm_load_si128( pDst ),12 );
				for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
				{
					__m128i newSrc = _mm_load_si128( pSrc );
					__m128i output = _mm_or_si128(
						_mm_srli_si128( prevSrc,12 ),_mm_slli_si128( newSrc,4 ) );
					_mm_store_si128( pDst,output );
					prevSrc = newSrc;
				}
				// finish off the last section that contains dest pixels that will not be overwritten
				const __m128i dstFinal = _mm_slli_si128(
					_mm_srli_si128( _mm_load_si128( pDst ),4 ),4 );
				__m128i output = _mm_or_si128(
					_mm_srli_si128( prevSrc,12 ),dstFinal );
				_mm_store_si128( pDst,output );
			}
			break;
		case 2:
			for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
				ySrc < ySrcEnd; ySrc++,yDst++ )
			{
				const __m128i* pSrc = reinterpret_cast<const __m128i*>(
					&src.GetBufferConst()[srcRect.left + srcPitch * ySrc] );
				// divide by 4 because were working with 128-bit pointer arithmetic
				const __m128i* const pSrcEnd = pSrc + srcRect.GetWidth() / 4;
				// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
				__m128i* pDst = reinterpret_cast<__m128i*>(
					&GetBuffer()[( dstPt.x & ~0x03 ) + dstPitch * yDst] );

				// prime the pump with destination pixels that will not be overwritten (pre-shift)
				__m128i prevSrc = _mm_slli_si128( _mm_load_si128( pDst ),8 );
				for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
				{
					__m128i newSrc = _mm_load_si128( pSrc );
					__m128i output = _mm_or_si128(
						_mm_srli_si128( prevSrc,8 ),_mm_slli_si128( newSrc,8 ) );
					_mm_store_si128( pDst,output );
					prevSrc = newSrc;
				}
				// finish off the last section that contains dest pixels that will not be overwritten
				const __m128i dstFinal = _mm_slli_si128(
					_mm_srli_si128( _mm_load_si128( pDst ),8 ),8 );
				__m128i output = _mm_or_si128(
					_mm_srli_si128( prevSrc,8 ),dstFinal );
				_mm_store_si128( pDst,output );
			}
			break;
		case 3:
			for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
				ySrc < ySrcEnd; ySrc++,yDst++ )
			{
				const __m128i* pSrc = reinterpret_cast<const __m128i*>(
					&src.GetBufferConst()[srcRect.left + srcPitch * ySrc] );
				// divide by 4 because were working with 128-bit pointer arithmetic
				const __m128i* const pSrcEnd = pSrc + srcRect.GetWidth() / 4;
				// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
				__m128i* pDst = reinterpret_cast<__m128i*>(
					&GetBuffer()[( dstPt.x & ~0x03 ) + dstPitch * yDst] );

				// prime the pump with destination pixels that will not be overwritten (pre-shift)
				__m128i prevSrc = _mm_slli_si128( _mm_load_si128( pDst ),4 );
				for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
				{
					__m128i newSrc = _mm_load_si128( pSrc );
					__m128i output = _mm_or_si128(
						_mm_srli_si128( prevSrc,4 ),_mm_slli_si128( newSrc,12 ) );
					_mm_store_si128( pDst,output );
					prevSrc = newSrc;
				}
				// finish off the last section that contains dest pixels that will not be overwritten
				const __m128i dstFinal = _mm_slli_si128(
					_mm_srli_si128( _mm_load_si128( pDst ),12 ),12 );
				__m128i output = _mm_or_si128(
					_mm_srli_si128( prevSrc,4 ),dstFinal );
				_mm_store_si128( pDst,output );
			}
			break;
		}
	}
	void BltSSESrcAlignedTemplated( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		const int alignment = dstPt.x % 4;
		switch( alignment )
		{
		case 0:
			BltSSESrcDstAligned( dstPt,srcRect,src );
			break;
		case 1:
			BltSSESrcAlignedTemplate<1>( dstPt,srcRect,src );
			break;
		case 2:
			BltSSESrcAlignedTemplate<2>( dstPt,srcRect,src );
			break;
		case 3:
			BltSSESrcAlignedTemplate<3>( dstPt,srcRect,src );
			break;
		}
	}
	void BltSSSE3SrcAlignedTemplated( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		const int alignment = dstPt.x % 4;
		switch( alignment )
		{
		case 0:
			BltSSESrcDstAligned( dstPt,srcRect,src );
			break;
		case 1:
			BltSSSE3SrcAlignedTemplate<1>( dstPt,srcRect,src );
			break;
		case 2:
			BltSSSE3SrcAlignedTemplate<2>( dstPt,srcRect,src );
			break;
		case 3:
			BltSSSE3SrcAlignedTemplate<3>( dstPt,srcRect,src );
			break;
		}
	}
	void BltSSE( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		if( srcRect.GetWidth() < 4 )
		{
			Blt( dstPt,srcRect,src );
			return;
		}

		const int srcLeftAligned = ( srcRect.left + 3 ) & ~0x03;
		const int srcRightAligned = srcRect.right & ~0x03;
		if( srcLeftAligned == srcRect.left && srcRightAligned == srcRect.right )
		{
			BltSSESrcAlignedTemplated( dstPt,srcRect,src );
			return;
		}

		const int alignment = ( dstPt.x + ( srcLeftAligned - srcRect.left ) ) & 0x03;
		switch( alignment )
		{
		case 0:
			BltSSEDstAligned( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		case 1:
			BltSSETemplate<1>( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		case 2:
			BltSSETemplate<2>( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		case 3:
			BltSSETemplate<3>( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		}
	}
	void BltSSSE3( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		if( srcRect.GetWidth() < 4 )
		{
			Blt( dstPt,srcRect,src );
			return;
		}

		const int srcLeftAligned = ( srcRect.left + 3 ) & ~0x03;
		const int srcRightAligned = srcRect.right & ~0x03;
		if( srcLeftAligned == srcRect.left && srcRightAligned == srcRect.right )
		{
			BltSSSE3SrcAlignedTemplated( dstPt,srcRect,src );
			return;
		}

		const int alignment = ( dstPt.x + ( srcLeftAligned - srcRect.left ) ) & 0x03;
		switch( alignment )
		{
		case 0:
			BltSSEDstAligned( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		case 1:
			BltSSSE3Template<1>( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		case 2:
			BltSSSE3Template<2>( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		case 3:
			BltSSSE3Template<3>( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		}
	}
	void BltSSEU( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		if( srcRect.GetWidth() < 4 )
		{
			Blt( dstPt,srcRect,src );
			return;
		}

		const int overhang = srcRect.GetWidth() & 0x03;
		const int srcPitch = src.GetPixelPitch();
		const int dstPitch = GetPixelPitch();
		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const __m128i* pSrc = reinterpret_cast<const __m128i*>(
				&src.GetBufferConst()[srcRect.left + srcPitch * ySrc] );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd = pSrc + ( srcRect.GetWidth() - overhang ) / 4;
			__m128i* pDst = reinterpret_cast<__m128i*>(
				&GetBuffer()[dstPt.x + dstPitch * yDst] );
			for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				_mm_storeu_si128( pDst,_mm_loadu_si128( pSrc ) );
			}

			const Color* pSrc2 = reinterpret_cast<const Color*>( pSrc );
			const Color* const pSrcEnd2 = pSrc2 + overhang;
			Color* pDst2 = reinterpret_cast<Color*>( pDst );
			for( ; pSrc2 < pSrcEnd2; pSrc2++,pDst2++ )
			{
				*pDst2 = *pSrc2;
			}
		}
	}
	void BltSSEUDstA( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		if( srcRect.GetWidth() < 4 )
		{
			Blt( dstPt,srcRect,src );
			return;
		}

		const int dstLeftAligned = ( dstPt.x + 3 ) & ~0x03;
		const int dstRightAligned = ( dstPt.x + srcRect.GetWidth() ) & ~0x03;
		const int srcPitch = src.GetPixelPitch();
		const int dstPitch = GetPixelPitch();
		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			// preamble
			const Color* pSrc = &src.GetBufferConst()[srcRect.left + srcPitch * ySrc];
			const Color* const pSrcEnd = pSrc + ( dstLeftAligned - dstPt.x );
			Color* pDst = &GetBuffer()[dstPt.x + dstPitch * yDst];
			for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				*pDst = *pSrc;
			}

			// main loop
			const __m128i* pSrc2 = reinterpret_cast<const __m128i*>( pSrc );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd2 = pSrc2 + ( dstRightAligned - dstLeftAligned ) / 4;
			__m128i* pDst2 = reinterpret_cast<__m128i*>( pDst );
			for( ; pSrc2 < pSrcEnd2; pSrc2++,pDst2++ )
			{
				_mm_store_si128( pDst2,_mm_loadu_si128( pSrc2 ) );
			}

			// post amble
			const Color* pSrc3 = reinterpret_cast<const Color*>( pSrc2 );
			const Color* const pSrcEnd3 = pSrc3 + 
				( ( dstPt.x + srcRect.GetWidth() ) - dstRightAligned );
			Color* pDst3 = reinterpret_cast<Color*>( pDst2 );
			for( ; pSrc3 < pSrcEnd3; pSrc3++,pDst3++ )
			{
				*pDst3 = *pSrc3;
			}
		}
	}
	void BltSSEUSrcA( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		if( srcRect.GetWidth() < 4 )
		{
			Blt( dstPt,srcRect,src );
			return;
		}

		const int srcLeftAligned = ( srcRect.left + 3 ) & ~0x03;
		const int srcRightAligned = srcRect.right & ~0x03;
		const int srcPitch = src.GetPixelPitch();
		const int dstPitch = GetPixelPitch();
		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			// preamble
			const Color* pSrc = &src.GetBufferConst()[srcRect.left + srcPitch * ySrc];
			const Color* const pSrcEnd = pSrc + ( srcLeftAligned - srcRect.left );
			Color* pDst = &GetBuffer()[dstPt.x + dstPitch * yDst];
			for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				*pDst = *pSrc;
			}

			// main loop
			const __m128i* pSrc2 = reinterpret_cast<const __m128i*>( pSrc );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd2 = pSrc2 + ( srcRightAligned - srcLeftAligned ) / 4;
			__m128i* pDst2 = reinterpret_cast<__m128i*>( pDst );
			for( ; pSrc2 < pSrcEnd2; pSrc2++,pDst2++ )
			{
				_mm_storeu_si128( pDst2,_mm_load_si128( pSrc2 ) );
			}

			// post amble
			const Color* pSrc3 = reinterpret_cast<const Color*>( pSrc2 );
			const Color* const pSrcEnd3 = pSrc3 +
				( srcRect.right - srcRightAligned );
			Color* pDst3 = reinterpret_cast<Color*>( pDst2 );
			for( ; pSrc3 < pSrcEnd3; pSrc3++,pDst3++ )
			{
				*pDst3 = *pSrc3;
			}
		}
	}
	void BltAlphaPremultipliedSSSE3USrcA( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		if( srcRect.GetWidth() < 4 )
		{
			BltAlphaPremultipliedPacked( dstPt,srcRect,src );
			return;
		}

		// constants
		const __m128i zero = _mm_setzero_si128();
		const __m128i alphaShuffleLo = _mm_set_epi8(
			128u,128u,128u,7u,128u,7u,128u,7u,
			128u,128u,128u,3u,128u,3u,128u,3u );
		const __m128i alphaShuffleHi = _mm_set_epi8(
			128u,128u,128u,15u,128u,15u,128u,15u,
			128u,128u,128u,11u,128u,11u,128u,11u );
		const auto DoBlend = [=]( const __m128i dstPixels,const __m128i srcPixels )
		{
			const __m128i dstLo16 = _mm_unpacklo_epi8( zero,dstPixels );
			const __m128i dstHi16 = _mm_unpackhi_epi8( zero,dstPixels );
			const __m128i calphaLo = _mm_shuffle_epi8( srcPixels,alphaShuffleLo );
			const __m128i calphaHi = _mm_shuffle_epi8( srcPixels,alphaShuffleHi );
			const __m128i rsltDstLo16 = _mm_mulhi_epu16( dstLo16,calphaLo );
			const __m128i rsltDstHi16 = _mm_mulhi_epu16( dstHi16,calphaHi );
			const __m128i rsltDst = _mm_packus_epi16( rsltDstLo16,rsltDstHi16 );
			const __m128i rslt = _mm_add_epi8( rsltDst,srcPixels );
			return rslt;
		};

		// setup alignment bulshit
		const int srcLeftAligned = ( srcRect.left + 3 ) & ~0x03;
		const int srcRightAligned = srcRect.right & ~0x03;
		const int srcPitch = src.GetPixelPitch();
		const int dstPitch = GetPixelPitch();
		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			// preamble
			const Color* pSrc = &src.GetBufferConst()[srcRect.left + srcPitch * ySrc];
			const Color* const pSrcEnd = pSrc + ( srcLeftAligned - srcRect.left );
			Color* pDst = &GetBuffer()[dstPt.x + dstPitch * yDst];
			const unsigned int mask = 0xFF;
			for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				// load (premultiplied) source and destination pixels
				const Color d = *pDst;
				const Color s = *pSrc;

				// extract alpha complement
				const unsigned int ca = s >> 24;

				// unpack source components and blend channels
				const unsigned int rsltRed = ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8 ) & mask ) * ca ) >> 8;
				const unsigned int rsltBlue = ( ( d         & mask ) * ca ) >> 8;

				// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
				*pDst = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
			}

			// main loop
			const __m128i* pSrc2 = reinterpret_cast<const __m128i*>( pSrc );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd2 = pSrc2 + ( srcRightAligned - srcLeftAligned ) / 4;
			__m128i* pDst2 = reinterpret_cast<__m128i*>( pDst );
			for( ; pSrc2 < pSrcEnd2; pSrc2++,pDst2++ )
			{
				_mm_storeu_si128( pDst2,DoBlend( _mm_loadu_si128( pDst2 ),_mm_load_si128( pSrc2 ) ) );
			}

			// post amble
			const Color* pSrc3 = reinterpret_cast<const Color*>( pSrc2 );
			const Color* const pSrcEnd3 = pSrc3 +
				( srcRect.right - srcRightAligned );
			Color* pDst3 = reinterpret_cast<Color*>( pDst2 );
			for( ; pSrc3 < pSrcEnd3; pSrc3++,pDst3++ )
			{
				// load (premultiplied) source and destination pixels
				const Color d = *pDst3;
				const Color s = *pSrc3;

				// extract alpha complement
				const unsigned int ca = s >> 24;

				// unpack source components and blend channels
				const unsigned int rsltRed = ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8 ) & mask ) * ca ) >> 8;
				const unsigned int rsltBlue = ( ( d         & mask ) * ca ) >> 8;

				// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
				*pDst3 = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
			}
		}
	}
	void BltAlphaPremultipliedSSSE3UDstA( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		if( srcRect.GetWidth() < 4 )
		{
			BltAlphaPremultipliedPacked( dstPt,srcRect,src );
			return;
		}

		// constants
		const __m128i zero = _mm_setzero_si128();
		const __m128i alphaShuffleLo = _mm_set_epi8(
			128u,128u,128u,7u,128u,7u,128u,7u,
			128u,128u,128u,3u,128u,3u,128u,3u );
		const __m128i alphaShuffleHi = _mm_set_epi8(
			128u,128u,128u,15u,128u,15u,128u,15u,
			128u,128u,128u,11u,128u,11u,128u,11u );
		const auto DoBlend = [=]( const __m128i dstPixels,const __m128i srcPixels )
		{
			const __m128i dstLo16 = _mm_unpacklo_epi8( zero,dstPixels );
			const __m128i dstHi16 = _mm_unpackhi_epi8( zero,dstPixels );
			const __m128i calphaLo = _mm_shuffle_epi8( srcPixels,alphaShuffleLo );
			const __m128i calphaHi = _mm_shuffle_epi8( srcPixels,alphaShuffleHi );
			const __m128i rsltDstLo16 = _mm_mulhi_epu16( dstLo16,calphaLo );
			const __m128i rsltDstHi16 = _mm_mulhi_epu16( dstHi16,calphaHi );
			const __m128i rsltDst = _mm_packus_epi16( rsltDstLo16,rsltDstHi16 );
			const __m128i rslt = _mm_add_epi8( rsltDst,srcPixels );
			return rslt;
		};

		const int dstLeftAligned = ( dstPt.x + 3 ) & ~0x03;
		const int dstRightAligned = ( dstPt.x + srcRect.GetWidth() ) & ~0x03;
		const int srcPitch = src.GetPixelPitch();
		const int dstPitch = GetPixelPitch();
		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			// preamble
			const unsigned int mask = 0xFF;
			const Color* pSrc = &src.GetBufferConst()[srcRect.left + srcPitch * ySrc];
			const Color* const pSrcEnd = pSrc + ( dstLeftAligned - dstPt.x );
			Color* pDst = &GetBuffer()[dstPt.x + dstPitch * yDst];
			for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				// load (premultiplied) source and destination pixels
				const Color d = *pDst;
				const Color s = *pSrc;

				// extract alpha complement
				const unsigned int ca = s >> 24;

				// unpack source components and blend channels
				const unsigned int rsltRed = ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8 ) & mask ) * ca ) >> 8;
				const unsigned int rsltBlue = ( ( d         & mask ) * ca ) >> 8;

				// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
				*pDst = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
			}

			// main loop
			const __m128i* pSrc2 = reinterpret_cast<const __m128i*>( pSrc );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd2 = pSrc2 + ( dstRightAligned - dstLeftAligned ) / 4;
			__m128i* pDst2 = reinterpret_cast<__m128i*>( pDst );
			for( ; pSrc2 < pSrcEnd2; pSrc2++,pDst2++ )
			{
				_mm_store_si128( pDst2,DoBlend( _mm_load_si128( pDst2 ),_mm_loadu_si128( pSrc2 ) ) );
			}

			// post amble
			const Color* pSrc3 = reinterpret_cast<const Color*>( pSrc2 );
			const Color* const pSrcEnd3 = pSrc3 +
				( ( dstPt.x + srcRect.GetWidth() ) - dstRightAligned );
			Color* pDst3 = reinterpret_cast<Color*>( pDst2 );
			for( ; pSrc3 < pSrcEnd3; pSrc3++,pDst3++ )
			{
				// load (premultiplied) source and destination pixels
				const Color d = *pDst3;
				const Color s = *pSrc3;

				// extract alpha complement
				const unsigned int ca = s >> 24;

				// unpack source components and blend channels
				const unsigned int rsltRed = ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8 ) & mask ) * ca ) >> 8;
				const unsigned int rsltBlue = ( ( d         & mask ) * ca ) >> 8;

				// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
				*pDst3 = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
			}
		}
	}
	void BltAlphaPremultipliedSSSE3( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		if( srcRect.GetWidth() < 4 )
		{
			BltAlphaPremultipliedPacked( dstPt,srcRect,src );
			return;
		}

		const int srcLeftAligned = ( srcRect.left + 3 ) & ~0x03;
		const int srcRightAligned = srcRect.right & ~0x03;
		if( srcLeftAligned == srcRect.left && srcRightAligned == srcRect.right )
		{
			BltAlphaPremultipliedSSSE3SrcAlignedTemplated( dstPt,srcRect,src );
			return;
		}

		const int alignment = ( dstPt.x + ( srcLeftAligned - srcRect.left ) ) & 0x03;
		switch( alignment )
		{
		case 0:
			BltAlphaPremultipliedSSSE3DstAligned( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		case 1:
			BltAlphaPremultipliedSSSE3Template<1>( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		case 2:
			BltAlphaPremultipliedSSSE3Template<2>( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		case 3:
			BltAlphaPremultipliedSSSE3Template<3>( dstPt,srcRect,src,srcLeftAligned,srcRightAligned );
			break;
		}
	}
	void BltAlphaPremultipliedSSSE3UDstAClip( Vei2 dstPt,const RectI& srcRectIn,const Surface& src,const RectI& clip )
	{
		RectI srcRect = srcRectIn;
		const int dstRight = dstPt.x + srcRect.GetWidth();
		if( dstRight > clip.right )
		{
			srcRect.right -= dstPt.x + dstRight - clip.right;
		}
		const int dstBottom = dstPt.y + srcRect.GetHeight();
		if( dstBottom > clip.bottom )
		{
			srcRect.bottom -= dstPt.y + dstBottom - clip.bottom;
		}
		if( dstPt.x < clip.left )
		{
			srcRect.left += clip.left - dstPt.x;
			dstPt.x = clip.left;
		}
		if( dstPt.y < clip.top )
		{
			srcRect.top += clip.top - dstPt.y;
			dstPt.y = clip.top;
		}
		BltAlphaPremultipliedSSSE3UDstA( dstPt,srcRect,src );
	}

private:
	static unsigned int CalculatePixelPitch( unsigned int width,unsigned int byteAlignment )
	{
		assert( byteAlignment % 4 == 0 );
		const unsigned int pixelAlignment = byteAlignment / sizeof( Color );
		return width + ( pixelAlignment - width % pixelAlignment ) % pixelAlignment;
	}
	
	////////////////////////////////////////////////////////////////////////
	//  Stuff for SSE Blt
	//
	template<int alignment>
	void BltSSESrcAlignedRowTemplate( const __m128i*& pSrc,
		const __m128i* const pSrcEnd,__m128i*& pDst )
	{
		// prime the pump with destination pixels that will not be overwritten (pre-shift)
		__m128i prevSrc = _mm_slli_si128( _mm_load_si128( pDst ),( 4 - alignment ) * 4 );
		for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
		{
			const __m128i newSrc = _mm_load_si128( pSrc );
			const __m128i output = _mm_or_si128(
				_mm_srli_si128( prevSrc,( 4 - alignment ) * 4 ),_mm_slli_si128( newSrc,alignment * 4 ) );
			_mm_store_si128( pDst,output );
			prevSrc = newSrc;
		}
		// finish off the last section that contains dest pixels that will not be overwritten
		const __m128i dstFinal = _mm_slli_si128(
			_mm_srli_si128( _mm_load_si128( pDst ),alignment * 4 ),alignment * 4 );
		const __m128i output = _mm_or_si128(
			_mm_srli_si128( prevSrc,( 4 - alignment ) * 4 ),dstFinal );
		_mm_store_si128( pDst,output );
	}
	template<int alignment>
	void BltSSSE3SrcAlignedRowTemplate( const __m128i*& pSrc,
		const __m128i* const pSrcEnd,__m128i*& pDst )
	{
		// prime the pump with destination pixels that will not be overwritten (pre-shift)
		__m128i prevSrc = _mm_slli_si128( _mm_load_si128( pDst ),( 4 - alignment ) * 4 );
		for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
		{
			const __m128i newSrc = _mm_load_si128( pSrc );
			const __m128i output = _mm_alignr_epi8( newSrc,prevSrc,( 4 - alignment ) * 4 );
			_mm_store_si128( pDst,output );
			prevSrc = newSrc;
		}
		// finish off the last section that contains dest pixels that will not be overwritten
		const __m128i dstFinal = _mm_srli_si128( _mm_load_si128( pDst ),alignment * 4 );
		const __m128i output = _mm_alignr_epi8( dstFinal,prevSrc,( 4 - alignment ) * 4 );
		_mm_store_si128( pDst,output );
	}
	void BltSSESrcDstAlignedRow( const __m128i*& pSrc,
		const __m128i* const pSrcEnd,__m128i*& pDst )
	{
		for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
		{
			_mm_store_si128( pDst,_mm_load_si128( pSrc ) );
		}
	}

	template<int alignment>
	void BltSSESrcAlignedTemplate( Vei2 dstPt,const RectI& srcRect,
		const Surface& src )
	{
		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );

		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const __m128i* pSrc = reinterpret_cast<const __m128i*>(
				&src.GetBufferConst()[srcRect.left + srcPitch * ySrc] );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd = pSrc + srcRect.GetWidth() / 4;
			// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
			__m128i* pDst = reinterpret_cast<__m128i*>(
				&GetBuffer()[( dstPt.x & ~0x03 ) + dstPitch * yDst] );

			BltSSESrcAlignedRowTemplate<alignment>( pSrc,pSrcEnd,pDst );
		}
	}
	template<int alignment>
	void BltSSSE3SrcAlignedTemplate( Vei2 dstPt,const RectI& srcRect,
		const Surface& src )
	{
		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );

		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const __m128i* pSrc = reinterpret_cast<const __m128i*>(
				&src.GetBufferConst()[srcRect.left + srcPitch * ySrc] );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd = pSrc + srcRect.GetWidth() / 4;
			// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
			__m128i* pDst = reinterpret_cast<__m128i*>(
				&GetBuffer()[( dstPt.x & ~0x03 ) + dstPitch * yDst] );

			BltSSSE3SrcAlignedRowTemplate<alignment>( pSrc,pSrcEnd,pDst );
		}
	}
	
	template<int alignment>
	void BltSSETemplate( Vei2 dstPt,const RectI& srcRect,
		const Surface& src,const int srcLeftAligned,const int srcRightAligned )
	{
		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );

		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const Color* pSrc = &src.GetBufferConst()[srcRect.left + srcPitch * ySrc];
			Color* pDst = &GetBuffer()[dstPt.x + dstPitch * yDst];
			for( const Color* const pSrcEnd = pSrc + ( srcLeftAligned - srcRect.left );
				pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				*pDst = *pSrc;
			}

			const __m128i* pSrc2 = reinterpret_cast<const __m128i*>( pSrc );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd2 = pSrc2 + (srcRightAligned - srcLeftAligned) / 4;
			// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
			__m128i* pDst2 = reinterpret_cast<__m128i*>( uintptr_t( pDst ) & ~0x0F );

			BltSSESrcAlignedRowTemplate<alignment>( pSrc2,pSrcEnd2,pDst2 );
			
			const Color* pSrc3 = reinterpret_cast<const Color*>( pSrc2 );
			Color* pDst3 = reinterpret_cast<Color*>( pDst2 ) + alignment;
			for( const Color* const pSrcEnd3 = 
				pSrc3 + ( srcRect.right - srcRightAligned );
				pSrc3 < pSrcEnd3; pSrc3++,pDst3++ )
			{
				*pDst3 = *pSrc3;
			}
		}
	}
	template<int alignment>
	void BltSSSE3Template( Vei2 dstPt,const RectI& srcRect,
		const Surface& src,const int srcLeftAligned,const int srcRightAligned )
	{
		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );

		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const Color* pSrc = &src.GetBufferConst()[srcRect.left + srcPitch * ySrc];
			Color* pDst = &GetBuffer()[dstPt.x + dstPitch * yDst];
			for( const Color* const pSrcEnd = pSrc + ( srcLeftAligned - srcRect.left );
				pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				*pDst = *pSrc;
			}

			const __m128i* pSrc2 = reinterpret_cast<const __m128i*>( pSrc );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd2 = pSrc2 + ( srcRightAligned - srcLeftAligned ) / 4;
			// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
			__m128i* pDst2 = reinterpret_cast<__m128i*>( uintptr_t( pDst ) & ~0x0F );

			BltSSSE3SrcAlignedRowTemplate<alignment>( pSrc2,pSrcEnd2,pDst2 );

			const Color* pSrc3 = reinterpret_cast<const Color*>( pSrc2 );
			// suspicious
			Color* pDst3 = reinterpret_cast<Color*>(pDst2) + alignment;
			for( const Color* const pSrcEnd3 =
				pSrc3 + ( srcRect.right - srcRightAligned );
				pSrc3 < pSrcEnd3; pSrc3++,pDst3++ )
			{
				*pDst3 = *pSrc3;
			}
		}
	}
	// case where destination is aligned AFTER loop peeling to handle source alignment
	void BltSSEDstAligned( Vei2 dstPt,const RectI& srcRect,
		const Surface& src,const int srcLeftAligned,const int srcRightAligned )
	{
		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );

		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const Color* pSrc = &src.GetBufferConst()[srcRect.left + srcPitch * ySrc];
			Color* pDst = &GetBuffer()[dstPt.x + dstPitch * yDst];
			for( const Color* const pSrcEnd = pSrc + ( srcLeftAligned - srcRect.left );
				pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				*pDst = *pSrc;
			}

			const __m128i* pSrc2 = reinterpret_cast<const __m128i*>( pSrc );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd2 = pSrc2 + ( srcRightAligned - srcLeftAligned ) / 4;
			// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
			__m128i* pDst2 = reinterpret_cast<__m128i*>( uintptr_t( pDst ) & ~0x0F );

			BltSSESrcDstAlignedRow( pSrc2,pSrcEnd2,pDst2 );

			const Color* pSrc3 = reinterpret_cast<const Color*>( pSrc2 );
			Color* pDst3 = reinterpret_cast<Color*>(pDst2);
			for( const Color* const pSrcEnd3 =
				pSrc3 + ( srcRect.right - srcRightAligned );
				pSrc3 < pSrcEnd3; pSrc3++,pDst3++ )
			{
				*pDst3 = *pSrc3;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////
	//  Stuff for SSE BltAlpha
	//

	// This was a little tricky!!
	template<int alignment>
	void BltAlphaPremultipliedSSSE3SrcAlignedRowTemplate( const __m128i*& pSrc,
		const __m128i* const pSrcEnd,__m128i*& pDst )
	{
		// constants
		const __m128i zero = _mm_setzero_si128();
		const __m128i alphaShuffleLo = _mm_set_epi8(
			128u,128u,128u,7u,128u,7u,128u,7u,
			128u,128u,128u,3u,128u,3u,128u,3u );
		const __m128i alphaShuffleHi = _mm_set_epi8(
			128u,128u,128u,15u,128u,15u,128u,15u,
			128u,128u,128u,11u,128u,11u,128u,11u );
		const auto DoBlend = [=]( const __m128i dstPixels,const __m128i srcPixels )
		{
			const __m128i dstLo16 = _mm_unpacklo_epi8( zero,dstPixels );
			const __m128i dstHi16 = _mm_unpackhi_epi8( zero,dstPixels );
			const __m128i calphaLo = _mm_shuffle_epi8( srcPixels,alphaShuffleLo );
			const __m128i calphaHi = _mm_shuffle_epi8( srcPixels,alphaShuffleHi );
			const __m128i rsltDstLo16 = _mm_mulhi_epu16( dstLo16,calphaLo );
			const __m128i rsltDstHi16 = _mm_mulhi_epu16( dstHi16,calphaHi );
			const __m128i rsltDst = _mm_packus_epi16( rsltDstLo16,rsltDstHi16 );
			const __m128i rslt = _mm_add_epi8( rsltDst,srcPixels );
			return rslt;
		};

		// prime the pump with FF ca pixels (completely transparent) that won't overwrite
		__m128i prevSrc = _mm_cmpeq_epi32( alphaShuffleLo,alphaShuffleLo );
		prevSrc = _mm_slli_epi32( prevSrc,24 );

		for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
		{
			const __m128i newSrc = _mm_load_si128( pSrc );
			const __m128i output = _mm_alignr_epi8( newSrc,prevSrc,( 4 - alignment ) * 4 );
			_mm_store_si128( pDst,DoBlend( _mm_load_si128( pDst ),output ) );
			prevSrc = newSrc;
		}
		// finish off the last section that contains dest pixels that will not be overwritten
		__m128i dstFinal = _mm_cmpeq_epi32( alphaShuffleLo,alphaShuffleLo );
		dstFinal = _mm_slli_epi32( dstFinal,24 );
      	const __m128i output = _mm_alignr_epi8( dstFinal,prevSrc,( 4 - alignment ) * 4 );
		_mm_store_si128( pDst,DoBlend( _mm_load_si128( pDst ),output ) );
	}
	template<int alignment>
	void BltAlphaPremultipliedSSSE3Template( Vei2 dstPt,const RectI& srcRect,
		const Surface& src,const int srcLeftAligned,const int srcRightAligned )
	{
		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );

		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const unsigned int mask = 0xFF;
			const Color* pSrc = &src.GetBufferConst()[srcRect.left + srcPitch * ySrc];
			Color* pDst = &GetBuffer()[dstPt.x + dstPitch * yDst];
			for( const Color* const pSrcEnd = pSrc + ( srcLeftAligned - srcRect.left );
				pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				// load (premultiplied) source and destination pixels
				const Color d = *pDst;
				const Color s = *pSrc;

				// extract alpha complement
				const unsigned int ca = s >> 24;

				// unpack source components and blend channels
				const unsigned int rsltRed = ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8 ) & mask ) * ca ) >> 8;
				const unsigned int rsltBlue = ( ( d         & mask ) * ca ) >> 8;

				// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
				*pDst = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
			}

			const __m128i* pSrc2 = reinterpret_cast<const __m128i*>( pSrc );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd2 = pSrc2 + ( srcRightAligned - srcLeftAligned ) / 4;
			// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
			__m128i* pDst2 = reinterpret_cast<__m128i*>( uintptr_t( pDst ) & ~0x0F );

			BltAlphaPremultipliedSSSE3SrcAlignedRowTemplate<alignment>( pSrc2,pSrcEnd2,pDst2 );

			const Color* pSrc3 = reinterpret_cast<const Color*>( pSrc2 );
			Color* pDst3 = reinterpret_cast<Color*>(pDst2)+alignment;
			for( const Color* const pSrcEnd3 =
				pSrc3 + ( srcRect.right - srcRightAligned );
				pSrc3 < pSrcEnd3; pSrc3++,pDst3++ )
			{
				// load (premultiplied) source and destination pixels
				// i forgot to change to '3' when copypasta
				const Color d = *pDst3;
				const Color s = *pSrc3;

				// extract alpha complement
				const unsigned int ca = s >> 24;

				// unpack source components and blend channels
				const unsigned int rsltRed = ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8 ) & mask ) * ca ) >> 8;
				const unsigned int rsltBlue = ( ( d         & mask ) * ca ) >> 8;

				// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
				*pDst3 = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
			}
		}
	}
	void BltAlphaPremultipliedSSSE3DstAligned( Vei2 dstPt,const RectI& srcRect,
		const Surface& src,const int srcLeftAligned,const int srcRightAligned )
	{
		// constants
		const __m128i zero = _mm_setzero_si128();
		const __m128i alphaShuffleLo = _mm_set_epi8(
			128u,128u,128u,7u,128u,7u,128u,7u,
			128u,128u,128u,3u,128u,3u,128u,3u );
		const __m128i alphaShuffleHi = _mm_set_epi8(
			128u,128u,128u,15u,128u,15u,128u,15u,
			128u,128u,128u,11u,128u,11u,128u,11u );
		const auto DoBlend = [=]( const __m128i dstPixels,const __m128i srcPixels )
		{
			const __m128i dstLo16 = _mm_unpacklo_epi8( zero,dstPixels );
			const __m128i dstHi16 = _mm_unpackhi_epi8( zero,dstPixels );
			const __m128i calphaLo = _mm_shuffle_epi8( srcPixels,alphaShuffleLo );
			const __m128i calphaHi = _mm_shuffle_epi8( srcPixels,alphaShuffleHi );
			const __m128i rsltDstLo16 = _mm_mulhi_epu16( dstLo16,calphaLo );
			const __m128i rsltDstHi16 = _mm_mulhi_epu16( dstHi16,calphaHi );
			const __m128i rsltDst = _mm_packus_epi16( rsltDstLo16,rsltDstHi16 );
			const __m128i rslt = _mm_add_epi8( rsltDst,srcPixels );
			return rslt;
		};

		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );

		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const unsigned int mask = 0xFF;
			const Color* pSrc = &src.GetBufferConst()[srcRect.left + srcPitch * ySrc];
			Color* pDst = &GetBuffer()[dstPt.x + dstPitch * yDst];
			for( const Color* const pSrcEnd = pSrc + ( srcLeftAligned - srcRect.left );
				pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				// load (premultiplied) source and destination pixels
				const Color d = *pDst;
				const Color s = *pSrc;

				// extract alpha complement
				const unsigned int ca = s >> 24;

				// unpack source components and blend channels
				const unsigned int rsltRed = ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8 ) & mask ) * ca ) >> 8;
				const unsigned int rsltBlue = ( ( d         & mask ) * ca ) >> 8;

				// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
				*pDst = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
			}

			const __m128i* pSrc2 = reinterpret_cast<const __m128i*>( pSrc );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd2 = pSrc2 + ( srcRightAligned - srcLeftAligned ) / 4;
			// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
			__m128i* pDst2 = reinterpret_cast<__m128i*>( uintptr_t( pDst ) & ~0x0F );

			BltAlphaPremultipliedSSESrcDstAlignedRow( pSrc2,pSrcEnd2,pDst2 );

			const Color* pSrc3 = reinterpret_cast<const Color*>( pSrc2 );
			Color* pDst3 = reinterpret_cast<Color*>( pDst2 );
			for( const Color* const pSrcEnd3 =
				pSrc3 + ( srcRect.right - srcRightAligned );
				pSrc3 < pSrcEnd3; pSrc3++,pDst3++ )
			{
				// load (premultiplied) source and destination pixels
				const Color d = *pDst3;
				const Color s = *pSrc3;

				// extract alpha complement
				const unsigned int ca = s >> 24;

				// unpack source components and blend channels
				const unsigned int rsltRed = ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8 ) & mask ) * ca ) >> 8;
				const unsigned int rsltBlue = ( ( d         & mask ) * ca ) >> 8;

				// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
				*pDst3 = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
			}
		}
	}
	void BltAlphaPremultipliedSSSE3SrcAlignedTemplated( Vei2 dstPt,const RectI& srcRect, const Surface& src )
	{
		const int alignment = dstPt.x % 4;
		switch( alignment )
		{
		case 0:
			BltAlphaPremultipliedSSESrcDstAligned( dstPt,srcRect,src );
			break;
		case 1:
			BltAlphaPremultipliedSSSE3SrcAlignedTemplate<1>( dstPt,srcRect,src );
			break;
		case 2:
			BltAlphaPremultipliedSSSE3SrcAlignedTemplate<2>( dstPt,srcRect,src );
			break;
		case 3:
			BltAlphaPremultipliedSSSE3SrcAlignedTemplate<3>( dstPt,srcRect,src );
			break;
		}
	}
	template<int alignment>
	void BltAlphaPremultipliedSSSE3SrcAlignedTemplate( Vei2 dstPt,const RectI& srcRect,
		const Surface& src )
	{
		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );

		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const __m128i* pSrc = reinterpret_cast<const __m128i*>(
				&src.GetBufferConst()[srcRect.left + srcPitch * ySrc] );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd = pSrc + srcRect.GetWidth() / 4;
			// round down to nearest multiple of 4 pixel (down to nearest 16-byte aligned)
			__m128i* pDst = reinterpret_cast<__m128i*>(
				&GetBuffer()[( dstPt.x & ~0x03 ) + dstPitch * yDst] );

			BltAlphaPremultipliedSSSE3SrcAlignedRowTemplate<alignment>( pSrc,pSrcEnd,pDst );
		}
	}
	void BltAlphaPremultipliedSSESrcDstAligned( Vei2 dstPt,const RectI& srcRect,const Surface& src )
	{
		// constants
		const __m128i zero = _mm_setzero_si128();
		const __m128i alphaShuffleLo = _mm_set_epi8(
			128u,128u,128u,7u,128u,7u,128u,7u,
			128u,128u,128u,3u,128u,3u,128u,3u );
		const __m128i alphaShuffleHi = _mm_set_epi8(
			128u,128u,128u,15u,128u,15u,128u,15u,
			128u,128u,128u,11u,128u,11u,128u,11u );
		const auto DoBlend = [=]( const __m128i dstPixels,const __m128i srcPixels )
		{
			const __m128i dstLo16 = _mm_unpacklo_epi8( zero,dstPixels );
			const __m128i dstHi16 = _mm_unpackhi_epi8( zero,dstPixels );
			const __m128i calphaLo = _mm_shuffle_epi8( srcPixels,alphaShuffleLo );
			const __m128i calphaHi = _mm_shuffle_epi8( srcPixels,alphaShuffleHi );
			const __m128i rsltDstLo16 = _mm_mulhi_epu16( dstLo16,calphaLo );
			const __m128i rsltDstHi16 = _mm_mulhi_epu16( dstHi16,calphaHi );
			const __m128i rsltDst = _mm_packus_epi16( rsltDstLo16,rsltDstHi16 );
			const __m128i rslt = _mm_add_epi8( rsltDst,srcPixels );
			return rslt;
		};

		const int srcPitch = int( src.GetPixelPitch() );
		const int dstPitch = int( GetPixelPitch() );
		for( int ySrc = srcRect.top,ySrcEnd = srcRect.bottom,yDst = dstPt.y;
			ySrc < ySrcEnd; ySrc++,yDst++ )
		{
			const __m128i* pSrc = reinterpret_cast<const __m128i*>(
				&src.GetBufferConst()[srcRect.left + srcPitch * ySrc] );
			// divide by 4 because were working with 128-bit pointer arithmetic
			const __m128i* const pSrcEnd = pSrc + srcRect.GetWidth() / 4;
			__m128i* pDst = reinterpret_cast<__m128i*>(
				&GetBuffer()[dstPt.x + dstPitch * yDst] );
			for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
			{
				_mm_store_si128( pDst,DoBlend( _mm_load_si128( pDst ) ,_mm_load_si128( pSrc ) ) );
			}
		}
	}
	void BltAlphaPremultipliedSSESrcDstAlignedRow( const __m128i*& pSrc,
		const __m128i* const pSrcEnd,__m128i*& pDst )
	{
		// constants
		const __m128i zero = _mm_setzero_si128();
		const __m128i alphaShuffleLo = _mm_set_epi8(
			128u,128u,128u,7u,128u,7u,128u,7u,
			128u,128u,128u,3u,128u,3u,128u,3u );
		const __m128i alphaShuffleHi = _mm_set_epi8(
			128u,128u,128u,15u,128u,15u,128u,15u,
			128u,128u,128u,11u,128u,11u,128u,11u );
		const auto DoBlend = [=]( const __m128i dstPixels,const __m128i srcPixels )
		{
			const __m128i dstLo16 = _mm_unpacklo_epi8( zero,dstPixels );
			const __m128i dstHi16 = _mm_unpackhi_epi8( zero,dstPixels );
			const __m128i calphaLo = _mm_shuffle_epi8( srcPixels,alphaShuffleLo );
			const __m128i calphaHi = _mm_shuffle_epi8( srcPixels,alphaShuffleHi );
			const __m128i rsltDstLo16 = _mm_mulhi_epu16( dstLo16,calphaLo );
			const __m128i rsltDstHi16 = _mm_mulhi_epu16( dstHi16,calphaHi );
			const __m128i rsltDst = _mm_packus_epi16( rsltDstLo16,rsltDstHi16 );
			const __m128i rslt = _mm_add_epi8( rsltDst,srcPixels );
			return rslt;
		};
		for( ; pSrc < pSrcEnd; pSrc++,pDst++ )
		{
			_mm_store_si128( pDst,DoBlend( _mm_load_si128( pDst ),_mm_load_si128( pSrc ) ) );
		}
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