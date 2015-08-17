/****************************************************************************************** 
 *	Chili DirectX Framework Version 14.03.22											  *	
 *	Timer.cpp																			  *
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
#include "Timer.h"
#pragma comment(lib, "winmm.lib" )

Timer::Timer()
{
	unsigned long long frequency;
	QueryPerformanceFrequency( (LARGE_INTEGER*)&frequency );
	invFreqMilli = 1.0f / (float)((double)frequency / 1000.0);
	StartWatch();
}

void Timer::StopWatch()
{
	if( !watchStopped )
	{
		QueryPerformanceCounter( (LARGE_INTEGER*)&currentCount );
		watchStopped = true;
	}
}

void Timer::StartWatch()
{
	watchStopped = false;
	QueryPerformanceCounter( (LARGE_INTEGER*)&startCount );
}

float Timer::GetTimeSec( ) const
{
	return GetTimeMilli() / 1000.0f;
}

float Timer::GetTimeMilli() const
{
	if( !watchStopped )
	{
		QueryPerformanceCounter( (LARGE_INTEGER*)&currentCount );
		return (float)(currentCount - startCount) * invFreqMilli;
	}
	else
	{
		return (float)(currentCount - startCount) * invFreqMilli;
	}
}