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

#include "LiveThread.h"
#include <QMutex>
#include <QWidget>
#include <QApplication>

#include "mjpegwrt.h"
#include "os_api.h"
#include "buffer.h"
#include "events.h"
#include "command.h"

static QMutex ImageMutex;

EdsError EDSCALLBACK handleObjectEvent(EdsObjectEvent event, EdsBaseRef object, EdsVoid * context);
EdsError EDSCALLBACK handlePropertyEvent(EdsPropertyEvent event, EdsPropertyID property, EdsUInt32 inParam, EdsVoid * context);
EdsError EDSCALLBACK handleStateEvent(EdsStateEvent event, EdsUInt32 parameter, EdsVoid * context);

// class GMyLiveThread
GMyLiveThread::GMyLiveThread(QWidget* owner)
 : QThread()
{
	Stoped = false;
	Owner = owner;
	CaptureWnd = 0;
	LiveViewStarted = false;
	camera = 0;
	isSDKLoaded = false;
	WriteMovie = false;
	max_frame_size = 0;
	Inited = false;
	WantHistogram = false;

	SkippedCount = 0;
	AllFramesCount = 0;
	WritenCount = 0;
	ElapsedTime = 0;
	FileName = strdup("out.avi");

	AvListSize = 0;
}

GMyLiveThread::~GMyLiveThread()
{
	free(FileName);
}

void GMyLiveThread::setFileName(const char* fname)
{
	if (FileName)
		free(FileName);
	FileName = strdup(fname);
}

void GMyLiveThread::setCaptureWnd(QWidget* wnd)
{
	CaptureWnd = wnd;
}

void GMyLiveThread::stop()
{
	WriteMovie = false;
	Stoped = true;
}

void GMyLiveThread::startWrite()
{
	WrtFlagMutex.lock();
	WriteMovie = true;
	WrtFlagMutex.unlock();
}

void GMyLiveThread::stopWrite()
{
	WrtFlagMutex.lock();
	WriteMovie = false;
	WrtFlagMutex.unlock();
}

void GMyLiveThread::cmdSetAEMode(int ae)
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_SET_AEMODE, ae, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdSetWB(int wb, int temp)
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_SET_WB, wb, temp);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdSetISO(int iso)
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_SET_ISO, iso, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdRequestISO()
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_REQ_ISO, 0, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdRequestISOList()
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_REQ_ISOLIST, 0, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdSetAv(int av, int dof)
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_SET_AV, av, dof);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdRequestAv()
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_REQ_AV, 0, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdRequestAvList()
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_REQ_AVLIST, 0, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdSetTv(int tv)
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_SET_TV, tv, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdRequestTv()
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_REQ_TV, 0, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdRequestTvList()
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_REQ_TVLIST, 0, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdRequestEvfOut()
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_REQ_EVF_OUT, 0, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdRequestAEMode()
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_REQ_AEMODE, 0, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdAdjFocus(int direction, int val)
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_ADJ_FOCUS, direction, val);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdRequestAFMode()
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_REQ_AFMODE, 0, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdSetZoom(int zoom)
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_SET_ZOOM, zoom, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

void GMyLiveThread::cmdSetZoomPos(int x, int y)
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_SET_ZOOMPOS, x, y);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

