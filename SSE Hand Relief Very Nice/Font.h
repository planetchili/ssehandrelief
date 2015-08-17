#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>

class Font
{
	friend class TextSurface;
public:
	enum Alignment
	{
		Left,
		Right,
		Center
	};
public:
	Font( const std::wstring& family,float size,bool bold = true )
		:
		font( family.c_str(),size,bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular )
	{}
private:
	inline operator const Gdiplus::Font*() const
	{
		return &font;
	}
private:
	Gdiplus::Font font;
};