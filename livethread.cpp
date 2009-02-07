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
#include "freqtimer.h"
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
	WriteMovie = true;
}

void GMyLiveThread::stopWrite()
{
	WriteMovie = false;
}

void GMyLiveThread::cmdSetWB(int wb, int temp)
{
	CommandMutex.lock();
	GCameraCommand cmd(COMMAND_SET_WB, wb, temp);
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

EdsError GMyLiveThread::processCommand()
{
	CommandMutex.lock();
	GCameraCommand cmd = CommandsQueue.takeFirst();
	CommandMutex.unlock();
	int param1 = cmd.param1();
	int param2 = cmd.param2();
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
	case COMMAND_SET_AV:		// set Av & DOF
		if (param1 > 0x1)
		{
			EdsUInt32 av;
			err = EdsGetPropertyData(camera, kEdsPropID_Av, 0, sizeof(EdsUInt32), &av);
			if (err == EDS_ERR_OK)
			{
				if (av != (int)param1)
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
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AV_CHANGED, QVariant((int)av)));
		}
		break;
	case COMMAND_REQ_AVLIST:	// request Av list
		err = fillAvList();
		if (err == EDS_ERR_OK)
			if (Owner)
				QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AVLIST_CHANGED, 0));
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
	case COMMAND_REQ_AEMODE:	// request AE mode
		{
			EdsUInt32 mode;
			err = EdsGetPropertyData(camera, kEdsPropID_AEMode, 0, sizeof(mode), &mode);
			if (err == EDS_ERR_OK)
				if (Owner)
					QApplication::postEvent(Owner, new GCameraEvent(CAMERA_EVENT_AEMODE_CHANGED, QVariant((int)mode)));
		}
		break;
	}
	return err;
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
		return;
	// get Av list to main window
	cmdRequestAvList();
	// get Av value to main window
	cmdRequestAv();
	//
	Inited = true;
	SkippedCount = 0;
	AllFramesCount = 0;
	WritenCount = 0;
	ElapsedTime = 0;
	int StartTime;
	int EndTime;
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
	int StartWriteTime;		// ms
	int StopWriteTime;		// ms
	// for temp fps
	int TempTime1 = WinGetTickCount();
	int TempTime2 = TempTime1;
	int TempFrameCount = 0;
	double TempFPS;
	// for internal EDSDK messqge queue
	int SDKMsgCheckTime1 = WinGetTickCount();
	int SDKMsgCheckTime2 = SDKMsgCheckTime1;
	//WinQueryPerformanceFrequency(&freq);
	while (!Stoped)
	{
/*static int c = 0;
FILE* f = fopen("d", "wt");
fprintf(f, "%d\n", c);
fclose(f);
c++;*/
		// process internal EDSDK message queue
		if (SDKMsgCheckTime2 - SDKMsgCheckTime1 > 800)
		{
			WinProcessMsg();
			SDKMsgCheckTime1 = SDKMsgCheckTime2;
		}

		if (!CommandsQueue.isEmpty())
			processCommand();
		//WinQueryPerformanceCounter(&t1);
		// fetch image.
		if (downloadEvfData() == EDS_ERR_OK)
		{
			AllFramesCount++;
			if (live_buffer::IsPainting)
				SkippedCount++;
			else if (CaptureWnd)
				QApplication::postEvent(CaptureWnd, new GCameraEvent(CAMERA_EVENT_EVF_TRANSMITED, 0));
			// write to file if needed
			if (!PrevWriteMovie && WriteMovie)			// start record
			{
				StartWriteTime = WinGetTickCount();
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
		mjpegCloseFile(mjpeg);
	}
	EndTime = WinGetTickCount();
	ElapsedTime = EndTime - StartTime;

	err = endLiveView();
	/*if (err != EDS_ERR_OK)
		QMessageBox::critical(this, "Error", "Can't off LiveView mode!");*/
	// Close session with camera
	err = EdsCloseSession(camera);
	// Release camera
	if (camera != NULL)
	{
		EdsRelease(camera);
	}
	// Terminate SDK
	if (isSDKLoaded)
	{
		EdsTerminateSDK();
	}

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
		err = getFirstCamera();
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

EdsError GMyLiveThread::getFirstCamera()
{
	EdsError err = EDS_ERR_OK;
	EdsCameraListRef cameraList = NULL;
	EdsUInt32 count = 0;
	// Get camera list
	err = EdsGetCameraList(&cameraList);
	// Get number of cameras
	if (err == EDS_ERR_OK)
	{
		err = EdsGetChildCount(cameraList, &count);
		if(count == 0)
		{
			err = EDS_ERR_DEVICE_NOT_FOUND;
		}
	}
	// Get first camera retrieved
	if(err == EDS_ERR_OK)
	{
		err = EdsGetChildAtIndex(cameraList, 0, &camera);
	}
	// Release camera list
	if(cameraList != NULL)
	{
		EdsRelease(cameraList);
		cameraList = NULL;
	}
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
		/* // Get the zoom ratio
		EdsUInt32 zoom;
		EdsGetPropertyData(evfImage, kEdsPropID_Evf_ZoomPosition, 0 , sizeof(zoom), &zoom);
		// Get the focus and zoom border position
		EdsPoint point;
		EdsGetPropertyData(evfImage, kEdsPropID_Evf_ZoomPosition, 0 , sizeof(point), &point);
		*/

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
	}
	if (property == kEdsPropID_Av)
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
			/*EdsUInt32 device;
			err = EdsGetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device);
			if (err == EDS_ERR_OK)
			{
				//printf("\t\tval = %d\n", (int)device);
				if (device & kEdsEvfOutputDevice_PC)
					LiveViewStarted = true;
			}*/
			cmdRequestEvfOut();
		}
		else if (property == kEdsPropID_Av)
		{
			/*EdsInt32 av;
			err = EdsGetPropertyData(camera, kEdsPropID_Av, 0, sizeof(av), &av);
			printf("err = %04X\n", err);
			fflush(stdout);
			if (err == EDS_ERR_OK)
			{
				Av = av;
				if (Owner)
					QApplication::postEvent(Owner, new QEvent((QEvent::Type)(QEvent::User + 2)));
			}*/
			cmdRequestAv();
		}
		else if (property == kEdsPropID_AEMode)
		{
			cmdRequestAEMode();
		}
	}
	else if (event == kEdsPropertyEvent_PropertyDescChanged)
	{
		if (property == kEdsPropID_Av)
		{
			/*err = fillAvList();
			printf("desc: err = %04X\n", err);
			fflush(stdout);
			if (err == EDS_ERR_OK)
				if (Owner)
					QApplication::postEvent(Owner, new QEvent((QEvent::Type)(QEvent::User + 3)));*/
			cmdRequestAvList();
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
