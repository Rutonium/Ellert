#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#define LGFX_USE_V1
#define LGFX_USE_QSPI
#include <LovyanGFX.hpp>

#include "board_config.h"
#include "Panel_AXS15231B_local.hpp"
#include "protocol_v1.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

namespace {
using namespace EllertProtocolV1;

// --- Touch read constants (same working method as JC3248 library) ---
constexpr uint8_t kTouchAddr = 0x3B;
constexpr uint8_t kMaxTouchPoints = 1;
constexpr bool kPanelDiagMode = true;

// --- Serial / protocol ---
Decoder gDecoder;
uint8_t gSeq = 0;
uint32_t gLastHeartbeatTxMs = 0;

struct RuntimeState {
  bool masterOnline = false;
  bool inputOnline = false;
  bool displayOnline = false;
  bool ready = false;
  bool ignitionOn = false;
  uint8_t speedKmt = 0;
  uint8_t socPct = 0;
  int8_t powerUsedPct = 0;
  uint8_t powerAskedPct = 0;
  uint8_t indicatorBits = 0;
  uint8_t lightBits = 0;
  uint8_t wiperMode = 0;
  uint8_t gear = GEAR_UNKNOWN;
  uint8_t lastCommand = 0xFF;
  uint16_t inputMask = 0;
  uint16_t tripTotalTenthsKm = 0;
  uint16_t tripSinceChargeTenthsKm = 0;
  uint32_t lastMasterHeartbeatMs = 0;
  uint32_t lastStatusRxMs = 0;
  uint32_t hbRxCount = 0;
  uint32_t hbTxCount = 0;
  uint32_t statusRxCount = 0;
} gState;

// --- UI/log state ---
constexpr size_t kLogLines = 10;
char gEventLog[kLogLines][64];
size_t gLogHead = 0;
size_t gLogCount = 0;
uint8_t gPrevLoggedCmd = 0xFF;
uint16_t gPrevLoggedMask = 0;

bool gDiagScreenActive = false;
bool gTouchHoldArmed = false;
bool gTouchHoldTriggered = false;
uint32_t gTouchHoldStartMs = 0;
constexpr uint32_t kDiagHoldMs = 1200;

// --- LVGL display backend (LovyanGFX migration path) ---
class LGFX_JC3248 : public lgfx::LGFX_Device {
  lgfx::Bus_SPI _bus;
  lgfx::Panel_AXS15231B _panel;

