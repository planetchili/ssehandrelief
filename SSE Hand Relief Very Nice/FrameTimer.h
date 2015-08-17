/******************************************************************************************
*	Chili DirectX Framework Version 14.03.22											  *
*	FrameTimer.h																		  *
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
#pragma once

#include "Timer.h"
#include <float.h>
#include <sstream>
#include <iomanip>

class FrameTimer
{
public:
	FrameTimer()
		:
	timeMin( FLT_MAX ),
	timeMax( 0.0f ),
	timeSum( 0.0f ),
	lastMin( 0.0f ),
	lastMax( 0.0f ),
	lastAvg( 0.0f ),
	frameCount( 0 )
	{}
	void StartFrame()
	{		
		timer.StartWatch();
	}
	bool StopFrame()
	{
		timer.StopWatch();
		const float frameTime = timer.GetTimeMilli();
		timeSum += frameTime;
		timeMin = min( timeMin,frameTime );
		timeMax = max( timeMax,frameTime );
		frameCount++;
		if( frameCount < nFramesAvg )
		{
			return false;
		}
		else
		{
			lastAvg = timeSum / (float)nFramesAvg;
			lastMin = timeMin;
			lastMax = timeMax;
			timeSum = 0.0f;
			timeMin = FLT_MAX;
			timeMax = 0.0f;
			frameCount = 0;
			return true;
		}
	}
	template< class T >
	bool StopFrame( T& output )
	{
		if( !StopFrame() )
		{
			return false;
		}
		else
		{
			std::wstringstream ss;
			ss.precision( 3 );
			ss << L"Avg: [" << std::fixed << GetAvg() << L"] Min: [" << GetMin()
				<< L"] Max: [" << GetMax() << L"]" << std::endl;
			output << ss.str();
			return true;
		}
	}
	float GetAvg() const
	{
		return lastAvg;
	}
	float GetMin() const
	{
		return lastMin;
	}
	float GetMax() const
	{
		return lastMax;
	}

private:
	const int nFramesAvg = 20;
	Timer timer;
	float timeSum;
	float timeMin;
	float timeMax;
	int frameCount;
	float lastMin;
	float lastMax;
	float lastAvg;
};