EdsError GMyLiveThread::processCommand()
{
	CommandMutex.lock();
	GCameraCommand cmd = CommandsQueue.takeFirst();
	CommandMutex.unlock();
	int param1 = cmd.param1();
	int param2 = cmd.param2();
	/*FILE* f = fopen("cmds.log", "at");
	if (f)
	{
		fprintf(f, "cmd = %d, p1 = %d, p2 = %d\n", cmd.command(), param1, param2);
		fclose(f);
	}*/
	EdsError err = EDS_ERR_OK;
	switch (cmd.command())
	{
	case COMMAND_SET_WB:		// set WB
		err = EdsSetPropertyData(camera, kEdsPropID_Evf_WhiteBalance, 0, sizeof(EdsInt32), &param1);
		if (err == EDS_ERR_OK)
		{
			if (param1 == 9)
				err = EdsSetPropertyData(camera, kEdsPropID_Evf_ColorTemperature, 0, sizeof(EdsInt32), &param2);
		}
		break;
	case COMMAND_SET_ISO:		// set ISO
		err = EdsSetPropertyData(camera, kEdsPropID_ISOSpeed, 0, sizeof(EdsUInt32), &param1);
		break;
	case COMMAND_REQ_ISO:
	{
		int iso = 0;
		err = EdsGetPropertyData(camera, kEdsPropID_ISOSpeed, 0, sizeof(EdsUInt32), &iso);
		if (err == EDS_ERR_OK)
			if (Owner)
			{
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_ISO_CHANGED, QVariant((int)iso)));
			}
	}
	break;
	case COMMAND_REQ_ISOLIST:	// request ISO list
		err = fillISOList();
		if (err == EDS_ERR_OK)
			if (Owner)
			{
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_ISOLIST_CHANGED, 0));
			}
			break;
	case COMMAND_SET_AV:		// set Av & DOF
		if (param1 > 0x1)
		{
			EdsUInt32 av;
			err = EdsGetPropertyData(camera, kEdsPropID_Av, 0, sizeof(EdsUInt32), &av);
			if (err == EDS_ERR_OK)
			{
				if (av != (EdsUInt32)param1)
					err = EdsSetPropertyData(camera, kEdsPropID_Av, 0, sizeof(EdsUInt32), &param1);
			}
		}
		if (err == EDS_ERR_OK)
			err = EdsSetPropertyData(camera, kEdsPropID_Evf_DepthOfFieldPreview, 0, sizeof(EdsUInt32), &param2);
		break;
	case COMMAND_REQ_AV:		// request Av
		{
			int av = 0;
			err = EdsGetPropertyData(camera, kEdsPropID_Av, 0, sizeof(EdsUInt32), &av);
			if (err == EDS_ERR_OK)
				if (Owner)
				{
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AV_CHANGED, QVariant((int)av)));
				}
		}
		break;
	case COMMAND_REQ_AVLIST:	// request Av list
		err = fillAvList();
		if (err == EDS_ERR_OK)
			if (Owner)
			{
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AVLIST_CHANGED, 0));
			}
		break;
	case COMMAND_SET_TV:		// set Tv
		if (param1 >= 0x10)
		{
			EdsUInt32 tv;
			err = EdsGetPropertyData(camera, kEdsPropID_Tv, 0, sizeof(EdsUInt32), &tv);
			if (err == EDS_ERR_OK)
			{
				if (tv != (EdsUInt32)param1)
					err = EdsSetPropertyData(camera, kEdsPropID_Tv, 0, sizeof(EdsUInt32), &param1);
			}
		}
		break;
	case COMMAND_REQ_TV:		// request Tv
		{
			int tv = 0;
			err = EdsGetPropertyData(camera, kEdsPropID_Tv, 0, sizeof(EdsUInt32), &tv);
			if (err == EDS_ERR_OK)
				if (Owner)
				{
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_TV_CHANGED, QVariant((int)tv)));
				}
		}
		break;
	case COMMAND_REQ_TVLIST:	// request Tv list
		err = fillTvList();
		if (err == EDS_ERR_OK)
			if (Owner)
			{
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_TVLIST_CHANGED, 0));
			}
		break;
	case COMMAND_REQ_EVF_OUT:	// request Evf output device
		{
			EdsUInt32 device;
			err = EdsGetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device);
			if (err == EDS_ERR_OK)
			{
				//printf("\t\tval = %d\n", (int)device);
				if (device & kEdsEvfOutputDevice_PC)
					LiveViewStarted = true;
			}
		}
		break;
	case COMMAND_SET_AEMODE:	// set AE mode
		err = EdsSetPropertyData(camera, kEdsPropID_AEMode, 0, sizeof(EdsUInt32), &param1);
		break;
	case COMMAND_REQ_AEMODE:	// request AE mode
		{
			EdsUInt32 mode;
			err = EdsGetPropertyData(camera, kEdsPropID_AEMode, 0, sizeof(mode), &mode);
			if (err == EDS_ERR_OK)
				if (Owner)
				{
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AEMODE_CHANGED, QVariant((int)mode)));
				}
		}
		break;
	case COMMAND_ADJ_FOCUS:	// adjust focus
		{
			EdsUInt32 inParam = 0;
			switch (param2)
			{
				case 1:
					inParam = param1 == 0 ? kEdsEvfDriveLens_Near1 : kEdsEvfDriveLens_Far1;
					break;
				case 2:
					inParam = param1 == 0 ? kEdsEvfDriveLens_Near2 : kEdsEvfDriveLens_Far2;
					break;
				case 3:
					inParam = param1 == 0 ? kEdsEvfDriveLens_Near3 : kEdsEvfDriveLens_Far3;
					break;
				default:
					break;
			}
			err = EdsSendCommand(camera, kEdsCameraCommand_DriveLensEvf, inParam);
		}
		break;
	case COMMAND_REQ_AFMODE:	// request AF mode
		{
			EdsUInt32 mode;
			err = EdsGetPropertyData(camera, kEdsPropID_AFMode, 0, sizeof(mode), &mode);
			if (err == EDS_ERR_OK)
				if (Owner)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AFMODE_CHANGED, QVariant((int)mode)));
		}
		break;
	case COMMAND_SET_ZOOM:
		err = EdsSetPropertyData(camera, kEdsPropID_Evf_Zoom, 0, sizeof(EdsUInt32), &param1);
		break;
	case COMMAND_SET_ZOOMPOS:
		{
			EdsPoint p;
			p.x = (EdsInt32)param1;
			p.y = (EdsInt32)param2;
			err = EdsSetPropertyData(camera, kEdsPropID_Evf_ZoomPosition, 0, sizeof(EdsPoint), &p);
		}
		break;
	default:
		break;
	}
	return err;
}

