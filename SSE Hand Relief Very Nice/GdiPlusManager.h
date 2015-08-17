#pragma once
#include <Windows.h>
#include <gdiplus.h>

class GdiPlusManager
{
public:
	GdiPlusManager();
	~GdiPlusManager();
private:
	ULONG_PTR gdiPlusToken = 0;
};