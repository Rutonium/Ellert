@ECHO OFF
CLS
ECHO.
ECHO =======================================================
ECHO Starting Ellert Project File Creation...
ECHO =======================================================

REM --- 1. Create Core .ino File (Single line to check functionality) ---
ECHO // Ellert.ino Main File (Code will be pasted by you) > Ellert.ino
ECHO Created Ellert.ino

REM --- 2. Create PinDefinitions.h ---
ECHO // PinDefinitions.h - Defines all I/O pins. > PinDefinitions.h
ECHO Created PinDefinitions.h

REM --- 3. Create InputFilter.h and .cpp ---
ECHO // InputFilter.h - Debouncing class header. > InputFilter.h
ECHO // InputFilter.cpp - Debouncing class implementation. > InputFilter.cpp
ECHO Created InputFilter.h and InputFilter.cpp

REM --- 4. Create MotorControl.h and .cpp ---
ECHO // MotorControl.h - Critical motor logic header. > MotorControl.h
ECHO // MotorControl.cpp - Critical motor logic implementation. > MotorControl.cpp
ECHO Created MotorControl.h and MotorControl.cpp

REM --- 5. Create AccessoryControl.h and .cpp ---
ECHO // AccessoryControl.h - Accessory logic header. > AccessoryControl.h
ECHO // AccessoryControl.cpp - Accessory logic implementation. > AccessoryControl.cpp
ECHO Created AccessoryControl.h and AccessoryControl.cpp

ECHO.
ECHO =======================================================
ECHO File Creation Complete!
ECHO Please manually fill content into the created files.
ECHO =======================================================
PAUSE