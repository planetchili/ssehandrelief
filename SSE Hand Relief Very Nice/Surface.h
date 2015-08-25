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
	Surface( const Surface& src )
		:
		buffer( nullptr ),
		width( src.width ),
		height( src.height ),
		pixelPitch( src.pixelPitch )
	{
		buffer = new Color[height * pixelPitch];
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
			delete[] buffer;
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
				PutPixel( x,y,{ d.x,rsltRed,rsltGreen,rsltBlue } );
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
				PutPixel( x,y,c );
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
			const unsigned int r = ( ( ( src >> 16 ) & mask ) * a ) >> 8;
			const unsigned int g = ( ( ( src >> 8  ) & mask ) * a ) >> 8;
			const unsigned int b = ( (   src         & mask ) * a ) >> 8;
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
	//void FadeHalfShift()
	//{
	//	const unsigned int mask = 0x7F;
	//	for( Color* i = buffer,*end = &buffer[pixelPitch * height]; i < end; i++ )
	//	{
	//		const Color src = *i;
	//		const unsigned int r = ( src >> 17 ) & mask;
	//		const unsigned int g = ( src >> 9 )  & mask;
	//		const unsigned int b = ( src >> 1 )  & mask;
	//		*i = ( r << 16 ) | ( g << 8 ) | b;
	//	}
	//}
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
	void Blend( Surface& s,unsigned char alpha )
	{
		const unsigned int mask = 0xFF;
		const unsigned int a = alpha;
		for( Color* i = buffer,*end = &buffer[pixelPitch * height], *j = s.GetBuffer(); 
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
	void BlendHalfPacked( Surface& s )
	{
		const unsigned int shiftMask = 0x007F7F7F;
		for( Color* i = buffer,*end = &buffer[pixelPitch * height],*j = s.GetBuffer();
			i < end; i++,j++ )
		{
			// divide source and destination channels by 2 and sum (no carry will happen)
			*i = ( ( *i >> 1 ) & shiftMask ) + ( ( *j >> 1 ) & shiftMask );
		}
	}
	void DrawRect( RectI& rect,Color c )
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
	void DrawRectBlendPrecomputedPacked( RectI& rect,Color c )
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
	void DrawRectBlendHalfPacked( RectI& rect,Color c )
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
	void Blt( Vei2 dstPt,RectI& srcRect,Surface& src )
	{
		for( int yDst = dstPt.y, 
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth(),
				*j = &src.GetBuffer()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
				i < iEnd; i++,j++ )
			{
				*i = *j;
			}
		}
	}
	void BltBlend( Vei2 dstPt,RectI& srcRect,Surface& src,unsigned char alpha )
	{
		const unsigned int mask = 0xFF;
		const unsigned int a = alpha;
		for( int yDst = dstPt.y,
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth(),
				*j = &src.GetBuffer()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
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
	void BltBlendHalfPacked( Vei2 dstPt,RectI& srcRect,Surface& src )
	{
		const unsigned int shiftMask = 0x007F7F7F;
		for( int yDst = dstPt.y,
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth(),
				*j = &src.GetBuffer()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
				i < iEnd; i++,j++ )
			{
				// divide source and destination channels by 2 and sum (no carry will happen)
				*i = ( ( *i >> 1 ) & shiftMask ) + ( ( *j >> 1 ) & shiftMask );
			}
		}
	}
	void BltAlpha( Vei2 dstPt,RectI& srcRect,Surface& src )
	{
		const unsigned int mask = 0xFF;
		for( int yDst = dstPt.y,
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth(),
				*j = &src.GetBuffer()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
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
	void BltAlphaPremultipliedPacked( Vei2 dstPt,RectI& srcRect,Surface& src )
	{
		const unsigned int mask = 0xFF;
		for( int yDst = dstPt.y,
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth(),
				*j = &src.GetBuffer()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
				i < iEnd; i++,j++ )
			{
				// load (premultiplied) source and destination pixels
				const Color d = *i;
				const Color s = *j;

				// calculate alpha complement
				const unsigned int ca = 255 - (s >> 24);

				// unpack source components and blend channels
				const unsigned int rsltRed =   ( ( ( d >> 16 ) & mask ) * ca ) >> 8;
				const unsigned int rsltGreen = ( ( ( d >> 8  ) & mask ) * ca ) >> 8;
				const unsigned int rsltBlue =  ( (   d         & mask ) * ca ) >> 8;

				// pack channels back into pixel, add premultiplied packed components, and fire pixel onto surface
				*i = ( ( rsltRed << 16 ) | ( rsltGreen << 8 ) | rsltBlue ) + s;
			}
		}
	}
	void BltKey( Vei2 dstPt,RectI& srcRect,Surface& src,Color key )
	{
		for( int yDst = dstPt.y,
			yDstEnd = yDst + srcRect.GetHeight(),
			ySrc = srcRect.top;
			yDst < yDstEnd; yDst++,ySrc++ )
		{
			for( Color* i = &buffer[yDst * width + dstPt.x],
				*iEnd = i + srcRect.GetWidth(),
				*j = &src.GetBuffer()[ySrc * (int)src.GetPixelPitch() + srcRect.left];
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