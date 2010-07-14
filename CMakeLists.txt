project(eos-movrec)

cmake_minimum_required(VERSION 2.6.2)

include(FindPkgConfig)
include(CheckIncludeFile)

#set(CMAKE_VERBOSE_MAKEFILE on)

#check mingw
if(WIN32)
	if(MINGW)
		message(STATUS "Using MinGW.")
	else(MINGW)
		message(FATAL_ERROR "On Windows supported only MinGW gcc!")
	endif(MINGW)
endif(WIN32)

find_package(Qt4 4.4.2 COMPONENTS QtCore QtGui REQUIRED)
include(${QT_USE_FILE})

add_definitions (${QT_DEFINITIONS})
include_directories (${QT_INCLUDES} ${CMAKE_BINARY_DIR})

set(EDSDK_LDFLAGS "")
if(WIN32)
	set(EDSDKPATH "${CMAKE_SOURCE_DIR}/EDSDK")
	include_directories(${EDSDKPATH}/Header)
	set(EDSDK_LDFLAGS "-L${EDSDKPATH}/Library -lEDSDK")
	CHECK_INCLUDE_FILE(${EDSDKPATH}/Header/EDSDK.h EDSDK_H)
	if(NOT EDSDK_H)
		message(FATAL_ERROR "EDSDK not found!\nYou must insert EDSDK into project folder.")
	endif(NOT EDSDK_H)
	add_definitions(-DEDSDK=1)
elseif(UNIX)
	pkg_check_modules(LIBGPHOTO2 REQUIRED libgphoto2>=2.4.9)
	include_directories(${LIBGPHOTO2_INCLUDE_DIRS})
	add_definitions(-DGPHOTO2=1)
endif(WIN32)

set(eos_movrec_SRCS
	main.cpp
	mainwnd.cpp
	livethread.cpp
	capturewnd.cpp
	blinklabel.cpp
	about.cpp
	command.cpp
	buffer.cpp
	autofocus.cpp
	cam_tables.c
	mjpegwrt.c
	os_api.c
	afthread.cpp
	histogramwnd.cpp
	optionsdlg.cpp
)

if(WIN32)
	set(eos_movrec_SRCS ${eos_movrec_SRCS} eos_movrec.rc)
endif(WIN32)

# headers with Q_OBJECT
set(eos_movrec_MOC_HEADERS
	mainwnd.h
	blinklabel.h
)

# force use exceptions
set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -fexceptions")
# add debug defines
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -DAF_DEBUG_LOG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGE_FILES=1)
QT4_WRAP_CPP(MOC_SRCS ${eos_movrec_MOC_HEADERS})

if (WIN32)
	set(CMAKE_RC_COMPILER windres)
	# set rc syntax
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> <DEFINES> -O coff -o <OBJECT> <SOURCE>")

	# enable resource language
	enable_language(RC)

	# -mwindows -> window application (without terminal screen)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
	# for gcc-4.5 and shared libstdc++
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--enable-auto-import")
endif(WIN32)

add_executable(eos_movrec ${eos_movrec_SRCS} ${MOC_SRCS})
if (WIN32)
	TARGET_LINK_LIBRARIES(eos_movrec ${QT_LIBRARIES} ${EDSDK_LDFLAGS})
elseif(UNIX)
	TARGET_LINK_LIBRARIES(eos_movrec ${QT_LIBRARIES} ${LIBGPHOTO2_LDFLAGS})
endif(WIN32)
install(TARGETS eos_movrec RUNTIME DESTINATION bin)
