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

#include "livethread.h"
#include <QMutex>
#include <QWidget>
#include <QApplication>

#include <QBuffer>
#include <QImageReader>
#include <QImage>

#include "mjpegwrt.h"
#include "os_api.h"
#include "buffer.h"
#include "events.h"
#include "command.h"
#include "cam_tables.h"

#include <math.h>
#include <stdio.h>

static QMutex ImageMutex;

#ifdef EDSDK
EdsError EDSCALLBACK handleObjectEvent(EdsObjectEvent event, EdsBaseRef object, EdsVoid * context);
EdsError EDSCALLBACK handlePropertyEvent(EdsPropertyEvent event, EdsPropertyID property, EdsUInt32 inParam, EdsVoid * context);
EdsError EDSCALLBACK handleStateEvent(EdsStateEvent event, EdsUInt32 parameter, EdsVoid * context);
#endif

#ifdef GPHOTO2
/*static void ctx_error_func (GPContext *context, const char *format, va_list args, void *data);
static void ctx_status_func (GPContext *context, const char *format, va_list args, void *data);
static void gp2_errordumper(GPLogLevel level, const char *domain, const char *format, va_list args, void *data);*/
static int _gp_lookup_widget(CameraWidget*widget, const char *key, CameraWidget **child);
static int _gp_get_config_value_string(Camera *camera, const char *key, char **str, GPContext *context);
static int _gp_set_config_value_string(Camera *camera, const char *key, const char *val, GPContext *context);
#endif

// class GMyLiveThread
GMyLiveThread::GMyLiveThread(QWidget* owner)
 : QThread()
{
	Stoped = false;
	Owner = owner;
	CaptureWnd = 0;
	LiveViewStarted = false;
	camera = 0;
#ifdef GPHOTO2
	camera_context = 0;
#endif
	isSDKLoaded = false;
	WriteMovie = false;
	max_frame_size = 0;
	Inited = false;
	WantHistogram = false;

	SkippedCount = 0;
	AllFramesCount = 0;
	WritenCount = 0;
	DuplicatedCount = 0;
	ElapsedTime = 0;
	FileName = strdup("out.avi");
	BufferSize = 1024*1024;
	UseStabFPS = true;
	StableFPS = 0.0;

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

void GMyLiveThread::setBufferSize(int buffer_sz)
{
	BufferSize = buffer_sz;
	// setup for mjpeg file only on starting writing!
}

void GMyLiveThread::setUseStabFPS(bool s)
{
	UseStabFPS = s;
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

void GMyLiveThread::cmdRequestAEModeList()
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_REQ_AEMODELIST, 0, 0);
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

void GMyLiveThread::cmdDoLVAF(int mode)
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_DO_LVAF, mode, 0);
	CommandsQueue.append(cmd);
	CommandMutex.unlock();
}

bool GMyLiveThread::processCommand()
{
	bool res = false;
	GCameraCommand cmd = CommandsQueue.takeFirst();
	int param1 = cmd.param1();
	int param2 = cmd.param2();
	/*FILE* f = fopen("cmds.log", "at");
	if (f)
	{
		fprintf(f, "cmd = %d, p1 = %d, p2 = %d\n", cmd.command(), param1, param2);
		fclose(f);
	}*/
#ifdef EDSDK
	EdsError err = EDS_ERR_OK;
#endif
#ifdef GPHOTO2
	int err = 0;
#endif
	switch (cmd.command())
	{
	case COMMAND_SET_WB:		// set WB
#ifdef EDSDK
		err = EdsSetPropertyData(camera, kEdsPropID_Evf_WhiteBalance, 0, sizeof(EdsInt32), &param1);
		if (err == EDS_ERR_OK)
		{
			if (param1 == 9)
				err = EdsSetPropertyData(camera, kEdsPropID_Evf_ColorTemperature, 0, sizeof(EdsInt32), &param2);
		}
#endif
#ifdef GPHOTO2
#warning "COMMAND_SET_WB: Not implemented yet!"
#endif
		break;
	case COMMAND_SET_ISO:		// set ISO
		if (param1 >= 0 && param1 < EOS_ISO_TABLE_SZ)
		{
#ifdef EDSDK
			err = EdsSetPropertyData(camera, kEdsPropID_ISOSpeed, 0, sizeof(EdsUInt32), &ISOTable[param1].edsdk_val);
#endif
#ifdef GPHOTO2
			err = _gp_set_config_value_string(camera, "iso", ISOTable[param1].ISO, camera_context);
#endif
		}
		break;
	case COMMAND_REQ_ISO:
	{
		int iso_ind;
#ifdef EDSDK
		int iso = 0;
		err = EdsGetPropertyData(camera, kEdsPropID_ISOSpeed, 0, sizeof(EdsUInt32), &iso);
		if (err == EDS_ERR_OK)
		{
			iso_ind = findISO_edsdk(iso);
			if (Owner)
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_ISO_CHANGED, QVariant((int)iso_ind)));
		}
#endif
#ifdef GPHOTO2
			char* str_val = 0;
			err = _gp_get_config_value_string(camera, "iso", &str_val, camera_context);
			if (err >= GP_OK && str_val)
			{
				iso_ind = findISO_str(str_val);
				if (Owner)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_ISO_CHANGED, QVariant(iso_ind)));
			}
			if (str_val)
				free(str_val);
#endif
	}
	break;
	case COMMAND_REQ_ISOLIST:	// request ISO list
		if (fillISOList())
			if (Owner)
			{
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_ISOLIST_CHANGED, 0));
			}
			break;
	case COMMAND_SET_AV:		// set Av & DOF
		if (param1 > 0 && param1 < EOS_AV_TABLE_SZ)
		{
#ifdef EDSDK
			EdsUInt32 av;
			err = EdsGetPropertyData(camera, kEdsPropID_Av, 0, sizeof(EdsUInt32), &av);
			if (err == EDS_ERR_OK)
			{
				if (av != (EdsUInt32)AvTable[param1].edsdk_val)
					err = EdsSetPropertyData(camera, kEdsPropID_Av, 0, sizeof(EdsUInt32), &param1);
			}
#endif
#ifdef GPHOTO2
			err = _gp_set_config_value_string(camera, "aperture", AvTable[param1].av, camera_context);
#endif
		}
#ifdef EDSDK
		if (err == EDS_ERR_OK)
			err = EdsSetPropertyData(camera, kEdsPropID_Evf_DepthOfFieldPreview, 0, sizeof(EdsUInt32), &param2);
#endif
#ifdef GPHOTO2
		#warning "COMMAND_SET_AV: set dof not implemented yet!"
#endif
		break;
	case COMMAND_REQ_AV:		// request Av
		{
			int av_ind = 0;
#ifdef EDSDK
			int av = 0;
			err = EdsGetPropertyData(camera, kEdsPropID_Av, 0, sizeof(EdsUInt32), &av);
			if (err == EDS_ERR_OK)
			{
				av_ind = findAV_edsdk(av);
				if (Owner)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AV_CHANGED, QVariant(av_ind)));
			}
