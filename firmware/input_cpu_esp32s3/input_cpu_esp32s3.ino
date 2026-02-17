#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "board_config.h"
#include "protocol_v1.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

namespace {
using namespace EllertProtocolV1;

enum CommandId : uint8_t {
  CMD_LIGHTS_OFF = 0,
  CMD_LIGHTS_PARK = 1,
  CMD_LIGHTS_LOW = 2,
  CMD_LIGHTS_HIGH = 3,
  CMD_IND_LEFT = 4,
  CMD_IND_RIGHT = 5,
  CMD_HAZARD = 6,
  CMD_HORN = 7,
  CMD_WIPER_INT = 8,
  CMD_WIPER_LOW = 9,
  CMD_WIPER_HIGH = 10,
  CMD_WASHER = 11,
  CMD_FAN_LOW = 12,
  CMD_FAN_MID = 13,
  CMD_FAN_HIGH = 14,
  CMD_DEMIST = 15
};

struct ButtonDef {
  const char *label;
  uint8_t commandId;
  uint16_t color;
};

const ButtonDef kButtons[16] = {
    {"L OFF", CMD_LIGHTS_OFF, 0x39E7}, {"L PARK", CMD_LIGHTS_PARK, 0x39E7},
    {"L LOW", CMD_LIGHTS_LOW, 0x39E7}, {"L HIGH", CMD_LIGHTS_HIGH, 0x39E7},
    {"IND L", CMD_IND_LEFT, 0x051D},   {"IND R", CMD_IND_RIGHT, 0x051D},
    {"HAZ", CMD_HAZARD, 0xF800},       {"HORN", CMD_HORN, 0xD8C0},
    {"W INT", CMD_WIPER_INT, 0x001F},  {"W LOW", CMD_WIPER_LOW, 0x001F},
    {"W HIGH", CMD_WIPER_HIGH, 0x001F},{"WASH", CMD_WASHER, 0x001F},
    {"F LOW", CMD_FAN_LOW, 0x07E0},    {"F MID", CMD_FAN_MID, 0x07E0},
    {"F HIGH", CMD_FAN_HIGH, 0x07E0},  {"DEM", CMD_DEMIST, 0xFD20},
};

Arduino_DataBus *gBus = new Arduino_ESP32QSPI(
    kLcdCsPin, kLcdSckPin, kLcdD0Pin, kLcdD1Pin, kLcdD2Pin, kLcdD3Pin);
Arduino_GFX *gPanel = new Arduino_AXS15231B(
    gBus, GFX_NOT_DEFINED, 0, false, kScreenWidth, kScreenHeight);
Arduino_Canvas *gGfx = new Arduino_Canvas(kScreenWidth, kScreenHeight, gPanel, 0, 0, 0);

Decoder gDecoder;
uint8_t gSeq = 0;
uint32_t gLastHeartbeatTxMs = 0;
uint32_t gLastMasterHeartbeatMs = 0;

void sendFrame(uint8_t type, const uint8_t *payload, uint8_t len) {
  uint8_t buffer[kMaxPayloadLen + kFrameOverhead];
  const size_t frameLen = encodeFrame(type, gSeq++, payload, len, buffer, sizeof(buffer));
  if (frameLen > 0) Serial1.write(buffer, frameLen);
}

void sendHeartbeatIfDue() {
  const uint32_t now = millis();
  if (now - gLastHeartbeatTxMs < kHeartbeatMs) return;
  gLastHeartbeatTxMs = now;
  const uint8_t payload[1] = {1};
  sendFrame(MSG_HEARTBEAT, payload, sizeof(payload));
}

void pollMaster() {
  while (Serial1.available() > 0) {
    const uint8_t b = static_cast<uint8_t>(Serial1.read());
    Frame frame;
    if (gDecoder.feed(b, frame) && frame.type == MSG_HEARTBEAT) {
      gLastMasterHeartbeatMs = millis();
    }
  }
}

void drawGrid() {
  gGfx->fillScreen(RGB565_BLACK);
  const int cols = 4;
  const int rows = 4;
  const int w = gGfx->width() / cols;
  const int h = gGfx->height() / rows;

  gGfx->setTextColor(RGB565_WHITE);
  gGfx->setTextSize(2);

  for (int i = 0; i < 16; ++i) {
    const int c = i % cols;
    const int r = i / cols;
    const int x = c * w;
    const int y = r * h;

    gGfx->fillRect(x + 2, y + 2, w - 4, h - 4, kButtons[i].color);
    gGfx->drawRect(x + 1, y + 1, w - 2, h - 2, RGB565_WHITE);
    gGfx->setCursor(x + 8, y + (h / 2) - 8);
    gGfx->print(kButtons[i].label);
  }

  gGfx->fillRect(0, gGfx->height() - 20, gGfx->width(), 20, RGB565_BLACK);
  gGfx->setTextColor(RGB565_YELLOW);
  gGfx->setTextSize(1);
  gGfx->setCursor(6, gGfx->height() - 14);
  gGfx->print("INPUT CPU");
  gGfx->flush();
}

void drawStatus() {
  const bool online = (millis() - gLastMasterHeartbeatMs) <= kNodeOfflineTimeoutMs;
  gGfx->fillRect(0, 0, gGfx->width(), 14, RGB565_BLACK);
  gGfx->setTextColor(online ? RGB565_GREEN : RGB565_RED);
  gGfx->setTextSize(1);
  gGfx->setCursor(2, 2);
  gGfx->print(online ? "MASTER:ONLINE" : "MASTER:OFFLINE");
  gGfx->flush();
}

} // namespace

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, kMasterUartRxPin, kMasterUartTxPin);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(kBacklightPin, OUTPUT);
  digitalWrite(kBacklightPin, kBacklightOnLevel);

  if (!gGfx->begin()) {
    while (true) delay(1000);
  }
  gGfx->setRotation(kScreenRotation);
  drawGrid();

  Serial.println("INPUT_ESP32S3_AXS_CANVAS_BOOT");
}

void loop() {
  pollMaster();
  sendHeartbeatIfDue();
  drawStatus();
  digitalWrite(LED_BUILTIN, (millis() - gLastMasterHeartbeatMs) <= kNodeOfflineTimeoutMs ? HIGH : LOW);
  delay(80);
}
