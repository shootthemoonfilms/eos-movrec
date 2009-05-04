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

#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QTimer>
#include <QBoxLayout>
#include <QFileDialog>
#include <QTextCodec>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>

//#include <QMessageBox>

#include "mainwnd.h"
#include "about.h"
#include "blinklabel.h"
#include "capturewnd.h"
#include "livethread.h"
#include "afthread.h"
#include "histogramwnd.h"
#include "events.h"
#include "cam_tables.h"
#include "freqtimer.h"

// icons
#include "pixmaps/usb-sign-black.xpm"
#include "pixmaps/record.xpm"
#include "pixmaps/stop.xpm"

#include <stdlib.h>

GEOSRecWnd::GEOSRecWnd()
 : QWidget(0)
{
	setWindowTitle(tr("EOS Camera Movie Record"));

	QVBoxLayout* main_layout = new QVBoxLayout(this);
	QHBoxLayout* btn_layout = new QHBoxLayout();

	selFileBtn = new QToolButton(this);
	selFileBtn->setText(tr("..."));
	selFileBtn->setEnabled(false);
	selFileBtn->setToolTip(tr("Select folder"));
	btn_layout->addWidget(selFileBtn, 0);

	startBtn = new QPushButton(tr("Write!"), this);
	startBtn->setIcon(QPixmap(record_xpm));
	startBtn->setEnabled(false);
	startBtn->setToolTip(tr("Start recording"));
	btn_layout->addWidget(startBtn, 0);

	stopBtn = new QPushButton(tr("Stop"), this);
	stopBtn->setIcon(QPixmap(stop_xpm));
	stopBtn->setEnabled(false);
	stopBtn->setToolTip(tr("Stop recording"));
	btn_layout->addWidget(stopBtn, 0);

	btn_layout->addSpacing(10);
	AEModeBox = new QComboBox(this);
	AEModeBox->setEditable(false);
	AEModeBox->addItem(tr("P"), QVariant((int)0));
	AEModeBox->addItem(tr("Tv"), QVariant((int)1));
	AEModeBox->addItem(tr("Av"), QVariant((int)2));
	AEModeBox->addItem(tr("M"), QVariant((int)3));
	AEModeBox->addItem(tr("A-DEP"), QVariant((int)5));
	AEModeBox->setToolTip(tr("Select AE Mode"));
	btn_layout->addWidget(AEModeBox, 0);

	isoBox = new QComboBox(this);
	isoBox->setEditable(false);
	isoBox->setEnabled(false);
	isoBox->setToolTip(tr("Select ISO"));
	btn_layout->addWidget(isoBox, 0);

	dofBtn = new QToolButton(this);
	dofBtn->setText("DOF");
	dofBtn->setCheckable(true);
	dofBtn->setEnabled(false);
	dofBtn->setToolTip(tr("Enable/disable depth of field"));
	btn_layout->addWidget(dofBtn, 0);

	avBox = new QComboBox(this);
	//avBox->setEnabled(false);
	avBox->setEditable(false);
	avBox->setToolTip(tr("Select AV"));
	btn_layout->addWidget(avBox, 0);

	tvBox = new QComboBox(this);
	//tvBox->setEnabled(false);
	tvBox->setEditable(false);
	tvBox->setToolTip(tr("Select TV"));
	btn_layout->addWidget(tvBox, 0);

	fpsLabel = new QLabel(tr("0 fps"), this);
	fpsLabel->setFrameShape(QFrame::Box);
	btn_layout->addWidget(fpsLabel, 0);

	btn_layout->addStretch(10);

	showBox = new QCheckBox(tr("Show captured image"), this);
	btn_layout->addWidget(showBox, 0);
	showBox->setCheckState(Qt::Checked);

	QToolButton* aboutBtn = new QToolButton(this);
	aboutBtn->setText(tr("A"));
	aboutBtn->setToolTip(tr("Show about box"));
	btn_layout->addWidget(aboutBtn, 0);

	QHBoxLayout* focus_layout = new QHBoxLayout();
	reconnBtn = new QToolButton(this);
	//reconnBtn->setText(tr("R"));
	reconnBtn->setIcon(QPixmap(usb_sign_black_xpm));
	reconnBtn->setEnabled(false);
	reconnBtn->setToolTip(tr("Reconnect to camera"));
	focus_layout->addWidget(reconnBtn, 0);
	focus_layout->addSpacing(10);
	focus_layout->addWidget(new QLabel(tr("Focus adjust"), this), 0);

	focusNear3Btn = new QToolButton(this);
	focusNear3Btn->setText(tr("<<<"));
	focusNear3Btn->setEnabled(false);
	focusNear3Btn->setToolTip(tr("Move focus point to near (1)"));
	focus_layout->addWidget(focusNear3Btn, 0);
	focusNear2Btn = new QToolButton(this);
	focusNear2Btn->setText(tr("<<"));
	focusNear2Btn->setEnabled(false);
	focusNear2Btn->setToolTip(tr("Move focus point to near (2)"));
	focus_layout->addWidget(focusNear2Btn, 0);
	focusNear1Btn = new QToolButton(this);
	focusNear1Btn->setText(tr("<"));
	focusNear1Btn->setEnabled(false);
	focusNear1Btn->setToolTip(tr("Move focus point to near (3)"));
	focus_layout->addWidget(focusNear1Btn, 0);

	focusFar1Btn = new QToolButton(this);
	focusFar1Btn->setText(tr(">"));
	focusFar1Btn->setEnabled(false);
	focusFar1Btn->setToolTip(tr("Move focus point to far (4)"));
	focus_layout->addWidget(focusFar1Btn, 0);
	focusFar2Btn = new QToolButton(this);
	focusFar2Btn->setText(tr(">>"));
	focusFar2Btn->setEnabled(false);
	focusFar2Btn->setToolTip(tr("Move focus point to far (5)"));
	focus_layout->addWidget(focusFar2Btn, 0);
	focusFar3Btn = new QToolButton(this);
	focusFar3Btn->setText(tr(">>>"));
	focusFar3Btn->setEnabled(false);
	focusFar3Btn->setToolTip(tr("Move focus point to far (6)"));
	focus_layout->addWidget(focusFar3Btn, 0);

	focus_layout->addSpacing(10);
	AFBtn = new QToolButton(this);
	AFBtn->setText(tr("AF"));
	AFBtn->setCheckable(true);
	AFBtn->setEnabled(false);
	AFBtn->setToolTip(tr("Try experimental autofocus"));
	focus_layout->addWidget(AFBtn, 0);

	focus_layout->addSpacing(10);

	focus_layout->addWidget(new QLabel(tr("Zoom")));
	zoom5xBtn = new QToolButton(this);
	zoom5xBtn->setText(tr("5x"));
	zoom5xBtn->setEnabled(false);
	zoom5xBtn->setCheckable(true);
	zoom5xBtn->setToolTip(tr("5x Zoom (crop)"));
	focus_layout->addWidget(zoom5xBtn, 0);

	HistBtn = new QToolButton(this);
	HistBtn->setText(tr("H"));
	HistBtn->setEnabled(false);
	HistBtn->setCheckable(true);
	HistBtn->setToolTip(tr("Show histogram"));
	focus_layout->addWidget(HistBtn, 0);

	focus_layout->addSpacing(20);
	QLabel* wbLabel = new QLabel(tr("WB"), this);
	focus_layout->addWidget(wbLabel, 0);

	wbBox = new QComboBox(this);
	wbBox->setEditable(false);
	wbBox->addItem(tr("Auto"), QVariant((int)0));
	wbBox->addItem(tr("Daylight"), QVariant((int)1));
	wbBox->addItem(tr("Cloudy"), QVariant((int)2));
	wbBox->addItem(tr("Tungsten"), QVariant((int)3));
	wbBox->addItem(tr("Fluorescent"), QVariant((int)4));
	wbBox->addItem(tr("Flash"), QVariant((int)5));
	wbBox->addItem(tr("Shade"), QVariant((int)8));
	wbBox->addItem(tr("Color Temperature"), QVariant((int)9));
	wbBox->setToolTip(tr("Select white balance"));
	focus_layout->addWidget(wbBox, 0);

	wbTempBox = new QSpinBox(this);
	wbTempBox->setMinimum(2800);
	wbTempBox->setMaximum(10000);
	wbTempBox->setValue(5200);
	wbTempBox->setSingleStep(100);
	wbTempBox->setEnabled(false);
	wbTempBox->setSuffix(tr("K"));
	wbTempBox->setToolTip(tr("Select white balance temperature"));
	focus_layout->addWidget(wbTempBox, 0);

	focus_layout->addStretch(1);

	blinkLabel = new QBlinkLabel(tr("Starting..."), this);
	main_layout->addWidget(blinkLabel, 0);

	main_layout->addLayout(btn_layout, 0);

	main_layout->addLayout(focus_layout, 0);

	CaptureWnd = new GEOSCaptureWnd(this);
	main_layout->addWidget(CaptureWnd, 0);
	main_layout->addStretch(1);

	//setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	QShortcut* focusNear3Shortcut = new QShortcut(QKeySequence(tr("1")), this);
	QShortcut* focusNear2Shortcut = new QShortcut(QKeySequence(tr("2")), this);
	QShortcut* focusNear1Shortcut = new QShortcut(QKeySequence(tr("3")), this);
	QShortcut* focusFar1Shortcut = new QShortcut(QKeySequence(tr("4")), this);
	QShortcut* focusFar2Shortcut = new QShortcut(QKeySequence(tr("5")), this);
	QShortcut* focusFar3Shortcut = new QShortcut(QKeySequence(tr("6")), this);

	connect(selFileBtn, SIGNAL(clicked()), this, SLOT(slotSelFile()));
	connect(reconnBtn, SIGNAL(clicked()), this, SLOT(slotReconnect()));
	connect(startBtn, SIGNAL(clicked()), this, SLOT(slotStart()));
	connect(stopBtn, SIGNAL(clicked()), this, SLOT(slotStop()));
	connect(AEModeBox, SIGNAL(activated(int)), this, SLOT(slotAESelected(int)));
	connect(dofBtn, SIGNAL(clicked()), this, SLOT(slotDofPressed()));
	connect(isoBox, SIGNAL(activated(int)), this, SLOT(slotISOSelected(int)));
	connect(avBox, SIGNAL(activated(int)), this, SLOT(slotAvSelected(int)));
	connect(tvBox, SIGNAL(activated(int)), this, SLOT(slotTvSelected(int)));
	connect(wbBox, SIGNAL(activated(int)), this, SLOT(slotWbSelected(int)));
	connect(wbTempBox, SIGNAL(valueChanged(int)), this, SLOT(slotWbTempSelected(int)));
	connect(showBox, SIGNAL(stateChanged(int)), this, SLOT(slotShowImageChanged(int)));
	connect(aboutBtn, SIGNAL(clicked()), this, SLOT(slotAbout()));
	connect(focusNear3Btn, SIGNAL(clicked()), this, SLOT(slotFocusNear3()));
	connect(focusNear2Btn, SIGNAL(clicked()), this, SLOT(slotFocusNear2()));
	connect(focusNear1Btn, SIGNAL(clicked()), this, SLOT(slotFocusNear1()));
	connect(focusFar1Btn, SIGNAL(clicked()), this, SLOT(slotFocusFar1()));
	connect(focusFar2Btn, SIGNAL(clicked()), this, SLOT(slotFocusFar2()));
	connect(focusFar3Btn, SIGNAL(clicked()), this, SLOT(slotFocusFar3()));
	connect(zoom5xBtn, SIGNAL(clicked()), this, SLOT(slotZoom5x()));
	connect(AFBtn, SIGNAL(clicked()), this, SLOT(slotAutoFocus()));
	connect(HistBtn, SIGNAL(clicked()), this, SLOT(slotHistogram()));

	connect(focusNear3Shortcut, SIGNAL(activated()), this, SLOT(slotFocusNear3()));
	connect(focusNear2Shortcut, SIGNAL(activated()), this, SLOT(slotFocusNear2()));
	connect(focusNear1Shortcut, SIGNAL(activated()), this, SLOT(slotFocusNear1()));
	connect(focusFar1Shortcut, SIGNAL(activated()), this, SLOT(slotFocusFar1()));
	connect(focusFar2Shortcut, SIGNAL(activated()), this, SLOT(slotFocusFar2()));
	connect(focusFar3Shortcut, SIGNAL(activated()), this, SLOT(slotFocusFar3()));

	CurrSettings.Path = tr("out.avi");
	CurrSettings.Av = -1;
	CurrSettings.Tv = -1;
	CurrSettings.ISO = -1;
	CurrSettings.Wb = -1;
	CurrSettings.WbTemp = -1;
	CurrSettings.AEMode = -1;

	BackupSettings.Path = CurrSettings.Path;
	BackupSettings.Av = CurrSettings.Av;
	BackupSettings.Tv = CurrSettings.Tv;
	BackupSettings.ISO = CurrSettings.ISO;
	BackupSettings.Wb = CurrSettings.Wb;
	BackupSettings.WbTemp = CurrSettings.WbTemp;
	BackupSettings.AEMode = CurrSettings.AEMode;

	LiveThread = new GMyLiveThread(this);
	LiveThread->setCaptureWnd(CaptureWnd);
	LiveThread->start(QThread::HighestPriority);

	AFThread = 0;
	HistogramWnd = 0;

	//QTimer::singleShot(4000, this, SLOT(slotStartTimeout()));
	StartTimer = new QTimer(this);
	StartTimer->setSingleShot(true);
	connect(StartTimer, SIGNAL(timeout()), this, SLOT(slotStartTimeout()));
	StartTimer->start(4000);
	QTimer::singleShot(1200000, this, SLOT(slotWorkTimeout()));		// max work time is 20 min

	/*QList<QByteArray> formats = QImageReader::supportedImageFormats();
	QList<QByteArray>::iterator it = formats.begin();
	while (it != formats.end())
	{
		printf("format: %s\n", (*it).data());
		fflush(stdout);
		it++;
	}*/
}

