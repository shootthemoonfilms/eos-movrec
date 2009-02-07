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

#ifndef _mainwnd_h
#define _mainwnd_h

#include <QWidget>
#include <QImage>

//class QTimer;
//class QImage;
class QCheckBox;
class QPushButton;
class QToolButton;
class QComboBox;
class QSpinBox;
class QLabel;

class GMyLiveThread;
class GEOSCaptureWnd;
class QBlinkLabel;

class GEOSRecWnd: public QWidget
{
	Q_OBJECT
public:
	GEOSRecWnd();
	~GEOSRecWnd();
protected slots:
	void slotSelFile();
	void slotStart();
	void slotStop();
	void slotStartTimeout();
	void slotWorkTimeout();
	void slotDofPressed();
	void slotAvSelected(int av_ind);
	void slotWbSelected(int wb_ind);
	void slotWbTempSelected(int wb_temp);
	void slotShowImageChanged(int);
	void slotAbout();
protected:
	virtual void customEvent(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);
private:
	QString giveNextName(const QString& path);
	void shutdown();
private:
	QBlinkLabel* blinkLabel;
	QToolButton* selFileBtn;
	QPushButton* startBtn;
	QPushButton* stopBtn;
	QToolButton* dofBtn;
	QComboBox* avBox;
	QComboBox* wbBox;
	QSpinBox* wbTempBox;
	QLabel* fpsLabel;
	QCheckBox* showBox;
	QString Path;
	GMyLiveThread* LiveThread;
	GEOSCaptureWnd* CaptureWnd;
};

#endif	// _mainwnd_h
