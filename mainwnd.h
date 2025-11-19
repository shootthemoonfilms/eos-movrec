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
class QSpinBox;
class QLabel;
class QStatusBar;
class QPoint;

class GMyLiveThread;
class GAFThread;
class GEOSCaptureWnd;
class QBlinkLabel;
class GHistogramWnd;

#include "livethread.h"	// to get the declaration for EOSZoomVars

class GEOSRecWnd: public QWidget
{
	Q_OBJECT
public:
	GEOSRecWnd();
	~GEOSRecWnd();
	void signal_handler(int s);
	void close_app();
	bool isLive();
	bool zoomPosByClick;
	EOSZoomVars getZoomVars();
protected slots:
	void slotReconnect();
	void slotStart();
	void slotStop();
	void slotCPT();
	void slotSettings();
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
	void doFocus(int nearFar, int amount);
	void slotFocusNear3();
	void slotFocusNear2();
	void slotFocusNear1();
	void slotFocusFar1();
	void slotFocusFar2();
	void slotFocusFar3();
	void slotZoomPosClick();
	void slotZoom5x();
	void slotZoom10x();
	void slotAutoFocus();
	void slotStopAutoFocus();
	void slotCameraAF();
	void slotHistogram();
	void slotTimeTimerSwitch(bool);
	void slotFramesTimerSwitch(bool);
protected:
	void setPathLabel();
	void derivePath();
	virtual void customEvent(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);
	virtual void resizeEvent(QResizeEvent *event) override;
private:
	QString giveNextName(const QString& path);
	QString getNextfName(const QString& fName);
	void shutdown();
	void loadSettingsFilepaths();
	void loadSettings();
	void saveSettingsFilepaths();
	void saveSettings();
	void doZoomPos(QPoint qp);
	void doZoom(int amount);
	void populateOptionBoxes();
	void populateOptionBox(QComboBox* box, valArr featureList, int selOpt);
	void changeSelection(QComboBox* box, int* setting, int idx);
	void setBatteryLevelLabel(QLabel* batteryLevelLabel, QString batteryLevelStr);
private:
	//QTimer* StartTimer;
	QBlinkLabel* blinkLabel;
	QToolButton* reconnBtn;
	QToolButton* selSettingsBtn;
	QPushButton* startBtn;
	QPushButton* stopBtn;
	QPushButton* cptBtn;
	QLabel* path_label;
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
	QToolButton* zoomPosClickBtn;
	QToolButton* zoom5xBtn;
	QToolButton* zoom10xBtn;
	QToolButton* AFBtn;
	QToolButton* AFCamBtn;
	QToolButton* HistBtn;
	QToolButton* optionsBtn;
	QStatusBar* statusBar;
	QLabel* framesLabel;
	QLabel* timeLabel;
	QCheckBox* timeTimerBox;
	QSpinBox* timeTimerSpinBox;
	QCheckBox* framesTimerBox;
	QSpinBox* framesTimerSpinBox;
	QLabel* batteryLevelIcon;
	QLabel* batteryLevelLabel;
	GMyLiveThread* LiveThread;
	GAFThread* AFThread;
	GEOSCaptureWnd* CaptureWnd;
	GHistogramWnd* HistogramWnd;
	QString commonFilePath;
	struct GSavedSettings
	{
		QString VidName;
		QString ImgName;
		int Av;
		int Tv;
		int ISO;
		int Wb;
		int WbTemp;
		int AEMode;
		int AFMode;
		int BufferSize;
		bool UseStabFPS;
		bool ShowWhiteBox;
	};
	struct GSavedSettings CurrSettings;
	struct GSavedSettings BackupSettings;
	
	void wheelEvent(QWheelEvent* event);
};

#endif	// _mainwnd_h