GEOSRecWnd::~GEOSRecWnd()
{
	if (StartTimer)
		delete StartTimer;
}

void GEOSRecWnd::closeEvent(QCloseEvent* event)
{
	// stop thread;
	if (LiveThread)
	{
		if (LiveThread->isInit())
			saveSettings();
		GMyLiveThread* p = LiveThread;
		LiveThread = 0;
		p->stop();
		p->wait();
		FILE* f = fopen("debug_info.txt", "wt");
		if (f)
		{
			fprintf(f, "all frames: %I64d\n", p->allFramesCount());
			fprintf(f, "all writen frames: %I64d\n", p->writenCount());
			fprintf(f, "skipped frames on painting: %I64d\n", p->skippedCount());
			fprintf(f, "elapsed time (sec): %d\n", p->elapsedTime()/1000);
			if (p->elapsedTime() > 0)
				fprintf(f, "refresh rate(fps): %.1f\n", 1000.0*(float)p->allFramesCount()/((float)p->elapsedTime()));
			fclose(f);
		}
		delete p;
	}
	if (AFThread)
	{
		AFThread->stop();
		AFThread->wait();
		delete AFThread;
		AFThread = 0;
	}
	if (HistogramWnd)
	{
		HistogramWnd->close();
		delete HistogramWnd;
		HistogramWnd = 0;
	}
}

