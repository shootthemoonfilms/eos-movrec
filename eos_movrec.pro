
TEMPLATE = app
CONFIG += qt
QT = core gui

HEADERS = mainwnd.h about.h blinklabel.h
SOURCES = main.cpp mainwnd.cpp livethread.cpp capturewnd.cpp blinklabel.cpp about.cpp command.cpp buffer.cpp cam_tables.c mjpegwrt.c freqtimer.c

TARGET  = bin/eos_movrec

EDSDKPATH = ./EDSDK

INCLUDEPATH = $$EDSDKPATH/Header
LIBS += -L$$EDSDKPATH/Library -ledsdk

win32:RC_FILE = eos_movrec.rc
