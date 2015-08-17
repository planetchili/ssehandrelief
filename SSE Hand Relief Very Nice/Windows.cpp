/****************************************************************************************** 
 *	Chili DirectX Framework Version 14.03.22											  *	
 *	Windows.cpp																			  *
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
#include <Windows.h>
#include <wchar.h>
#include "D3DGraphics.h"
#include "Game.h"
#include "resource.h"
#include "Mouse.h"

static KeyboardServer kServ;
static MouseServer mServ;

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

		// ************ KEYBOARD MESSAGES ************ //
		case WM_KEYDOWN:
			if( !( lParam & 0x40000000 ) ) // no thank you on the autorepeat
			{
				kServ.OnKeyPressed( (unsigned char)wParam );
			}
			break;
		case WM_KEYUP:
			kServ.OnKeyReleased( (unsigned char)wParam );
			break;
		case WM_CHAR:
			kServ.OnChar( (unsigned char)wParam );
			break;
		// ************ END KEYBOARD MESSAGES ************ //

		// ************ MOUSE MESSAGES ************ //
		case WM_MOUSEMOVE:
		{
			int x = (short)LOWORD( lParam );
			int y = (short)HIWORD( lParam );
			if( x > 0 && x < D3DGraphics::screenWidth && y > 0 && y < D3DGraphics::screenHeight )
			{
				mServ.OnMouseMove( x,y );
				if( !mServ.IsInWindow() )
				{
					SetCapture( hWnd );
					mServ.OnMouseEnter();
				}
			}
			else
			{
				if( wParam & (MK_LBUTTON | MK_RBUTTON) )
				{
					x = max( 0,x );
					x = min( D3DGraphics::screenWidth - 1,x );
					y = max( 0,y );
					y = min( D3DGraphics::screenHeight - 1,y );
					mServ.OnMouseMove( x,y );
				}
				else
				{
					ReleaseCapture();
					mServ.OnMouseLeave();
					mServ.OnLeftReleased( x,y );
					mServ.OnRightReleased( x,y );
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			int x = (short)LOWORD( lParam );
			int y = (short)HIWORD( lParam );
			mServ.OnLeftPressed( x,y );
			break;
		}
		case WM_RBUTTONDOWN:
		{
			int x = (short)LOWORD( lParam );
			int y = (short)HIWORD( lParam );
			mServ.OnRightPressed( x,y );
			break;
		}
		case WM_LBUTTONUP:
		{
			int x = (short)LOWORD( lParam );
			int y = (short)HIWORD( lParam );
			mServ.OnLeftReleased( x,y );
			break;
		}
		case WM_RBUTTONUP:
		{
			int x = (short)LOWORD( lParam );
			int y = (short)HIWORD( lParam );
			mServ.OnRightReleased( x,y );
			break;
		}
		case WM_MOUSEWHEEL:
		{
			int x = (short)LOWORD( lParam );
			int y = (short)HIWORD( lParam );
			if( GET_WHEEL_DELTA_WPARAM( wParam ) > 0 )
			{
				mServ.OnWheelUp( x,y );
			}
			else if( GET_WHEEL_DELTA_WPARAM( wParam ) < 0 )
			{
				mServ.OnWheelDown( x,y );
			}
			break;
		}
		// ************ END MOUSE MESSAGES ************ //
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}

int WINAPI wWinMain( HINSTANCE hInst,HINSTANCE,LPWSTR,INT )
{
	WNDCLASSEX wc = { sizeof( WNDCLASSEX ),CS_CLASSDC,MsgProc,0,0,
                      GetModuleHandle( NULL ),NULL,NULL,NULL,NULL,
                      L"Chili DirectX Framework Window",NULL };
    wc.hIconSm = (HICON)LoadImage( hInst,MAKEINTRESOURCE( IDI_APPICON16 ),IMAGE_ICON,16,16,0 );
	wc.hIcon   = (HICON)LoadImage( hInst,MAKEINTRESOURCE( IDI_APPICON32 ),IMAGE_ICON,32,32,0 );
	wc.hCursor = LoadCursor( NULL,IDC_ARROW );
    RegisterClassEx( &wc );
	
	RECT wr;
	wr.left = 650;
	wr.right = D3DGraphics::screenWidth + wr.left;
	wr.top = 150;
	wr.bottom = D3DGraphics::screenHeight + wr.top;
	AdjustWindowRect( &wr,WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,FALSE );
    HWND hWnd = CreateWindowW( L"Chili DirectX Framework Window",L"Chili DirectX Framework",
                              WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,wr.left,wr.top,wr.right-wr.left,wr.bottom-wr.top,
                              NULL,NULL,wc.hInstance,NULL );

    ShowWindow( hWnd,SW_SHOWDEFAULT );
    UpdateWindow( hWnd );

	Game theGame( hWnd,kServ,mServ );
	
    MSG msg;
    ZeroMemory( &msg,sizeof( msg ) );
	while( true )
	{
		while( PeekMessage( &msg,NULL,0,0,PM_REMOVE ) )
		{
			if( msg.message == WM_QUIT )
			{
				UnregisterClass( L"Chili DirectX Framework Window",wc.hInstance );
				return 0;
			}

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		theGame.Go();
	}
}