void GEOSRecWnd::slotStartTimeout()
{
	if (LiveThread)
	{
		if (LiveThread->isInit())
		{
			if (!LiveThread->liveViewStarted())
			{
				shutdown();
				blinkLabel->setText(tr("You camera not have LiveView!"));
				blinkLabel->start();
				QMessageBox::critical(this, tr("Error"), tr("You camera not have LiveView!"));
				// close();
			}
			else
			{
				selFileBtn->setEnabled(true);
				startBtn->setEnabled(true);
				AEModeBox->setEnabled(true);
				dofBtn->setEnabled(true);
				zoom5xBtn->setEnabled(true);
				HistBtn->setEnabled(true);
				blinkLabel->stop();
				blinkLabel->setText(tr("Ready"));
				// at this time we already received all settings from camera
				loadSettings();
			}
		}
		else
		{
			shutdown();
			blinkLabel->setText(tr("Camera not found/not supported/not connected!"));
			blinkLabel->start();
			QMessageBox::critical(this, tr("Error"), tr("Can't initialize your camera!\nCheck connection."));
			//close();
		}
	}
	StartTimer->stop();
}

void GEOSRecWnd::slotReconnect()
{
	blinkLabel->stop();
	blinkLabel->setText(tr("Starting..."));
	reconnBtn->setEnabled(false);

	LiveThread = new GMyLiveThread(this);
	LiveThread->setCaptureWnd(CaptureWnd);
	LiveThread->start(QThread::HighestPriority);

	//QTimer::singleShot(4000, this, SLOT(slotStartTimeout()));
	StartTimer->start(4000);
	QTimer::singleShot(1200000, this, SLOT(slotWorkTimeout()));		// max work time is 20 min
}

