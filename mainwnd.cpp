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

//#include <QMessageBox>

#include "mainwnd.h"
#include "about.h"
#include "blinklabel.h"
#include "capturewnd.h"
#include "livethread.h"
#include "events.h"

#include <stdlib.h>

struct EOSAvTable
{
	unsigned int val;
	char av[4];
} AvTable[] = {
	0x00, "0",
	0x08, "1",
	0x0B, "1.1",
	0x0C, "1.2",
	0x0D, "1.2",	// (1/3)
	0x10, "1.4",
	0x13, "1.6",
	0x14, "1.8",
	0x15, "1.8",	// (1/3) 
	0x18, "2",
	0x1B, "2.2",
	0x1C, "2.5",
	0x1D, "2.5",	// (1/3) 
	0x20, "2.8",
	0x23, "3.2",
	0x24, "3.5",
	0x25, "3.5",	// (1/3) 
	0x28, "4",
	0x2B, "4.5",
	0x2C, "4.5",	// (1/3)
	0x2D, "5.0",
	0x30, "5.6",
	0x33, "6.3",
	0x34, "6.7",
	0x35, "7.1",
	0x38, "8",
	0x3B, "9",
	0x3C, "9.5",
	0x3D, "10",
	0x40, "11",
	0x43, "13",
	0x44, "13",
	0x45, "14",
	0x48, "16",
	0x4B, "18",
	0x4C, "19",
	0x4D, "20",
	0x50, "22",
	0x53, "25",
	0x54, "27",
	0x55, "29",
	0x58, "32",
	0x5B, "36",
	0x5C, "38",
	0x5D, "40",
	0x60, "45",
	0x63, "51",
	0x64, "54",
	0x65, "57",
	0x68, "64",
	0x6B, "72",
	0x6C, "76",
	0x6D, "80",
	0x70, "91",
	0xffffffff, "bad"
};

