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
#include "Cpuid.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <random>

std::vector<std::pair<RectI,Vei2>> MakeBltBatch( const RectI& src,const RectI& dst,size_t nJobs,unsigned int seed )
{
	const auto order_swap = []( RectI& rect )
	{
		if( rect.left > rect.right )
		{
			const int temp = rect.left;
			rect.left = rect.right;
			rect.right = temp;
		}
		if( rect.top > rect.bottom )
		{
			const int temp = rect.top;
			rect.top = rect.bottom;
			rect.bottom = temp;
		}
	};

	std::vector<std::pair<RectI,Vei2>> jobs;
	jobs.reserve( nJobs );

	std::mt19937 rng( seed );
	std::uniform_int_distribution<> distSrcX( 0,int( src.GetWidth() ) );
	std::uniform_int_distribution<> distSrcY( 0,int( src.GetHeight() ) );

	for( size_t i = 0; i < nJobs; i++ )
	{
		RectI srcRect;
		srcRect.left = distSrcX( rng );
		srcRect.right = distSrcX( rng );
		srcRect.top = distSrcY( rng );
		srcRect.bottom = distSrcY( rng );
		order_swap( srcRect );
		Vei2 dstPt;
		std::uniform_int_distribution<> distDstX( 0,int( dst.GetWidth() - srcRect.GetWidth() ) );
		std::uniform_int_distribution<> distDstY( 0,int( dst.GetHeight() - srcRect.GetHeight() ) );
		dstPt.x = distDstX( rng );
		dstPt.y = distDstY( rng );
		jobs.push_back( { srcRect,dstPt } );
	}

	return jobs;
}

std::vector<std::pair<RectI,Vei2>> MakeBltBatch2( const RectI& src,const RectI& dst,size_t nJobs,unsigned int seed )
{
	const auto order_swap = []( RectI& rect )
	{
		if( rect.left > rect.right )
		{
			const int temp = rect.left;
			rect.left = rect.right;
			rect.right = temp;
		}
		if( rect.top > rect.bottom )
		{
			const int temp = rect.top;
			rect.top = rect.bottom;
			rect.bottom = temp;
		}
	};

	std::vector<std::pair<RectI,Vei2>> jobs;
	jobs.reserve( nJobs );

	std::mt19937 rng( seed );
	std::uniform_int_distribution<> distSrcX( 0,int( src.GetWidth() ) );
	std::uniform_int_distribution<> distSrcY( 0,int( src.GetHeight() ) );

	for( size_t i = 0; i < nJobs; i++ )
	{
		RectI srcRect;
		srcRect.left = distSrcX( rng );
		srcRect.right = distSrcX( rng );
		srcRect.top = distSrcY( rng );
		srcRect.bottom = distSrcY( rng );
		order_swap( srcRect );
		Vei2 dstPt;
		std::uniform_int_distribution<> distDstX( -int( srcRect.GetWidth() ),dst.GetWidth() );
		std::uniform_int_distribution<> distDstY( -int( srcRect.GetHeight() ),dst.GetHeight() );
		dstPt.x = distDstX( rng );
		dstPt.y = distDstY( rng );
		jobs.push_back( { srcRect,dstPt } );
	}

	return jobs;
}

Game::Game( HWND hWnd,KeyboardServer& kServer,MouseServer& mServer )
	:
	gfx( hWnd ),
	kbd( kServer ),
	mouse( mServer ),
	bees( Surface::FromFile( L"bees.jpg" ) ),
	flare( Surface::FromFile( L"flare.png" ) ),
	logFile( L"logfile.txt" )
{
	flare.PremultiplyAlpha();

	jobs = MakeBltBatch(
		{ 0,int( flare.GetWidth() ),0,int( flare.GetHeight() ) },
		{ 0,int( gfx.sysBuffer.GetWidth() ),0,int( gfx.sysBuffer.GetHeight() ) },
		20,420 );
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
			alpha -= 1;
		}
		else if( e.GetType() == MouseEvent::WheelUp )
		{
			alpha += 1;
		}
	}

	while( !kbd.KeyEmpty() )
	{
		KeyEvent e = kbd.ReadKey();
		if( e.IsPress() )
		{
			if( e.GetCode() == VK_RIGHT )
			{
				ptk.x++;
			}
			else if( e.GetCode() == VK_LEFT )
			{
				ptk.x--;
			}
			else if( e.GetCode() == 'F' )
			{
				rectBase.right++;
			}
			else if( e.GetCode() == 'D' )
			{
				rectBase.right--;
			}
			else if( e.GetCode() == 'S' )
			{
				rectBase.left++;
			}
			else if( e.GetCode() == 'A' )
			{
				rectBase.left--;
			}
		}
	}
}

void Game::ComposeFrame()
{
	const RectI clip = { 0,int( gfx.screenWidth ),0,int( gfx.screenHeight ) };

	gfx.sysBuffer.Copy( bees );
	ft.StartFrame();
	//for( auto& job : jobs )
	//{
	//	gfx.sysBuffer.BltAlphaPremultipliedSSSE3UDstA( job.second,job.first,flare );
	//}
	ft.StopFrame( logFile );
}