#endif
#ifdef GPHOTO2
			char* str_val = 0;
			err = _gp_get_config_value_string(camera, "aperture", &str_val, camera_context);
			if (err >= GP_OK && str_val)
			{
				av_ind = findAV_str(str_val);
				if (Owner)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AV_CHANGED, QVariant(av_ind)));
			}
			if (str_val)
				free(str_val);
#endif
		}
		break;
	case COMMAND_REQ_AVLIST:	// request Av list
		if (fillAvList())
			if (Owner)
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AVLIST_CHANGED, 0));
		break;
	case COMMAND_SET_TV:		// set Tv
		if (param1 >= 0 && param1 < EOS_TV_TABLE_SZ)
		{
#ifdef EDSDK
			EdsUInt32 tv;
			err = EdsGetPropertyData(camera, kEdsPropID_Tv, 0, sizeof(EdsUInt32), &tv);
			if (err == EDS_ERR_OK)
			{
				if (tv != (EdsUInt32)TvTable[param1].edsdk_val)
					err = EdsSetPropertyData(camera, kEdsPropID_Tv, 0, sizeof(EdsUInt32), &TvTable[param1].edsdk_val);
			}
#endif
#ifdef GPHOTO2
			err = _gp_set_config_value_string(camera, "shutterspeed", TvTable[param1].tv, camera_context);
#endif
		}
		break;
	case COMMAND_REQ_TV:		// request Tv
		{
			int tv_ind = 0;
#ifdef EDSDK
			int tv = 0;
			err = EdsGetPropertyData(camera, kEdsPropID_Tv, 0, sizeof(EdsUInt32), &tv);
			if (err == EDS_ERR_OK)
			{
				tv_ind = findTV_edsdk(tv);
				if (Owner)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_TV_CHANGED, QVariant((int)tv_ind)));
			}
#endif
#ifdef GPHOTO2
			char* str_val = 0;
			err = _gp_get_config_value_string(camera, "shutterspeed", &str_val, camera_context);
			if (err >= GP_OK && str_val)
			{
				tv_ind = findTV_str(str_val);
				if (Owner)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_TV_CHANGED, QVariant(tv_ind)));
			}
			if (str_val)
				free(str_val);
#endif
		}
		break;
	case COMMAND_REQ_TVLIST:	// request Tv list
		if (fillTvList())
			if (Owner)
			{
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_TVLIST_CHANGED, 0));
			}
		break;
	case COMMAND_REQ_EVF_OUT:	// request Evf output device
		{
#ifdef EDSDK
			EdsUInt32 device;
			err = EdsGetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device);
			if (err == EDS_ERR_OK)
			{
				//printf("\t\tval = %d\n", (int)device);
				if (device & kEdsEvfOutputDevice_PC)
					LiveViewStarted = true;
			}
#endif
#ifdef GPHOTO2
			#warning "COMMAND_REQ_EVF_OUT: not implementet yet!"
#endif
		}
		break;
	case COMMAND_SET_AEMODE:	// set AE mode
		if (param1 >= 0 && param1 < EOS_AEM_TABLE_SZ)
		{
#ifdef EDSDK
			err = EdsSetPropertyData(camera, kEdsPropID_AEMode, 0, sizeof(EdsUInt32), &param1);

			EdsUInt32 aem;
			err = EdsGetPropertyData(camera, kEdsPropID_AEMode, 0, sizeof(EdsUInt32), &aem);
			if (err == EDS_ERR_OK)
			{
				if (aem != (EdsUInt32)AEMTable[param1].edsdk_val)
					err = EdsSetPropertyData(camera, kEdsPropID_AEMode, 0, sizeof(EdsUInt32), &AEMTable[param1].edsdk_val);
			}
#endif
#ifdef GPHOTO2
			char str[3];
			err = _gp_set_config_value_string(camera, "autoexposuremode", AEMTable[param1].gphoto_str, camera_context);
#endif
		}
		break;
	case COMMAND_REQ_AEMODE:	// request AE mode
		{
			int aem_ind = 0;
#ifdef EDSDK
			EdsUInt32 mode;
			err = EdsGetPropertyData(camera, kEdsPropID_AEMode, 0, sizeof(mode), &mode);
			if (err == EDS_ERR_OK)
			{
				aem_ind = findAEM_edsdk(mode);
				if (Owner)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AEMODE_CHANGED, QVariant(aem_ind)));
			}
#endif
#ifdef GPHOTO2
			char* str_val = 0;
			err = _gp_get_config_value_string(camera, "autoexposuremode", &str_val, camera_context);
			if (err >= GP_OK && str_val)
			{
				aem_ind = findAEM_str(str_val);
				if (Owner)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AEMODE_CHANGED, QVariant(aem_ind)));
			}
			if (str_val)
				free(str_val);
#endif
		}
		break;
	case COMMAND_REQ_AEMODELIST:	// request AE mode list
		if (fillAEMList())
			if (Owner)
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AEMODELIST_CHANGED, 0));
		break;
	case COMMAND_ADJ_FOCUS:	// adjust focus
		{
#ifdef EDSDK
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
#endif
#ifdef GPHOTO2
			#warning "COMMAND_ADJ_FOCUS: not implemented yet!"
#endif
		}
		break;
	case COMMAND_REQ_AFMODE:	// request AF mode
		{
#ifdef EDSDK
			EdsUInt32 mode;
			err = EdsGetPropertyData(camera, kEdsPropID_AFMode, 0, sizeof(mode), &mode);
			if (err == EDS_ERR_OK)
				if (Owner)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AFMODE_CHANGED, QVariant((int)mode)));
#endif
#ifdef GPHOTO2
			#warning "COMMAND_REQ_AFMODE: not implemented yet!"
#endif
		}
		break;
	case COMMAND_SET_ZOOM:
#ifdef EDSDK
		err = EdsSetPropertyData(camera, kEdsPropID_Evf_Zoom, 0, sizeof(EdsUInt32), &param1);
#endif
#ifdef GPHOTO2
		{
			char str[10];
			sprintf(str, "%u", param1);
			err = _gp_set_config_value_string(camera, "eoszoom", str, camera_context);
		}
#endif
		break;
	case COMMAND_SET_ZOOMPOS:
		{
#ifdef EDSDK
			EdsPoint p;
			p.x = (EdsInt32)param1;
			p.y = (EdsInt32)param2;
			err = EdsSetPropertyData(camera, kEdsPropID_Evf_ZoomPosition, 0, sizeof(EdsPoint), &p);
#endif
#ifdef GPHOTO2
			char str[20];
			sprintf(str, "%u,%u", param1, param2);
			err = _gp_set_config_value_string(camera, "eoszoomposition", str, camera_context);
#endif
		}
		break;
	case COMMAND_DO_LVAF:
		{
#ifdef EDSDK
			EdsEvfAFMode mode = (EdsEvfAFMode)param1;
			err = EdsSendCommand(camera, kEdsCameraCommand_DoEvfAf, mode);
#endif
#ifdef GPHOTO2
			#warning "COMMAND_DO_LVAF: not implemented yet!"
#endif
		}
		break;
	default:
		break;
	}