void GEOSRecWnd::loadSettings()
{
	BackupSettings.AEMode = CurrSettings.AEMode;
	BackupSettings.Av = CurrSettings.Av;
	BackupSettings.Tv = CurrSettings.Tv;
	BackupSettings.ISO = CurrSettings.ISO;
	BackupSettings.Wb = CurrSettings.Wb;
	BackupSettings.WbTemp = CurrSettings.WbTemp;

	QSettings settings(QSettings::UserScope, QString("eos_movrec"));
	CurrSettings.Path = settings.value(QString("Path"), QVariant(QString("out.avi"))).toString();
	int i;
	int ae = settings.value(QString("AEMode"), QVariant((int)-1)).toInt();
	for (i = 0; i < AEModeBox->count(); i++)
		if (ae == AEModeBox->itemData(i, Qt::UserRole).toInt())
		{
			AEModeBox->setCurrentIndex(i);
			slotAESelected(i);
			break;
		}
	int av = settings.value(QString("Av"), QVariant((int)-1)).toInt();
	for (i = 0; i < avBox->count(); i++)
		if (av == avBox->itemData(i, Qt::UserRole).toInt())
		{
			avBox->setCurrentIndex(i);
			slotAvSelected(i);
			break;
		}
	int tv = settings.value(QString("Tv"), QVariant((int)-1)).toInt();
	for (i = 0; i < tvBox->count(); i++)
		if (tv == tvBox->itemData(i, Qt::UserRole).toInt())
		{
			tvBox->setCurrentIndex(i);
			slotTvSelected(i);
			break;
		}
	int wb_temp = settings.value(QString("WBTemp"), QVariant((int)-1)).toInt();
	if (wb_temp > 0)
	{
		wbTempBox->setValue(wb_temp);
		slotWbTempSelected(wb_temp);
	}
	int wb = settings.value(QString("WB"), QVariant((int)-1)).toInt();
	for (i = 0; i < wbBox->count(); i++)
		if (wb == wbBox->itemData(i, Qt::UserRole).toInt())
		{
			wbBox->setCurrentIndex(i);
			slotWbSelected(i);
			break;
		}
	int iso = settings.value(QString("ISO"), QVariant((int)-1)).toInt();
	for (i = 0; i < isoBox->count(); i++)
		if (iso == isoBox->itemData(i, Qt::UserRole).toInt())
		{
			isoBox->setCurrentIndex(i);
			slotISOSelected(i);
			break;
		}
}