 public:
  LGFX_JC3248() {
    {
      auto cfg = _bus.config();
      cfg.spi_mode = 0;
      cfg.freq_write = 20000000;
      cfg.freq_read = 16000000;
      cfg.spi_host = SPI3_HOST;
      cfg.spi_3wire = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = kLcdSckPin;
      cfg.pin_io0 = kLcdD0Pin;
      cfg.pin_io1 = kLcdD1Pin;
      cfg.pin_io2 = kLcdD2Pin;
      cfg.pin_io3 = kLcdD3Pin;
      cfg.use_lock = true;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = kLcdCsPin;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.panel_width = kScreenWidth;
      cfg.panel_height = kScreenHeight;
      cfg.offset_x = 1;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.memory_width = kScreenWidth;
      cfg.memory_height = kScreenHeight;
      cfg.readable = true;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

LGFX_JC3248 gTft;

constexpr int32_t kHorRes = 480;
constexpr int32_t kVerRes = 320;
constexpr uint32_t kDrawBufLines = 32;
static lv_color_t gLvBuf[kHorRes * kDrawBufLines];
lv_display_t *gDisplay = nullptr;
lv_indev_t *gTouchIndev = nullptr;

// --- LVGL objects ---
lv_obj_t *gMainScreen = nullptr;
lv_obj_t *gDiagScreen = nullptr;
lv_obj_t *gHdrTime = nullptr;
lv_obj_t *gHdrReady = nullptr;
lv_obj_t *gHdrNodes = nullptr;
lv_obj_t *gSocIcon = nullptr;
lv_obj_t *gSocPct = nullptr;
lv_obj_t *gSpeedValue = nullptr;
lv_obj_t *gSpeedUnit = nullptr;
lv_obj_t *gGear = nullptr;
lv_obj_t *gIndicators = nullptr;
lv_obj_t *gPowerAskLabel = nullptr;
lv_obj_t *gPowerAskBar = nullptr;
lv_obj_t *gPowerUsedLabel = nullptr;
lv_obj_t *gPowerUsedBar = nullptr;
lv_obj_t *gFooterTrip = nullptr;
lv_obj_t *gFooterSince = nullptr;
lv_obj_t *gFooterHint = nullptr;
lv_obj_t *gMapNavTitle = nullptr;
lv_obj_t *gMapRouteInfo = nullptr;
lv_obj_t *gDiagComms = nullptr;
lv_obj_t *gDiagPorts = nullptr;
lv_obj_t *gDiagLog = nullptr;

void pushLog(const char *msg) {
  if (!msg) return;
  snprintf(gEventLog[gLogHead], sizeof(gEventLog[gLogHead]), "%s", msg);
  gLogHead = (gLogHead + 1U) % kLogLines;
  if (gLogCount < kLogLines) gLogCount++;
}

const char *commandName(uint8_t cmd) {
  switch (cmd) {
    case 0: return "L_OFF";
    case 1: return "L_PARK";
    case 2: return "L_LOW";
    case 3: return "L_HIGH";
    case 4: return "IND_L";
    case 5: return "IND_R";
    case 6: return "HAZARD";
    case 7: return "HORN";
    case 8: return "WIP_INT";
    case 9: return "WIP_LOW";
    case 10: return "WIP_HIGH";
    case 11: return "WASH";
    case 12: return "FAN_LOW";
    case 13: return "FAN_MID";
    case 14: return "FAN_HIGH";
    case 15: return "DEMIST";
    default: return "NONE";
  }
}

const char *gearName(uint8_t gear) {
  switch (gear) {
    case GEAR_P: return "P";
    case GEAR_R: return "R";
    case GEAR_N: return "N";
    case GEAR_D: return "D";
    default: return "-";
  }
}

bool readTouchPoint(uint16_t &x, uint16_t &y) {
  uint8_t data[kMaxTouchPoints * 6 + 2] = {0};
  const uint8_t readCmd[11] = {
      0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00,
      static_cast<uint8_t>((sizeof(data) >> 8) & 0xFF),
      static_cast<uint8_t>(sizeof(data) & 0xFF),
      0x00, 0x00, 0x00};

  Wire.beginTransmission(kTouchAddr);
  Wire.write(readCmd, sizeof(readCmd));
  if (Wire.endTransmission() != 0) return false;

  const int req = Wire.requestFrom(kTouchAddr, static_cast<uint8_t>(sizeof(data)));
  if (req != static_cast<int>(sizeof(data))) return false;

  for (size_t i = 0; i < sizeof(data); ++i) data[i] = static_cast<uint8_t>(Wire.read());

  if (data[1] == 0 || data[1] > kMaxTouchPoints) return false;

  const uint16_t rawX = static_cast<uint16_t>(((data[2] & 0x0F) << 8) | data[3]);
  const uint16_t rawY = static_cast<uint16_t>(((data[4] & 0x0F) << 8) | data[5]);
  if (rawX == 273 && rawY == 273) return false;
  if (rawX > 4000 || rawY > 4000) return false;

  // Matches known-good transform for this board family in landscape path.
  y = map(rawX, 0, 320, 320, 0);
  x = rawY;

  if (x >= kHorRes) x = kHorRes - 1;
  if (y >= kVerRes) y = kVerRes - 1;
  return true;
}

void maybeToggleDiagByHold(uint16_t x, uint16_t y, bool pressed) {
  const bool inLowerLeft = (x < 80U) && (y > (kVerRes - 80));
  const uint32_t now = millis();

  if (!pressed || !inLowerLeft) {
    gTouchHoldArmed = false;
    gTouchHoldTriggered = false;
    return;
  }

  if (!gTouchHoldArmed) {
    gTouchHoldArmed = true;
    gTouchHoldStartMs = now;
    gTouchHoldTriggered = false;
    return;
  }

  if (!gTouchHoldTriggered && (now - gTouchHoldStartMs) >= kDiagHoldMs) {
    gTouchHoldTriggered = true;
    gDiagScreenActive = !gDiagScreenActive;
    lv_screen_load(gDiagScreenActive ? gDiagScreen : gMainScreen);
  }
}

void drawPanelDiagPattern() {
  const int w = kHorRes;
  const int h = kVerRes;
  const int cell = 16;
  const int x0 = 1;           // Intentionally skip column 0 for isolation.
  const int drawW = w - x0;
  gTft.fillScreen(TFT_BLACK);

  for (int y = 0; y < h; y += cell) {
    for (int x = x0; x < w; x += cell) {
      const bool odd = ((((x - x0) / cell) + (y / cell)) & 1) != 0;
      const int cw = (x + cell > w) ? (w - x) : cell;
      gTft.fillRect(x, y, cw, cell, odd ? TFT_NAVY : TFT_DARKGREY);
    }
  }

  // Draw border in shifted area only.
  gTft.drawRect(5, 4, drawW - 8, h - 8, TFT_WHITE);
  gTft.drawFastVLine(1, 0, h, TFT_GREEN);  // shifted-start marker

  gTft.setTextColor(TFT_CYAN, TFT_BLACK);
  gTft.setTextSize(2);
  gTft.setCursor(12, 10);
  gTft.print("PANEL DIAG STATIC");
  gTft.setCursor(12, 34);
  gTft.print("COL0 SKIPPED");
}

void lvglFlushCb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  const int32_t w = area->x2 - area->x1 + 1;
  const int32_t h = area->y2 - area->y1 + 1;
  gTft.pushImage(area->x1, area->y1, w, h, reinterpret_cast<uint16_t *>(px_map));
  lv_display_flush_ready(disp);
}

void lvglTouchReadCb(lv_indev_t * /* indev */, lv_indev_data_t *data) {
  uint16_t x = 0;
  uint16_t y = 0;
  const bool touched = readTouchPoint(x, y);

  data->state = touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  data->point.x = static_cast<int16_t>(x);
  data->point.y = static_cast<int16_t>(y);

  maybeToggleDiagByHold(x, y, touched);
}

void sendFrame(uint8_t type, const uint8_t *payload, uint8_t len) {
  uint8_t buffer[kMaxPayloadLen + kFrameOverhead];
  const size_t n = encodeFrame(type, gSeq++, payload, len, buffer, sizeof(buffer));
  if (n > 0) Serial1.write(buffer, n);
}

void sendHeartbeatIfDue() {
  const uint32_t now = millis();
  if (now - gLastHeartbeatTxMs < kHeartbeatMs) return;
  gLastHeartbeatTxMs = now;

  uint8_t payload[1] = {1};
  sendFrame(MSG_HEARTBEAT, payload, sizeof(payload));
  gState.hbTxCount++;
}

void onStatusSnapshot(const Frame &frame) {
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
  gState.displayOnline = frame.payload[13] != 0;
  gState.ready = frame.payload[14] != 0;
  gState.lastCommand = frame.payload[15];
  gState.inputMask = static_cast<uint16_t>(frame.payload[16]) |
                     (static_cast<uint16_t>(frame.payload[17]) << 8);

  if (gState.lastCommand != gPrevLoggedCmd && gState.lastCommand != 0xFF) {
    char line[64];
    snprintf(line, sizeof(line), "%lus CMD %s", millis() / 1000UL, commandName(gState.lastCommand));
    pushLog(line);
    gPrevLoggedCmd = gState.lastCommand;
  }

  if (gState.inputMask != gPrevLoggedMask) {
    char line[64];
    snprintf(line, sizeof(line), "%lus MASK 0x%04X", millis() / 1000UL,
             static_cast<unsigned>(gState.inputMask));
    pushLog(line);
    gPrevLoggedMask = gState.inputMask;
  }

  gState.lastStatusRxMs = millis();
  gState.statusRxCount++;
}

void pollMaster() {
  while (Serial1.available() > 0) {
    const uint8_t b = static_cast<uint8_t>(Serial1.read());
    Frame frame;
    if (!gDecoder.feed(b, frame)) continue;

    if (frame.type == MSG_HEARTBEAT) {
      gState.lastMasterHeartbeatMs = millis();
      gState.hbRxCount++;
      if (frame.len >= 2) {
        // Master's view: [0]=input online, [1]=display online.
        gState.inputOnline = frame.payload[0] != 0;
        gState.displayOnline = frame.payload[1] != 0;
      }
    } else if (frame.type == MSG_STATUS_SNAPSHOT) {
      onStatusSnapshot(frame);
    } else if (frame.type == MSG_INPUT_STATE) {
      // Optional direct input frame (if routed in future)
      char line[64];
      uint8_t cmd = (frame.len >= 4) ? frame.payload[3] : 0xFF;
      snprintf(line, sizeof(line), "%lus IN %s", millis() / 1000UL, commandName(cmd));
      pushLog(line);
    }
  }

  gState.masterOnline = (millis() - gState.lastMasterHeartbeatMs) <= kNodeOfflineTimeoutMs;
}

void styleCard(lv_obj_t *obj) {
  lv_obj_set_style_radius(obj, 12, 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(0x2B3D57), 0);
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x142236), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(0xF7FBFF), 0);
}

void stylePod(lv_obj_t *obj) {
  lv_obj_set_style_radius(obj, 34, 0);
  lv_obj_set_style_border_width(obj, 2, 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(0x4E6185), 0);
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x111A2A), 0);
  lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x0A1220), 0);
  lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(0xF7FBFF), 0);
  lv_obj_set_style_pad_all(obj, 8, 0);
}