void GMyLiveThread::waitCommands()
{
	//CommandMutex.lock();
	CommandCond.wait(&CommandMutex);
	//CommandMutex.unlock();
}

void GMyLiveThread::run()
{
	// init
	EdsError err = initializeEds();
	if (err == EDS_ERR_OK)
	{
		err = startLiveView();
	}
	if (err != EDS_ERR_OK)
	{
		deInitializeEds();
		return;
	}
	// get Av list to main window
	cmdRequestAvList();
	// get Tv list to main window
	cmdRequestTvList();
	// get ISO list to main window
	cmdRequestISOList();
	// get Av value to main window
	cmdRequestAv();
	// get Tv value to main window
	cmdRequestTv();
	// get ISO value to main window
	cmdRequestISO();
	// get AF mode
	cmdRequestAFMode();
	// get AE mode
	cmdRequestAEMode();
	// get camera name & its resolution
	fillCameraName();
	//
	SkippedCount = 0;
	AllFramesCount = 0;
	WritenCount = 0;
	ElapsedTime = 0;
	int StartTime;
	int EndTime;
	int OldZoom = -1;
	int OldZoomPosX = -1;
	int OldZoomPosY = -1;
	//
	live_buffer::frame = 0;
	void* mjpeg = 0;
	bool PrevWriteMovie = false;
	//__int64_t freq;
	//__int64_t t1;
	//__int64_t t2;
	//int code_time;
	//int sleep_time;
	StartTime = WinGetTickCount();
	int StartWriteTime = -1;		// ms
	int StopWriteTime = -1;			// ms
	// for temp fps
	int TempTime1 = WinGetTickCount();
	int TempTime2 = TempTime1;
	int TempFrameCount = 0;
	double TempFPS;

	/*while (!CommandsQueue.isEmpty())
		processCommand();*/
	// for internal EDSDK message queue
	int SDKMsgCheckTime1 = WinGetTickCount();
	int SDKMsgCheckTime2 = SDKMsgCheckTime1;
	//WinQueryPerformanceFrequency(&freq);
	// Wait a camera
	int RealyStartT1 = WinGetTickCount();
	int RealyStartT2 = WinGetTickCount();
	while (!LiveViewStarted && RealyStartT2 - RealyStartT1 < 4000)
	{
		if (SDKMsgCheckTime2 - SDKMsgCheckTime1 > 100)
		{
			WinProcessMsg();
			SDKMsgCheckTime1 = SDKMsgCheckTime2;
		}
		while (!CommandsQueue.isEmpty())
			processCommand();
		WinSleep(100);
		RealyStartT2 = WinGetTickCount();
	}
	while (!CommandsQueue.isEmpty())
		processCommand();
	if (!LiveViewStarted)
	{
		deInitializeEds();
		return;
	}
	Inited = true;
	if (Owner)
		QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_LV_STARTED));
	while (!Stoped)
	{
/*static int c = 0;
FILE* f = fopen("d", "wt");
fprintf(f, "%d\n", c);
fclose(f);
c++;*/
		if (!CommandsQueue.isEmpty())
		{
			while (!CommandsQueue.isEmpty())
				processCommand();
			CommandCond.wakeAll();
		}

		// process internal EDSDK message queue
		if (SDKMsgCheckTime2 - SDKMsgCheckTime1 > 500)
		{
			WinProcessMsg();
			SDKMsgCheckTime1 = SDKMsgCheckTime2;
		}

		//WinQueryPerformanceCounter(&t1);
		// fetch image.
		if (downloadEvfData() == EDS_ERR_OK)
		{
			WrtFlagMutex.lock();
			AllFramesCount++;
			if (live_buffer::IsPainting)
				SkippedCount++;
			else if (CaptureWnd)
				QApplication::postEvent(CaptureWnd, new GCameraEvent(CAMERA_EVENT_EVF_TRANSMITED, 0));
			if (WantHistogram)
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_HISTOGRAM, 0));
			if (Zoom != OldZoom && WriteMovie)
			{
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_ZOOM_CHANGED_STOP, QVariant(Zoom)));
				WriteMovie = false;
			}
			if (Zoom != OldZoom || OldZoomPosX != ZoomPosX || OldZoomPosY != ZoomPosY)
				QApplication::postEvent(CaptureWnd, new GCameraEvent(CAMERA_EVENT_ZOOM_CHANGED, QVariant(QRect(Zoom, 0, ZoomPosX, ZoomPosY))));
			OldZoom = Zoom;
			OldZoomPosX = ZoomPosX;
			OldZoomPosY = ZoomPosY;
			// write to file if needed
			if (!PrevWriteMovie && WriteMovie)			// start record
			{
				StartWriteTime = WinGetTickCount();
				WritenCount = 0;
				if (mjpeg)
					mjpegCloseFile(mjpeg);
				mjpeg = mjpegCreateFile(FileName);
				max_frame_size = 0;
				mjpegSetup(mjpeg, 0, 0, 25.0, 10000);
				// here we read buffer - this is not a critical section
				mjpegWriteChunk(mjpeg, (unsigned char*)live_buffer::frame, live_buffer::frame_size);
				WritenCount++;
			}
			else if (PrevWriteMovie && !WriteMovie)	// stop recording
			{
				StopWriteTime = WinGetTickCount();
				double fps = ((double)WritenCount*1000.0)/(double)(StopWriteTime - StartWriteTime);
				if (fps > 25.0)
					fps = 25.0;
				mjpegSetup(mjpeg, live_buffer::frame_width, live_buffer::frame_height, fps, 10000);
				//mjpegSetup(mjpeg, frame_width, frame_height, 25.0, 10000);
				mjpegSetMaxChunkSize(mjpeg, max_frame_size);
				mjpegCloseFile(mjpeg);
				mjpeg = 0;
			}
			else if (PrevWriteMovie && WriteMovie)	// simple write next image
			{
				mjpegWriteChunk(mjpeg, (unsigned char*)live_buffer::frame, live_buffer::frame_size);
				WritenCount++;
			}
			PrevWriteMovie = WriteMovie;
			TempFrameCount++;
			WrtFlagMutex.unlock();
		}

		// calc temp fps
		if (TempTime2 - TempTime1 >= 2000)
		{
			TempFPS = ((double)TempFrameCount*1000.0)/(double)(TempTime2 - TempTime1);
			TempTime1 = TempTime2;
			TempFrameCount = 0;
			if (Owner)
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_FPS_UPDATED, QVariant(TempFPS)));
		}
		TempTime2 = WinGetTickCount();

		SDKMsgCheckTime2 = TempTime2;

		//WinQueryPerformanceCounter(&t2);
		//code_time = 1000*(t2 - t1)/freq;
		//sleep_time = 40 - code_time;
		// 1
		/*if (sleep_time > 0)
			WorkSleep(1000*sleep_time);*/
		// 2
		/*if (sleep_time > 0)		// !!!!!!!!!!
		{
			if (sleep_time > 13)
			{
				WinSleep(sleep_time - 6);
				WinQueryPerformanceCounter(&t2);
				code_time = 1000*(t2 - t1)/freq;
				sleep_time = 40 - code_time;
			}
			if (sleep_time > 0)
				WorkSleep(1000*sleep_time);
		}*/
	}
	// cleanup
	if (mjpeg)
	{
		WriteMovie = false;
		StopWriteTime = WinGetTickCount();
		double fps = ((double)WritenCount*1000.0)/(double)(StopWriteTime - StartWriteTime);
		if (fps > 25.0)
			fps = 25.0;
		mjpegSetup(mjpeg, live_buffer::frame_width, live_buffer::frame_height, fps, 10000);
		//mjpegSetup(mjpeg, frame_width, frame_height, 25.0, 10000);
		mjpegSetMaxChunkSize(mjpeg, max_frame_size);
		mjpegCloseFile(mjpeg);
		mjpeg = 0;
	}
	EndTime = WinGetTickCount();
	ElapsedTime = EndTime - StartTime;

	err = endLiveView();
	deInitializeEds();

	if (live_buffer::frame)
	{
		free(live_buffer::frame);
		live_buffer::frame = 0;
	}
}

