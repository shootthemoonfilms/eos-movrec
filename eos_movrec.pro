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
    FocuserClass.h \
    mjpegwrt.h \
    events.h \
    command.h \
    cam_tables.h \
    histogramwnd.h
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
    os_api.c \
    afthread.cpp \
    histogramwnd.cpp
TARGET = bin/eos_movrec
EDSDKPATH = ./EDSDK
INCLUDEPATH = $$EDSDKPATH/Header
LIBS += -L$$EDSDKPATH/Library \
    -ledsdk
win32:RC_FILE = eos_movrec.rc