void styleMapPanel(lv_obj_t *obj) {
  lv_obj_set_style_radius(obj, 16, 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(0x506593), 0);
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x0B1424), 0);
  lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x1B2A45), 0);
  lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(0xF7FBFF), 0);
  lv_obj_set_style_pad_all(obj, 6, 0);
}

void buildUi() {
  gMainScreen = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(gMainScreen, lv_color_hex(0x040811), 0);
  lv_obj_set_style_bg_grad_color(gMainScreen, lv_color_hex(0x0D172A), 0);
  lv_obj_set_style_bg_grad_dir(gMainScreen, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_text_color(gMainScreen, lv_color_hex(0xF7FBFF), 0);

  gHdrTime = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gHdrTime, &lv_font_montserrat_16, 0);
  lv_obj_set_pos(gHdrTime, 16, 10);

  gHdrReady = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gHdrReady, &lv_font_montserrat_16, 0);
  lv_obj_set_pos(gHdrReady, 205, 10);

  gHdrNodes = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gHdrNodes, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gHdrNodes, 310, 12);

  gIndicators = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gIndicators, &lv_font_montserrat_20, 0);
  lv_obj_set_pos(gIndicators, 44, 56);

  gSpeedValue = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gSpeedValue, &lv_font_montserrat_48, 0);
  lv_obj_set_pos(gSpeedValue, 42, 88);

  gSpeedUnit = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gSpeedUnit, &lv_font_montserrat_16, 0);
  lv_label_set_text(gSpeedUnit, "km/t");
  lv_obj_set_style_text_color(gSpeedUnit, lv_color_hex(0xBFD0EA), 0);
  lv_obj_set_pos(gSpeedUnit, 66, 174);

  gGear = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gGear, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(gGear, lv_color_hex(0x9ED0FF), 0);
  lv_obj_set_pos(gGear, 78, 204);

  lv_obj_t *centerMap = lv_obj_create(gMainScreen);
  lv_obj_set_size(centerMap, 224, 214);
  lv_obj_set_pos(centerMap, 128, 52);
  styleMapPanel(centerMap);
  lv_obj_set_style_border_width(centerMap, 0, 0);
  lv_obj_set_style_bg_opa(centerMap, LV_OPA_0, 0);

  // Faux "fade map" lanes.
  for (int i = 0; i < 9; ++i) {
    lv_obj_t *lane = lv_obj_create(centerMap);
    lv_obj_remove_style_all(lane);
    lv_obj_set_size(lane, 1, 158);
    lv_obj_set_pos(lane, 34 + i * 20, 34);
    lv_obj_set_style_bg_color(lane, lv_color_hex(0x2A3E63), 0);
    lv_obj_set_style_bg_opa(lane, (i % 2 == 0) ? LV_OPA_60 : LV_OPA_30, 0);
  }

  lv_obj_t *road = lv_obj_create(centerMap);
  lv_obj_set_size(road, 26, 170);
  lv_obj_set_pos(road, 98, 30);
  lv_obj_set_style_radius(road, 4, 0);
  lv_obj_set_style_border_width(road, 0, 0);
  lv_obj_set_style_bg_color(road, lv_color_hex(0xBFCDEA), 0);
  lv_obj_set_style_bg_grad_color(road, lv_color_hex(0xE6EEFF), 0);
  lv_obj_set_style_bg_grad_dir(road, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(road, LV_OPA_90, 0);

  gMapNavTitle = lv_label_create(centerMap);
  lv_obj_set_style_text_font(gMapNavTitle, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(gMapNavTitle, lv_color_hex(0xDCE8FF), 0);
  lv_label_set_text(gMapNavTitle, "Map: route plugin ready");
  lv_obj_align(gMapNavTitle, LV_ALIGN_TOP_MID, 0, 10);

  gMapRouteInfo = lv_label_create(centerMap);
  lv_obj_set_style_text_font(gMapRouteInfo, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(gMapRouteInfo, lv_color_hex(0xDCE8FF), 0);
  lv_obj_align(gMapRouteInfo, LV_ALIGN_BOTTOM_MID, 0, -8);

  lv_obj_t *poiA = lv_obj_create(centerMap);
  lv_obj_set_size(poiA, 54, 24);
  lv_obj_set_pos(poiA, 20, 92);
  lv_obj_set_style_radius(poiA, 12, 0);
  lv_obj_set_style_bg_color(poiA, lv_color_hex(0x5B84C6), 0);
  lv_obj_set_style_border_width(poiA, 0, 0);
  lv_obj_t *poiATxt = lv_label_create(poiA);
  lv_obj_set_style_text_font(poiATxt, &lv_font_montserrat_14, 0);
  lv_label_set_text(poiATxt, "A");
  lv_obj_center(poiATxt);

  lv_obj_t *poiB = lv_obj_create(centerMap);
  lv_obj_set_size(poiB, 54, 24);
  lv_obj_set_pos(poiB, 148, 120);
  lv_obj_set_style_radius(poiB, 12, 0);
  lv_obj_set_style_bg_color(poiB, lv_color_hex(0xB96565), 0);
  lv_obj_set_style_border_width(poiB, 0, 0);
  lv_obj_t *poiBTxt = lv_label_create(poiB);
  lv_obj_set_style_text_font(poiBTxt, &lv_font_montserrat_14, 0);
  lv_label_set_text(poiBTxt, "B");
  lv_obj_center(poiBTxt);

  gPowerUsedLabel = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gPowerUsedLabel, &lv_font_montserrat_48, 0);
  lv_obj_set_pos(gPowerUsedLabel, 370, 98);

  gPowerAskLabel = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gPowerAskLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(gPowerAskLabel, lv_color_hex(0xBFD0EA), 0);
  lv_obj_set_pos(gPowerAskLabel, 387, 178);

  // Slim side bars hugging the screen edges.
  gPowerAskBar = lv_bar_create(gMainScreen);
  lv_obj_set_size(gPowerAskBar, 7, 206);
  lv_obj_set_pos(gPowerAskBar, 471, 56);
  lv_bar_set_range(gPowerAskBar, 0, 100);
  lv_obj_set_style_radius(gPowerAskBar, 0, LV_PART_MAIN | LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(gPowerAskBar, lv_color_hex(0x12243D), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(gPowerAskBar, LV_OPA_60, LV_PART_MAIN);
  lv_obj_set_style_bg_color(gPowerAskBar, lv_color_hex(0x59D7FF), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(gPowerAskBar, LV_OPA_90, LV_PART_INDICATOR);

  gSocIcon = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gSocIcon, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(gSocIcon, lv_color_hex(0x8CEAA3), 0);
  lv_label_set_text(gSocIcon, LV_SYMBOL_BATTERY_FULL);
  lv_obj_set_pos(gSocIcon, 94, 10);

  gSocPct = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gSocPct, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(gSocPct, 116, 12);

  gPowerUsedBar = lv_bar_create(gMainScreen);
  lv_obj_set_size(gPowerUsedBar, 84, 8);
  lv_obj_set_pos(gPowerUsedBar, 356, 214);
  lv_bar_set_range(gPowerUsedBar, 0, 100);
  lv_obj_set_style_bg_color(gPowerUsedBar, lv_color_hex(0x1A2B43), LV_PART_MAIN);
  lv_obj_set_style_bg_color(gPowerUsedBar, lv_color_hex(0xF6C35A), LV_PART_INDICATOR);
  lv_obj_set_style_radius(gPowerUsedBar, 7, LV_PART_MAIN | LV_PART_INDICATOR);

  gFooterTrip = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gFooterTrip, &lv_font_montserrat_16, 0);
  lv_obj_set_pos(gFooterTrip, 18, 292);

  gFooterSince = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gFooterSince, &lv_font_montserrat_16, 0);
  lv_obj_set_pos(gFooterSince, 170, 292);

  gFooterHint = lv_label_create(gMainScreen);
  lv_obj_set_style_text_font(gFooterHint, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(gFooterHint, lv_color_hex(0xE9F2FF), 0);
  lv_label_set_text(gFooterHint, "hold LL for diag");
  lv_obj_set_pos(gFooterHint, 344, 294);

  gDiagScreen = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(gDiagScreen, lv_color_hex(0x111111), 0);
  lv_obj_set_style_text_color(gDiagScreen, lv_color_hex(0xF2F2F2), 0);

  lv_obj_t *diagTitle = lv_label_create(gDiagScreen);
  lv_obj_set_style_text_font(diagTitle, &lv_font_montserrat_14, 0);
  lv_label_set_text(diagTitle, "Diagnostics (hold lower-left to exit)");
  lv_obj_align(diagTitle, LV_ALIGN_TOP_LEFT, 10, 6);

  gDiagComms = lv_label_create(gDiagScreen);
  lv_obj_set_style_text_font(gDiagComms, &lv_font_montserrat_14, 0);
  lv_obj_set_width(gDiagComms, 460);
  lv_label_set_long_mode(gDiagComms, LV_LABEL_LONG_WRAP);
  lv_obj_align(gDiagComms, LV_ALIGN_TOP_LEFT, 10, 38);

  gDiagPorts = lv_label_create(gDiagScreen);
  lv_obj_set_style_text_font(gDiagPorts, &lv_font_montserrat_14, 0);
  lv_obj_set_width(gDiagPorts, 460);
  lv_label_set_long_mode(gDiagPorts, LV_LABEL_LONG_WRAP);
  lv_obj_align(gDiagPorts, LV_ALIGN_TOP_LEFT, 10, 112);

  gDiagLog = lv_label_create(gDiagScreen);
  lv_obj_set_style_text_font(gDiagLog, &lv_font_montserrat_14, 0);
  lv_obj_set_width(gDiagLog, 460);
  lv_label_set_long_mode(gDiagLog, LV_LABEL_LONG_WRAP);
  lv_obj_align(gDiagLog, LV_ALIGN_TOP_LEFT, 10, 196);

  lv_screen_load(gMainScreen);
}

void refreshUi() {
  const uint32_t hbAge = millis() - gState.lastMasterHeartbeatMs;
  const uint32_t stAge = millis() - gState.lastStatusRxMs;
  const uint32_t totalMin = (millis() / 60000UL) + (12UL * 60UL + 34UL);
  const unsigned hh = static_cast<unsigned>((totalMin / 60) % 24);
  const unsigned mm = static_cast<unsigned>(totalMin % 60);
  lv_label_set_text_fmt(gHdrTime, "%02u:%02u", hh, mm);

  lv_label_set_text(gHdrReady, gState.ready ? "READY" : "WAIT");
  lv_obj_set_style_text_color(gHdrReady, gState.ready ? lv_color_hex(0x84F6A4) : lv_color_hex(0xF5D26A), 0);

  lv_label_set_text_fmt(gHdrNodes, "M %s  IN %s  DSP %s",
                        gState.masterOnline ? "ON" : "OFF",
                        gState.inputOnline ? "ON" : "OFF",
                        gState.displayOnline ? "ON" : "OFF");
  lv_obj_set_style_text_color(gHdrNodes, gState.masterOnline ? lv_color_hex(0xF2F7FF) : lv_color_hex(0xFFB0B0), 0);

  lv_label_set_text_fmt(gSocPct, "%u%%", static_cast<unsigned>(gState.socPct));
  lv_label_set_text(gSocIcon,
                    (gState.socPct >= 80) ? LV_SYMBOL_BATTERY_FULL
                    : (gState.socPct >= 60) ? LV_SYMBOL_BATTERY_3
                    : (gState.socPct >= 40) ? LV_SYMBOL_BATTERY_2
                    : (gState.socPct >= 20) ? LV_SYMBOL_BATTERY_1
                                            : LV_SYMBOL_BATTERY_EMPTY);
  lv_obj_set_style_text_color(gSocIcon,
                              (gState.socPct >= 35) ? lv_color_hex(0x8CEAA3) : lv_color_hex(0xFF8A8A),
                              0);

  lv_label_set_text_fmt(gSpeedValue, "%u", static_cast<unsigned>(gState.speedKmt));
  lv_label_set_text(gGear, gearName(gState.gear));
  lv_label_set_text_fmt(gIndicators, "%s  %s",
                        (gState.indicatorBits & IND_LEFT) ? LV_SYMBOL_LEFT : "-",
                        (gState.indicatorBits & IND_RIGHT) ? LV_SYMBOL_RIGHT : "-");
  lv_obj_set_style_text_color(gIndicators,
                              (gState.indicatorBits & (IND_LEFT | IND_RIGHT | IND_HAZARD))
                                  ? lv_color_hex(0xFFE178)
                                  : lv_color_hex(0xB7C7DB),
                              0);

  const uint8_t usedAbs = static_cast<uint8_t>(abs(gState.powerUsedPct));
  lv_label_set_text_fmt(gPowerUsedLabel, "%u", static_cast<unsigned>(usedAbs));
  lv_obj_set_style_text_color(gPowerUsedLabel,
                              (gState.powerUsedPct < 0) ? lv_color_hex(0x8FF0AB) : lv_color_hex(0xF8F3FF),
                              0);
  lv_label_set_text(gPowerAskLabel, "Power");
  lv_bar_set_value(gPowerAskBar, gState.powerAskedPct, LV_ANIM_ON);

  lv_bar_set_value(gPowerUsedBar, usedAbs, LV_ANIM_ON);
  lv_obj_set_style_bg_color(gPowerUsedBar,
                            (gState.powerUsedPct < 0) ? lv_color_hex(0x5BE584) : lv_color_hex(0xF6C35A),
                            LV_PART_INDICATOR);

  lv_label_set_text_fmt(gFooterTrip, "Trip %u.%ukm",
                        static_cast<unsigned>(gState.tripTotalTenthsKm / 10),
                        static_cast<unsigned>(gState.tripTotalTenthsKm % 10));
  lv_label_set_text_fmt(gFooterSince, "Since %u.%ukm",
                        static_cast<unsigned>(gState.tripSinceChargeTenthsKm / 10),
                        static_cast<unsigned>(gState.tripSinceChargeTenthsKm % 10));

  lv_label_set_text_fmt(gMapRouteInfo, "Route: %s   HB:%lums",
                        gState.masterOnline ? "link online" : "waiting for master", hbAge);

  lv_label_set_text_fmt(
      gDiagComms,
      "Comms\n"
      " master=%s hbAge=%lums statusAge=%lums\n"
      " hbRx=%lu hbTx=%lu statusRx=%lu\n"
      " ready=%s ignition=%s\n"
      " inputNode=%s displayNode=%s",
      gState.masterOnline ? "ONLINE" : "OFFLINE", hbAge, stAge,
      gState.hbRxCount, gState.hbTxCount, gState.statusRxCount,
      gState.ready ? "YES" : "NO", gState.ignitionOn ? "ON" : "OFF",
      gState.inputOnline ? "ON" : "OFF", gState.displayOnline ? "ON" : "OFF");

  const bool drl = (gState.lightBits & LIGHT_DRL) != 0;
  const bool near = (gState.lightBits & LIGHT_NEAR) != 0;
  const bool high = (gState.lightBits & LIGHT_HIGH) != 0;
  const bool brake = (gState.lightBits & LIGHT_BRAKE) != 0;
  const bool indL = (gState.indicatorBits & IND_LEFT) != 0;
  const bool indR = (gState.indicatorBits & IND_RIGHT) != 0;
  const bool haz = (gState.indicatorBits & IND_HAZARD) != 0;

  lv_label_set_text_fmt(
      gDiagPorts,
      "Main Outputs (from status snapshot)\n"
      " DRL=%s NEAR=%s HIGH=%s BRAKE=%s\n"
      " IND_L=%s IND_R=%s HAZ=%s WIPER_MODE=%u\n"
      " lastCmd=%s inputMask=0x%04X",
      drl ? "ON" : "OFF", near ? "ON" : "OFF", high ? "ON" : "OFF", brake ? "ON" : "OFF",
      indL ? "ON" : "OFF", indR ? "ON" : "OFF", haz ? "ON" : "OFF",
      static_cast<unsigned>(gState.wiperMode), commandName(gState.lastCommand),
      static_cast<unsigned>(gState.inputMask));

  char logBuf[640];
  size_t off = 0;
  off += snprintf(logBuf + off, sizeof(logBuf) - off, "Input/Event log (latest last)\n");
  const size_t first = (gLogCount < kLogLines) ? 0 : gLogHead;
  for (size_t i = 0; i < gLogCount; ++i) {
    const size_t idx = (first + i) % kLogLines;
    off += snprintf(logBuf + off, sizeof(logBuf) - off, "%s\n", gEventLog[idx]);
    if (off >= sizeof(logBuf) - 2) break;
  }
  lv_label_set_text(gDiagLog, logBuf);
}

} // namespace

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, kMasterUartRxPin, kMasterUartTxPin);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(kBacklightPin, OUTPUT);
  digitalWrite(kBacklightPin, kBacklightOnLevel);

  Wire.begin(kTouchSdaPin, kTouchSclPin);

  gTft.init();
  gTft.setRotation(kScreenRotation);

  if (kPanelDiagMode) {
    drawPanelDiagPattern();
    return;
  }

  lv_init();

  gDisplay = lv_display_create(kHorRes, kVerRes);
  lv_display_set_flush_cb(gDisplay, lvglFlushCb);
  lv_display_set_buffers(gDisplay, gLvBuf, nullptr,
                         sizeof(gLvBuf), LV_DISPLAY_RENDER_MODE_PARTIAL);

  gTouchIndev = lv_indev_create();
  lv_indev_set_type(gTouchIndev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(gTouchIndev, lvglTouchReadCb);

  buildUi();
  pushLog("boot: lvgl display target started");
}

void loop() {
  if (kPanelDiagMode) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(5);
    return;
  }

  static uint32_t lastTickMs = millis();
  static uint32_t lastUiMs = 0;

  const uint32_t now = millis();
  const uint32_t delta = now - lastTickMs;
  lastTickMs = now;

  lv_tick_inc(delta);
  pollMaster();
  sendHeartbeatIfDue();

  if (now - lastUiMs >= 100) {
    lastUiMs = now;
    refreshUi();
  }

  lv_timer_handler();
  digitalWrite(LED_BUILTIN, gState.masterOnline ? HIGH : LOW);
  delay(5);
}
