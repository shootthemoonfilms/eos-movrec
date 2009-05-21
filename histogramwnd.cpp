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

#include "histogramwnd.h"
#include "livethread.h"
#include "events.h"

#include <QPainter>
#include <QApplication>

GHistogramWnd::GHistogramWnd(QWidget* parent, GMyLiveThread* thread)
 : QWidget(0, Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTitleHint |
 		   Qt::WindowSystemMenuHint |
 #ifndef __MACOS__
 		   Qt::WindowCloseButtonHint |
 #endif
		   Qt::MSWindowsFixedSizeDialogHint)
{
	Histogram = 0;
	HistogramSize = 0;
	LiveThread = thread;
	Owner = parent;
	setFixedSize(260, 3*256/2);
}

GHistogramWnd::~GHistogramWnd()
{
	if (Histogram)
		free(Histogram);
}

static int max_4(int x, int a, int b, int c)
{
	register int t = x;
	if (x < a)
		t = a;
	if (t < b)
		t = b;
	if (t < c)
		t = c;
	return t;
}

void GHistogramWnd::updateHistogram()
{
	if (!LiveThread)
		return;
	LiveThread->lockHistogram();
	if (LiveThread->histogramSize()/4 != HistogramSize)
	{
		HistogramSize = LiveThread->histogramSize()/4;
		Histogram = (struct YRGB*)realloc(Histogram, sizeof(struct YRGB)*HistogramSize);
	}
	memcpy((void*)Histogram, (void*)LiveThread->histogram(), HistogramSize*sizeof(struct YRGB));
	LiveThread->unlockHistogram();
	int i;
	int v;
	HistogramMax = max_4(Histogram[0].Y, Histogram[0].R, Histogram[0].G, Histogram[0].B);
	for (i = 0; i < HistogramSize; i++)
	{
		v = max_4(Histogram[i].Y, Histogram[i].R, Histogram[i].G, Histogram[i].B);
		if (HistogramMax < v)
			HistogramMax = v;
	}
	int new_width = HistogramSize + 4;
	int new_height = 3*HistogramSize/2;
	if (width() != new_width || height() != new_height)
		setFixedSize(new_width, new_height);
	update();
}

void GHistogramWnd::paintEvent(QPaintEvent * event)
{
	QPainter p(this);
	int h_step = height()/4;
	int h = h_step;
	double kx = (double)(width() - 4)/(double)HistogramSize;
	double ky = (double)h_step/((double)HistogramMax*1.1);
	int i;
	int x, y;
	p.setPen(QColor(0, 0, 0));
	for (i = 0; i < HistogramSize; i++)
	{
		x = kx*(double)i + 2;
		y = h - ky*(double)Histogram[i].Y;
		p.drawLine(x, h - 1, x, y);
	}
	h += h_step;
	p.setPen(QColor(255, 0, 0));
	for (i = 0; i < HistogramSize; i++)
	{
		x = kx*(double)i + 2;
		y = h - ky*(double)Histogram[i].R;
		p.drawLine(x, h - 1, x, y);
	}
	h += h_step;
	p.setPen(QColor(0, 255, 0));
	for (i = 0; i < HistogramSize; i++)
	{
		x = kx*(double)i + 2;
		y = h - ky*(double)Histogram[i].G;
		p.drawLine(x, h - 1, x, y);
	}
	h += h_step;
	p.setPen(QColor(0, 0, 255));
	for (i = 0; i < HistogramSize; i++)
	{
		x = kx*(double)i + 2;
		y = h - ky*(double)Histogram[i].B;
		p.drawLine(x, h - 1, x, y);
	}
}

void GHistogramWnd::closeEvent(QCloseEvent* event)
{
	QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_HISTOGRAM_CLOSED));
}
