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

#ifndef _livethread_h
#define _livethread_h

#if !defined(EDSDK) && !defined(GPHOTO2)
#  error "You must define one of EDSDK or GPHOTO2!"
#endif

#if defined(EDSDK) && defined(GPHOTO2)
#  error "You must define only EDSDK or GPHOTO2!"
#endif

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QSize>

#include "command.h"
#ifdef EDSDK
#include "EDSDK.h"
#endif
#ifdef GPHOTO2
#include <gphoto2/gphoto2.h>
#include <gphoto2/gphoto2-camera.h>
#endif
#include "types.h"

class QWidget;

struct EOSCamFeatures
{
	unsigned int JpegLargeSize_x;
	unsigned int JpegLargeSize_y;
	unsigned int LiveViewSize_x;
	unsigned int LiveViewSize_y;
	bool HasAF;
};

class GMyLiveThread: public QThread
{
public:
	GMyLiveThread(QWidget* owner);
	virtual ~GMyLiveThread();
	void setCaptureWnd(QWidget* wnd);
	bool isInit() { return Inited; }
	bool liveViewStarted() { return LiveViewStarted; }
	void stop();
	void startWrite();
	void stopWrite();
	bool writeEnabled() { return WriteMovie; }
	void setFileName(const char* fname);
	void setBufferSize(int);
	void setUseStabFPS(bool s);
	void cmdSetAEMode(int ae);
	void cmdSetWB(int wb, int temp);
	void cmdSetISO(int iso);
	void cmdRequestISO();
	void cmdRequestISOList();
	void cmdSetAv(int av, int dof);
	void cmdRequestAv();
	void cmdRequestAvList();
	void cmdSetTv(int tv);
	void cmdRequestTv();
	void cmdRequestTvList();
	void cmdRequestEvfOut();
	void cmdRequestAEMode();
	void cmdRequestAEModeList();
	void cmdRequestAFMode();
	void cmdAdjFocus(int direction, int val);
	void cmdSetZoom(int zoom);
	void cmdSetZoomPos(int x, int y);
	void cmdDoLVAF(int mode);
	// wait for all commands
	// call only from other threads!
	void waitCommands();
	const unsigned int* avList() const { return AvList; }
	int avListSize() const { return AvListSize; }
	const unsigned int* tvList() const { return TvList; }
	int tvListSize() const { return TvListSize; }
	const unsigned int* isoList() const { return ISOList; }
	int isoListSize() const { return ISOListSize; }
	const unsigned int* aemList() const { return AEMList; }
	int aemListSize() const { return AEMListSize; }
	QString cameraName() const { return CameraName; }
	struct EOSCamFeatures cameraFeatures() const { return CamFeatures; }
	// stat function
	long long int allFramesCount() { return AllFramesCount; }
	long long int writenCount() { return WritenCount; }
	long long int skippedCount() { return SkippedCount; }
	long long int duplicatedCount() { return DuplicatedCount; }
	double stableFPS() { return StableFPS; }
	int elapsedTime() { return ElapsedTime; }
	int zoom() const { return Zoom; }
	int zoomPosX() const { return ZoomPosX; }
	int zoomPosY() const { return ZoomPosY; }
	int* histogram() { return (int*)Histogram; }
	int histogramSize() { return 256 * 4; }
	void setWantHistogram(bool w) { WantHistogram = w; }
	void lockHistogram() { HistogramMutex.lock(); }
	void unlockHistogram() { HistogramMutex.unlock(); }
	// handlers
#ifdef EDSDK
	// ED-SDK documentation say:
	// "The callback function is executed on a newly generated thread"
	void objectEvent(EdsObjectEvent event, EdsBaseRef object);
	void propertyEvent(EdsPropertyEvent event, EdsPropertyID property, EdsUInt32 inParam);
	void stateEvent(EdsStateEvent event, EdsUInt32 parameter);
#endif
protected:
	virtual void run();
private:
#ifdef EDSDK
	bool initializeEds();
	bool deInitializeEds();
#endif
#ifdef GPHOTO2
	bool initializeGPhoto2();
	bool deInitializeGPhoto2();
	int gp2_camera_check_event();
	void propertyEvent(const char* prop_name);
#endif
	bool startLiveView();
	bool downloadEvfData();
	bool endLiveView();
	bool processCommand();
	bool fillAvList();
	bool fillTvList();
	bool fillISOList();
	bool fillAEMList();
	bool fillCameraName();
private:
	bool Stoped;
	bool Inited;
	bool LiveViewStarted;
	QWidget* Owner;
	QWidget* CaptureWnd;
	bool WriteMovie;
	char* FileName;
	int BufferSize;
	double StableFPS;
	QMutex CommandMutex;
	QMutex WaitMutex;
	QWaitCondition CommandCond;
	QMutex WrtFlagMutex;
	QMutex HistogramMutex;
	GCameraCommandsQueue CommandsQueue;
	int max_frame_size;
#ifdef EDSDK
	EdsCameraRef camera;
#endif
#ifdef GPHOTO2
	GPContext *camera_context;
	Camera* camera;
#endif
	unsigned int AvList[128];
	int AvListSize;
	unsigned int TvList[128];
	int TvListSize;
	unsigned int ISOList[32];
	int ISOListSize;
	unsigned int AEMList[16];
	int AEMListSize;
	int Zoom;
	int ZoomPosX;
	int ZoomPosY;
	bool WantHistogram;
	int Histogram[256*4];
	bool isSDKLoaded;
	bool UseStabFPS;
	QString CameraName;
	struct EOSCamFeatures CamFeatures;
	// for statistics
	long long int AllFramesCount;
	long long int WritenCount;
	long long int SkippedCount;
	long long int DuplicatedCount;
	int ElapsedTime;
};

#endif	// _livethread_h
