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

#include "afthread.h"
#include "livethread.h"
#include "capturewnd.h"
#include "freqtimer.h"
#include "events.h"
#include "command.h"
#include "FocuserClass.h"

#include <QWidget>
#include <QApplication>

GAFThread::GAFThread(QWidget* owner, GMyLiveThread* liveThread, GEOSCaptureWnd* capwnd)
 : QThread()
{
	Owner = owner;
	LiveThread = liveThread;
	CapWnd = capwnd;
	Stopped = false;
	fc = new FocusingClass;
}

GAFThread::~GAFThread()
{
	delete fc;
}

void GAFThread::stop()
{
	Stopped = true;
}

void GAFThread::run()
{
	double** pict;
	QSize pictSz;
	int nextfocus;
	int dir;
	int val;
	while (!Stopped)
	{
		pict = CapWnd->getFocusingArea();
		if (pict)
		{
			pictSz = CapWnd->getFocusingAreaSize();
			fc->NextIter(pict, pictSz.width(), pictSz.height());
			nextfocus = fc->getNextFocus();
			dir = nextfocus < 0 ? 0 : 1;
			val = nextfocus; if (val < 0) val = -val;
			if (val != 0)
				LiveThread->cmdAdjFocus(dir, val);
		}
		WinSleep(200);
	}
}