#ifdef EDSDK
	res = err == EDS_ERR_OK;
#endif
#ifdef GPHOTO2
	res = err >= GP_OK;
#endif
	return res;
}

void GMyLiveThread::waitCommands()
{
	WaitMutex.lock();
	CommandCond.wait(&WaitMutex);
	WaitMutex.unlock();
}

void GMyLiveThread::run()
{
	// init
#ifdef EDSDK
	bool ok = initializeEds();
#endif
#ifdef GPHOTO2
	bool ok = initializeGPhoto2();
#endif
	if (!ok)
	{
		if (Owner)
			QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_NOCAMERA));
#ifdef EDSDK
		deInitializeEds();
#endif
#ifdef GPHOTO2
		deInitializeGPhoto2();
#endif
		return;
	}
	ok = startLiveView();
	if (!ok)
	{
#ifdef EDSDK
		deInitializeEds();
#endif
#ifdef GPHOTO2
		deInitializeGPhoto2();
#endif
		if (Owner)
			QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_LV_NOTSTARTED));
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
	cmdRequestAEModeList();
	cmdRequestAEMode();

	SkippedCount = 0;
	AllFramesCount = 0;
	WritenCount = 0;
	DuplicatedCount = 0;
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
	//int code_time;
	//int sleep_time;
	StartTime = OSGetTickCount();
	int StartWriteTime = -1;		// ms
	int StopWriteTime = -1;			// ms
	// for temp fps
	int TempFrameCount = 0;
	double TempFPS;
	int StableFPSCount = 0;
	long long int MustBeFrames = 0;	// need to control stable fps recording
	int CurrTime;

	// for internal EDSDK message queue
	int SDKMsgCheckTime1 = StartTime;
	int SDKMsgCheckTime2 = StartTime;
	//WinQueryPerformanceFrequency(&freq);
	// Wait a camera
	int RealyStartT1 = StartTime;
	int RealyStartT2 = StartTime;
	while (!LiveViewStarted && RealyStartT2 - RealyStartT1 < 4000)
	{
		if (SDKMsgCheckTime2 - SDKMsgCheckTime1 > 100)
		{
#ifdef EDSDK
			OSProcessMsg();
#endif
#if GPHOTO2
			gp2_camera_check_event();
#endif
			SDKMsgCheckTime1 = SDKMsgCheckTime2;
		}
		while (!CommandsQueue.isEmpty())
			processCommand();
		OSSleep(50);
		RealyStartT2 = OSGetTickCount();
		SDKMsgCheckTime2 = RealyStartT2;
	}
	/*while (!CommandsQueue.isEmpty())
	{
		processCommand();
		WinProcessMsg();
	}*/
	if (!LiveViewStarted)
	{
#ifdef EDSDK
		deInitializeEds();
#endif
#ifdef GPHOTO2
		deInitializeGPhoto2();
#endif
		if (Owner)
			QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_LV_NOTSTARTED));
		return;
	}
	// Wait for a first preview image
	RealyStartT1 = OSGetTickCount();
	RealyStartT2 = RealyStartT1;
	bool start_ok = false;
	while (!(start_ok = downloadEvfData()) && RealyStartT2 - RealyStartT1 < 10000)
	{
#ifdef EDSDK
		OSProcessMsg();
#endif
#if GPHOTO2
		gp2_camera_check_event();
#endif
		OSSleep(50);
		RealyStartT2 = OSGetTickCount();
	}
	if (!start_ok)
	{
#ifdef EDSDK
		deInitializeEds();
#endif
#ifdef GPHOTO2
		deInitializeGPhoto2();
#endif
		if (Owner)
			QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_LV_NOTSTARTED));
		return;
	}
	// get camera name & its resolution
	fillCameraName();
	Inited = true;
	if (Owner)
		QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_LV_STARTED));
	StartTime = OSGetTickCount();
	int TempTime1 = StartTime;
	int TempTime2 = StartTime;
	int TempTime3 = StartTime;
	SDKMsgCheckTime1 = StartTime;
	SDKMsgCheckTime2 = StartTime;
	// main job
	while (!Stoped)
	{
		CommandMutex.lock();
		if (!CommandsQueue.isEmpty())
		{
			while (!CommandsQueue.isEmpty())
				processCommand();
			CommandCond.wakeAll();
		}
		CommandMutex.unlock();

		// process internal EDSDK message queue
		if (SDKMsgCheckTime2 - SDKMsgCheckTime1 > 500)
		{
#ifdef EDSDK
			OSProcessMsg();
#endif
#if GPHOTO2
			gp2_camera_check_event();
#endif
			SDKMsgCheckTime1 = SDKMsgCheckTime2;
		}

		//WinQueryPerformanceCounter(&t1);
		// fetch image.
		if (downloadEvfData())
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
				StartWriteTime = OSGetTickCount();
				WritenCount = 0;
				if (mjpeg)
					mjpegCloseFile(mjpeg);
				mjpeg = mjpegCreateFile(FileName);
				max_frame_size = 0;
				mjpegSetup(mjpeg, 0, 0, 25.0, 10000);
				if (!mjpegSetCache(mjpeg, BufferSize))
				{
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_SHOWMSG, tr("Can't alloc buffer with size %1 MB").arg(BufferSize/(1024*1024))));
				}
				// here we read buffer - this is not a critical section
				mjpegWriteChunk(mjpeg, (unsigned char*)live_buffer::frame, live_buffer::frame_size);
				WritenCount++;
			}
			else if (PrevWriteMovie && !WriteMovie)	// stop recording
			{
				mjpegWriteChunk(mjpeg, (unsigned char*)live_buffer::frame, live_buffer::frame_size);
				WritenCount++;
				StopWriteTime = OSGetTickCount();
				double fps = ((double)WritenCount*1000.0)/(double)(StopWriteTime - StartWriteTime);
				if (fps > 60.0)
					fps = 60.0;
				mjpegSetup(mjpeg, live_buffer::frame_width, live_buffer::frame_height, fps, 10000);
				mjpegSetMaxChunkSize(mjpeg, max_frame_size);
				mjpegCloseFile(mjpeg);
				mjpeg = 0;
			}
			else if (PrevWriteMovie && WriteMovie)	// simple write next image
			{
				if (WritenCount > 10)
				{
					if (!UseStabFPS)
					{
						mjpegWriteChunk(mjpeg, (unsigned char*)live_buffer::frame, live_buffer::frame_size);
						WritenCount++;
					}
					else
					{
						CurrTime = OSGetTickCount();
						MustBeFrames = (int)round((double)(CurrTime - StartWriteTime)*StableFPS/1000.0);
						if (MustBeFrames < WritenCount + 1)			// too fast, we must skip frame
						{
							;										// do nothing...
						}
						else
						{
							mjpegWriteChunk(mjpeg, (unsigned char*)live_buffer::frame, live_buffer::frame_size);
							WritenCount++;
							while (MustBeFrames > WritenCount)		// too less, we must add dublicated frames
							{
								mjpegWriteChunk(mjpeg, (unsigned char*)live_buffer::frame, live_buffer::frame_size);
								WritenCount++;
								DuplicatedCount++;
							}
						}
					}
				}
				else
				{
					mjpegWriteChunk(mjpeg, (unsigned char*)live_buffer::frame, live_buffer::frame_size);
					WritenCount++;
				}
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
			StableFPSCount++;
			if (StableFPSCount == 2)
				StableFPS = TempFPS;
			else if (StableFPSCount > 2 && StableFPSCount < 5)
				StableFPS = ((double)(StableFPSCount - 2)*StableFPS + TempFPS)/(double)(StableFPSCount - 1);
			if (Owner)
			{
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_FPS_UPDATED, QVariant(TempFPS)));
				if (StableFPSCount == 4)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_FPS_CALCULATED, QVariant((int)StableFPS)));
			}
		}
		if (TempTime2 - TempTime3 >= 1000)
		{
			TempTime3 = TempTime2;
			QList<QVariant> counters;
			int wrt_time = 0;
			if (WriteMovie)
				wrt_time = (CurrTime - StartWriteTime)/1000;
			else //if (PrevWriteMovie)
				wrt_time = (StopWriteTime - StartWriteTime)/1000;
			counters.append(QVariant(WritenCount));
			counters.append(QVariant(wrt_time));
			QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_UPDATE_COUNTERS, QVariant(counters)));
		}
		TempTime2 = OSGetTickCount();
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
		StopWriteTime = OSGetTickCount();
		double fps = ((double)WritenCount*1000.0)/(double)(StopWriteTime - StartWriteTime);
		if (fps > 60.0)
			fps = 60.0;
		mjpegSetup(mjpeg, live_buffer::frame_width, live_buffer::frame_height, fps, 10000);
		//mjpegSetup(mjpeg, frame_width, frame_height, 25.0, 10000);
		mjpegSetMaxChunkSize(mjpeg, max_frame_size);
		mjpegCloseFile(mjpeg);
		mjpeg = 0;
	}
	EndTime = OSGetTickCount();
	ElapsedTime = EndTime - StartTime;

	endLiveView();