EdsError GMyLiveThread::fillAvList()
{
	EdsPropertyDesc desc;
	EdsError err = EdsGetPropertyDesc(camera, kEdsPropID_Av, &desc);
	if (err == EDS_ERR_OK)
	{
		AvListSize = desc.numElements;
		for (int i = 0; i < AvListSize; i++)
			AvList[i] = desc.propDesc[i];
	}
	return err;
}

EdsError GMyLiveThread::fillTvList()
{
	EdsPropertyDesc desc;
	EdsError err = EdsGetPropertyDesc(camera, kEdsPropID_Tv, &desc);
	if (err == EDS_ERR_OK)
	{
		TvListSize = desc.numElements;
		for (int i = 0; i < TvListSize; i++)
			TvList[i] = desc.propDesc[i];
	}
	return err;
}

EdsError GMyLiveThread::fillISOList()
{
	EdsPropertyDesc desc;
	EdsError err = EdsGetPropertyDesc(camera, kEdsPropID_ISOSpeed, &desc);
	if (err == EDS_ERR_OK)
	{
		ISOListSize = desc.numElements;
		for (int i = 0; i < ISOListSize; i++)
			ISOList[i] = desc.propDesc[i];
	}
	return err;
}

EdsError GMyLiveThread::fillCameraName()
{
	CameraName.clear();
	CameraFotoLargeSize = QSize(0, 0);
	CameraLVSize = QSize(0, 0);
	EdsChar str[EDS_MAX_NAME];
	EdsError err = EdsGetPropertyData(camera, kEdsPropID_ProductName, 0, sizeof(EdsChar)*EDS_MAX_NAME, str);
	if (err == EDS_ERR_OK)
	{
		CameraName = QString((const char*)str);
		if (CameraName == "Canon EOS-1D Mark III")
		{
			CameraFotoLargeSize = QSize(3888, 2592);
#warning "Resolution on 1D Mark III is unknow!"
			CameraLVSize = QSize(1024, 680);
		}
		else if (CameraName == "Canon EOS-1Ds Mark III")
		{
			CameraFotoLargeSize = QSize(5616, 3744);
			CameraLVSize = QSize(1024, 680);
		}
		else if (CameraName == "Canon EOS 5D Mark II")
		{
			CameraFotoLargeSize = QSize(5616, 3744);
			CameraLVSize = QSize(1024, 680);
		}
		else if (CameraName == "Canon EOS 40D")
		{
			CameraFotoLargeSize = QSize(3888, 2592);
			CameraLVSize = QSize(1024, 680);
		}
		else if (CameraName == "Canon EOS 50D")
		{
			CameraFotoLargeSize = QSize(4752, 3168);
			CameraLVSize = QSize(1024, 680);
		}
		else if (CameraName == "Canon EOS 450D" || CameraName == "Canon EOS DIGITAL REBEL XSi" || CameraName == "Canon EOS Kiss X2")
		{
			CameraFotoLargeSize = QSize(4272, 2848);
			CameraLVSize = QSize(848, 560);
		}
		else if (CameraName == "Canon EOS 1000D" || CameraName == "Canon EOS DIGITAL REBEL XS" || CameraName == "Canon EOS Kiss F")
		{
			CameraFotoLargeSize = QSize(3888, 2592);
			CameraLVSize = QSize(768, 512);
		}
		else if (CameraName == "Canon EOS 500D" || CameraName == "Canon EOS DIGITAL REBEL T1i" || CameraName == "Canon EOS Kiss X3")
		{
			CameraFotoLargeSize = QSize(4752, 3168);
#warning "Resolution on 500D is unknow!"
			CameraLVSize = QSize(1024, 680);
		}
	}
	if (CameraName.isEmpty())
		CameraName = tr("Unknown camera");
	return err;
}

