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

#include "capturewnd.h"
#include "buffer.h"
#include "events.h"

#include <QBuffer>
#include <QImageReader>
#include <QPainter>
#include <QLayout>
#include <QMouseEvent>
#include <QApplication>

GEOSCaptureWnd::GEOSCaptureWnd(QWidget* parent)
 : QWidget(parent)
{
	max_frame_size = 0;
	frame = 0;
	ShowLiveImage = true;
	//setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setMinimumSize(768, 512);
	setMouseTracking(true);
	ZoomRectMoving = false;
	FocusArea = 0;
}

GEOSCaptureWnd::~GEOSCaptureWnd()
{
	if (frame)
	{
		free(frame);
		frame = 0;
	}
	if (FocusArea)
	{
		int i;
		for (i = 0; i < FocusAreaSize.width(); i++)
			free(FocusArea[i]);
		free(FocusArea);
		FocusArea = 0;
	}
}

void GEOSCaptureWnd::paintEvent(QPaintEvent * event)
{
	QPainter painter(this);
	if (ShowLiveImage)
	{
		if (!LiveImage.isNull())
		{
			painter.drawImage(QPoint(0, 0), LiveImage);
			if (Zoom == 1)
			{
				QPen p(QColor(255, 255, 255));
				p.setWidth(2);
				painter.setPen(p);
				painter.drawRect(ZoomRect);
			}
		}
	}
	else
	{
		QBrush br(QColor(0, 0, 0), Qt::SolidPattern);
		painter.fillRect(0, 0, width(), height(), br);
	}
}

void GEOSCaptureWnd::closeEvent(QCloseEvent* event)
{
}

void GEOSCaptureWnd::mousePressEvent(QMouseEvent* event)
{
	ZoomRectMoving = Zoom == 1 && ZoomRect.contains(event->pos(), true) || Zoom == 5;
	if (ZoomRectMoving)
		MousePressPoint = event->pos();
}

void GEOSCaptureWnd::mouseReleaseEvent(QMouseEvent* event)
{
	if (ZoomRectMoving)
	{
		QPoint p = ZoomRect.topLeft()*5;
		QApplication::postEvent(parentWidget(), new GCameraEvent(CAMERA_EVENT_ZOOMPOS_NEEDCHANGE, QVariant(p)));
		ZoomRectMoving = false;
	}
}

void GEOSCaptureWnd::mouseMoveEvent(QMouseEvent* event)
{
	static int cursor_type = 0;
	QPoint pos = event->pos();
	if (Zoom == 1)
	{
		if (ZoomRect.contains(pos, true))
		{
			if (cursor_type == 0)
			{
				setCursor(Qt::OpenHandCursor);
				cursor_type = 1;
			}
			if (ZoomRectMoving)
			{
				QRect bak_rect = ZoomRect;
				QPoint bak_point = MousePressPoint;
				QPoint off = pos - MousePressPoint;
				ZoomRect.translate(off);
				MousePressPoint = pos;
				if (ZoomRect.left() < 0 || ZoomRect.top() < 0 ||
					ZoomRect.right() >= LiveImage.width() || ZoomRect.bottom() >= LiveImage.height())
				{
					ZoomRect = bak_rect;
					MousePressPoint = bak_point;
				}
			}
		}
		else
		{
			if (cursor_type == 1)
			{
				setCursor(Qt::ArrowCursor);
				cursor_type = 0;
			}
		}
	}
	else if (Zoom == 5)
	{
		if (cursor_type == 0)
		{
			setCursor(Qt::OpenHandCursor);
			cursor_type = 1;
		}
		if (ZoomRectMoving)
		{
			QRect bak_rect = ZoomRect;
			QPoint bak_point = MousePressPoint;
			QPoint off = MousePressPoint - pos;
			off /= 3;
			ZoomRect.translate(off);
			MousePressPoint = pos;
			if (ZoomRect.left() < 0 || ZoomRect.top() < 0 ||
				ZoomRect.right() >= LiveImage.width() || ZoomRect.bottom() >= LiveImage.height())
			{
				ZoomRect = bak_rect;
				MousePressPoint = bak_point;
			}
		}
	}
	else
	{
		if (cursor_type == 1)
		{
			setCursor(Qt::ArrowCursor);
			cursor_type = 0;
		}
	}
}

void GEOSCaptureWnd::customEvent(QEvent* event)
{
	static int old_width = 0;
	static int old_height = 0;
	if (event->type() == CAMERA_EVENT_EVF_TRANSMITED && live_buffer::frame)
	{
		if (ShowLiveImage)
		{
			live_buffer::IsPainting = true;
// !!! Start critical section
			live_buffer::ImageMutex.lock();
			int size = live_buffer::frame_size;
			if (max_frame_size < size)
			{
				frame = (unsigned char*)realloc(frame, size);
				max_frame_size = size;
			}
			memcpy(frame, live_buffer::frame, size);
			live_buffer::ImageMutex.unlock();
// !!! End of critical section
			//LiveImage.loadFromData(frame, size, "JPG");
			QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(frame), size);
			QBuffer b;
			b.setData(a);
			b.open(QIODevice::ReadOnly);
			static QImageReader ir;
			ir.setDevice(&b);
			ir.setFormat("jpeg");
			if (ir.read(&LiveImage))
			{
				if (!LiveImage.isNull())
				{
					live_buffer::frame_width = LiveImage.width();
					live_buffer::frame_height = LiveImage.height();
					if (old_width != live_buffer::frame_width || old_height != live_buffer::frame_height)
					{
						setMinimumSize(live_buffer::frame_width, live_buffer::frame_height);
						setMaximumSize(live_buffer::frame_width, live_buffer::frame_height);
					}
					old_width = live_buffer::frame_width;
					old_height = live_buffer::frame_height;
				}
				update(0, 0, live_buffer::frame_width, live_buffer::frame_height);
			}
			live_buffer::IsPainting = false;
		}
		event->accept();
	}
	else if (event->type() == CAMERA_EVENT_ZOOM_CHANGED)
	{
		if (!ZoomRectMoving)
		{
			GCameraEvent* e = (GCameraEvent*)event;
			QRect r = e->value().toRect();
			Zoom = r.x();
			ZoomRect = QRect(r.width()/5, r.height()/5, LiveImage.width()/5, LiveImage.height()/5);
		}
	}
}

double** GEOSCaptureWnd::getFocusingArea()
{
	if (ZoomRect.width() == 0 || LiveImage.isNull())
		return 0;
	if (!FocusArea)
	{
		int w = ZoomRect.width();
		int h = ZoomRect.height();
		int i, j;
		for (i = 0; i < h; i++)
		{
			FocusArea = (double**)malloc(sizeof(double*)*h);
			for (j = 0; j < w; j++)
				FocusArea[i] = (double*)malloc(sizeof(double)*w);
		}
		FocusAreaSize = QSize(w, h);
	}
	// to-do: fill FocusArea
	return FocusArea;
}

QSize GEOSCaptureWnd::getFocusingAreaSize()
{
	return FocusAreaSize;
}