#ifdef EDSDK
	deInitializeEds();
#endif
#ifdef GPHOTO2
	deInitializeGPhoto2();
#endif

	if (live_buffer::frame)
	{
		free(live_buffer::frame);
		live_buffer::frame = 0;
	}
}

bool GMyLiveThread::fillAvList()
{
	int i;
#ifdef EDSDK
	EdsPropertyDesc desc;
	int ind = 0;
	int j;
	EdsError err = EdsGetPropertyDesc(camera, kEdsPropID_Av, &desc);
	if (err == EDS_ERR_OK)
	{
		//AvListSize = desc.numElements;
		for (i = 0; i < desc.numElements; i++)
		{
			j = findAV_edsdk(desc.propDesc[i]);
			if (j < EOS_AV_TABLE_SZ - 1)
			{
				AvList[ind] = j;
				ind++;
			}
		}
		AvListSize = ind;
	}
	return err == EDS_ERR_OK;
#endif
#ifdef GPHOTO2
	int ret = GP_OK;
	CameraWidget* widget = 0, *child = 0;
	const char* choice = 0;

	AvListSize = 0;
	ret = gp_camera_get_config(camera, &widget, camera_context);
	if (ret >= GP_OK)
		ret = _gp_lookup_widget(widget, "aperture", &child);
	if (ret >= GP_OK)
		ret = gp_widget_count_choices(child);
	if (ret >= GP_OK)
		AvListSize = ret;
	for (i = 0; i < AvListSize; i++)
	{
		AvList[i] = 0;
		ret = gp_widget_get_choice(child, i, &choice);
		if (ret >= GP_OK)
			AvList[i] = findAV_str(choice);
	}
	if (widget)
		gp_widget_free(widget);
	return AvListSize > 0;
#endif
}

bool GMyLiveThread::fillTvList()
{
	int i;
#ifdef EDSDK
	int ind = 0;
	int j;
	EdsPropertyDesc desc;
	EdsError err = EdsGetPropertyDesc(camera, kEdsPropID_Tv, &desc);
	if (err == EDS_ERR_OK)
	{
		//TvListSize = desc.numElements;
		for (i = 0; i < desc.numElements; i++)
		{
			j = findTV_edsdk(desc.propDesc[i]);
			if (j < EOS_TV_TABLE_SZ - 1)
			{
				TvList[ind] = j;
				ind++;
			}
		}
		TvListSize = ind;
	}
	return err == EDS_ERR_OK;
#endif
#ifdef GPHOTO2
	int ret = GP_OK;
	CameraWidget* widget = 0, *child = 0;
	const char* choice = 0;

	TvListSize = 0;
	ret = gp_camera_get_config(camera, &widget, camera_context);
	if (ret >= GP_OK)
		ret = _gp_lookup_widget(widget, "shutterspeed", &child);
	if (ret >= GP_OK)
		ret = gp_widget_count_choices(child);
	if (ret >= GP_OK)
		TvListSize = ret;
	for (i = 0; i < TvListSize; i++)
	{
		TvList[i] = 0;
		ret = gp_widget_get_choice(child, i, &choice);
		if (ret >= GP_OK)
			TvList[i] = findTV_str(choice);
	}
	if (widget)
		gp_widget_free(widget);
	return TvListSize > 0;
#endif
}

bool GMyLiveThread::fillISOList()
{
	int i;
#ifdef EDSDK
	EdsPropertyDesc desc;
	EdsError err = EdsGetPropertyDesc(camera, kEdsPropID_ISOSpeed, &desc);
	int ind = 0;
	if (err == EDS_ERR_OK)
	{
		for (i = 0; i < desc.numElements; i++)
			if (desc.propDesc[i] > 0)
			{
				ISOList[ind] = findISO_edsdk(desc.propDesc[i]);
				ind++;
			}
		ISOListSize = ind;
	}
	return err == EDS_ERR_OK;
#endif
#ifdef GPHOTO2
	int ret = GP_OK;
	CameraWidget* widget = 0, *child = 0;
	const char* choice = 0;

	ISOListSize = 0;
	ret = gp_camera_get_config(camera, &widget, camera_context);
	if (ret >= GP_OK)
		ret = _gp_lookup_widget(widget, "iso", &child);
	if (ret >= GP_OK)
		ret = gp_widget_count_choices(child);
	if (ret >= GP_OK)
		ISOListSize = ret;
	for (i = 0; i < ISOListSize; i++)
	{
		ISOList[i] = 0;
		ret = gp_widget_get_choice(child, i, &choice);
		if (ret >= GP_OK)
			ISOList[i] = findISO_str(choice);
	}
	if (widget)
		gp_widget_free(widget);
	return ISOListSize > 0;
#endif
}

