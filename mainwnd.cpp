/***************************************************************************
 *   Copyright (C) 2008-2010 by Чернов А.А.                                *
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

#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QSpinBox>
#include <QStatusBar>
//#include <QTimer>
#include <QBoxLayout>
#include <QFileDialog>
#include <QTextCodec>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>
#include <QWheelEvent>
#include <QIcon>


#include <fcntl.h>
#include <stdio.h>
// #include "jpegwrt.h"

#include "debug.h"
#include <QDebug>
#include <QStyleFactory>

#include "mainwnd.h"
#include "about.h"
#include "pathsdlg.h"
#include "optionsdlg.h"
#include "blinklabel.h"
#include "capturewnd.h"
#include "livethread.h"
#include "afthread.h"
#include "histogramwnd.h"
#include "events.h"
#include "cam_tables.h"
#include "os_api.h"


// icons
#include "pixmaps/usb-sign-black.xpm"
#include "pixmaps/record.xpm"
#include "pixmaps/stop.xpm"

#include <stdlib.h>

#ifdef __MACOS__
	#include <CarbonCore/Folders.h>
#endif

int MAX_FILENAME_ATTEMPTS = 9999;

GEOSRecWnd::GEOSRecWnd()
 : QWidget(0)
{
	setWindowTitle(tr("EOS Camera Movie Record"));
	//setWindowIcon(QIcon::fromTheme("media-record"));
	// setWindowIcon(QIcon::fromTheme("camera-photo"));
	setWindowIcon(QIcon(":/eos-movrec2.ico"));
	QVBoxLayout* main_layout = new QVBoxLayout(this);
	QHBoxLayout* btn_layout = new QHBoxLayout();

	// QIcon::setThemeName("oxygen");
	
	// https://openapplibrary.org/dev-tutorials/qt-icon-themes
	// https://stackoverflow.com/questions/34417617/how-do-theme-icons-work-in-qt-creator-designer
	if (DEBUG) fprintf(stderr, "themeName: %s\n", QIcon::themeName().toLocal8Bit().constData());
	if (DEBUG) qDebug() << "Available QStyleFactory themes on your system = " << QStyleFactory::keys(); 
	if (DEBUG) qDebug() << "QIcon::themeSearchPaths() = " << QIcon::themeSearchPaths();
	
	reconnBtn = new QToolButton(this);
	//reconnBtn->setText(tr("R"));
	reconnBtn->setIcon(QPixmap(usb_sign_black_xpm));
	reconnBtn->setEnabled(false);
	reconnBtn->setToolTip(tr("Reconnect to camera (O)"));
	btn_layout->addWidget(reconnBtn, 0);
	btn_layout->addSpacing(10);

	startBtn = new QPushButton(tr("Write!"), this);
	startBtn->setIcon(QPixmap(record_xpm));
	// startBtn->setIcon(QIcon::fromTheme("media-record"));
	startBtn->setEnabled(false);
	startBtn->setToolTip(tr("Start recording (R)"));
	btn_layout->addWidget(startBtn, 0);

	stopBtn = new QPushButton(tr("Stop"), this);
	stopBtn->setIcon(QPixmap(stop_xpm));
	stopBtn->setEnabled(false);
	stopBtn->setToolTip(tr("Stop recording (S)"));
	stopBtn->setVisible(false);
	btn_layout->addWidget(stopBtn, 0);

	cptBtn = new QPushButton(tr("CPT"), this);
	// cptBtn->setIcon(QPixmap(cpt_xpm));
	// cptBtn->setIcon(QIcon::fromTheme("camera-photo"));
	// cptBtn->setIconSize(QSize(16, 16));
	// widget->setIcon(widget->style()->standardIcon(QStyle::SP_BrowserReload));
	cptBtn->setEnabled(false);
	cptBtn->setToolTip(tr("Take a screenshot (C)"));
	btn_layout->addWidget(cptBtn, 0);
		
	selSettingsBtn = new QToolButton(this);
	selSettingsBtn->setEnabled(true);
	selSettingsBtn->setText(tr("..."));
	// selSettingsBtn->setIcon(QIcon::fromTheme("configure"));
	selSettingsBtn->setIcon(QIcon::fromTheme("document-open-folder"));
	selSettingsBtn->setIconSize(QSize(16, 16));
	selSettingsBtn->setToolTip(tr("Select filepaths"));
	btn_layout->addWidget(selSettingsBtn, 0);
	
	btn_layout->addWidget(new QLabel(tr("Path:"), this), 0);
	path_label = new QLabel("[]", this);
	// path_label->setFrameShape(QFrame::Box);
	path_label->setMinimumSize(1, 1);
	btn_layout->addWidget(path_label, 1);

	showBox = new QCheckBox(tr("Show LiveView"), this);
	showBox->setToolTip(tr("Show captured image (I)"));
	btn_layout->addWidget(showBox, 0);
	showBox->setCheckState(Qt::Checked);

	optionsBtn = new QToolButton(this);
	optionsBtn->setText(tr("O"));
	optionsBtn->setIcon(QIcon::fromTheme("configure"));
	optionsBtn->setIconSize(QSize(16, 16));
	optionsBtn->setToolTip(tr("Open camera options dialog"));
	btn_layout->addWidget(optionsBtn, 0);

	QToolButton* aboutBtn = new QToolButton(this);
	aboutBtn->setText(tr("A"));
	aboutBtn->setIcon(QIcon::fromTheme("dialog-information"));
	aboutBtn->setIconSize(QSize(16, 16));
	aboutBtn->setToolTip(tr("Show about box"));
	btn_layout->addWidget(aboutBtn, 0);

	QHBoxLayout* focus_layout = new QHBoxLayout();

	focus_layout->addWidget(new QLabel(tr("Focus adjust"), this), 0);

	focusNear3Btn = new QToolButton(this);
	focusNear3Btn->setText(tr("<<<"));
	focusNear3Btn->setEnabled(false);
	focusNear3Btn->setToolTip(tr("Move focus point to near (1) or (scroll down)"));
	focus_layout->addWidget(focusNear3Btn, 0);
	focusNear2Btn = new QToolButton(this);
	focusNear2Btn->setText(tr("<<"));
	focusNear2Btn->setEnabled(false);
	focusNear2Btn->setToolTip(tr("Move focus point to near (2) or (scroll down + ctrl)"));
	focus_layout->addWidget(focusNear2Btn, 0);
	focusNear1Btn = new QToolButton(this);
	focusNear1Btn->setText(tr("<"));
	focusNear1Btn->setEnabled(false);
	focusNear1Btn->setToolTip(tr("Move focus point to near (3) or (scroll down + shift)"));
	focus_layout->addWidget(focusNear1Btn, 0);

	focusFar1Btn = new QToolButton(this);
	focusFar1Btn->setText(tr(">"));
	focusFar1Btn->setEnabled(false);
	focusFar1Btn->setToolTip(tr("Move focus point to far (4) or (scroll up + shift)"));
	focus_layout->addWidget(focusFar1Btn, 0);
	focusFar2Btn = new QToolButton(this);
	focusFar2Btn->setText(tr(">>"));
	focusFar2Btn->setEnabled(false);
	focusFar2Btn->setToolTip(tr("Move focus point to far (5) or (scroll up + ctrl)"));
	focus_layout->addWidget(focusFar2Btn, 0);
	focusFar3Btn = new QToolButton(this);
	focusFar3Btn->setText(tr(">>>"));
	focusFar3Btn->setEnabled(false);
	focusFar3Btn->setToolTip(tr("Move focus point to far (6) or (scroll up)"));
	focus_layout->addWidget(focusFar3Btn, 0);

	focus_layout->addSpacing(10);
	AFBtn = new QToolButton(this);
	AFBtn->setText(tr("AF"));
	AFBtn->setCheckable(true);
	AFBtn->setEnabled(false);
	AFBtn->setToolTip(tr("Try experimental autofocus"));
	focus_layout->addWidget(AFBtn, 0);

	AFCamBtn = new QToolButton(this);
	AFCamBtn->setText(tr("CAF"));
	AFCamBtn->setCheckable(false);
	AFCamBtn->setEnabled(false);
	AFCamBtn->setToolTip(tr("Camera Auto focus (only on 50D and later!)"));
	focus_layout->addWidget(AFCamBtn, 0);

	focus_layout->addSpacing(10);

	focus_layout->addWidget(new QLabel(tr("Zoom")));

	zoomPosClickBtn = new QToolButton(this);
	zoomPosClickBtn->setText(tr("*"));
	zoomPosClickBtn->setEnabled(false);
	zoomPosClickBtn->setCheckable(true);
	zoomPosClickBtn->setToolTip(tr("Select zoom position by clicking on preview"));
	zoomPosByClick = zoomPosClickBtn->isChecked();
	focus_layout->addWidget(zoomPosClickBtn, 0);
	
	zoom5xBtn = new QToolButton(this);
	zoom5xBtn->setText(tr("5x"));
	zoom5xBtn->setEnabled(false);
	zoom5xBtn->setCheckable(true);
	zoom5xBtn->setToolTip(tr("5x Zoom (crop) (Z)"));
	focus_layout->addWidget(zoom5xBtn, 0);

	zoom10xBtn = new QToolButton(this);
	zoom10xBtn->setText(tr("10x"));
	zoom10xBtn->setEnabled(false);
	zoom10xBtn->setCheckable(true);
	zoom10xBtn->setToolTip(tr("10x Zoom (crop) (UNIMPLEMENTED IN DRIVER)"));
	focus_layout->addWidget(zoom10xBtn, 0);

	HistBtn = new QToolButton(this);
	HistBtn->setText(tr("H"));
	HistBtn->setEnabled(false);
	HistBtn->setCheckable(true);
	HistBtn->setToolTip(tr("Show histogram"));
	focus_layout->addWidget(HistBtn, 0);

	focus_layout->addSpacing(20);
	QLabel* wbLabel = new QLabel(tr("WB:"), this);
	focus_layout->addWidget(wbLabel, 0);

	wbBox = new QComboBox(this);
	wbBox->setToolTip(tr("Select white balance"));
	wbBox->setEditable(false);
	wbBox->setEnabled(false);
	wbBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
#ifdef EDSDK
	wbBox->addItem(tr("Auto"), QVariant((int)0));
	wbBox->addItem(tr("Daylight"), QVariant((int)1));
	wbBox->addItem(tr("Cloudy"), QVariant((int)2));
	wbBox->addItem(tr("Tungsten"), QVariant((int)3));
	wbBox->addItem(tr("Fluorescent"), QVariant((int)4));
	wbBox->addItem(tr("Flash"), QVariant((int)5));
	wbBox->addItem(tr("Shade"), QVariant((int)8));
	wbBox->addItem(tr("Color Temperature"), QVariant((int)9));
#endif
	focus_layout->addWidget(wbBox, 0);

	wbTempBox = new QSpinBox(this);
	wbTempBox->setToolTip(tr("Select white balance temperature"));
	wbTempBox->setEnabled(false);
	wbTempBox->setMinimum(2800);
	wbTempBox->setMaximum(10000);
	wbTempBox->setValue(5200);
	wbTempBox->setSingleStep(100);
	wbTempBox->setSuffix(tr("K"));
	focus_layout->addWidget(wbTempBox, 0);

	focus_layout->addStretch(1);
	
	// ------------------
	
	QHBoxLayout* cam_opts_layout = new QHBoxLayout();

	QLabel* aeLabel = new QLabel(tr("AE:"), this);
	cam_opts_layout->addWidget(aeLabel, 0);
	
	AEModeBox = new QComboBox(this);
	AEModeBox->setToolTip(tr("Select AE Mode (auto exposure)"));
	AEModeBox->setEditable(false);
	AEModeBox->setEnabled(false);
	AEModeBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	cam_opts_layout->addWidget(AEModeBox, 0);

	isoBox = new QComboBox(this);
	isoBox->setToolTip(tr("Select ISO"));
	isoBox->setEditable(false);
	isoBox->setEnabled(false);
	cam_opts_layout->addWidget(isoBox, 0);

	dofBtn = new QToolButton(this);
	dofBtn->setText("DOF");
	dofBtn->setToolTip(tr("Enable/disable depth of field (D)"));
	dofBtn->setCheckable(true);
	dofBtn->setEnabled(false);
	cam_opts_layout->addWidget(dofBtn, 0);

	avBox = new QComboBox(this);
	avBox->setToolTip(tr("Select Aperture / AV"));
	//avBox->setEnabled(false);
	avBox->setEditable(false);
	cam_opts_layout->addWidget(avBox, 0);

	tvBox = new QComboBox(this);
	tvBox->setToolTip(tr("Select Shutterspeed / TV"));
	//tvBox->setEnabled(false);
	tvBox->setEditable(false);
	cam_opts_layout->addWidget(tvBox, 0);
	
	cam_opts_layout->addSpacing(10);

	QHBoxLayout* timer_layout = new QHBoxLayout();

	timeTimerBox = new QCheckBox(tr("Time timer"), this);
	timeTimerBox->setEnabled(false);
	timeTimerBox->setChecked(false);
	timer_layout->addWidget(timeTimerBox, 0);

	timeTimerSpinBox = new QSpinBox(this);
	timeTimerSpinBox->setEnabled(false);
	timeTimerSpinBox->setRange(1, 1800);
	timeTimerSpinBox->setSingleStep(1);
	timeTimerSpinBox->setSuffix(tr("s."));
	timer_layout->addWidget(timeTimerSpinBox, 0);

	framesTimerBox = new QCheckBox(tr("Frames count timer"), this);
	framesTimerBox->setEnabled(false);
	framesTimerBox->setChecked(false);
	timer_layout->addWidget(framesTimerBox, 0);

	framesTimerSpinBox = new QSpinBox(this);
	framesTimerSpinBox->setEnabled(false);
	framesTimerSpinBox->setRange(1, 54000);
	framesTimerSpinBox->setSingleStep(1);
	timer_layout->addWidget(framesTimerSpinBox, 0);

	timer_layout->addStretch(1);

	QHBoxLayout* battery_layout = new QHBoxLayout();

	if (QIcon::hasThemeIcon("battery")) {
		batteryLevelIcon = new QLabel("", this);
		QPixmap batteryIcon = QIcon::fromTheme("battery").pixmap(16, 16);
		batteryLevelIcon->setPixmap(batteryIcon);
		batteryLevelIcon->setToolTip(tr("Battery level: "));
		battery_layout->addWidget(batteryLevelIcon, 0);
		}
	else {
		battery_layout->addWidget(new QLabel(tr("Batt: "), this), 0);
		}

	batteryLevelLabel = new QLabel("-", this);
	batteryLevelLabel->setToolTip(tr("Battery level (as reported by driver)"));
	battery_layout->addWidget(batteryLevelLabel, 0);

	
	QHBoxLayout* bottom_layout = new QHBoxLayout();
	bottom_layout->addLayout(cam_opts_layout, 0);
	bottom_layout->addLayout(timer_layout, 0);
	bottom_layout->addLayout(battery_layout, 0);

	main_layout->addLayout(btn_layout, 0);

	main_layout->addLayout(focus_layout, 0);

	main_layout->addLayout(bottom_layout, 0);

	CaptureWnd = new GEOSCaptureWnd(this);
	main_layout->addWidget(CaptureWnd, 0);
	main_layout->addStretch(1);
	

	blinkLabel = new QBlinkLabel(tr("Starting..."), this);

	fpsLabel = new QLabel(tr("0 fps"), this);
	//fpsLabel->setFrameShape(QFrame::Box);

	statusBar = new QStatusBar(this);
	//statusBar->showMessage(tr("xyz"));
	statusBar->addWidget(blinkLabel, 10);
	statusBar->addWidget(fpsLabel, 0);

	framesLabel = new QLabel(this);
	statusBar->addWidget(framesLabel, 0);
	timeLabel = new QLabel(this);
	statusBar->addWidget(timeLabel, 0);

	main_layout->addWidget(statusBar, 0);

	//setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	QShortcut* focusNear3Shortcut = new QShortcut(QKeySequence(Qt::Key_1), this);
	QShortcut* focusNear2Shortcut = new QShortcut(QKeySequence(Qt::Key_2), this);
	QShortcut* focusNear1Shortcut = new QShortcut(QKeySequence(Qt::Key_3), this);
	QShortcut* focusFar1Shortcut = new QShortcut(QKeySequence(Qt::Key_4), this);
	QShortcut* focusFar2Shortcut = new QShortcut(QKeySequence(Qt::Key_5), this);
	QShortcut* focusFar3Shortcut = new QShortcut(QKeySequence(Qt::Key_6), this);
	QShortcut* CPTShortcut = new QShortcut(QKeySequence(Qt::Key_C), this);
	QShortcut* recordShortcut = new QShortcut(QKeySequence(Qt::Key_R), this);
	QShortcut* stopShortcut = new QShortcut(QKeySequence(Qt::Key_S), this);
	QShortcut* reconnectShortcut = new QShortcut(QKeySequence(Qt::Key_O), this);
	QShortcut* dofShortcut = new QShortcut(QKeySequence(Qt::Key_D), this);
	QShortcut* zoomShortcut = new QShortcut(QKeySequence(Qt::Key_Z), this);
	QShortcut* captureShortcut = new QShortcut(QKeySequence(Qt::Key_I), this);

	connect(selSettingsBtn, SIGNAL(clicked()), this, SLOT(slotSettings()));
	connect(reconnBtn, SIGNAL(clicked()), this, SLOT(slotReconnect()));
	connect(startBtn, SIGNAL(clicked()), this, SLOT(slotStart()));
	connect(stopBtn, SIGNAL(clicked()), this, SLOT(slotStop()));
	connect(cptBtn, SIGNAL(clicked()), this, SLOT(slotCPT()));
	connect(AEModeBox, SIGNAL(activated(int)), this, SLOT(slotAESelected(int)));
	connect(dofBtn, SIGNAL(clicked()), this, SLOT(slotDofPressed()));
	connect(isoBox, SIGNAL(activated(int)), this, SLOT(slotISOSelected(int)));
	connect(avBox, SIGNAL(activated(int)), this, SLOT(slotAvSelected(int)));
	connect(tvBox, SIGNAL(activated(int)), this, SLOT(slotTvSelected(int)));
	connect(wbBox, SIGNAL(activated(int)), this, SLOT(slotWbSelected(int)));
	connect(wbTempBox, SIGNAL(valueChanged(int)), this, SLOT(slotWbTempSelected(int)));
	connect(showBox, SIGNAL(stateChanged(int)), this, SLOT(slotShowImageChanged(int)));
	connect(optionsBtn, SIGNAL(clicked()), this, SLOT(slotOptions()));
	connect(aboutBtn, SIGNAL(clicked()), this, SLOT(slotAbout()));
	connect(focusNear3Btn, SIGNAL(clicked()), this, SLOT(slotFocusNear3()));
	connect(focusNear2Btn, SIGNAL(clicked()), this, SLOT(slotFocusNear2()));
	connect(focusNear1Btn, SIGNAL(clicked()), this, SLOT(slotFocusNear1()));
	connect(focusFar1Btn, SIGNAL(clicked()), this, SLOT(slotFocusFar1()));
	connect(focusFar2Btn, SIGNAL(clicked()), this, SLOT(slotFocusFar2()));
	connect(focusFar3Btn, SIGNAL(clicked()), this, SLOT(slotFocusFar3()));
	connect(zoomPosClickBtn, SIGNAL(clicked()), this, SLOT(slotZoomPosClick()));
	connect(zoom5xBtn, SIGNAL(clicked()), this, SLOT(slotZoom5x()));
	connect(zoom10xBtn, SIGNAL(clicked()), this, SLOT(slotZoom10x()));
	connect(AFBtn, SIGNAL(clicked()), this, SLOT(slotAutoFocus()));
	connect(AFCamBtn, SIGNAL(clicked()), this, SLOT(slotCameraAF()));
	connect(HistBtn, SIGNAL(clicked()), this, SLOT(slotHistogram()));

	connect(focusNear3Shortcut, SIGNAL(activated()), this, SLOT(slotFocusNear3()));
	connect(focusNear2Shortcut, SIGNAL(activated()), this, SLOT(slotFocusNear2()));
	connect(focusNear1Shortcut, SIGNAL(activated()), this, SLOT(slotFocusNear1()));
	connect(focusFar1Shortcut, SIGNAL(activated()), this, SLOT(slotFocusFar1()));
	connect(focusFar2Shortcut, SIGNAL(activated()), this, SLOT(slotFocusFar2()));
	connect(focusFar3Shortcut, SIGNAL(activated()), this, SLOT(slotFocusFar3()));
	connect(CPTShortcut, SIGNAL(activated()), this, SLOT(slotCPT()));
	connect(recordShortcut, SIGNAL(activated()), this, SLOT(slotStart()));
	connect(stopShortcut, SIGNAL(activated()), this, SLOT(slotStop()));
	connect(reconnectShortcut, SIGNAL(activated()), this, SLOT(slotReconnect()));
	connect(dofShortcut, SIGNAL(activated()), dofBtn, SLOT(click()));
	connect(zoomShortcut, SIGNAL(activated()), zoom5xBtn, SLOT(click()));
	// connect(zoomShortcut, SIGNAL(activated()), zoom10xBtn, SLOT(click()));
	connect(captureShortcut, SIGNAL(activated()), showBox, SLOT(click()));

	connect(timeTimerBox, SIGNAL(clicked(bool)), this, SLOT(slotTimeTimerSwitch(bool)));
	connect(framesTimerBox, SIGNAL(clicked(bool)), this, SLOT(slotFramesTimerSwitch(bool)));

	// defaults
	commonFilePath = QDir::homePath() + QDir::separator() + "eos_movrec"; //  QDir::currentPath();
	CurrSettings.VidName = commonFilePath + QDir::separator() + tr("vid_out.avi");
	CurrSettings.ImgName = commonFilePath + QDir::separator() + tr("img_out.jpg");
	CurrSettings.Av = -1;
	CurrSettings.Tv = -1;
	CurrSettings.ISO = -1;
	CurrSettings.Wb = -1;
	CurrSettings.WbTemp = -1;
	CurrSettings.AEMode = -1;
	CurrSettings.AFMode = 1;
	CurrSettings.BufferSize = 1024*1024;
	CurrSettings.UseStabFPS = true;
	CurrSettings.ShowWhiteBox = false;

	BackupSettings.VidName = CurrSettings.VidName;
	BackupSettings.ImgName = CurrSettings.ImgName;
	BackupSettings.Av = CurrSettings.Av;
	BackupSettings.Tv = CurrSettings.Tv;
	BackupSettings.ISO = CurrSettings.ISO;
	BackupSettings.Wb = CurrSettings.Wb;
	BackupSettings.WbTemp = CurrSettings.WbTemp;
	BackupSettings.AEMode = CurrSettings.AEMode;
	BackupSettings.AFMode = CurrSettings.AFMode;
	BackupSettings.BufferSize = CurrSettings.BufferSize;
	BackupSettings.UseStabFPS = CurrSettings.UseStabFPS;
	BackupSettings.ShowWhiteBox = CurrSettings.ShowWhiteBox;

	LiveThread = new GMyLiveThread(this);
	LiveThread->setCaptureWnd(CaptureWnd);
	LiveThread->start(QThread::HighestPriority);

	AFThread = 0;
	HistogramWnd = 0;

	loadSettingsFilepaths();

	//QTimer::singleShot(4000, this, SLOT(slotStartTimeout()));
	/*StartTimer = new QTimer(this);
	StartTimer->setSingleShot(true);
	connect(StartTimer, SIGNAL(timeout()), this, SLOT(slotStartTimeout()));
	StartTimer->start(10000);*/
	// disabled at the request of members of the forum (forum.ixbt.com)
	//QTimer::singleShot(1200000, this, SLOT(slotWorkTimeout()));		// max work time is 20 min
}

