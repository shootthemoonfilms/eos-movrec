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

#ifndef _livethread_h
#define _livethread_h

#include <QThread>
#include <QMutex>

#include "command.h"

#include "EDSDK.h"
#include "types.h"

class QWidget;

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
	void cmdSetWB(int wb, int temp);
	void cmdSetAv(int av, int dof);
	void cmdRequestAv();
	void cmdRequestAvList();
	void cmdSetTv(int tv);
	void cmdRequestTv();
	void cmdRequestTvList();
	void cmdRequestEvfOut();
	void cmdRequestAEMode();
	void cmdRequestAFMode();
	void cmdAdjFocus(int direction, int val);
	void cmdSetZoom(int zoom);
	void cmdSetZoomPos(int x, int y);
	const int* avList() const { return AvList; }
	int avListSize() const { return AvListSize; }
	const int* tvList() const { return TvList; }
	int tvListSize() const { return TvListSize; }
	// stat function
	__uint64_t allFramesCount() { return AllFramesCount; }
	__uint64_t writenCount() { return WritenCount; }
	__uint64_t skippedCount() { return SkippedCount; }
	int elapsedTime() { return ElapsedTime; }
	int zoom() const { return Zoom; }
	int zoomPosX() const { return ZoomPosX; }
	int zoomPosY() const { return ZoomPosY; }
	// handlers
	// ED-SDK documentation say:
	// "The callback function is executed on a newly generated thread"
	void objectEvent(EdsObjectEvent event, EdsBaseRef object);
	void propertyEvent(EdsPropertyEvent event, EdsPropertyID property, EdsUInt32 inParam);
	void stateEvent(EdsStateEvent event, EdsUInt32 parameter);
protected:
	virtual void run();
private:
	EdsError initializeEds();
	EdsError getFirstCamera();
	EdsError startLiveView();
	EdsError downloadEvfData();
	EdsError endLiveView();
	EdsError processCommand();
	EdsError fillAvList();
	EdsError fillTvList();
private:
	bool Stoped;
	bool Inited;
	bool LiveViewStarted;
	QWidget* Owner;
	QWidget* CaptureWnd;
	bool WriteMovie;
	char* FileName;
	QMutex CommandMutex;
	QMutex WrtFlagMutex;
	GCameraCommandsQueue CommandsQueue;
	int max_frame_size;
	EdsCameraRef camera;
	int AvList[128];
	int AvListSize;
	int TvList[128];
	int TvListSize;
	int Zoom;
	int ZoomPosX;
	int ZoomPosY;
	bool isSDKLoaded;
	// for statistics
	__uint64_t AllFramesCount;
	__uint64_t WritenCount;
	__uint64_t SkippedCount;
	int ElapsedTime;
};

#endif	// _livethread_h