EdsError GMyLiveThread::initializeEds()
{
	EdsError err = EDS_ERR_OK;
	camera = 0;
	isSDKLoaded = false;
	// Initialize SDK
	err = EdsInitializeSDK();
	if (err == EDS_ERR_OK)
	{
		isSDKLoaded = true;
	}
	// Get first camera
	if (err == EDS_ERR_OK)
	{
		EdsCameraListRef cameraList = NULL;
		EdsUInt32 count = 0;
		// Get camera list
		err = EdsGetCameraList(&cameraList);
		// Get number of cameras
		if (err == EDS_ERR_OK)
		{
			err = EdsGetChildCount(cameraList, &count);
			if (count == 0)
			{
				err = EDS_ERR_DEVICE_NOT_FOUND;
			}
		}
		// Get first camera retrieved
		if (err == EDS_ERR_OK)
		{
			err = EdsGetChildAtIndex(cameraList, 0, &camera);
		}
		// Release camera list
		if (cameraList != NULL)
			EdsRelease(cameraList);
	}
	// Set event handler
	if (err == EDS_ERR_OK)
	{
		err = EdsSetObjectEventHandler(camera, kEdsObjectEvent_All, handleObjectEvent, this);
	}
	// Set event handler
	if (err == EDS_ERR_OK)
	{
		err = EdsSetPropertyEventHandler(camera, kEdsPropertyEvent_All, handlePropertyEvent, this);
	}
	// Set event handler
	if (err == EDS_ERR_OK)
	{
		err = EdsSetCameraStateEventHandler(camera, kEdsStateEvent_All, handleStateEvent, this);
	}
	// Open session with camera
	if (err == EDS_ERR_OK)
	{
		err = EdsOpenSession(camera);
	}
	return err;
}

