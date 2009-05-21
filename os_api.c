/***************************************************************************
 *   Copyright (C) 2008-2009 by Чернов А.А.                                *
 *   valexlin@gmail.com                                                    *
 *   Golubinsky Yury - Mac OS X port                                       *
 *   urix@me.com                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "os_api.h"

#include <limits.h>

#if defined(_WIN32) || defined(_WIN64)
#define _WINDOWS
#endif

#ifdef _WINDOWS
#include <windows.h>
#endif

#if defined(__MACOS__) || defined(__APPLE__) || defined(__GLIBC__) || defined(__FreeBSD__)
#define _UNIX
#endif

#ifdef _UNIX
#include <unistd.h>
#endif

void WinSleep(int ms)
{
#ifdef _WINDOWS
	Sleep(ms);
#else
#ifdef _UNIX
	usleep(ms*1000);
#endif
#endif
}

int WinGetTickCount()
{
#ifdef _WINDOWS
	return GetTickCount();
#else
#ifdef __MACOS__
	return 100*TickCount()/6;
#endif
#endif
}

int WinProcessMsg()
{
#ifdef _WIN32
	MSG msg;
	if (GetMessage(&msg, NULL, 0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
#else
	;
#endif
}