bool GMyLiveThread::fillAEMList()
{
	int ind, i, j;
#ifdef EDSDK
#if 0
	// this code not work. Why? I don't know, but desc.numElements == 0
	EdsPropertyDesc desc;
	EdsError err = EdsGetPropertyDesc(camera, kEdsPropID_AEMode, &desc);
	if (err == EDS_ERR_OK)
	{
		ind = 0;
		for (i = 0; i < desc.numElements; i++)
		{
			j = findAEM_edsdk(desc.propDesc[i]);
			if (j < EOS_AEM_TABLE_SZ - 1)
			{
				AEMList[ind] = j;
				ind++;
			}
		}
		AEMListSize = ind;
	}
	return err == EDS_ERR_OK;
#else
	AEMListSize = EOS_AEM_TABLE_SZ - 1;
	for (i = 0; i < EOS_AEM_TABLE_SZ; i++)
		AEMList[i] = i;
	return true;
#endif
#endif
#ifdef GPHOTO2
	int ret = GP_OK;
	CameraWidget* widget = 0, *child = 0;
	const char* choice = 0;
	int size;

	AEMListSize = 0;
	ret = gp_camera_get_config(camera, &widget, camera_context);
	if (ret >= GP_OK)
		ret = _gp_lookup_widget(widget, "autoexposuremode", &child);
	if (ret >= GP_OK)
		ret = gp_widget_count_choices(child);
	if (ret >= GP_OK)
		size = ret;
	ind = 0;
	for (i = 0; i < size; i++)
	{
		AEMList[ind] = 0;
		ret = gp_widget_get_choice(child, i, &choice);
		if (ret >= GP_OK)
		{
			j = findAEM_str(choice);
			if (j < EOS_AEM_TABLE_SZ - 1)
			{
				AEMList[ind] = j;
				ind++;
			}
		}
	}
	AEMListSize = ind;
	if (widget)
		gp_widget_free(widget);
	return AEMListSize > 0;
#endif
}

// call only first successfull downloadEvfData()!
bool GMyLiveThread::fillCameraName()
{
#ifdef EDSDK
	CameraName.clear();
	//CameraFotoLargeSize = QSize(0, 0);
	//CameraLVSize = QSize(0, 0);
	CamFeatures.JpegLargeSize_x = 0;
	CamFeatures.JpegLargeSize_y = 0;
	CamFeatures.LiveViewSize_x = 0;
	CamFeatures.LiveViewSize_y = 0;
	CamFeatures.HasAF = false;
	EdsChar str[EDS_MAX_NAME];
	EdsError err = EdsGetPropertyData(camera, kEdsPropID_ProductName, 0, sizeof(EdsChar)*EDS_MAX_NAME, str);
	if (err == EDS_ERR_OK)
	{
		CameraName = QString((const char*)str);
		if (CameraName == "Canon EOS-1D Mark III")
		{
			//CamFeatures.JpegLargeSize_x = 3888;
			//CamFeatures.JpegLargeSize_y = 2592;
			//CamFeatures.LiveViewSize_x = 1024;
			//CamFeatures.LiveViewSize_y = 680;
			CamFeatures.HasAF = false;
//#warning "LiveView resolution on EOS 1D Mark III is unknow!"
		}
		else if (CameraName == "Canon EOS-1D Mark IV")
		{
			//CamFeatures.JpegLargeSize_x = 4896;
			//CamFeatures.JpegLargeSize_y = 3264;
			//CamFeatures.LiveViewSize_x = 1024;
			//CamFeatures.LiveViewSize_y = 680;
			CamFeatures.HasAF = true;
//#warning "LiveView resolution on EOS 1D Mark IV is unknow!"
		}
		else if (CameraName == "Canon EOS-1Ds Mark III")
		{
			//CamFeatures.JpegLargeSize_x = 5616;
			//CamFeatures.JpegLargeSize_y = 3744;
			//CamFeatures.LiveViewSize_x = 1024;
			//CamFeatures.LiveViewSize_y = 680;
			CamFeatures.HasAF = false;
		}
		else if (CameraName == "Canon EOS 5D Mark II")
		{
			//CamFeatures.JpegLargeSize_x = 5616;
			//CamFeatures.JpegLargeSize_y = 3744;
			//CamFeatures.LiveViewSize_x = 1024;
			//CamFeatures.LiveViewSize_y = 680;
			CamFeatures.HasAF = true;
		}
		else if (CameraName == "Canon EOS 7D")
		{
			//CamFeatures.JpegLargeSize_x = 5184;
			//CamFeatures.JpegLargeSize_y = 3456;
			//CamFeatures.LiveViewSize_x = 1024;
			//CamFeatures.LiveViewSize_y = 680;
			CamFeatures.HasAF = true;
//#warning "LiveView resolution on EOS 7D is unknow!"
		}
		else if (CameraName == "Canon EOS 40D")
		{
			//CamFeatures.JpegLargeSize_x = 3888;
			//CamFeatures.JpegLargeSize_y = 2592;
			//CamFeatures.LiveViewSize_x = 1024;
			//CamFeatures.LiveViewSize_y = 680;
			CamFeatures.HasAF = false;
		}
		else if (CameraName == "Canon EOS 50D")
		{
			//CamFeatures.JpegLargeSize_x = 4752;
			//CamFeatures.JpegLargeSize_y = 3168;
			//CamFeatures.LiveViewSize_x = 1024;
			//CamFeatures.LiveViewSize_y = 680;
			CamFeatures.HasAF = true;
		}
		else if (CameraName == "Canon EOS 450D" || CameraName == "Canon EOS DIGITAL REBEL XSi" || CameraName == "Canon EOS Kiss X2")
		{
			//CamFeatures.JpegLargeSize_x = 4272;
			//CamFeatures.JpegLargeSize_y = 2848;
			//CamFeatures.LiveViewSize_x = 848;
			//CamFeatures.LiveViewSize_y = 560;
			CamFeatures.HasAF = false;
		}
		else if (CameraName == "Canon EOS 1000D" || CameraName == "Canon EOS DIGITAL REBEL XS" || CameraName == "Canon EOS Kiss F")
		{
			//CamFeatures.JpegLargeSize_x = 3888;
			//CamFeatures.JpegLargeSize_y = 2592;
			//CamFeatures.LiveViewSize_x = 768;
			//CamFeatures.LiveViewSize_y = 512;
			CamFeatures.HasAF = false;
		}
		else if (CameraName == "Canon EOS 500D" || CameraName == "Canon EOS DIGITAL REBEL T1i" || CameraName == "Canon EOS Kiss X3")
		{
			//CamFeatures.JpegLargeSize_x = 4752;
			//CamFeatures.JpegLargeSize_y = 3168;
			//CamFeatures.LiveViewSize_x = 928;
			//CamFeatures.LiveViewSize_y = 616;
			CamFeatures.HasAF = true;
		}
		else if (CameraName == "Canon EOS 550D" || CameraName == "Canon REBEL T2i" || CameraName == "Canon EOS Kiss X4")
		{
			//CamFeatures.LiveViewSize_x = 928;
			//CamFeatures.LiveViewSize_y = 616;
			CamFeatures.HasAF = true;
		}
	}



	EdsStreamRef stream = NULL;
	EdsEvfImageRef evfImage = NULL;
	EdsVoid* ptr;
	EdsUInt32 stream_len = 0;

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
	err = EdsGetPointer(stream, &ptr);
	if (err == EDS_ERR_OK)
		err = EdsGetLength(stream, &stream_len);
	if (stream_len == 0)
		err = EDS_ERR_OBJECT_NOTREADY;

	if (err == EDS_ERR_OK)
	{
		QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(ptr), stream_len);
		QBuffer b;
		QImage img;
		b.setData(a);
		b.open(QIODevice::ReadOnly);
		static QImageReader ir;
		ir.setDevice(&b);
		ir.setFormat("jpeg");
		if (ir.read(&img))
		{
			if (!img.isNull())
			{
				CamFeatures.LiveViewSize_x = img.width();
				CamFeatures.LiveViewSize_y = img.height();
			}
		}
	}

	// Get properties...
	if (err == EDS_ERR_OK)
	{
		EdsSize sz;
		err = EdsGetPropertyData(evfImage, kEdsPropID_Evf_CoordinateSystem, 0 , sizeof(sz), &sz);
		if (err == EDS_ERR_OK)
		{
			CamFeatures.JpegLargeSize_x = sz.width;
			CamFeatures.JpegLargeSize_y = sz.height;
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






	if (CameraName.isEmpty())
		CameraName = tr("Unknown camera");
	return err == EDS_ERR_OK;
#endif
#ifdef GPHOTO2
	return false;
#endif
}

#ifdef EDSDK
bool GMyLiveThread::initializeEds()
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
	return err == EDS_ERR_OK;
}

