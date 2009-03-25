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
		   Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint |
		   Qt::MSWindowsFixedSizeDialogHint)
{
	Histogram = 0;
	HistogramSize = 0;
	LiveThread = thread;
	Owner = parent;
	setFixedSize(256, 128);
}

GHistogramWnd::~GHistogramWnd()
{
	if (Histogram)
		free(Histogram);
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
	HistogramMax = Histogram[0].Y;
	for (i = 0; i < HistogramSize; i++)
		if (HistogramMax < Histogram[i].Y)
			HistogramMax = Histogram[i].Y;
	if (width() != HistogramSize || height()*2 != HistogramSize)
		setFixedSize(HistogramSize, HistogramSize/2);
	update();
}

void GHistogramWnd::paintEvent(QPaintEvent * event)
{
	QPainter p(this);
	int h = height();
	double kx = (double)width()/(double)HistogramSize;
	double ky = (double)h/((double)HistogramMax*1.1);
	int i;
	int x, y;
	for (i = 0; i < HistogramSize; i++)
	{
		x = kx*(double)i;
		y = h - ky*(double)Histogram[i].Y;
		p.drawLine(x, h - 1, x, y);
	}
}

void GHistogramWnd::closeEvent(QCloseEvent* event)
{
	QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_HISTOGRAM_CLOSED));
}
