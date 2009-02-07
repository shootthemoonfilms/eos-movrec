/***************************************************************************
 *   Copyright (C) 2008-2009 by Чернов А.А.                                *
 *   valexlin@gmail.com                                                    *
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

#include "freqtimer.h"

#include <windows.h>

int WinQueryPerformanceFrequency(__int64_t* freq)
{
	LARGE_INTEGER i;
	BOOL res = QueryPerformanceFrequency(&i);
	*freq = i.QuadPart;
	return res != 0;
}

int WinQueryPerformanceCounter(__int64_t* count)
{
	LARGE_INTEGER i;
	BOOL res = QueryPerformanceCounter(&i);
	*count = i.QuadPart;
	return res != 0;
}

void WinSleep(int ms)
{
	Sleep(ms);
}

void WorkSleep(int us)
{
	if (us <= 0)
		return;
	LARGE_INTEGER count1, count2, freq;
	QueryPerformanceCounter(&count1);
	QueryPerformanceFrequency(&freq);
	__int64_t time_cmp = freq.QuadPart * (__int64_t)us / 1000000L;
	QueryPerformanceCounter(&count2);

	while (count2.QuadPart - count1.QuadPart < time_cmp - 1)
		QueryPerformanceCounter(&count2);
}

int WinGetTickCount()
{
	return GetTickCount();
}

int WinProcessMsg()
{
	MSG msg;
	if (GetMessage(&msg, NULL, 0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