GEOSRecWnd::~GEOSRecWnd()
{
	/*if (StartTimer)
		delete StartTimer;*/
}



void GEOSRecWnd::signal_handler(int s){
	printf("\nCaught signal %d\n",s);
	close_app();
	exit(1); 
}

void GEOSRecWnd::closeEvent(QCloseEvent* event)
{
	close_app();
}

void GEOSRecWnd::close_app()
{
	printf("** Closing app...\n");
	// stop thread;
	if (LiveThread)
	{
		if (LiveThread->isInit())
			saveSettings();
		GMyLiveThread* p = LiveThread;
		LiveThread = 0;
		p->stop();
		p->wait();
		FILE* f = 0;
#ifdef __MACOS__
		FSRef desktopFolderRef;
		OSErr err = FSFindFolder(kUserDomain, kDocumentsFolderType, kDontCreateFolder, &desktopFolderRef);
		char path[PATH_MAX]	= "";
		err = FSRefMakePath(&desktopFolderRef, (UInt8*)path, PATH_MAX);
		sprintf(path, "%s/eos_movrec.debug_info.txt", path);
		f = fopen(path, "wt");
#else
		f = fopen("debug_info.txt", "wt");
#endif
		if (f)
		{
			fprintf(f, "all frames: " INT64_PRINTF_SPEC "\n", p->allFramesCount());
			fprintf(f, "all writen frames: " INT64_PRINTF_SPEC "\n", p->writenCount());
			fprintf(f, "skipped frames on painting: " INT64_PRINTF_SPEC "\n", p->skippedCount());
			fprintf(f, "elapsed time (sec): %d\n", p->elapsedTime()/1000);
			if (p->elapsedTime() > 0)
			{
				fprintf(f, "duplicated frames on writing: " INT64_PRINTF_SPEC "\n", p->duplicatedCount());
				fprintf(f, "stable FPS (calc at start): %.2f\n", p->stableFPS());
				fprintf(f, "refresh rate(fps): %.1f\n", 1000.0*(float)p->allFramesCount()/((float)p->elapsedTime()));
			}
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

bool GEOSRecWnd::isLive()
{
	return (LiveThread);
}

/*void GEOSRecWnd::slotStartTimeout()
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
				struct EOSCamFeatures features = LiveThread->cameraFeatures();
				QSizeF largeSize = QSizeF(features.JpegLargeSize_x, features.JpegLargeSize_y);
				QSizeF lvSize = QSizeF(features.LiveViewSize_x, features.LiveViewSize_y);
				if (!largeSize.isEmpty() && !lvSize.isEmpty())
					CaptureWnd->setZoomPositionDivisor(largeSize.width()/lvSize.width(), largeSize.height()/lvSize.height());
				//startBtn->setEnabled(true);
				AEModeBox->setEnabled(true);
				dofBtn->setEnabled(true);
				zoom5xBtn->setEnabled(true);
				HistBtn->setEnabled(true);
				showBox->setEnabled(true);
				// next line realy work in customEvents()
				//AFCamBtn->setEnabled(features.HasAF);
				blinkLabel->stop();
				QString str = LiveThread->cameraName() + QString(": ");
				blinkLabel->setText(str + tr("Ready"));
				optionsBtn->setEnabled(true);
				// at this time we already received all settings from camera
				loadSettings();
				LiveThread->setUseStabFPS(CurrSettings.UseStabFPS);
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
}*/

void GEOSRecWnd::slotReconnect()
{
	if (!LiveThread)
	{
		blinkLabel->stop();
		blinkLabel->setText(tr("Starting...  (reconnect)"));
		reconnBtn->setEnabled(false);

		LiveThread = new GMyLiveThread(this);
		LiveThread->setCaptureWnd(CaptureWnd);
		LiveThread->start(QThread::HighestPriority);

		//QTimer::singleShot(4000, this, SLOT(slotStartTimeout()));
		//StartTimer->start(4000);
		// disabled at the request of members of the forum (forum.ixbt.com)
		//QTimer::singleShot(1200000, this, SLOT(slotWorkTimeout()));		// max work time is 20 min
	}
}

void GEOSRecWnd::loadSettingsFilepaths()
{
	QSettings settings(QSettings::UserScope, QString("eos_movrec"));
	CurrSettings.VidName = settings.value(QString("VidName"), CurrSettings.VidName).toString();
	CurrSettings.ImgName = settings.value(QString("ImgName"), CurrSettings.ImgName).toString();
	derivePath();
	setPathLabel();
}

void GEOSRecWnd::loadSettings()
{
	BackupSettings.AEMode = CurrSettings.AEMode;
	BackupSettings.Av = CurrSettings.Av;
	BackupSettings.Tv = CurrSettings.Tv;
	BackupSettings.ISO = CurrSettings.ISO;
	BackupSettings.Wb = CurrSettings.Wb;
	BackupSettings.WbTemp = CurrSettings.WbTemp;
	BackupSettings.AFMode = CurrSettings.AFMode;
	BackupSettings.BufferSize = CurrSettings.BufferSize;
	BackupSettings.UseStabFPS = CurrSettings.UseStabFPS;
	BackupSettings.ShowWhiteBox = CurrSettings.ShowWhiteBox;

	loadSettingsFilepaths();
	
	QSettings settings(QSettings::UserScope, QString("eos_movrec"));
	
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
	{
		if (wb == wbBox->itemData(i, Qt::UserRole).toInt())
		{
			wbBox->setCurrentIndex(i);
			slotWbSelected(i);
			break;
		}
	}
	int iso = settings.value(QString("ISO"), QVariant((int)-1)).toInt();
	for (i = 0; i < isoBox->count(); i++)
		if (iso == isoBox->itemData(i, Qt::UserRole).toInt())
		{
			isoBox->setCurrentIndex(i);
			slotISOSelected(i);
			break;
		}
	CurrSettings.AFMode = settings.value(QString("AFMode"), (int)1).toInt();
	CurrSettings.BufferSize = settings.value(QString("BufferSize"), (int)1024*1024).toInt();
	CurrSettings.UseStabFPS = settings.value(QString("UseStabFPS"), true).toBool();
	CurrSettings.ShowWhiteBox = settings.value(QString("ShowWhiteBox"), false).toBool();
}

void GEOSRecWnd::saveSettingsFilepaths()
{
	QSettings settings(QSettings::UserScope, QString("eos_movrec"));
	
	settings.setValue(QString("VidName"), QVariant(CurrSettings.VidName));
	settings.setValue(QString("ImgName"), QVariant(CurrSettings.ImgName));
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
		OSSleep(100);
	}
	QSettings settings(QSettings::UserScope, QString("eos_movrec"));
	
	settings.setValue(QString("AEMode"), QVariant(CurrSettings.AEMode));
	settings.setValue(QString("Av"), QVariant(CurrSettings.Av));
	settings.setValue(QString("Tv"), QVariant(CurrSettings.Tv));
	settings.setValue(QString("ISO"), QVariant(CurrSettings.ISO));
	settings.setValue(QString("WB"), QVariant(CurrSettings.Wb));
	settings.setValue(QString("WBTemp"), QVariant(CurrSettings.WbTemp));
	settings.setValue(QString("AFMode"), QVariant(CurrSettings.AFMode));
	settings.setValue(QString("BufferSize"), QVariant(CurrSettings.BufferSize));
	settings.setValue(QString("UseStabFPS"), QVariant(CurrSettings.UseStabFPS));
	settings.setValue(QString("ShowWhiteBox"), QVariant(CurrSettings.ShowWhiteBox));
}

void GEOSRecWnd::resizeEvent(QResizeEvent* event)
{
	setPathLabel();
	QWidget::resizeEvent(event);
}

EOSZoomVars GEOSRecWnd::getZoomVars()
{
	return LiveThread->cameraFeatures().zoomVars;
}

void GEOSRecWnd::populateOptionBox(QComboBox* box, valArr featureList, int selOpt)
{
	if (!isLive())
		return;
#ifdef GPHOTO2
	if (DEBUG) fprintf(stderr, "GEOSRecWnd::populateOptionBox   box [%s]  selOpt [%d]\n", box->toolTip().toLocal8Bit().constData(), selOpt );
	
	box->clear();
	for (int i = 0; i < featureList.length; i++)
	{
		box->addItem(featureList.opt_name[i], QVariant((int)i));
		if (DEBUG) fprintf(stderr, "    [%d] [%s]\n", i, featureList.opt_name[i].toLocal8Bit().constData() );
		
		if (i == selOpt)
		{
			box->setCurrentIndex(i);
		}
	}
#endif
}

void GEOSRecWnd::changeSelection(QComboBox* box, int* setting, int idx)
{
	// changeSelection(isoBox, &CurrSettings.ISO, e->value().toInt());
	*setting = idx;
	int val;
	for (int i = 0; i < box->count(); i++)
	{
		val = box->itemData(i, Qt::UserRole).toInt();
		if (val == idx)
		{
			box->setCurrentIndex(i);
			break;
		}
	}
}

void GEOSRecWnd::setBatteryLevelLabel(QLabel* batteryLevelLabel, QString batteryLevelStr)
{
	batteryLevelLabel->setText(batteryLevelStr);

	QString batteryLevel_tmp = batteryLevelStr.trimmed(); // Remove leading/trailing whitespace
	if (batteryLevel_tmp.endsWith("%")) {
		batteryLevel_tmp.chop(1); // Remove the last character (the '%')
	}

	bool ok;
	int batteryLevel = batteryLevel_tmp.toInt(&ok);
	if (!ok)
	{
		batteryLevelLabel->setStyleSheet("QLabel { color: red;}"); // red
	}
	else
	{
		if (batteryLevel < 30)
		{
			batteryLevelLabel->setStyleSheet("QLabel { color: red;}"); // red
		}
		else if (batteryLevel < 80)
		{
			batteryLevelLabel->setStyleSheet("QLabel { color: #a60;}"); // orange
		}
		else
		{
			batteryLevelLabel->setStyleSheet("QLabel { color: green;}"); // green
			//batteryLevelLabel->setStyleSheet("");  // default style
		}
	}
}

void GEOSRecWnd::customEvent(QEvent* event)
{
	if (!LiveThread /*|| !LiveThread->isInit()*/)
		return;
	struct EOSCamFeatures features = LiveThread->cameraFeatures();
	GCameraEvent* e = (GCameraEvent*)event;
	switch (e->type())	// events.h
	{
	case CAMERA_EVENT_NOCAMERA:
		shutdown();
		blinkLabel->setText(tr("Camera not found / not supported / not connected!"));
		blinkLabel->start();
		QMessageBox::critical(this, tr("Error"), tr("Can't initialize your camera!\n\nCheck camera connection.\n\nYou might need to reconnect the USB cable or turn your camera off then back on again."));
		break;
	case CAMERA_EVENT_LV_NOTSTARTED:
		shutdown();
		blinkLabel->setText(tr("Can't initialize LiveView mode!"));
		blinkLabel->start();
		QMessageBox::critical(this, tr("Error"), tr("Can't initialize LiveView mode!\n\nDoes your camera support LiveView mode?"));
		break;
	case CAMERA_EVENT_LV_STARTED:
		{
			QSizeF largeSize = QSizeF(features.JpegLargeSize_x, features.JpegLargeSize_y);
			QSizeF lvSize = QSizeF(features.LiveViewSize_x, features.LiveViewSize_y);
			if (!largeSize.isEmpty() && !lvSize.isEmpty())
				CaptureWnd->setZoomPositionDivisor(largeSize.width()/lvSize.width(), largeSize.height()/lvSize.height());
			
			//startBtn->setEnabled(true);
			cptBtn->setEnabled(true);
			AEModeBox->setEnabled(true);
			dofBtn->setEnabled(true);
			zoomPosClickBtn->setEnabled(getZoomVars().enabled);
			if (!getZoomVars().enabled)
				zoomPosClickBtn->setToolTip(tr("Select zoom position by clicking on preview (this feature not yet enabled for your camera)"));
			zoom5xBtn->setEnabled(true);
			zoom10xBtn->setEnabled(true);
			HistBtn->setEnabled(true);
			showBox->setEnabled(true);
			wbBox->setEnabled(true);
			// next line realy work in customEvents()
			//AFCamBtn->setEnabled(features.HasAF);
			blinkLabel->stop();
			QString str = LiveThread->cameraName() + QString(": ");
			blinkLabel->setText(str + tr("Ready"));
			timeTimerBox->setEnabled(true);
			framesTimerBox->setEnabled(true);
			slotTimeTimerSwitch(timeTimerBox->isChecked());
			slotFramesTimerSwitch(framesTimerBox->isChecked());
			// at this time we already received all settings from camera
			loadSettings();
			LiveThread->setUseStabFPS(CurrSettings.UseStabFPS);
			CaptureWnd->setShowWhiteBox(CurrSettings.ShowWhiteBox);
			//batteryLevelLabel->setText(LiveThread->getBatteryLevel());
			setBatteryLevelLabel(batteryLevelLabel, LiveThread->getBatteryLevel());
		}
		break;
	case CAMERA_EVENT_WRITE_STOPPED:
		{
			blinkLabel->stop();
			QString str = LiveThread->cameraName() + QString(": ");
			blinkLabel->setText(str + tr("Ready"));
			showBox->setEnabled(true);
			cptBtn->setEnabled(true);
			startBtn->setEnabled(true);
			startBtn->setVisible(true);
			stopBtn->setEnabled(false);
			stopBtn->setVisible(false);
			optionsBtn->setEnabled(true);
			timeTimerBox->setEnabled(true);
			framesTimerBox->setEnabled(true);
			slotTimeTimerSwitch(timeTimerBox->isChecked());
			slotFramesTimerSwitch(framesTimerBox->isChecked());
		}
		break;
	case CAMERA_EVENT_UPDATE_BATTERY:
		//batteryLevelLabel->setText(LiveThread->getBatteryLevel());
		setBatteryLevelLabel(batteryLevelLabel, LiveThread->getBatteryLevel());
		break;
	case CAMERA_EVENT_WB_CHANGED:
		{
			// wb changed
			changeSelection(wbBox, &CurrSettings.Wb, e->value().toInt());
			/*
			int wb_ind = e->value().toInt();
			CurrSettings.Wb = wb_ind;
			int val;
			for (int i = 0; i < wbBox->count(); i++)
			{
				val = wbBox->itemData(i, Qt::UserRole).toInt();
				if (val == wb_ind)
				{
					wbBox->setCurrentIndex(i);
					break;
				}
			}
			*/
		}
		break;
	case CAMERA_EVENT_WBLIST_CHANGED:
		{
#ifdef EDSDK
			fprintf(stderr, "GEOSRecWnd::customEvent     CAMERA_EVENT_WBLIST_CHANGED: not implementet yet! \n" );
#endif
#ifdef GPHOTO2
			populateOptionBox(wbBox,  LiveThread->cameraFeatures().whitebalance, CurrSettings.Wb);
#endif
		}
		break;
	case CAMERA_EVENT_ISO_CHANGED:
		{
			// ISO changed
			changeSelection(isoBox, &CurrSettings.ISO, e->value().toInt());
		}
		break;
	case CAMERA_EVENT_ISOLIST_CHANGED:
		{
			// ISO list changed
			unsigned int curr_iso = isoBox->itemData(isoBox->currentIndex(), Qt::UserRole).toInt();
#ifdef EDSDK
			const unsigned int* isoList = LiveThread->isoList();
			int isoListSize = LiveThread->isoListSize();
			// fill combo
			isoBox->clear();
			int i, j, ind = 0;
			for (i = 0; i < isoListSize; i++)
			{
				j = isoList[i];
				isoBox->addItem(QString(ISOTable[j].ISO), QVariant(j));
				if (isoList[i] == curr_iso)
					isoBox->setCurrentIndex(ind);
				ind++;
			}
#endif
#ifdef GPHOTO2
			populateOptionBox(isoBox,  LiveThread->cameraFeatures().iso, CurrSettings.ISO);
#endif
		}
		break;
	case CAMERA_EVENT_AV_CHANGED:
		{
			changeSelection(avBox, &CurrSettings.Av, e->value().toInt());
		}
		break;
	case CAMERA_EVENT_AVLIST_CHANGED:
		{
#ifdef EDSDK
			unsigned int curr_av = avBox->itemData(avBox->currentIndex(), Qt::UserRole).toInt();
			const unsigned int* avList = LiveThread->avList();
			int avListSize = LiveThread->avListSize();
			// fill combo
			avBox->clear();
			int i, j, ind = 0;
			for (i = 0; i < avListSize; i++)
			{
				j = avList[i];
				avBox->addItem(QString(AvTable[j].av), QVariant((int)j));
				if (avList[i] == curr_av)
					avBox->setCurrentIndex(ind);
				ind++;
			}
#endif
#ifdef GPHOTO2
			populateOptionBox(avBox,  LiveThread->cameraFeatures().aperture, CurrSettings.Av);
#endif
		}
		break;
	case CAMERA_EVENT_TV_CHANGED:
		{
			// Tv changed
			changeSelection(tvBox, &CurrSettings.Tv, e->value().toInt());
		}
		break;
	case CAMERA_EVENT_TVLIST_CHANGED:
		{
#ifdef EDSDK
			unsigned int curr_tv = tvBox->itemData(tvBox->currentIndex(), Qt::UserRole).toInt();
			const unsigned int* tvList = LiveThread->tvList();
			int tvListSize = LiveThread->tvListSize();
			// fill combo
			tvBox->clear();
			int i, j, ind = 0;
			for (i = 0; i < tvListSize; i++)
			{
				j = tvList[i];
				tvBox->addItem(QString(TvTable[j].tv), QVariant((int)j));
				if (tvList[i] == curr_tv)
					tvBox->setCurrentIndex(ind);
				ind++;
			}
#endif
#ifdef GPHOTO2
			populateOptionBox(tvBox,  LiveThread->cameraFeatures().shutterspeed, CurrSettings.Tv);
#endif
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
				LiveThread->cmdRequestWBList();
				LiveThread->cmdRequestAv();
				LiveThread->cmdRequestTv();
				LiveThread->cmdRequestISO();
				LiveThread->cmdRequestWB();
			}
		}
		break;
	case CAMERA_EVENT_AEMODELIST_CHANGED:
		{
#ifdef EDSDK
			unsigned int curr_aem = AEModeBox->itemData(AEModeBox->currentIndex(), Qt::UserRole).toInt();
			const unsigned int* aemList = LiveThread->aemList();
			int aemListSize = LiveThread->aemListSize();
			// fill combo
			AEModeBox->clear();
			int i, j, ind = 0;
			for (i = 0; i < aemListSize; i++)
			{
				j = aemList[i];
				AEModeBox->addItem(QString(AEMTable[j].aem), QVariant((int)j));
				if (aemList[i] == curr_aem)
					AEModeBox->setCurrentIndex(ind);
				ind++;
			}
#endif
#ifdef GPHOTO2
			populateOptionBox(AEModeBox,  LiveThread->cameraFeatures().autoexposuremode, CurrSettings.AEMode);
#endif
		}
		break;
	case CAMERA_EVENT_AFMODE_CHANGED:
		{
			int mode = e->value().toInt();
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
				if (features.HasAF)
					AFCamBtn->setEnabled(true);
				break;
			case 3:	// NOTE: if you change this 3 to something else, you must also change the 3 in COMMAND_REQ_AFMODE in livethread.cpp
				focusNear3Btn->setEnabled(false);
				focusNear2Btn->setEnabled(false);
				focusNear1Btn->setEnabled(false);
				focusFar1Btn->setEnabled(false);
				focusFar2Btn->setEnabled(false);
				focusFar3Btn->setEnabled(false);
				AFBtn->setEnabled(false);
				AFCamBtn->setEnabled(false);
				slotStopAutoFocus();
				break;
			default:
				break;
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
	case CAMERA_EVENT_UPDATE_COUNTERS:
		{
			QList<QVariant> counters = e->value().toList();
			if (counters.length() == 2)
			{
				framesLabel->setText(tr("frames: %1").arg(counters.at(0).toInt()));
				timeLabel->setText(tr("time: %1 s").arg(counters.at(1).toInt()));
			}
		}
		break;
	case CAMERA_EVENT_FPS_CALCULATED:
		cptBtn->setEnabled(true);
		startBtn->setEnabled(true);
		break;
	case CAMERA_EVENT_ZOOM_CHANGED_STOP:
		break;
	case CAMERA_EVENT_ZOOMPOS_NEEDCHANGE:
		if (LiveThread)
		{
			LiveThread->cmdSetZoomPos(e->value().toPoint());
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
	case CAMERA_EVENT_SHOWMSG:
		{
			QString msg = e->value().toString();
			QMessageBox::warning(this, tr("Info from Thread"), msg);
		}
		break;
	case CAMERA_EVENT_SHUTDOWN:
		blinkLabel->setText(tr("Lost connection with camera."));
		blinkLabel->start();
		shutdown();
		QMessageBox::critical(this, tr("Error"), tr("Lost connection with camera."));
		break;
	default:
		if (DEBUG) fprintf(stderr, " DEFAULT ");
		break;
	}
	e->accept();
}

void GEOSRecWnd::setPathLabel()
{
	path_label->setToolTip(commonFilePath);
	//path_label->setText("[" + commonFilePath + "]");
	
	QFontMetrics metrics(path_label->font());
	path_label->setText("[" + metrics.elidedText(commonFilePath, Qt::ElideMiddle, path_label->width() - 10) + "]");
	
	QString cmpIMGpath = QFileInfo(CurrSettings.ImgName).path().toLocal8Bit().constData();
	if (cmpIMGpath != commonFilePath)
	{
		if (DEBUG) fprintf(stderr, "GEOSRecWnd::derivePath: PATHS DO NOT MATCH: [%s] != [%s]\n",
				commonFilePath.toLocal8Bit().constData(),
				cmpIMGpath.toLocal8Bit().constData() );
		path_label->setToolTip("VID PATH: [" + commonFilePath + "]\nCPT PATH: [" + cmpIMGpath + "]");
		path_label->setText(" * [" + metrics.elidedText(commonFilePath, Qt::ElideMiddle, path_label->width() - 10) + "]");
	}
}

void GEOSRecWnd::derivePath()
{
	commonFilePath = QFileInfo(CurrSettings.VidName).path().toLocal8Bit().constData(); // FIXME for i18n, this should be a QSTRING
}

void GEOSRecWnd::slotSettings()
{
	GPathsDlg dlg(this);
	dlg.setFilePaths(CurrSettings.VidName, CurrSettings.ImgName);
	if (dlg.exec() == QDialog::Accepted) {
		CurrSettings.VidName = dlg.getVidName();
		CurrSettings.ImgName = dlg.getImgName();
		derivePath();
		saveSettingsFilepaths();
		setPathLabel();
		if (LiveThread && LiveThread->isInit()) {
			// NOTE: check if changing vid name during rec is a problem
		}
	}
}


void GEOSRecWnd::slotCPT()
{
	QString availImgName = getNextfName(CurrSettings.ImgName);
	
	if (LiveThread && LiveThread->isInit()) {
		QString availImgName = getNextfName(CurrSettings.ImgName);
		if (availImgName != "") {
			LiveThread->setImgFileName(availImgName.toLatin1().constData());
			LiveThread->cmdDoCPT();
			// path_label->setText(tr("saved image: [") + CurrSettings.ImgName +tr("]"));
			// path_label->setText("captured: [" + availImgName + "]");
			fprintf(stderr, "Captured image: [%s]\n", availImgName.toLatin1().constData());
			}
		else {
			fprintf(stderr, "could not set img filename!\n");
			}
		}
	else {	
		fprintf(stderr, "not LiveThread!\n");
		}
}

void GEOSRecWnd::slotStart()
{
	if (LiveThread && LiveThread->isInit())
	{
		static QTextCodec* tcodec = QTextCodec::codecForLocale();
fprintf(stderr, "GEOSRecWnd::slotStart: FIXME -- CHECK for no filename (error)\n");
#warning GEOSRecWnd::slotStart: FIXME -- CHECK for no filename (error)
		LiveThread->setVidFileName(tcodec->fromUnicode(getNextfName(CurrSettings.VidName)).data());				// FIXME FIXME -- CHECK for no filename (error)
		LiveThread->setBufferSize(CurrSettings.BufferSize);
		showBox->setEnabled(false);
		startBtn->setEnabled(false);
		startBtn->setVisible(false);
		stopBtn->setEnabled(true);
		stopBtn->setVisible(true);
		optionsBtn->setEnabled(false);
		if (timeTimerBox->isChecked())
			LiveThread->setTimeTimer(timeTimerSpinBox->value()*1000);
		else
			LiveThread->setTimeTimer(-1);
		if (framesTimerBox->isChecked())
			LiveThread->setFramesTimer(framesTimerSpinBox->value());
		else
			LiveThread->setFramesTimer(-1);
		/*char info[48];
		sprintf(info, "AEMODE %s; ISO %s; AV %s; TV %s",
			AEModeBox->currentText().toLocal8Bit().data(),
			isoBox->currentText().toLocal8Bit().data(),
			avBox->currentText().toLocal8Bit().data(),
			tvBox->currentText().toLocal8Bit().data());*/
		QString aemode = AEModeBox->currentText();
		QString info = QString("AE: %1").arg(aemode);
		if (aemode == "M" || aemode == "Manual" || aemode == "TV" || aemode == "Tv")
		{
			info += QString("; ISO: %1").arg(isoBox->currentText());
			info += QString("; Tv: %1").arg(tvBox->currentText());
		}
		if (aemode == "M" || aemode == "Manual" || aemode == "AV" || aemode == "Av")
		{
			info += QString("; Av: %1").arg(avBox->currentText());
		}
		info.replace(".", ",");
		LiveThread->setMovieInfo(info.toLatin1().data());
		// LiveThread->setMovieInfo(info.toLocal8Bit().data()); // <-- FIXME should probably be this instead
		timeTimerBox->setEnabled(false);
		timeTimerSpinBox->setEnabled(false);
		framesTimerBox->setEnabled(false);
		framesTimerSpinBox->setEnabled(false);
		if (DEBUG) fprintf(stderr, "GEOSRecWnd::slotStart  -- BEGIN RECORDING!\n");
		LiveThread->startWrite();
		blinkLabel->setText(tr("WRITING"));
		blinkLabel->start();
	}
}

void GEOSRecWnd::slotStop()
{
	if (LiveThread && LiveThread->isInit())
	{
		stopBtn->setEnabled(false);
		LiveThread->stopWrite();
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
	
		// FIXME: check if the last option will always be manual select
	if (wbBox->currentIndex() + 1 == wbBox->count())		//  Manually select Color Temperature
	{
		wbTempBox->setEnabled(true);
	}
	else
	{
		wbTempBox->setEnabled(false);
	}
	
	if (LiveThread && LiveThread->isInit() && ok)
	{
		LiveThread->cmdSetWB(wb, wbTempBox->value());
		CurrSettings.Wb = wb;
	}
}

void GEOSRecWnd::slotWbTempSelected(int wb_temp)
{
	if (LiveThread && LiveThread->isInit())
	{
			// FIXME: check if the last option will always be manual select
		LiveThread->cmdSetWB(wbBox->count() - 1, wb_temp);
		//LiveThread->cmdSetWB(9, wb_temp);
		CurrSettings.WbTemp = wb_temp;
	}
}

void GEOSRecWnd::doFocus(int nearFar, int amount)
{
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdAdjFocus(nearFar, amount);
	}
}

void GEOSRecWnd::slotFocusNear3()
{
	doFocus(0, 3);
}

void GEOSRecWnd::slotFocusNear2()
{
	doFocus(0, 2);
}

void GEOSRecWnd::slotFocusNear1()
{
	doFocus(0, 1);
}

void GEOSRecWnd::slotFocusFar1()
{
	doFocus(1, 1);
}

void GEOSRecWnd::slotFocusFar2()
{
	doFocus(1, 2);
}

void GEOSRecWnd::slotFocusFar3()
{
	doFocus(1, 3);
}

void GEOSRecWnd::slotZoomPosClick()
{
	zoomPosByClick = zoomPosClickBtn->isChecked();
}

void GEOSRecWnd::slotZoom5x()
{
	int zoom = zoom5xBtn->isChecked() ? 5 : 1;
	zoom10xBtn->setChecked(false);
	doZoom(zoom);
}

void GEOSRecWnd::slotZoom10x()
{
	int zoom = zoom10xBtn->isChecked() ? 10 : 1;
	zoom5xBtn->setChecked(false);
	doZoom(zoom);
}

void GEOSRecWnd::doZoom(int amount)
{
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdSetZoom(amount);
	}
	HistBtn->setEnabled(amount == 1);
}

void GEOSRecWnd::doZoomPos(QPoint qp)
{
	if (LiveThread && LiveThread->isInit())
	{
		// http://gphoto.10949.n7.nabble.com/Canon-EOS-1000D-Zoom-in-LiveView-Evf-td8940.html
		// https://indilib.org/forum/ccds-dslrs/3686-poor-fps-when-streaming/33911.html?start=12
		LiveThread->cmdSetZoomPos(qp);
	}
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

void GEOSRecWnd::slotCameraAF()
{
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdDoLVAF(CurrSettings.AFMode);
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

// disabled at the request of members of the forum (forum.ixbt.com)
/*void GEOSRecWnd::slotWorkTimeout()
{
	shutdown();
	blinkLabel->stop();
	blinkLabel->setText(tr("Make some tea"));
	QMessageBox::information(this, tr("Timeout"), tr("To save you device program will be stopped."));
}*/

void GEOSRecWnd::shutdown()
{
	GMyLiveThread* p = LiveThread;
	LiveThread = 0;
	p->setCaptureWnd(0);
	p->stop();
	p->wait();
	FILE* f = 0;
#ifdef __MACOS__
	FSRef desktopFolderRef;
	OSErr err = FSFindFolder(kUserDomain, kDocumentsFolderType, kDontCreateFolder, &desktopFolderRef);
	char path[PATH_MAX]	= "";
	err = FSRefMakePath(&desktopFolderRef, (UInt8*)path, PATH_MAX);
	sprintf(path, "%s/eos_movrec.debug_info.txt", path);
	f = fopen(path, "wt");
#else
	f = fopen("debug_info.txt", "wt");
#endif
	if (f)
	{
		fprintf(f, "all frames: " INT64_PRINTF_SPEC "\n", p->allFramesCount());
		fprintf(f, "all writen frames: " INT64_PRINTF_SPEC "\n", p->writenCount());
		fprintf(f, "skipped frames on painting: " INT64_PRINTF_SPEC "\n", p->skippedCount());
		fprintf(f, "elapsed time (sec): %d\n", p->elapsedTime()/1000);
		if (p->elapsedTime() > 0)
		{
			fprintf(f, "duplicated frames on writing: " INT64_PRINTF_SPEC "\n", p->duplicatedCount());
			fprintf(f, "stable FPS (calc at start): %.2f\n", p->stableFPS());
			fprintf(f, "refresh rate(fps): %.1f\n", 1000.0*(float)p->allFramesCount()/((float)p->elapsedTime()));
		}
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
	cptBtn->setEnabled(false);
	startBtn->setEnabled(false);
	stopBtn->setEnabled(false);
	startBtn->setVisible(true);
	stopBtn->setVisible(false);
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
	zoom10xBtn->setEnabled(false);
	zoomPosClickBtn->setEnabled(false);
	AFBtn->setEnabled(false);
	AFCamBtn->setEnabled(false);
	HistBtn->setEnabled(false);
	timeTimerBox->setEnabled(false);
	framesTimerBox->setEnabled(false);
	fpsLabel->setText(tr("0 fps"));
}

void GEOSRecWnd::slotShowImageChanged(int state)
{
	CaptureWnd->setShowLiveImage(state == Qt::Checked);
	CaptureWnd->update();
}

void GEOSRecWnd::slotOptions()
{
	GOptionsDlg dlg(this);
	dlg.setOptions(CurrSettings.BufferSize, CurrSettings.AFMode, CurrSettings.UseStabFPS, CurrSettings.ShowWhiteBox);
	if (dlg.exec() == QDialog::Accepted)
	{
		CurrSettings.BufferSize = dlg.bufferSize();
		CurrSettings.AFMode = dlg.afMode();
		CurrSettings.UseStabFPS = dlg.useStabFPS();
		CurrSettings.ShowWhiteBox = dlg.showWhiteBox();
		CaptureWnd->setShowWhiteBox(CurrSettings.ShowWhiteBox);
		if (LiveThread && LiveThread->isInit())
		{
			LiveThread->setBufferSize(CurrSettings.BufferSize);
			LiveThread->setUseStabFPS(CurrSettings.UseStabFPS);
		}
	}
}

void GEOSRecWnd::slotTimeTimerSwitch(bool c)
{
	timeTimerSpinBox->setEnabled(c);
}

void GEOSRecWnd::slotFramesTimerSwitch(bool c)
{
	framesTimerSpinBox->setEnabled(c);
}

void GEOSRecWnd::slotAbout()
{
	GAboutDlg dlg(this);
	dlg.exec();
}

QString GEOSRecWnd::getNextfName(const QString& fName)
{
	QString retname = fName;
 
	if (QFileInfo(retname).isRelative())
	{
		retname = QFileInfo(commonFilePath, fName).absoluteFilePath();
	}
 
	// QString path = QFileInfo(retname).absoluteFilePath();
	QString path = QFileInfo(retname).path();
	QString base = QFileInfo(retname).completeBaseName();
	QString ext  = QFileInfo(retname).suffix();
 
	QFileInfo fi(retname);
	int i=0;
	while (fi.exists()) {
		fi.setFile(retname);
		retname = QDir(path).filePath(QString("%1_%2.%3").arg(base).arg(i, 3, 10, QChar('0')).arg(ext));
		//retname = QString("%1/%2_%3.%4").arg(path).arg(base).arg(i,3,10,QChar('0')).arg(ext);
		fi.setFile(retname);
		if (i >= MAX_FILENAME_ATTEMPTS) {
			fprintf(stderr, "Could not create filename. Giving up after [%d] tries.\n", i);		// FIXME -- find a better strategy than this
			retname = "";
			break;
			}
		i++;
	}
	return retname;
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



void GEOSRecWnd::wheelEvent(QWheelEvent* event)
{
	int d =  (event->angleDelta().y()/120)*10;
		
	if (d < 0) { // scroll down
		if( event->modifiers() & Qt::ShiftModifier ) {
			slotFocusNear1();
			}
		else if( event->modifiers() & Qt::ControlModifier ) {
			slotFocusNear2();
			}
		else {
			slotFocusNear3();
			}
		}
	else { // scroll up
		if( event->modifiers() & Qt::ShiftModifier ) {
			slotFocusFar1();
			}
		else if( event->modifiers() & Qt::ControlModifier ) {
			slotFocusFar2();
			}
		else {
			slotFocusFar3();
			}
		}
};
