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
class QStatusBar;

class GMyLiveThread;
class GAFThread;
class GEOSCaptureWnd;
class QBlinkLabel;
class GHistogramWnd;

class GEOSRecWnd: public QWidget
{
	Q_OBJECT
public:
	GEOSRecWnd();
	~GEOSRecWnd();
protected slots:
	void slotReconnect();
	void slotSelFile();
	void slotStart();
	void slotStop();
	//void slotStartTimeout();
	//void slotWorkTimeout();
	void slotAESelected(int ae_ind);
	void slotDofPressed();
	void slotISOSelected(int iso_ind);
	void slotAvSelected(int av_ind);
	void slotTvSelected(int tv_ind);
	void slotWbSelected(int wb_ind);
	void slotWbTempSelected(int wb_temp);
	void slotShowImageChanged(int);
	void slotOptions();
	void slotAbout();
	void slotFocusNear3();
	void slotFocusNear2();
	void slotFocusNear1();
	void slotFocusFar1();
	void slotFocusFar2();
	void slotFocusFar3();
	void slotZoom5x();
	void slotAutoFocus();
	void slotStopAutoFocus();
	void slotCameraAF();
	void slotHistogram();
protected:
	virtual void customEvent(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);
private:
	QString giveNextName(const QString& path);
	void shutdown();
	void loadSettings();
	void saveSettings();
private:
	//QTimer* StartTimer;
	QBlinkLabel* blinkLabel;
	QToolButton* reconnBtn;
	QToolButton* selFileBtn;
	QPushButton* startBtn;
	QPushButton* stopBtn;
	QComboBox* AEModeBox;
	QToolButton* dofBtn;
	QComboBox* isoBox;
	QComboBox* avBox;
	QComboBox* tvBox;
	QComboBox* wbBox;
	QSpinBox* wbTempBox;
	QLabel* fpsLabel;
	QCheckBox* showBox;
	QToolButton* focusNear3Btn;
	QToolButton* focusNear2Btn;
	QToolButton* focusNear1Btn;
	QToolButton* focusFar1Btn;
	QToolButton* focusFar2Btn;
	QToolButton* focusFar3Btn;
	QToolButton* zoom5xBtn;
	QToolButton* AFBtn;
	QToolButton* AFCamBtn;
	QToolButton* HistBtn;
	QToolButton* optionsBtn;
	QStatusBar* statusBar;
	QLabel* framesLabel;
	QLabel* timeLabel;
	GMyLiveThread* LiveThread;
	GAFThread* AFThread;
	GEOSCaptureWnd* CaptureWnd;
	GHistogramWnd* HistogramWnd;
	struct GSavedSettings
	{
		QString Path;
		int Av;
		int Tv;
		int ISO;
		int Wb;
		int WbTemp;
		int AEMode;
		int AFMode;
		int BufferSize;
		bool UseStabFPS;
	};
	struct GSavedSettings CurrSettings;
	struct GSavedSettings BackupSettings;
};

#endif	// _mainwnd_h