bool GMyLiveThread::deInitializeEds()
{
	EdsError err = EDS_ERR_OK;
	endLiveView();
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
	return err == EDS_ERR_OK;
}
#endif

#ifdef GPHOTO2
bool GMyLiveThread::initializeGPhoto2()
{
	camera_context = gp_context_new();
	if (!camera_context)
		return false;

	//gp_context_set_error_func(camera_context, ctx_error_func, NULL);
	//gp_context_set_status_func(camera_context, ctx_status_func, NULL);

	//gp_log_add_func(GP_LOG_ERROR, gp2_errordumper, NULL);



	int ret;
	int i;

	GPPortInfoList *portinfolist = NULL;
	CameraAbilitiesList *abilities = NULL;
	CameraAbilitiesList *want_abilities = NULL;
	CameraList* camera_list = NULL;
	CameraList* xlist = NULL;
	char camera_model[128];
	char camera_port[128];
	int ac;
	int cc;
	CameraAbilities	a;
	GPPortInfo	pi;
	Camera* tmp_camera = 0;

	camera_model[0] = 0;
	camera_port[0] = 0;
	/* Load all the port drivers we have... */
	ret = gp_port_info_list_new(&portinfolist);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_port_info_list_new() failed.\n");
		return false;
	}
	ret = gp_port_info_list_load(portinfolist);
	if (ret < 0)
	{
		fprintf(stderr, "gp_port_info_list_load() failed.\n");
		gp_port_info_list_free(portinfolist);
		return false;
	}

	/* Load all the camera drivers we have... */
	ret = gp_abilities_list_new(&abilities);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_abilities_list_new() failed.\n");
		gp_port_info_list_free(portinfolist);
		return false;
	}
	ret = gp_abilities_list_load(abilities, camera_context);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_abilities_list_load() failed.\n");
		gp_abilities_list_free(abilities);
		gp_port_info_list_free(portinfolist);
		return false;
	}

	ret = gp_abilities_list_count(abilities);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_abilities_list_count() failed.\n");
		gp_abilities_list_free(abilities);
		gp_port_info_list_free(portinfolist);
		return false;
	}
	ac = ret;

	ret = gp_abilities_list_new(&want_abilities);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_abilities_list_new() failed.\n");
		gp_abilities_list_free(abilities);
		gp_port_info_list_free(portinfolist);
		return false;
	}

	for (i = 0; i < ac; i++)
	{
		ret = gp_abilities_list_get_abilities(abilities, i, &a);
		if (ret < GP_OK)
		{
			fprintf(stderr, "gp_abilities_list_get_abilities() failed.\n");
		}
		else
		{
			if ((a.operations & GP_OPERATION_CAPTURE_PREVIEW) == GP_OPERATION_CAPTURE_PREVIEW)
				gp_abilities_list_append(want_abilities, a);
		}
	}
	gp_abilities_list_free(abilities);

	ret = gp_list_new(&xlist);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_list_new() failed.\n");
		gp_abilities_list_free(want_abilities);
		gp_port_info_list_free(portinfolist);
		return false;
	}

	/* ... and autodetect the currently attached cameras. */
	ret = gp_abilities_list_detect(want_abilities, portinfolist, xlist, camera_context);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_abilities_list_detect() failed.\n");
		gp_abilities_list_free(want_abilities);
		gp_port_info_list_free(portinfolist);
		return false;
	}

	ret = gp_list_count(xlist);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_list_count() failed.\n");
		gp_abilities_list_free(want_abilities);
		gp_port_info_list_free(portinfolist);
		gp_list_free(xlist);
		return false;
	}
	cc = ret;
	ret = gp_list_new(&camera_list);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_list_new() failed.\n");
		gp_abilities_list_free(want_abilities);
		gp_port_info_list_free(portinfolist);
		gp_list_free(xlist);
		return false;
	}

	for (i = 0; i < cc; i++)
	{
		const char *name, *value;

		gp_list_get_name(xlist, i, &name);
		gp_list_get_value(xlist, i, &value);
		if (!strcmp("usb:",value))
			continue;
		gp_list_append(camera_list, name, value);
	}
	gp_list_free(xlist);

	ret = gp_list_count(camera_list);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_list_count() failed.\n");
		gp_abilities_list_free(want_abilities);
		gp_port_info_list_free(portinfolist);
		gp_list_free(camera_list);
		return false;
	}
	cc = ret;
	if (cc == 0)
	{
		fprintf(stderr, "No cameras found!\n");
		gp_abilities_list_free(want_abilities);
		gp_port_info_list_free(portinfolist);
		gp_list_free(camera_list);
		return false;
	}
	for (i = 0; i < cc; i++)
	{
		const char *name, *value;

		gp_list_get_name(camera_list, i, &name);
		gp_list_get_value(camera_list, i, &value);
		strncpy(camera_model, name, 127);
		camera_model[127] = 0;
		strncpy(camera_port, value, 127);
		camera_port[127] = 0;
	}
	gp_list_free(camera_list);

	printf("camera_model = '%s'\n", camera_model);
	printf("camera_port = '%s'\n", camera_port);

	gp_camera_new(&tmp_camera);

	ret = gp_abilities_list_lookup_model(want_abilities, camera_model);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_abilities_list_lookup_model failed.\n");
		gp_camera_free(tmp_camera);
		gp_abilities_list_free(want_abilities);
		gp_port_info_list_free(portinfolist);
		return false;
	}
	else
	{
		i = ret;
		ret = gp_abilities_list_get_abilities(want_abilities, i, &a);
		if (ret < GP_OK)
		{
			fprintf(stderr, "gp_abilities_list_get_abilities failed.\n");
			gp_camera_free(tmp_camera);
			gp_abilities_list_free(want_abilities);
			gp_port_info_list_free(portinfolist);
			return false;
		}
		else
		{
			ret = gp_camera_set_abilities(tmp_camera, a);
			if (ret < GP_OK)
			{
				fprintf(stderr, "gp_camera_set_abilities failed.\n");
				gp_camera_free(tmp_camera);
				gp_abilities_list_free(want_abilities);
				gp_port_info_list_free(portinfolist);
				return false;
			}
		}
	}
	gp_abilities_list_free(want_abilities);

	/* Then associate the camera with the specified port */
	ret = gp_port_info_list_lookup_path(portinfolist, camera_port);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_port_info_list_lookup_path failed.\n");
		gp_camera_free(tmp_camera);
		gp_port_info_list_free(portinfolist);
		return false;
	}
	i = ret;
	ret = gp_port_info_list_get_info(portinfolist, i, &pi);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_port_info_list_get_info failed.\n");
		gp_camera_free(tmp_camera);
		gp_port_info_list_free(portinfolist);
		return false;
	}
	ret = gp_camera_set_port_info(tmp_camera, pi);
	if (ret < GP_OK)
	{
		fprintf(stderr, "gp_camera_set_port_info failed.\n");
		gp_camera_free(tmp_camera);
		gp_port_info_list_free(portinfolist);
		return false;
	}
	gp_port_info_list_free(portinfolist);


	ret = gp_camera_init(tmp_camera, camera_context);
	if (ret < GP_OK)
	{
		fprintf(stderr, "No camera auto detected.\n");
		gp_camera_free(tmp_camera);
		return false;
	}