GEOSRecWnd::GEOSRecWnd()
 : QWidget(0)
{
	setWindowTitle(tr("EOS Camera Movie Record"));

	QVBoxLayout* main_layout = new QVBoxLayout(this);
	QHBoxLayout* btn_layout = new QHBoxLayout();

	selFileBtn = new QToolButton(this);
	selFileBtn->setText(tr("..."));
	btn_layout->addWidget(selFileBtn, 0);

	startBtn = new QPushButton(tr("Write!"), this);
	startBtn->setEnabled(false);
	btn_layout->addWidget(startBtn, 0);

	stopBtn = new QPushButton(tr("Stop"), this);
	stopBtn->setEnabled(false);
	btn_layout->addWidget(stopBtn, 0);

	dofBtn = new QToolButton(this);
	dofBtn->setText("DOF");
	dofBtn->setCheckable(true);
	btn_layout->addWidget(dofBtn, 0);

	avBox = new QComboBox(this);
	//avBox->setEnabled(false);
	avBox->setEditable(false);
	btn_layout->addWidget(avBox, 0);

	QLabel* wbLabel = new QLabel(tr("WB"), this);
	btn_layout->addWidget(wbLabel, 0);

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
	btn_layout->addWidget(wbBox, 0);

	wbTempBox = new QSpinBox(this);
	wbTempBox->setMinimum(2800);
	wbTempBox->setMaximum(10000);
	wbTempBox->setValue(5200);
	wbTempBox->setSingleStep(100);
	wbTempBox->setEnabled(false);
	wbTempBox->setSuffix(tr("K"));
	btn_layout->addWidget(wbTempBox, 0);

	fpsLabel = new QLabel(tr("0 fps"), this);
	fpsLabel->setFrameShape(QFrame::Box);
	btn_layout->addWidget(fpsLabel, 0);

	btn_layout->addStretch(10);

	showBox = new QCheckBox(tr("Show captured image"), this);
	btn_layout->addWidget(showBox, 0);
	showBox->setCheckState(Qt::Checked);

	QToolButton* aboutBtn = new QToolButton(this);
	aboutBtn->setText(tr("A"));
	btn_layout->addWidget(aboutBtn, 0);

	QHBoxLayout* focus_layout = new QHBoxLayout();
	QLabel* focusLabel = new QLabel(tr("Focus adjust"), this);
	focus_layout->addWidget(focusLabel, 0);
	focusFar1Btn = new QToolButton(this);
	focusFar1Btn->setText(tr("<<<"));
	focusFar1Btn->setEnabled(false);
	focus_layout->addWidget(focusFar1Btn, 0);
	focusFar2Btn = new QToolButton(this);
	focusFar2Btn->setText(tr("<<"));
	focusFar2Btn->setEnabled(false);
	focus_layout->addWidget(focusFar2Btn, 0);
	focusFar3Btn = new QToolButton(this);
	focusFar3Btn->setText(tr("<"));
	focusFar3Btn->setEnabled(false);
	focus_layout->addWidget(focusFar3Btn, 0);

	focusNear3Btn = new QToolButton(this);
	focusNear3Btn->setText(tr(">"));
	focusNear3Btn->setEnabled(false);
	focus_layout->addWidget(focusNear3Btn, 0);
	focusNear2Btn = new QToolButton(this);
	focusNear2Btn->setText(tr(">>"));
	focusNear2Btn->setEnabled(false);
	focus_layout->addWidget(focusNear2Btn, 0);
	focusNear1Btn = new QToolButton(this);
	focusNear1Btn->setText(tr(">>>"));
	focusNear1Btn->setEnabled(false);
	focus_layout->addWidget(focusNear1Btn, 0);

	focus_layout->addSpacing(10);

	zoom5xBtn = new QToolButton(this);
	zoom5xBtn->setText(tr("Zoom 5x"));
	zoom5xBtn->setEnabled(true);
	zoom5xBtn->setCheckable(true);
	focus_layout->addWidget(zoom5xBtn, 0);

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

	connect(selFileBtn, SIGNAL(clicked()), this, SLOT(slotSelFile()));
	connect(startBtn, SIGNAL(clicked()), this, SLOT(slotStart()));
	connect(stopBtn, SIGNAL(clicked()), this, SLOT(slotStop()));
	connect(dofBtn, SIGNAL(clicked()), this, SLOT(slotDofPressed()));
	connect(avBox, SIGNAL(activated(int)), this, SLOT(slotAvSelected(int)));
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

	Path = tr("out.avi");

	LiveThread = new GMyLiveThread(this);
	LiveThread->setCaptureWnd(CaptureWnd);
	LiveThread->start(QThread::HighestPriority);

	QTimer::singleShot(4000, this, SLOT(slotStartTimeout()));
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
}

/*void GEOSRecWnd::showEvent(QShowEvent* event)
{
	static bool appInit = true;
	if (appInit)
	{
		EdsError err = initializeEds();
		if (err == EDS_ERR_OK)
		{
			startLiveView();
			LiveThread = new QMyLiveThread(this);
			LiveThread->start();
		}
	}
	appInit = false;
}*/