void GEOSRecWnd::saveSettings()
{
	if (LiveThread)
	{
		LiveThread->cmdSetAEMode(BackupSettings.AEMode);
		LiveThread->cmdSetAv(BackupSettings.Av, 0);
		LiveThread->cmdSetTv(BackupSettings.Tv);
		LiveThread->cmdSetISO(BackupSettings.ISO);
		//LiveThread->cmdSetWB(9, BackupSettings.WbTemp);
		//LiveThread->cmdSetWB(BackupSettings.Wb, 0);
		// bad code!!!
		// give some time to thread to perform commands
		WinSleep(100);
	}
	QSettings settings(QSettings::UserScope, QString("eos_movrec"));
	settings.setValue(QString("Path"), QVariant(CurrSettings.Path));
	settings.setValue(QString("AEMode"), QVariant(CurrSettings.AEMode));
	settings.setValue(QString("Av"), QVariant(CurrSettings.Av));
	settings.setValue(QString("Tv"), QVariant(CurrSettings.Tv));
	settings.setValue(QString("ISO"), QVariant(CurrSettings.ISO));
	settings.setValue(QString("WB"), QVariant(CurrSettings.Wb));
	settings.setValue(QString("WBTemp"), QVariant(CurrSettings.WbTemp));
}

void GEOSRecWnd::customEvent(QEvent* event)
{
	if (!LiveThread /*|| !LiveThread->isInit()*/)
		return;
	GCameraEvent* e = (GCameraEvent*)event;
	switch (e->type())
	{
	case CAMERA_EVENT_LV_STARTED:
		slotStartTimeout();
		break;
	case CAMERA_EVENT_ISO_CHANGED:
		{
			// ISO changed
			int iso = e->value().toInt();
			CurrSettings.ISO = iso;
			int val;
			for (int i = 0; i < isoBox->count(); i++)
			{
				val = isoBox->itemData(i, Qt::UserRole).toInt();
				if (val == iso)
				{
					isoBox->setCurrentIndex(i);
					break;
				}
			}
		}
		break;
	case CAMERA_EVENT_ISOLIST_CHANGED:
		{
			// ISO list changed
			int curr_iso = isoBox->itemData(isoBox->currentIndex(), Qt::UserRole).toInt();
			const int* isoList = LiveThread->isoList();
			int isoListSize = LiveThread->isoListSize();
			// fill combo
			isoBox->clear();
			int i, j, ind = 0;
			for (i = 0; i < isoListSize; i++)
			{
				for (j = 0; j < EOS_ISO_TABLE_SZ; j++)
					if (ISOTable[j].val == isoList[i])
					{
						isoBox->addItem(QString(ISOTable[j].ISO), QVariant((int)ISOTable[j].val));
						if (isoList[i] == curr_iso)
							isoBox->setCurrentIndex(ind);
						ind++;
					}
			}
		}
		break;
	case CAMERA_EVENT_AV_CHANGED:
		{
			// Av changed
			int av = e->value().toInt();
			CurrSettings.Av = av;
			//QString str = tr("Av changed to %1").arg(av);
			//blinkLabel->setText(str);
			//QMessageBox::information(this, tr("Info"), str);
			// find in combo
			int val;
			for (int i = 0; i < avBox->count(); i++)
			{
				val = avBox->itemData(i, Qt::UserRole).toInt();
				if (val == av)
				{
					avBox->setCurrentIndex(i);
					break;
				}
			}
		}
		break;
	case CAMERA_EVENT_AVLIST_CHANGED:
		{
			// Av list changed
			//QMessageBox::information(this, tr("Info"), tr("Av list changed."));
			//blinkLabel->setText(tr("Av list changed."));
			int curr_av = avBox->itemData(avBox->currentIndex(), Qt::UserRole).toInt();
			const int* avList = LiveThread->avList();
			int avListSize = LiveThread->avListSize();
			// fill combo
			avBox->clear();
			int i, j, ind = 0;
			for (i = 0; i < avListSize; i++)
			{
				for (j = 0; j < EOS_AV_TABLE_SZ; j++)
					if (AvTable[j].val == avList[i])
					{
						avBox->addItem(QString(AvTable[j].av), QVariant((int)AvTable[j].val));
						if (avList[i] == curr_av)
							avBox->setCurrentIndex(ind);
						ind++;
					}
			}
		}
		break;
	case CAMERA_EVENT_TV_CHANGED:
		{
			// Tv changed
			int tv = e->value().toInt();
			CurrSettings.Tv = tv;
			//QString str = tr("Tv changed to %1").arg(tv);
			//blinkLabel->setText(str);
			//QMessageBox::information(this, tr("Info"), str);
			// find in combo
			int val;
			for (int i = 0; i < tvBox->count(); i++)
			{
				val = tvBox->itemData(i, Qt::UserRole).toInt();
				if (val == tv)
				{
					tvBox->setCurrentIndex(i);
					break;
				}
			}
		}
		break;
	case CAMERA_EVENT_TVLIST_CHANGED:
		{
			// Tv list changed
			//QMessageBox::information(this, tr("Info"), tr("Tv list changed."));
			//blinkLabel->setText(tr("Tv list changed."));
			int curr_tv = tvBox->itemData(tvBox->currentIndex(), Qt::UserRole).toInt();
			const int* tvList = LiveThread->tvList();
			int tvListSize = LiveThread->tvListSize();
			// fill combo
			tvBox->clear();
			int i, j, ind = 0;
			for (i = 0; i < tvListSize; i++)
			{
				for (j = 0; j < EOS_TV_TABLE_SZ; j++)
					if (TvTable[j].val == tvList[i])
					{
						tvBox->addItem(QString(TvTable[j].tv), QVariant((int)TvTable[j].val));
						if (tvList[i] == curr_tv)
							tvBox->setCurrentIndex(ind);
						ind++;
					}
			}
		}
		break;
	case CAMERA_EVENT_FPS_UPDATED:
		{
			double fps = e->value().toDouble();
			char str[10];
			sprintf(str, "%.1f fps", fps);
			fpsLabel->setText(QString(str));
		}
		break;
	case CAMERA_EVENT_AEMODE_CHANGED:
		{
			int mode = e->value().toInt();
			CurrSettings.AEMode = mode;
			//blinkLabel->setText(tr("AE Mode changed to %1").arg(mode));
			switch (mode)
			{
			case 2:
			case 3:
				dofBtn->setEnabled(true);
				avBox->setEnabled(true);
				break;
			default:
				dofBtn->setEnabled(false);
				dofBtn->setChecked(false);
				//avBox->clear();
				avBox->setEnabled(false);
				break;
			}
			switch (mode)
			{
			case 1:
			case 3:
				tvBox->setEnabled(true);
				isoBox->setEnabled(true);
				break;
			default:
				tvBox->setEnabled(false);
				//tvBox->clear();
				isoBox->setEnabled(false);
				//isoBox->clear();
				break;
			}
			if (mode > 6)
			{
				blinkLabel->setText(tr("You set invalid AE mode on the camera!"));
				blinkLabel->start();
				shutdown();
				fpsLabel->setText(tr("0 fps"));
				//QMessageBox::critical(this, tr("Error"), tr("You set invalid AE mode on the camera!"));
			}
			else
			{
				int val;
				for (int i = 0; i < AEModeBox->count(); i++)
				{
					val = AEModeBox->itemData(i, Qt::UserRole).toInt();
					if (val == mode)
					{
						AEModeBox->setCurrentIndex(i);
						break;
					}
				}
			}
			if (LiveThread)
			{
				LiveThread->cmdRequestAvList();
				LiveThread->cmdRequestTvList();
				LiveThread->cmdRequestISOList();
				LiveThread->cmdRequestAv();
				LiveThread->cmdRequestTv();
				LiveThread->cmdRequestISO();
			}
		}
		break;
	case CAMERA_EVENT_AFMODE_CHANGED:
		{
			int mode = e->value().toInt();
			//blinkLabel->setText(tr("AE Mode changed to %1").arg(mode));
			switch (mode)
			{
			case 0:
			case 1:
			case 2:
				focusNear3Btn->setEnabled(true);
				focusNear2Btn->setEnabled(true);
				focusNear1Btn->setEnabled(true);
				focusFar1Btn->setEnabled(true);
				focusFar2Btn->setEnabled(true);
				focusFar3Btn->setEnabled(true);
				AFBtn->setEnabled(true);
				break;
			case 3:
				focusNear3Btn->setEnabled(false);
				focusNear2Btn->setEnabled(false);
				focusNear1Btn->setEnabled(false);
				focusFar1Btn->setEnabled(false);
				focusFar2Btn->setEnabled(false);
				focusFar3Btn->setEnabled(false);
				AFBtn->setEnabled(false);
				slotStopAutoFocus();
				break;
			default:
				break;
			}
		}
		break;
	case CAMERA_EVENT_ZOOM_CHANGED_STOP:
		slotStop();
		blinkLabel->setText(tr("Can't continue write when zoom changed!"));
		break;
	// from capture windows
	case CAMERA_EVENT_ZOOMPOS_NEEDCHANGE:
		if (LiveThread)
		{
			QPoint p = e->value().toPoint();
			LiveThread->cmdSetZoomPos(p.x(), p.y());
		}
		break;
	case CAMERA_EVENT_AF_STOPPED:
		slotStopAutoFocus();
		AFBtn->setChecked(false);
		break;
	case CAMERA_EVENT_HISTOGRAM:
		if (HistogramWnd)
			HistogramWnd->updateHistogram();
		break;
	case CAMERA_EVENT_HISTOGRAM_CLOSED:
		if (HistogramWnd)
		{
			delete HistogramWnd;
			HistogramWnd = 0;
		}
		HistBtn->setChecked(false);
		break;
	case CAMERA_EVENT_SHUTDOWN:
		// thread say about camera shutdow
		blinkLabel->setText(tr("Lost connection with camera."));
		blinkLabel->start();
		shutdown();
		QMessageBox::critical(this, tr("Error"), tr("Lost connection with camera."));
		break;
	default:
		break;
	}
	e->accept();
}

