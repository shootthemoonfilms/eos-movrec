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

#if defined(__MACOS__) || defined(__APPLE__) || defined(__GLIBC__) || defined(__FreeBSD__)
#define _UNIX
#endif

#ifdef _WINDOWS
#include <windows.h>
#endif

#ifdef _UNIX
#include <unistd.h>
#include <sys/time.h>
#endif

void OSSleep(int ms)
{
#ifdef _WINDOWS
	Sleep(ms);
#else
#ifdef _UNIX
	usleep(ms*1000);
#endif
#endif
}

int OSGetTickCount()
{
#ifdef _WINDOWS
	return GetTickCount();
#else	// _WINDOWS
#ifdef _UNIX
	// this is not true implementation of GetTickCount() for Unix
	// but it usefull to compute time difference.
	static int start_time_sec = 0;
	struct timeval tv;
	if (start_time_sec == 0)			// first call
	{
		gettimeofday(&tv, NULL);
		start_time_sec = tv.tv_sec;
	}
	gettimeofday(&tv, NULL);
	// to exclude integer overflow decrement start time.
	return (tv.tv_sec - start_time_sec)*1000 + tv.tv_usec/1000;
#endif	// _UNIX
#endif	// _WINDOWS
}

int OSProcessMsg()
{
#ifdef _WINDOWS
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