void GEOSRecWnd::closeEvent(QCloseEvent* event)
{
	// stop thread;
	if (LiveThread)
	{
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
}

void GEOSRecWnd::customEvent(QEvent* event)
{
	if (!LiveThread || !LiveThread->isInit())
		return;
	GCameraEvent* e = (GCameraEvent*)event;
	switch (e->type())
	{
	case CAMERA_EVENT_AV_CHANGED:
		{
			// Av changed
			int av = e->value().toInt();
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
			int* avList = LiveThread->avList();
			int avListSize = LiveThread->avListSize();
			// fill combo
			avBox->clear();
			int i, j, ind = 0;
			for (i = 0; i < avListSize; i++)
			{
				for (j = 0; j < sizeof(AvTable)/sizeof(EOSAvTable); j++)
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
	case CAMERA_EVENT_FPS_UPDATED:
		{
			double fps = e->value().toDouble();
			char str[10];
			sprintf(str, "%.1f fps", e->value().toDouble());
			fpsLabel->setText(QString(str));
		}
		break;
	case CAMERA_EVENT_AEMODE_CHANGED:
		{
			int mode = e->value().toInt();
			//blinkLabel->setText(tr("AE Mode changed to %1").arg(mode));
			switch (mode)
			{
			case 0:
			case 1:
			case 4:
			case 5:
			case 6:
				dofBtn->setEnabled(false);
				dofBtn->setChecked(false);
				avBox->clear();
				avBox->setEnabled(false);
				break;
			case 2:
			case 3:
				dofBtn->setEnabled(true);
				avBox->setEnabled(true);
				break;
			default:
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
				break;
			case 3:
				focusNear3Btn->setEnabled(false);
				focusNear2Btn->setEnabled(false);
				focusNear1Btn->setEnabled(false);
				focusFar1Btn->setEnabled(false);
				focusFar2Btn->setEnabled(false);
				focusFar3Btn->setEnabled(false);
				break;
			default:
				break;
			}
		}
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
}

void GEOSRecWnd::slotSelFile()
{
	QString path = QFileDialog::getSaveFileName(this, tr("Save file"), tr("out.avi"),
		tr("Video (*.avi)"), 0, QFileDialog::DontConfirmOverwrite);
	if (!path.isNull() && !path.isEmpty())
	{
		Path = path;
	}
}

void GEOSRecWnd::slotStart()
{
	if (LiveThread && LiveThread->isInit())
	{
		QFileInfo fi(Path);
		while (fi.exists())
		{
			Path = giveNextName(Path);
			fi.setFile(Path);
		}
		//QMessageBox::information(this, tr("path"), Path);
		static QTextCodec* tcodec = QTextCodec::codecForLocale();
		LiveThread->setFileName(tcodec->fromUnicode(Path).data());
		selFileBtn->setEnabled(false);
		showBox->setEnabled(false);
		startBtn->setEnabled(false);
		stopBtn->setEnabled(true);
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
				startBtn->setEnabled(true);
				blinkLabel->stop();
				blinkLabel->setText(tr("Ready"));
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
}

void GEOSRecWnd::slotDofPressed()
{
	int av = 0;
	int dof = dofBtn->isChecked() ? 1 : 0;
	int curr_ind = avBox->currentIndex();
	if (curr_ind >= 0)
		av = avBox->itemData(curr_ind, Qt::UserRole).toInt();
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdSetAv(av, dof);
	}
}

void GEOSRecWnd::slotAvSelected(int av_ind)
{
	int dof = dofBtn->isChecked() ? 1 : 0;
	int av = avBox->itemData(av_ind, Qt::UserRole).toInt();
	//int dof = dofBtn->isChecked() ? 1 : 0;
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdSetAv(av, dof);
	}
}

void GEOSRecWnd::slotWbSelected(int wb_ind)
{
	int wb = wbBox->itemData(wb_ind, Qt::UserRole).toInt();
	if (wb == 9)		// Color Temperature
	{
		wbTempBox->setEnabled(true);
	}
	else
	{
		wbTempBox->setEnabled(false);
	}
	// set to camera
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdSetWB(wb, wbTempBox->value());
	}
}

void GEOSRecWnd::slotWbTempSelected(int wb_temp)
{
	// set to camera
	if (LiveThread && LiveThread->isInit())
	{
		LiveThread->cmdSetWB(9, wb_temp);
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
	showBox->setEnabled(false);
	startBtn->setEnabled(false);
	stopBtn->setEnabled(false);
	selFileBtn->setEnabled(false);
	dofBtn->setEnabled(false);
	avBox->setEnabled(false);
	wbBox->setEnabled(false);
	wbTempBox->setEnabled(false);
	fpsLabel->setText(tr("0 fps"));
}

void GEOSRecWnd::slotShowImageChanged(int state)
{
	CaptureWnd->setShowLiveImage(state == Qt::Checked);
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