EdsError GMyLiveThread::deInitializeEds()
{
	EdsError err = endLiveView();
	// Close session with camera
	// and release camera
	if (camera != NULL)
	{
		err = EdsCloseSession(camera);
		EdsRelease(camera);
	}
	// Terminate SDK
	if (isSDKLoaded)
		EdsTerminateSDK();
	return err;
}

EdsError GMyLiveThread::startLiveView()
{
	EdsError err = EDS_ERR_OK;
	// Get the output device for the live view image
	EdsUInt32 device;
	//err = EdsGetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device );
	// PC live view starts by setting the PC as the output device for the live view image.
	//if (err == EDS_ERR_OK)
	{
		device = kEdsEvfOutputDevice_PC;
		err = EdsSetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device);
	}
	// A property change event notification is issued from the camera if property settings are made successfully.
	// Start downloading of the live view image once the property change notification arrives.
	return err;
}

EdsError GMyLiveThread::downloadEvfData()
{
	EdsError err = EDS_ERR_OK;
	EdsStreamRef stream = NULL;
	EdsEvfImageRef evfImage = NULL;
	// Create memory stream.
	err = EdsCreateMemoryStream(0, &stream);
	// Create EvfImageRef.
	if (err == EDS_ERR_OK)
	{
		err = EdsCreateEvfImageRef(stream, &evfImage);
	}
	// Download live view image data.
	if (err == EDS_ERR_OK)
	{
		err = EdsDownloadEvfImage(camera, evfImage);
	}
	EdsVoid* ptr;
	EdsUInt32 stream_len;
	// Get the incidental data of the image.
	if (err == EDS_ERR_OK)
	{
		 // Get the zoom ratio
		EdsUInt32 zoom;
		EdsGetPropertyData(evfImage, kEdsPropID_Evf_Zoom, 0 , sizeof(zoom), &zoom);
		// Get the focus and zoom border position
		EdsPoint point;
		EdsGetPropertyData(evfImage, kEdsPropID_Evf_ZoomPosition, 0 , sizeof(point), &point);
		Zoom = (int)zoom;
		ZoomPosX = point.x;
		ZoomPosY = point.y;

		err = EdsGetPointer(stream, &ptr);
		if (err == EDS_ERR_OK)
			err = EdsGetLength(stream, &stream_len);
		if (stream_len == 0)
			err = EDS_ERR_OBJECT_NOTREADY;
	}
	if (err == EDS_ERR_OK)
	{
// start critical section!!!
		live_buffer::ImageMutex.lock();
		if (max_frame_size < (int)stream_len)
		{
			live_buffer::frame = (unsigned char*)realloc(live_buffer::frame, stream_len);
			max_frame_size = (int)stream_len;
		}
		memcpy(live_buffer::frame, ptr, stream_len);
		live_buffer::frame_size = (int)stream_len;
		live_buffer::ImageMutex.unlock();
// end of critical section!!!
		if (WantHistogram)
		{
// start critical section!!!
			HistogramMutex.lock();
			EdsGetPropertyData(evfImage, kEdsPropID_Evf_Histogram, 0, sizeof(Histogram), Histogram);
			HistogramMutex.unlock();
// end of critical section!!!
		}
	}

	// Release stream
	if (stream != NULL)
	{
		EdsRelease(stream);
		stream = NULL;
	}
	// Release evfImage
	if (evfImage != NULL)
	{
		EdsRelease(evfImage);
		evfImage = NULL;
	}
	return err;
}

