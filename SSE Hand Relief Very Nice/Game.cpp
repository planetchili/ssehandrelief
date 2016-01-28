/****************************************************************************************** 
 *	Chili DirectX Framework Version 14.03.22											  *	
 *	Game.cpp																			  *
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
#include "Game.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <random>

Game::Game( HWND hWnd,KeyboardServer& kServer,MouseServer& mServer )
	:
	gfx( hWnd ),
	kbd( kServer ),
	mouse( mServer ),
	dice( Surface::FromFile( L"dice.png" ) ),
	bees( Surface::FromFile( L"bees.jpg" ) ),
	marle( Surface::FromFile( L"marle.png" ) ),
	flare( Surface::FromFile( L"flare.png" ) ),
	logFile( L"logfile.txt" )
{
	//flare.PremultiplyAlpha();
}

Game::~Game()
{
}

void Game::Go()
{
	gfx.BeginFrame();
	UpdateModel();
	ComposeFrame();
	gfx.EndFrame();
}

void Game::UpdateModel()
{
	while( !mouse.MouseEmpty() )
	{
		MouseEvent e = mouse.ReadMouse();
		if( e.GetType() == MouseEvent::WheelDown )
		{
			alpha -= 3;
		}
		else if( e.GetType() == MouseEvent::WheelUp )
		{
			alpha += 3;
		}
	}
}

void Game::ComposeFrame()
{
	Vei2 p = { mouse.GetMouseX(),mouse.GetMouseY() };

	gfx.sysBuffer.Copy( bees );
	ft.StartFrame();
	ft.StopFrame( logFile );
}
