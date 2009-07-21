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
#include "os_api.h"
#include "events.h"
#include "autofocus.h"

#include <QWidget>
#include <QApplication>

#define AF_DEBUG_LOG	1

#if AF_DEBUG_LOG
#include <stdio.h>
#endif

GAFThread::GAFThread(QWidget* owner, GMyLiveThread* liveThread, GEOSCaptureWnd* capwnd)
 : QThread()
{
	Owner = owner;
	LiveThread = liveThread;
	CapWnd = capwnd;
	Stopped = false;
	fc = new GAutoFocus;
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
	int** pict;
	QSize pictSz;
	int nextfocus;
	int dir;
	int count1, count2, count3;
	int i;
#if AF_DEBUG_LOG
	int old_nf;
	int noise;
	int pos;
	int disp;
	QString str_disp;
	FILE* f = fopen("af.log", "at");
	fprintf(f, "af start!\n");;
#endif
	while (!Stopped)
	{
		CapWnd->lockFocusingArea();
		pict = CapWnd->getFocusingArea();
		if (pict)
		{
#if AF_DEBUG_LOG
			old_nf = fc->getNextFocus();
			noise = fc->noise();
#endif
			pictSz = CapWnd->getFocusingAreaSize();
			fc->NextIter(pict, pictSz.width(), pictSz.height());
			nextfocus = fc->getNextFocus();
#if AF_DEBUG_LOG
			pos = fc->lastPosition();
			disp = fc->lastDispersion();
			str_disp.sprintf("%d, %d", disp, nextfocus);
			CapWnd->setText(str_disp);
			fprintf(f, "%d, pos=%d, %d -> %d, noise=%d\n", disp, pos, old_nf, nextfocus, noise);
#endif
			if (!fc->stop)
			{
				if (nextfocus != 0)
				{
					dir = nextfocus < 0 ? 0 : 1;
					if (nextfocus < 0)
						nextfocus = -nextfocus;
					// adjust nextfocus
					//nextfocus *= 7;
					count3 = nextfocus / 30;
					count2 = (nextfocus - count3*30) / 8;
					count1 = nextfocus - count3*30 - count2*8;
					for (i = 0; i < count3; i++)
						LiveThread->cmdAdjFocus(dir, 3);
					for (i = 0; i < count2; i++)
						LiveThread->cmdAdjFocus(dir, 2);
					for (i = 0; i < count1; i++)
						LiveThread->cmdAdjFocus(dir, 1);
					//LiveThread->waitCommands();
				}
			}
			else
			{
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AF_STOPPED));
				//delete fc;
				//fc = new FocusingClass;
			}
		}
		CapWnd->unlockFocusingArea();
		WinSleep(100);
	}
#if AF_DEBUG_LOG
	fprintf(f, "af end.\n\n");
	fclose(f);
#endif
}