#if 0
	CameraFunctions* cam_funcs = tmp_camera->functions;
	_drv_CameraPrePostFunc = cam_funcs->post_func;
	cam_funcs->post_func = handleCameraPrePostFunc;
#endif
	camera = tmp_camera;

	return true;
}

bool GMyLiveThread::deInitializeGPhoto2()
{
	if (camera && camera_context)
	{
		gp_camera_exit(camera, camera_context);
		gp_camera_unref(camera);
		camera = 0;
	}
	if (camera_context)
	{
		gp_context_unref(camera_context);
		//free(camera_context);
		camera_context = 0;
	}
}
#endif

bool GMyLiveThread::startLiveView()
{
#ifdef EDSDK
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
	return err == EDS_ERR_OK;
#endif
#ifdef GPHOTO2
	CameraWidget *widget = NULL, *child = NULL;
	CameraWidgetType type;
	int ret;

	ret = gp_camera_get_config(camera, &widget, camera_context);
	if (ret >= GP_OK)
	{
		ret = _gp_lookup_widget(widget, "capture", &child);
	}
	if (ret >= GP_OK)
	{
		ret = gp_widget_get_type(child, &type);
	}
	if (ret >= GP_OK)
	{
		if (type != GP_WIDGET_TOGGLE)
			ret = GP_ERROR_BAD_PARAMETERS;
	}
	if (ret >= GP_OK)
	{
		/* Now set the toggle to the wanted value */
		int onoff = 1;
		ret = gp_widget_set_value (child, &onoff);
	}
	if (ret >= GP_OK)
	{
		ret = gp_camera_set_config(camera, widget, camera_context);
	}
	if (ret >= GP_OK)
		LiveViewStarted = true;
	if (widget)
		gp_widget_free (widget);
	return ret >= GP_OK;
#endif
}

bool GMyLiveThread::downloadEvfData()
{
#ifdef EDSDK
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
	return err == EDS_ERR_OK;
#endif
#if GPHOTO2
	int ret;
	CameraFile *gpfile = 0;
	const char* ptr = 0;
	unsigned long int data_size = 0;

	ret = gp_file_new(&gpfile);
	if (ret == GP_OK)
	{
		ret = gp_camera_capture_preview(camera, gpfile, camera_context);
	}
	if (ret == GP_OK)
	{
		// owner of 'data' is gpfile
		ret = gp_file_get_data_and_size(gpfile, &ptr, &data_size);
	}

	if (ret == GP_OK)
	{
		// start critical section!!!
		live_buffer::ImageMutex.lock();
		if (max_frame_size < (int)data_size)
		{
			live_buffer::frame = (unsigned char*)realloc(live_buffer::frame, data_size);
			max_frame_size = (int)data_size;
		}
		memcpy(live_buffer::frame, ptr, data_size);
		live_buffer::frame_size = (int)data_size;
		live_buffer::ImageMutex.unlock();
		// end of critical section!!!
	}
#if 0
	// this code not work, libgphoto2 always get me zero.
	char* str_val = 0;
	int x, y, z, c;
	ret = _gp_get_config_value_string(camera, "eoszoom", &str_val, camera_context);
	if (ret >= GP_OK && str_val)
	{
		c = sscanf(str_val, "%u", &z);
		if (c == 1)
			Zoom = z;
	}
	if (str_val)
		free(str_val);
	ret = _gp_get_config_value_string(camera, "eoszoomposition", &str_val, camera_context);
	if (ret >= GP_OK && str_val)
	{
		c = sscanf(str_val, "%d,%d", &x, &y);
		if (c == 2)
		{
			ZoomPosX = x;
			ZoomPosY = y;
		}
	}
	if (str_val)
		free(str_val);
#endif

	// TO-DO: get histogram
	if (gpfile)
		gp_file_free(gpfile);
	return ret == GP_OK;
#endif
}

