#ifndef ELLERT_DISPLAY_CPU_ESP32S3_BOARD_CONFIG_H
#define ELLERT_DISPLAY_CPU_ESP32S3_BOARD_CONFIG_H

// Matches known working sample: JC3248W535EN + AXS15231B + QSPI.
static const int kScreenWidth = 320;
static const int kScreenHeight = 480;
static const int kScreenRotation = 1; // landscape 480x320

static const int kBacklightPin = 1;
static const int kBacklightOnLevel = HIGH;

static const int kLcdCsPin = 45;
static const int kLcdSckPin = 47;
static const int kLcdD0Pin = 21;
static const int kLcdD1Pin = 48;
static const int kLcdD2Pin = 40;
static const int kLcdD3Pin = 39;

// Touch (future use)
static const int kTouchSdaPin = 4;
static const int kTouchSclPin = 8;
static const int kTouchRstPin = 12;
static const int kTouchIntPin = 11;

// UART to Master CPU.
static const int kMasterUartRxPin = 44;
static const int kMasterUartTxPin = 43;

#endif
