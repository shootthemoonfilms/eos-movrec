TEMPLATE = app
CONFIG += qt
QT = core \
    gui
HEADERS = mainwnd.h \
    about.h \
    blinklabel.h \
	freqtimer.h \
	afthread.h \
	capturewnd.h \
	types.h \
	livethread.h \
	buffer.h \
	FocuserClass.h \
	mjpegwrt.h \
	events.h \
	command.h \
	cam_tables.h
SOURCES = main.cpp \
    mainwnd.cpp \
    livethread.cpp \
    capturewnd.cpp \
    blinklabel.cpp \
    about.cpp \
    command.cpp \
    buffer.cpp \
    cam_tables.c \
    mjpegwrt.c \
    freqtimer.c \
    afthread.cpp
TARGET = bin/eos_movrec
EDSDKPATH = ./EDSDK
INCLUDEPATH = $$EDSDKPATH/Header
LIBS += -L$$EDSDKPATH/Library \
    -ledsdk
win32:RC_FILE = eos_movrec.rc