EdsError GMyLiveThread::endLiveView()
{
	EdsError err = EDS_ERR_OK;
	// Get the output device for the live view image
	EdsUInt32 device;
	err = EdsGetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device);
	// PC live view ends if the PC is disconnected from the live view image output device.
	if (err == EDS_ERR_OK)
	{
		device &= ~kEdsEvfOutputDevice_PC;
		err = EdsSetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device);
	}
	if (err == EDS_ERR_OK)
		LiveViewStarted = false;
	return err;
}

void GMyLiveThread::objectEvent(EdsObjectEvent event, EdsBaseRef object)
{
	/*FILE* f = fopen("obj_event.log", "at");
	if (f)
	{
		fprintf(f, "event = %04X\n", (unsigned int)event);
		fclose(f);
	}*/

	// do something
	/*
	switch(event)
	{
	case kEdsObjectEvent_DirItemRequestTransfer:
		downloadImage(object);
		break;
	default:
		break;
	}
	*/
	// Object must be released
	if(object)
	{
		EdsRelease(object);
	}
}

void GMyLiveThread::propertyEvent(EdsPropertyEvent event, EdsPropertyID property, EdsUInt32 inParam)
{
	/*FILE* f = fopen("prop_event.log", "at");
	if (f)
	{
		fprintf(f, "event = %04X, prop = %04X, param = %04X\n", (unsigned int)event, (unsigned int)property, (unsigned int)inParam);
		fclose(f);
	}*/
	/*if (property == kEdsPropID_Av)
	{
		FILE* f = fopen("prop_event_av.log", "at");
		if (f)
		{
			fprintf(f, "event = %04X, prop = %04X, param = %04X\n", (unsigned int)event, (unsigned int)property, (unsigned int)inParam);
			fclose(f);
		}
	}*/

	if (event == kEdsPropertyEvent_PropertyChanged)
	{
		if (property == kEdsPropID_Evf_OutputDevice)
		{
			cmdRequestEvfOut();
		}
		else if (property == kEdsPropID_ISOSpeed)
		{
			cmdRequestISO();
		}
		else if (property == kEdsPropID_Av)
		{
			cmdRequestAv();
		}
		else if (property == kEdsPropID_Tv)
		{
			cmdRequestTv();
		}
		else if (property == kEdsPropID_AEMode)
		{
			cmdRequestAEMode();
		}
		else if (property == kEdsPropID_AFMode)
		{
			cmdRequestAFMode();
		}
	}
	else if (event == kEdsPropertyEvent_PropertyDescChanged)
	{
		if (property == kEdsPropID_Av)
		{
			cmdRequestAvList();
		}
		else if (property == kEdsPropID_Tv)
		{
			cmdRequestTvList();
		}
		else if (property == kEdsPropID_ISOSpeed)
		{
			cmdRequestISOList();
		}
	}
}

