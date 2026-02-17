#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "board_config.h"
#include "protocol_v1.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

namespace {
using namespace EllertProtocolV1;

Arduino_DataBus *gBus = new Arduino_ESP32QSPI(
    kLcdCsPin, kLcdSckPin, kLcdD0Pin, kLcdD1Pin, kLcdD2Pin, kLcdD3Pin);
Arduino_GFX *gPanel = new Arduino_AXS15231B(
    gBus, GFX_NOT_DEFINED, 0, false, kScreenWidth, kScreenHeight);
Arduino_Canvas *gGfx = new Arduino_Canvas(kScreenWidth, kScreenHeight, gPanel, 0, 0, 0);

Decoder gDecoder;
uint8_t gSeq = 0;
uint32_t gLastHeartbeatTxMs = 0;
uint32_t gLastMasterHeartbeatMs = 0;
uint32_t gLastStatusRxMs = 0;

struct DashboardState {
  uint8_t speedKmt = 0;
  uint8_t socPct = 34;
  int8_t powerUsedPct = 32;
  uint8_t powerAskedPct = 46;
  uint8_t indicatorBits = IND_LEFT;
  uint8_t lightBits = LIGHT_DRL;
  uint8_t wiperMode = 0;
  uint8_t gear = GEAR_D;
  uint16_t tripTotalTenthsKm = 3450;
  uint16_t tripSinceChargeTenthsKm = 220;
  bool inputOnline = true;
  bool displayOnlineFromMaster = true;
  bool ready = true;
  uint8_t lastCommand = 0;
  uint16_t inputMask = 0;
  int8_t outsideTempC = 20;
} gState;

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

void applyStatusSnapshot(const Frame &frame) {
  if (frame.len < kStatusPayloadLen) return;
  gState.speedKmt = frame.payload[0];
  gState.socPct = frame.payload[1];
  gState.powerUsedPct = static_cast<int8_t>(frame.payload[2]);
  gState.powerAskedPct = frame.payload[3];
  gState.indicatorBits = frame.payload[4];
  gState.lightBits = frame.payload[5];
  gState.wiperMode = frame.payload[6];
  gState.gear = frame.payload[7];
  gState.tripTotalTenthsKm = static_cast<uint16_t>(frame.payload[8]) |
                             (static_cast<uint16_t>(frame.payload[9]) << 8);
  gState.tripSinceChargeTenthsKm = static_cast<uint16_t>(frame.payload[10]) |
                                   (static_cast<uint16_t>(frame.payload[11]) << 8);
  gState.inputOnline = frame.payload[12] != 0;
  gState.displayOnlineFromMaster = frame.payload[13] != 0;
  gState.ready = frame.payload[14] != 0;
  gState.lastCommand = frame.payload[15];
  gState.inputMask = static_cast<uint16_t>(frame.payload[16]) |
                     (static_cast<uint16_t>(frame.payload[17]) << 8);
  gLastStatusRxMs = millis();
}

void pollMaster() {
  while (Serial1.available() > 0) {
    const uint8_t b = static_cast<uint8_t>(Serial1.read());
    Frame frame;
    if (!gDecoder.feed(b, frame)) continue;
    if (frame.type == MSG_HEARTBEAT) gLastMasterHeartbeatMs = millis();
    else if (frame.type == MSG_STATUS_SNAPSHOT) applyStatusSnapshot(frame);
  }
}

const char *gearLabel(uint8_t gear) {
  switch (gear) {
    case GEAR_P: return "P";
    case GEAR_R: return "R";
    case GEAR_N: return "N";
    case GEAR_D: return "D";
    default: return "-";
  }
}

void drawDashboard() {
  const int W = gGfx->width();
  const int H = gGfx->height();
  const bool blink = ((millis() / 700UL) % 2UL) == 0;

  gGfx->fillScreen(RGB565_BLACK);
  gGfx->fillRoundRect(6, 6, W - 12, 50, 8, 0x18C3);
  gGfx->fillRoundRect(6, 62, 142, H - 108, 8, 0x18C3);
  gGfx->fillRoundRect(154, 62, 172, H - 108, 8, 0x18C3);
  gGfx->fillRoundRect(332, 62, W - 338, H - 108, 8, 0x18C3);
  gGfx->fillRoundRect(6, H - 38, W - 12, 32, 8, 0x18C3);

  gGfx->setTextColor(RGB565_WHITE);
  gGfx->setTextSize(2);
  gGfx->setCursor(178, 80); gGfx->print("SPEED");
  gGfx->setCursor(358, 80); gGfx->print("POWER");

  if ((gState.indicatorBits & IND_LEFT) && blink) gGfx->fillCircle(170, 88, 12, RGB565_YELLOW);
  if ((gState.indicatorBits & IND_RIGHT) && blink) gGfx->fillCircle(310, 88, 12, RGB565_YELLOW);
  gGfx->setCursor(164, 82); gGfx->print("<");
  gGfx->setCursor(304, 82); gGfx->print(">");

  char speedBuf[8];
  snprintf(speedBuf, sizeof(speedBuf), "%u", static_cast<unsigned>(gState.speedKmt));
  gGfx->setTextSize(7);
  gGfx->setCursor(198, 130); gGfx->print(speedBuf);
  gGfx->setTextSize(2);
  gGfx->setCursor(220, 236); gGfx->print("km/t");
  gGfx->setTextSize(5);
  gGfx->setCursor(282, 216); gGfx->print(gearLabel(gState.gear));

  gGfx->drawRect(28, 110, 54, 120, RGB565_WHITE);
  gGfx->fillRect(48, 100, 14, 8, RGB565_WHITE);
  int fillH = map(gState.socPct, 0, 100, 0, 116);
  gGfx->fillRect(30, 112, 50, 116, RGB565_BLACK);
  gGfx->fillRect(30, 228 - fillH, 50, fillH, (gState.socPct >= 35) ? RGB565_GREEN : RGB565_RED);
  gGfx->setTextSize(2);
  gGfx->setCursor(92, 136);
  gGfx->print(gState.socPct); gGfx->print("%");

  int askH = map(gState.powerAskedPct, 0, 100, 0, 118);
  int usedAbs = gState.powerUsedPct < 0 ? -gState.powerUsedPct : gState.powerUsedPct;
  int usedH = map(usedAbs, 0, 100, 0, 118);
  gGfx->drawRect(356, 118, 32, 122, RGB565_WHITE);
  gGfx->drawRect(408, 118, 32, 122, RGB565_WHITE);
  gGfx->fillRect(358, 120, 28, 118, RGB565_BLACK);
  gGfx->fillRect(410, 120, 28, 118, RGB565_BLACK);
  gGfx->fillRect(358, 238 - askH, 28, askH, RGB565_CYAN);
  gGfx->fillRect(410, 238 - usedH, 28, usedH, (gState.powerUsedPct < 0) ? RGB565_GREEN : 0xFD20);
  gGfx->setTextSize(1);
  gGfx->setCursor(362, 244); gGfx->print("ASK");
  gGfx->setCursor(414, 244); gGfx->print("USED");

  const uint32_t totalMin = (millis() / 60000UL) + (12UL * 60UL + 34UL);
  char timeBuf[8];
  snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u", static_cast<unsigned>((totalMin / 60) % 24),
           static_cast<unsigned>(totalMin % 60));
  gGfx->setTextSize(2);
  gGfx->setCursor(16, 22); gGfx->print(timeBuf);
  gGfx->setTextColor(gState.ready ? RGB565_GREEN : RGB565_YELLOW);
  gGfx->setCursor(180, 22); gGfx->print(gState.ready ? "READY" : "WAIT");
  gGfx->setTextColor(RGB565_WHITE);
  gGfx->setCursor(260, 22); gGfx->print("IN:"); gGfx->print(gState.inputOnline ? "OK" : "NO");
  gGfx->setCursor(430, 22); gGfx->print(gState.outsideTempC); gGfx->print("C");

  gGfx->setCursor(18, H - 30);
  gGfx->print("Trip:"); gGfx->print(gState.tripTotalTenthsKm / 10); gGfx->print(".");
  gGfx->print(gState.tripTotalTenthsKm % 10); gGfx->print("km");

  gGfx->setCursor(200, H - 30);
  gGfx->print("Since:"); gGfx->print(gState.tripSinceChargeTenthsKm / 10); gGfx->print(".");
  gGfx->print(gState.tripSinceChargeTenthsKm % 10); gGfx->print("km");

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

  gLastMasterHeartbeatMs = millis();
  gLastStatusRxMs = millis();
  drawDashboard();

  Serial.println("DISPLAY_ESP32S3_AXS_CANVAS_BOOT");
}

void loop() {
  pollMaster();
  sendHeartbeatIfDue();
  drawDashboard();
  digitalWrite(LED_BUILTIN, (millis() - gLastMasterHeartbeatMs) <= kNodeOfflineTimeoutMs ? HIGH : LOW);
  delay(120);
}
