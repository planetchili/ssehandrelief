/****************************************************************************************** 
 *	Chili DirectX Framework Version 14.03.22											  *	
 *	Keyboard.h																			  *
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
#include <Windows.h>
#include <queue>

class KeyEvent
{
public:
	enum EventType
	{
		Press,
		Release,
		Invalid
	};
private:
	EventType type;
	unsigned char code;
public:
	KeyEvent( EventType type,unsigned char code )
		:
	type( type ),
	code( code )
	{}
	bool IsPress() const
	{
		return type == Press;
	}
	bool IsRelease() const
	{
		return type == Release;
	}
	bool IsValid() const
	{
		return type != Invalid;
	}
	unsigned char GetCode() const
	{
		return code;
	}
};

class KeyboardServer;

class KeyboardClient
{
public:
	KeyboardClient( KeyboardServer& kServer );
	bool KeyIsPressed( unsigned char keycode ) const;
	KeyEvent ReadKey();
	KeyEvent PeekKey() const;
	bool KeyEmpty() const;
	unsigned char ReadChar();
	unsigned char PeekChar() const;
	bool CharEmpty() const;
	void FlushKeyBuffer();
	void FlushCharBuffer();
	void FlushBuffers();
private:
	KeyboardServer& server;
};

class KeyboardServer
{
	friend KeyboardClient;
public:
	KeyboardServer();
	void OnKeyPressed( unsigned char keycode );
	void OnKeyReleased( unsigned char keycode );
	void OnChar( unsigned char character );
private:
	static const int nKeys = 256;
	static const int bufferSize = 4;
	bool keystates[ nKeys ];
	std::queue<KeyEvent> keybuffer;
	std::queue<unsigned char> charbuffer;
};