void GEOSRecWnd::slotSelFile()
{
	QString path = QFileDialog::getSaveFileName(this, tr("Save file"), CurrSettings.Path,
		tr("Video (*.avi)"), 0, QFileDialog::DontConfirmOverwrite);
	if (!path.isNull() && !path.isEmpty())
	{
		CurrSettings.Path = path;
	}
}

void GEOSRecWnd::slotStart()
{
	if (LiveThread && LiveThread->isInit())
	{
		QFileInfo fi(CurrSettings.Path);
		while (fi.exists())
		{
			CurrSettings.Path = giveNextName(CurrSettings.Path);
			fi.setFile(CurrSettings.Path);
		}
		//QMessageBox::information(this, tr("path"), Path);
		static QTextCodec* tcodec = QTextCodec::codecForLocale();
		LiveThread->setFileName(tcodec->fromUnicode(CurrSettings.Path).data());
		selFileBtn->setEnabled(false);
		showBox->setEnabled(false);
		startBtn->setEnabled(false);
		stopBtn->setEnabled(true);
		zoom5xBtn->setEnabled(false);
		LiveThread->startWrite();
		blinkLabel->setText(tr("WRITING"));
		blinkLabel->start();
	}
}

void GEOSRecWnd::slotStop()
{
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->stopWrite();
		blinkLabel->stop();
		blinkLabel->setText(tr("Ready"));
		showBox->setEnabled(true);
		startBtn->setEnabled(true);
		stopBtn->setEnabled(false);
		selFileBtn->setEnabled(true);
		zoom5xBtn->setEnabled(true);
	}
}

