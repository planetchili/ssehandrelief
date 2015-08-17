#include "GdiPlusManager.h"
#pragma comment( lib,"gdiplus.lib" )

GdiPlusManager::GdiPlusManager()
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput = { 0 };
	Gdiplus::GdiplusStartup( &gdiPlusToken,&gdiplusStartupInput,nullptr );
}

GdiPlusManager::~GdiPlusManager()
{
	Gdiplus::GdiplusShutdown( gdiPlusToken );
}