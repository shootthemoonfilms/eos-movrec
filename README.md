
# eos-movrec

*View and control your Canon EOS camera on your PC (Windows and Linux) or Mac via USB cable.*

![Preview image of eos-movrec in action](/preview.png "eos-movrec in action")

## Capture

Take screenshots, photos, and video.

## Control

Control focus, ISO, aperture, shutterspeed, camera mode, white balance, and digital zoom (5x).

## Info

View current settings, and information including the battery level.

## How to compile:

### Requirements

```
cmake-2.6.2
Qt >= 5
EDSDK-2.4 (for Windows)
libgphoto2-2.5.1 (for Linux)
```

###	1. Windows (using MinGW)

####	1.1 Windows console

```
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make
```

####	1.2 MSYS console

```
mkdir build
cd build
cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make
```

###	2. Linux

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