void GEOSRecWnd::slotAESelected(int ae_ind)
{
	bool ok = false;
	int ae = AEModeBox->itemData(ae_ind, Qt::UserRole).toInt(&ok);
	if (LiveThread && LiveThread->isInit() && ok)
	{
		LiveThread->cmdSetAEMode(ae);
		CurrSettings.AEMode = ae;
	}
}

void GEOSRecWnd::slotDofPressed()
{
	bool ok = false;
	int av = 0;
	int dof = dofBtn->isChecked() ? 1 : 0;
	int curr_ind = avBox->currentIndex();
	if (curr_ind >= 0)
		av = avBox->itemData(curr_ind, Qt::UserRole).toInt(&ok);
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdSetAv(av, dof);
	}
}

void GEOSRecWnd::slotISOSelected(int iso_ind)
{
	bool ok = false;
	int iso = isoBox->itemData(iso_ind, Qt::UserRole).toInt(&ok);
	if (LiveThread && LiveThread->isInit() && ok)
	{
		LiveThread->cmdSetISO(iso);
		CurrSettings.ISO = iso;
	}
}

void GEOSRecWnd::slotAvSelected(int av_ind)
{
	bool ok = false;
	int dof = dofBtn->isChecked() ? 1 : 0;
	int av = avBox->itemData(av_ind, Qt::UserRole).toInt(&ok);
	//int dof = dofBtn->isChecked() ? 1 : 0;
	if (LiveThread && LiveThread->isInit() && ok)
	{
		LiveThread->cmdSetAv(av, dof);
		CurrSettings.Av = av;
	}
}

void GEOSRecWnd::slotTvSelected(int tv_ind)
{
	bool ok = false;
	int tv = tvBox->itemData(tv_ind, Qt::UserRole).toInt(&ok);
	if (LiveThread && LiveThread->isInit() && ok)
	{
		LiveThread->cmdSetTv(tv);
		CurrSettings.Tv = tv;
	}
}

void GEOSRecWnd::slotWbSelected(int wb_ind)
{
	bool ok = false;
	int wb = wbBox->itemData(wb_ind, Qt::UserRole).toInt(&ok);
	if (wb == 9)		// Color Temperature
	{
		wbTempBox->setEnabled(true);
	}
	else
	{
		wbTempBox->setEnabled(false);
	}
	// set to camera
	if (LiveThread && LiveThread->isInit() && ok)
	{
		LiveThread->cmdSetWB(wb, wbTempBox->value());
		CurrSettings.Wb = wb;
	}
}

void GEOSRecWnd::slotWbTempSelected(int wb_temp)
{
	// set to camera
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdSetWB(9, wb_temp);
		CurrSettings.WbTemp = wb_temp;
	}
}

void GEOSRecWnd::slotFocusNear3()
{
	// send command to camera
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdAdjFocus(0, 3);
	}
}

void GEOSRecWnd::slotFocusNear2()
{
	// send command to camera
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdAdjFocus(0, 2);
	}
}

void GEOSRecWnd::slotFocusNear1()
{
	// send command to camera
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdAdjFocus(0, 1);
	}
}

void GEOSRecWnd::slotFocusFar1()
{
	// send command to camera
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdAdjFocus(1, 1);
	}
}

void GEOSRecWnd::slotFocusFar2()
{
	// send command to camera
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdAdjFocus(1, 2);
	}
}

void GEOSRecWnd::slotFocusFar3()
{
	// send command to camera
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdAdjFocus(1, 3);
	}
}