void GMyLiveThread::stateEvent(EdsStateEvent event, EdsUInt32 parameter)
{
	/*FILE* f = fopen("state_event.log", "at");
	if (f)
	{
		fprintf(f, "event = %04X, param = %04X\n", (unsigned int)event, (unsigned int)parameter);
		fclose(f);
	}*/

	if (event == kEdsStateEvent_Shutdown)
	{
		if (Owner)
			QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_SHUTDOWN, 0));
	}
}

EdsError EDSCALLBACK handleObjectEvent(EdsObjectEvent event, EdsBaseRef object, EdsVoid * context)
{
	GMyLiveThread* thread = (GMyLiveThread*)context;
	thread->objectEvent(event, object);
	return EDS_ERR_OK;
}

EdsError EDSCALLBACK handlePropertyEvent(EdsPropertyEvent event, EdsPropertyID property, EdsUInt32 inParam, EdsVoid *context)
{
	GMyLiveThread* thread = (GMyLiveThread*)context;
	thread->propertyEvent(event, property, inParam);
	return EDS_ERR_OK;
}

EdsError EDSCALLBACK handleStateEvent(EdsStateEvent event, EdsUInt32 parameter, EdsVoid * context)
{
	GMyLiveThread* thread = (GMyLiveThread*)context;
	thread->stateEvent(event, parameter);
	return EDS_ERR_OK;
}
