TEMPLATE = app
CONFIG += qt
QT = core \
	gui
HEADERS = mainwnd.h \
	about.h \
	blinklabel.h \
	os_api.h \
	afthread.h \
	capturewnd.h \
	types.h \
	livethread.h \
	buffer.h \
	autofocus.h \
	mjpegwrt.h \
	events.h \
	command.h \
	cam_tables.h \
	histogramwnd.h \
	optionsdlg.h
SOURCES = main.cpp \
	mainwnd.cpp \
	livethread.cpp \
	capturewnd.cpp \
	blinklabel.cpp \
	about.cpp \
	command.cpp \
	buffer.cpp \
	autofocus.cpp \
	cam_tables.c \
	mjpegwrt.c \
	os_api.c \
	afthread.cpp \
	histogramwnd.cpp \
	optionsdlg.cpp
TARGET = bin/eos_movrec
EDSDKPATH = ./EDSDK
#DEFINES += AF_DEBUG_LOG=1
win32:DEFINES += EDSDK=1
unix:DEFINES += GPHOTO2=1
INCLUDEPATH = $$EDSDKPATH/Header
LIBS += -L$$EDSDKPATH/Library \
	-ledsdk
win32:RC_FILE = eos_movrec.rc