void GEOSRecWnd::slotZoom5x()
{
	int zoom = zoom5xBtn->isChecked() ? 5 : 1;
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdSetZoom(zoom);
	}
	HistBtn->setEnabled(zoom == 1);
}

void GEOSRecWnd::slotAutoFocus()
{
	bool af = AFBtn->isChecked();
	if (af)
	{
		if (!AFThread)
		{
			AFThread = new GAFThread(this, LiveThread, CaptureWnd);
			AFThread->start();
		}
	}
	else
		slotStopAutoFocus();
}

void GEOSRecWnd::slotStopAutoFocus()
{
	if (AFThread)
	{
		AFThread->stop();
		AFThread->wait();
		delete AFThread;
		AFThread = 0;
		//AFBtn->setChecked(false);
	}
}

void GEOSRecWnd::slotHistogram()
{
	bool hist = HistBtn->isChecked();
	if (LiveThread)
		LiveThread->setWantHistogram(hist);
	if (!hist)
	{
		if (HistogramWnd)
		{
			HistogramWnd->close();
			// don't delete here
			// it deleted in customEvent()
		}
	}
	else
	{
		if (!HistogramWnd)
		{
			HistogramWnd = new GHistogramWnd(this, LiveThread);
			HistogramWnd->show();
		}
	}
}

void GEOSRecWnd::slotWorkTimeout()
{
	shutdown();
	blinkLabel->stop();
	blinkLabel->setText(tr("Make some tea"));
	QMessageBox::information(this, tr("Timeout"), tr("To save you device program will be stopped."));
}

void GEOSRecWnd::shutdown()
{
	GMyLiveThread* p = LiveThread;
	LiveThread = 0;
	p->setCaptureWnd(0);
	p->stop();
	p->wait();
	FILE* f = fopen("debug_info.txt", "wt");
	if (f)
	{
		fprintf(f, "all frames: %I64d\n", p->allFramesCount());
		fprintf(f, "all writen frames: %I64d\n", p->writenCount());
		fprintf(f, "skipped frames on painting: %I64d\n", p->skippedCount());
		fprintf(f, "elapsed time (sec): %d\n", p->elapsedTime()/1000);
		if (p->elapsedTime() > 0)
			fprintf(f, "refresh rate(fps): %.1f\n", 1000.0*(float)p->allFramesCount()/((float)p->elapsedTime()));
		fclose(f);
	}
	delete p;
	slotStopAutoFocus();
	if (HistogramWnd)
	{
		HistogramWnd->close();
		delete HistogramWnd;
		HistogramWnd = 0;
		HistBtn->setChecked(false);
	}
	showBox->setEnabled(false);
	startBtn->setEnabled(false);
	stopBtn->setEnabled(false);
	selFileBtn->setEnabled(false);
	reconnBtn->setEnabled(true);
	AEModeBox->setEnabled(false);
	isoBox->setEnabled(false);
	dofBtn->setEnabled(false);
	avBox->setEnabled(false);
	tvBox->setEnabled(false);
	wbBox->setEnabled(false);
	wbTempBox->setEnabled(false);
	focusNear3Btn->setEnabled(false);
	focusNear2Btn->setEnabled(false);
	focusNear1Btn->setEnabled(false);
	focusFar1Btn->setEnabled(false);
	focusFar2Btn->setEnabled(false);
	focusFar3Btn->setEnabled(false);
	zoom5xBtn->setEnabled(false);
	AFBtn->setEnabled(false);
	HistBtn->setEnabled(false);
	fpsLabel->setText(tr("0 fps"));
}

void GEOSRecWnd::slotShowImageChanged(int state)
{
	CaptureWnd->setShowLiveImage(state == Qt::Checked);
	CaptureWnd->update();
}

void GEOSRecWnd::slotAbout()
{
	GAboutDlg dlg(this);
	dlg.exec();
}

QString GEOSRecWnd::giveNextName(const QString& path)
{
	QString dirname = "";
	QString basename;
	int sep_pos = path.lastIndexOf("\\");
	if (sep_pos < 0)
		sep_pos = path.lastIndexOf("/");
	if (sep_pos > 0)
	{
		dirname = path.left(sep_pos + 1);
		basename = path.mid(sep_pos + 1, -1);
	}
	else
		basename = path;

	int key_pos = basename.lastIndexOf("_");
	int dot_pos = basename.lastIndexOf(".");
	if (dot_pos < 0)
		dot_pos = basename.length();
	QString base;
	QString name;
	if (key_pos < 0)		// not found
	{
		base = basename.left(dot_pos);
		name = base + QString("_1");
	}
	else
	{
		base = basename.left(key_pos);
		QString num_part = basename.mid(key_pos + 1, dot_pos - key_pos - 1);
		bool ok;
		int num = num_part.toInt(&ok, 10);
		if (ok)
		{
			num++;
			num_part.setNum(num, 10);
			name = base + QString("_") + num_part;
		}
		else
			name = basename.left(dot_pos) + QString("_1");
	}
	return dirname + name + QString(".avi");
}
