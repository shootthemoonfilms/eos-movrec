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

#include "blinklabel.h"

#include <QTimer>
#include <QPainter>

QBlinkLabel::QBlinkLabel(QWidget* parent)
 : QLabel(parent, 0)
{
	BlinkTimer = new QTimer(this);
	BlinkPos = 0;
	connect(BlinkTimer, SIGNAL(timeout()), this, SLOT(slotTimer()));
}

QBlinkLabel::QBlinkLabel(const QString& text, QWidget* parent)
 : QLabel(text, parent, 0)
{
	BlinkTimer = new QTimer(this);
	BlinkPos = 0;
	connect(BlinkTimer, SIGNAL(timeout()), this, SLOT(slotTimer()));
}

QBlinkLabel::~QBlinkLabel()
{
	// ????????????
	delete BlinkTimer;
}

void QBlinkLabel::start()
{
	BlinkTimer->start(700);
}

void QBlinkLabel::stop()
{
	BlinkTimer->stop();
	BlinkPos = 0;
	update();
}

void QBlinkLabel::slotTimer()
{
	BlinkPos = 1 - BlinkPos;
	update();
}

void QBlinkLabel::paintEvent(QPaintEvent* event)
{
	QPainter p(this);
	p.setBackgroundMode(Qt::TransparentMode);
	QRect r = rect();
	if (BlinkPos > 0)
	{
		QBrush b(QColor(211, 30, 30), Qt::SolidPattern);
		QPen pen(QColor(255, 255, 255));
		p.fillRect(r, b);
		p.setPen(pen);
		p.drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, text());
	}
	else
		p.drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, text());
}
