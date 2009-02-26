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

#ifndef _events_h
#define _events_h

#include <QEvent>
#include <QVariant>

class GCameraEvent: public QEvent
{
public:
	GCameraEvent(QEvent::Type type, QVariant val)
	 : QEvent(type), Value(val) {}
	const QVariant& value() const { return Value; }
private:
	QVariant Value;
};

#define CAMERA_EVENT_EVF_TRANSMITED	(QEvent::Type)(QEvent::User + 1)
#define CAMERA_EVENT_AV_CHANGED 	(QEvent::Type)(QEvent::User + 2)
#define CAMERA_EVENT_AVLIST_CHANGED	(QEvent::Type)(QEvent::User + 3)
#define CAMERA_EVENT_FPS_UPDATED	(QEvent::Type)(QEvent::User + 4)
#define CAMERA_EVENT_AEMODE_CHANGED	(QEvent::Type)(QEvent::User + 5)
#define CAMERA_EVENT_AFMODE_CHANGED	(QEvent::Type)(QEvent::User + 6)
#define CAMERA_EVENT_SHUTDOWN		(QEvent::Type)(QEvent::User + 100)

#endif	// _events_h