bool GMyLiveThread::endLiveView()
{
#ifdef EDSDK
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
	return err == EDS_ERR_OK;
#endif
#ifdef GPHOTO2
	CameraWidget *widget = NULL, *child = NULL;
	CameraWidgetType type;
	int ret;

	ret = gp_camera_get_config(camera, &widget, camera_context);
	if (ret >= GP_OK)
	{
		ret = _gp_lookup_widget(widget, "capture", &child);
	}
	if (ret >= GP_OK)
	{
		ret = gp_widget_get_type(child, &type);
	}
	if (ret >= GP_OK)
	{
		if (type != GP_WIDGET_TOGGLE)
			ret = GP_ERROR_BAD_PARAMETERS;
	}
	if (ret >= GP_OK)
	{
		/* Now set the toggle to the wanted value */
		int onoff = 0;
		ret = gp_widget_set_value (child, &onoff);
	}
	if (ret >= GP_OK)
	{
		ret = gp_camera_set_config(camera, widget, camera_context);
	}
	if (ret >= GP_OK)
		LiveViewStarted = false;
	if (widget)
		gp_widget_free(widget);
	return ret >= GP_OK;
#endif
}

#ifdef EDSDK
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
#endif

#ifdef GPHOTO2
void GMyLiveThread::propertyEvent(const char* prop_name)
{
	fprintf(stderr, "Property '%s' changed!\n", prop_name);
	if (strncasecmp(prop_name, "d1b0", 4) == 0)			// EVF Output device
	{
		fprintf(stderr, "evf\n");
		cmdRequestEvfOut();
	}
	else if (strncasecmp(prop_name, "d103", 4) == 0)	// ISO
	{
		fprintf(stderr, "iso\n");
		cmdRequestISO();
	}
	else if (strncasecmp(prop_name, "d101", 4) == 0)	// Av
	{
		fprintf(stderr, "av\n");
		cmdRequestAv();
	}
	else if (strncasecmp(prop_name, "d102", 4) == 0)	// Tv
	{
		fprintf(stderr, "tv\n");
		cmdRequestTv();
	}
	else if (strncasecmp(prop_name, "d105", 4) == 0)	// AE Mode
	{
		fprintf(stderr, "aemode\n");
		cmdRequestAEMode();
	}
	else if (strncasecmp(prop_name, "d108", 4) == 0)	// AF Mode
	{
		fprintf(stderr, "afmode\n");
		cmdRequestAFMode();
	}
	else if (strncasecmp(prop_name, "d1b3", 4) == 0)	// EOS Zoom
	{
#if 0
		// this not work in libghoto 2.4.10
		fprintf(stderr, "eoszoom\n");
		char* str_val = 0;
		int z, c;
		int ret = _gp_get_config_value_string(camera, "eoszoom", &str_val, camera_context);
		if (ret >= GP_OK && str_val)
		{
			c = sscanf(str_val, "%u", &z);
			if (c == 1)
				Zoom = z;
		}
		if (str_val)
			free(str_val);
#endif
	}
	/*else if (strncasecmp(prop_name, "", 4) == 0)	//
	{
		;
	}
	else if (strncasecmp(prop_name, "", 4) == 0)	//
	{
		;
	}*/
}
#endif

#ifdef EDSDK
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
#endif

#ifdef GPHOTO2
/*static void
ctx_error_func (GPContext *context, const char *format, va_list args, void *data)
{
	fprintf  (stderr, "\n");
	fprintf  (stderr, "*** Contexterror ***              \n");
	vfprintf (stderr, format, args);
	fprintf  (stderr, "\n");
	fflush   (stderr);
}

static void
ctx_status_func (GPContext *context, const char *format, va_list args, void *data)
{
	vfprintf (stderr, format, args);
	fprintf  (stderr, "\n");
	fflush   (stderr);
}

static void gp2_errordumper(GPLogLevel level, const char *domain, const char *format,
				 va_list args, void *data) {
  vfprintf(stdout, format, args);
  fprintf(stdout, "\n");
}*/

int GMyLiveThread::gp2_camera_check_event()
{
	CameraEventType event_type = GP_EVENT_UNKNOWN;
	char* event_data;
	int ret;
	char prop_name[64];

	prop_name[0] = 0;
	while (event_type != GP_EVENT_TIMEOUT)
	{
		event_data = 0;
		ret = gp_camera_wait_for_event(camera, 0, &event_type, (void**)&event_data, camera_context);
		if (ret >= GP_OK)
		{
			fprintf(stderr, "event_type: ");
			switch(event_type)
			{
			case GP_EVENT_UNKNOWN:
				if (event_data)
				{
					char* ptr = strstr(event_data, "changed");
					char* end_ptr = 0;
					int str_len = 0;
					if (ptr)
					{
						end_ptr = ptr;
						ptr = strstr(event_data, "PTP Property");
						if (ptr)
						{
							str_len = end_ptr - ptr - 14;
							if (str_len > 63)
								str_len = 63;
							strncpy(prop_name, event_data + 13, str_len);
							prop_name[str_len] = 0;
							propertyEvent(prop_name);
						}
					}
				}
				fprintf(stderr, "unknown");
				break;
			case GP_EVENT_TIMEOUT:
				//fprintf(stderr, "timeout");
				break;
			case GP_EVENT_FILE_ADDED:
				//fprintf(stderr, "file added");
				break;
			case GP_EVENT_FOLDER_ADDED:
				//fprintf(stderr, "folder added");
				break;
			case GP_EVENT_CAPTURE_COMPLETE:
				//fprintf(stderr, "capture complete");
				break;
			default:
				fprintf(stderr, "%d", event_type);
				;
			}
			if (event_data)
			{
				fprintf(stderr, "; event_data = '%s'", event_data);
				free(event_data);
			}
			fprintf(stderr, "\n");
		}
	}
}

static int _gp_lookup_widget(CameraWidget*widget, const char *key, CameraWidget **child)
{
	int ret;
	ret = gp_widget_get_child_by_name (widget, key, child);
	if (ret < GP_OK)
		ret = gp_widget_get_child_by_label (widget, key, child);
	return ret;
}

int _gp_get_config_value_string(Camera *camera, const char *key, char **str, GPContext *context)
{
	CameraWidget* widget = 0;
	CameraWidget* child = 0;
	int ret;
	char *val;

	ret = gp_camera_get_config(camera, &widget, context);
	if (ret >= GP_OK)
		ret = _gp_lookup_widget(widget, key, &child);
	if (ret >= GP_OK)
		ret = gp_widget_get_value(child, &val);
	if (ret >= GP_OK)
		*str = strdup(val);
	if (widget)
		gp_widget_free(widget);
	return ret;
}

int _gp_set_config_value_string (Camera *camera, const char *key, const char *val, GPContext *context)
{
	CameraWidget* widget = 0;
	CameraWidget* child = 0;
	int ret;

	ret = gp_camera_get_config(camera, &widget, context);
	if (ret >= GP_OK)
		ret = _gp_lookup_widget(widget, key, &child);
	if (ret >= GP_OK)
		ret = gp_widget_set_value(child, val);
	if (ret >= GP_OK)
		ret = gp_camera_set_config(camera, widget, context);
	if (widget)
		gp_widget_free (widget);
	return ret;
}

#endif
