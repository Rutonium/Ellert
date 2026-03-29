#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <PNGdec.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <math.h>
#include <stdlib.h>

#include "board_config.h"
#include "protocol_v1.h"
#if __has_include("wifi_local.h")
#include "wifi_local.h"
#define ELLERT_DISPLAY_WIFI_LOCAL_AVAILABLE 1
#else
#define ELLERT_DISPLAY_WIFI_LOCAL_AVAILABLE 0
#endif

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
uint32_t gLastDebugMs = 0;
uint32_t gLastRenderMs = 0;
uint32_t gHbTxCount = 0;
uint32_t gHbRxCount = 0;
uint32_t gStatusRxCount = 0;
constexpr bool kShowLinkTestScreen = false;
constexpr bool kShowMapScreen = true;
constexpr bool kEnableSerialDebug = false;
constexpr uint32_t kRenderPeriodMs = 33; // ~30 FPS cap to reduce flicker/power spikes.
constexpr uint32_t kMapAnimPeriodMs = 120;
constexpr float kSmoothSpeedAlpha = 0.25f;
constexpr float kSmoothPowerAlpha = 0.30f;
constexpr float kEarthRadiusM = 6378137.0f;
constexpr size_t kMapTrailCapacity = 40;
constexpr uint32_t kWifiStatusPeriodMs = 5000;
constexpr uint32_t kTileFetchPeriodMs = 1200;
constexpr int kTileSizePx = 256;

enum class MapTileStyle : uint8_t {
  kNative = 0,
  kDark = 1,
  kGray = 2,
  kDarkGray = 3,
};

// Default tile style to improve contrast and avoid bright map glare on the cluster screen.
constexpr MapTileStyle kMapTileStyle = MapTileStyle::kGray;

bool gUiDirty = true;
bool gLastBlinkPhase = false;
uint32_t gLastMapAnimMs = 0;
uint32_t gLastTrailSampleMs = 0;
uint32_t gLastWifiStatusMs = 0;
uint32_t gLastTileFetchMs = 0;
float gSmoothSpeed = 0.0f;
float gSmoothPowerAsked = 0.0f;
float gSmoothPowerUsed = 0.0f;
bool gMapOriginValid = false;
int32_t gMapOriginLatE7 = 0;
int32_t gMapOriginLonE7 = 0;
float gTrailX[kMapTrailCapacity] = {0.0f};
float gTrailY[kMapTrailCapacity] = {0.0f};
size_t gTrailCount = 0;
size_t gTrailHead = 0;

struct CachedTile {
  uint8_t z = 0;
  int32_t x = 0;
  int32_t y = 0;
  bool valid = false;
  uint16_t *pixels = nullptr;
};

CachedTile gTiles[9];
PNG gTilePng;
uint16_t gPngLineBuffer[kTileSizePx];
bool gTileFetchEnabled = ELLERT_DISPLAY_WIFI_LOCAL_AVAILABLE;
bool gTileAnyReady = false;
bool gTileFsReady = false;
int32_t gCenterTileX = 0;
int32_t gCenterTileY = 0;
float gCenterTileFracX = 0.0f;
float gCenterTileFracY = 0.0f;
uint8_t gMapZoomActive = 17;

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
  int32_t latE7 = 0;
  int32_t lonE7 = 0;
  uint16_t headingCdeg = 0;
  bool gpsFix = false;
  uint8_t gpsSats = 0;
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
  ++gHbTxCount;
}

float radiansf(float deg) {
  return deg * 0.01745329252f;
}

void mapMetersFromLatLonE7(int32_t latE7, int32_t lonE7, float &outX, float &outY) {
  if (!gMapOriginValid) {
    gMapOriginValid = true;
    gMapOriginLatE7 = latE7;
    gMapOriginLonE7 = lonE7;
  }

  const float lat0 = static_cast<float>(gMapOriginLatE7) / 10000000.0f;
  const float lon0 = static_cast<float>(gMapOriginLonE7) / 10000000.0f;
  const float lat = static_cast<float>(latE7) / 10000000.0f;
  const float lon = static_cast<float>(lonE7) / 10000000.0f;
  const float dLatRad = radiansf(lat - lat0);
  const float dLonRad = radiansf(lon - lon0);
  const float lat0Rad = radiansf(lat0);

  outX = dLonRad * cosf(lat0Rad) * kEarthRadiusM;
  outY = dLatRad * kEarthRadiusM;
}

void appendTrailPoint(float x, float y) {
  if (gTrailCount < kMapTrailCapacity) {
    gTrailX[gTrailCount] = x;
    gTrailY[gTrailCount] = y;
    ++gTrailCount;
    return;
  }
  gTrailX[gTrailHead] = x;
  gTrailY[gTrailHead] = y;
  gTrailHead = (gTrailHead + 1) % kMapTrailCapacity;
}

void ensureTileBuffers() {
  for (size_t i = 0; i < 9; ++i) {
    if (gTiles[i].pixels == nullptr) {
      gTiles[i].pixels = static_cast<uint16_t *>(malloc(kTileSizePx * kTileSizePx * sizeof(uint16_t)));
      if (gTiles[i].pixels == nullptr) {
        gTileFetchEnabled = false;
        Serial.println("DISPLAY_TILE_OOM");
        return;
      }
    }
  }
}

bool computeTileCoords(int32_t latE7, int32_t lonE7, uint8_t zoom, int32_t &tileX, int32_t &tileY,
                       float &fracX, float &fracY) {
  const float lat = static_cast<float>(latE7) / 10000000.0f;
  const float lon = static_cast<float>(lonE7) / 10000000.0f;
  if (!isfinite(lat) || !isfinite(lon)) return false;

  const float clampLat = fmaxf(-85.0511f, fminf(85.0511f, lat));
  const float n = static_cast<float>(1UL << zoom);
  const float x = ((lon + 180.0f) / 360.0f) * n;
  const float latRad = radiansf(clampLat);
  const float y = (1.0f - logf(tanf(latRad) + (1.0f / cosf(latRad))) / 3.14159265359f) * 0.5f * n;
  if (!isfinite(x) || !isfinite(y)) return false;

  tileX = static_cast<int32_t>(floorf(x));
  tileY = static_cast<int32_t>(floorf(y));
  fracX = x - static_cast<float>(tileX);
  fracY = y - static_cast<float>(tileY);
  return true;
}

int tileIndexFromOffset(int ox, int oy) {
  return (oy + 1) * 3 + (ox + 1);
}

String tilePath(uint8_t z, int32_t x, int32_t y) {
  return String("/tiles/") + z + "/" + x + "/" + y + ".png";
}

bool ensureTileDirTree(uint8_t z, int32_t x) {
  if (!gTileFsReady) return false;
  const String d0 = "/tiles";
  const String d1 = String("/tiles/") + z;
  const String d2 = String("/tiles/") + z + "/" + x;
  if (!LittleFS.exists(d0)) LittleFS.mkdir(d0);
  if (!LittleFS.exists(d1)) LittleFS.mkdir(d1);
  if (!LittleFS.exists(d2)) LittleFS.mkdir(d2);
  return true;
}

struct TileDecodeCtx {
  uint16_t *dest;
};

int tilePngDraw(PNGDRAW *pDraw);

uint16_t packRgb565(uint8_t r8, uint8_t g8, uint8_t b8) {
  return static_cast<uint16_t>(((r8 & 0xF8) << 8) | ((g8 & 0xFC) << 3) | (b8 >> 3));
}

uint16_t styleMapTilePixel(uint16_t px) {
  uint8_t r = static_cast<uint8_t>((px >> 11) & 0x1F);
  uint8_t g = static_cast<uint8_t>((px >> 5) & 0x3F);
  uint8_t b = static_cast<uint8_t>(px & 0x1F);

  // Expand to 8-bit channels for style operations.
  uint8_t r8 = static_cast<uint8_t>((r << 3) | (r >> 2));
  uint8_t g8 = static_cast<uint8_t>((g << 2) | (g >> 4));
  uint8_t b8 = static_cast<uint8_t>((b << 3) | (b >> 2));

  if (kMapTileStyle == MapTileStyle::kGray || kMapTileStyle == MapTileStyle::kDarkGray) {
    const uint8_t y = static_cast<uint8_t>((static_cast<uint16_t>(r8) * 30U +
                                            static_cast<uint16_t>(g8) * 59U +
                                            static_cast<uint16_t>(b8) * 11U) / 100U);
    r8 = y;
    g8 = y;
    b8 = y;
  }

  if (kMapTileStyle == MapTileStyle::kDark || kMapTileStyle == MapTileStyle::kDarkGray) {
    r8 = static_cast<uint8_t>((static_cast<uint16_t>(r8) * 60U) / 100U);
    g8 = static_cast<uint8_t>((static_cast<uint16_t>(g8) * 60U) / 100U);
    b8 = static_cast<uint8_t>((static_cast<uint16_t>(b8) * 70U) / 100U);
  }

  return packRgb565(r8, g8, b8);
}

bool decodePngToTilePixels(const uint8_t *pngData, const size_t len, uint16_t *outPixels) {
  if (!pngData || len == 0 || !outPixels) return false;
  TileDecodeCtx ctx = {outPixels};
  const int rc = gTilePng.openRAM(const_cast<uint8_t *>(pngData), static_cast<int>(len), tilePngDraw);
  bool ok = false;
  if (rc == PNG_SUCCESS && gTilePng.getWidth() == kTileSizePx && gTilePng.getHeight() == kTileSizePx) {
    ok = (gTilePng.decode(&ctx, 0) == PNG_SUCCESS);
  }
  gTilePng.close();
  return ok;
}

bool loadTileFromCache(uint8_t z, int32_t x, int32_t y, uint16_t *outPixels) {
  if (!gTileFsReady || !outPixels) return false;
  const String path = tilePath(z, x, y);
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  const size_t len = static_cast<size_t>(f.size());
  if (len == 0 || len > 180000) {
    f.close();
    return false;
  }
  uint8_t *pngData = static_cast<uint8_t *>(malloc(len));
  if (!pngData) {
    f.close();
    return false;
  }
  const size_t got = f.read(pngData, len);
  f.close();
  if (got != len) {
    free(pngData);
    return false;
  }
  const bool ok = decodePngToTilePixels(pngData, len, outPixels);
  free(pngData);
  if (ok) {
    Serial.print("DISPLAY_TILE_CACHE_HIT z=");
    Serial.print(z);
    Serial.print(" x=");
    Serial.print(x);
    Serial.print(" y=");
    Serial.println(y);
  }
  return ok;
}

bool saveTileToCache(uint8_t z, int32_t x, int32_t y, const uint8_t *pngData, const size_t len) {
  if (!gTileFsReady || !pngData || len == 0) return false;
  if (!ensureTileDirTree(z, x)) return false;
  const String path = tilePath(z, x, y);
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  const size_t wr = f.write(pngData, len);
  f.close();
  return wr == len;
}

void pollDisplayWifiStatus() {
#if ELLERT_DISPLAY_WIFI_LOCAL_AVAILABLE
  const uint32_t now = millis();
  if (now - gLastWifiStatusMs < kWifiStatusPeriodMs) return;
  gLastWifiStatusMs = now;
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("DISPLAY_WIFI_OK ip=");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("DISPLAY_WIFI_WAIT status=");
    Serial.println(static_cast<int>(WiFi.status()));
    WiFi.disconnect(false, false);
    WiFi.begin(kWifiSsid, kWifiPassword);
  }
#endif
}

int tilePngDraw(PNGDRAW *pDraw) {
  TileDecodeCtx *ctx = static_cast<TileDecodeCtx *>(pDraw->pUser);
  if (!ctx || !ctx->dest || pDraw->y < 0 || pDraw->y >= kTileSizePx) return 0;
  gTilePng.getLineAsRGB565(pDraw, gPngLineBuffer, PNG_RGB565_LITTLE_ENDIAN, 0x00000000);
  for (int i = 0; i < pDraw->iWidth; ++i) {
    gPngLineBuffer[i] = styleMapTilePixel(gPngLineBuffer[i]);
  }
  memcpy(ctx->dest + (pDraw->y * kTileSizePx), gPngLineBuffer, pDraw->iWidth * sizeof(uint16_t));
  return 1;
}

bool fetchAndDecodeTile(uint8_t z, int32_t tileX, int32_t tileY, uint16_t *outPixels) {
#if !ELLERT_DISPLAY_WIFI_LOCAL_AVAILABLE
  (void)z;
  (void)tileX;
  (void)tileY;
  (void)outPixels;
  return false;
#else
  if (!outPixels) return false;
  if (loadTileFromCache(z, tileX, tileY, outPixels)) return true;
  if (WiFi.status() != WL_CONNECTED) return false;
  const int32_t maxCoord = (1L << z);
  if (tileX < 0 || tileY < 0 || tileX >= maxCoord || tileY >= maxCoord) return false;

  const String url = String("https://tile.openstreetmap.org/") + z + "/" + tileX + "/" + tileY + ".png";
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setUserAgent("Ellert-Display/0.2 (tile prototype, respect OSM policy)");
  if (!http.begin(client, url)) return false;

  const int code = http.GET();
  const int len = http.getSize();
  if (code != HTTP_CODE_OK || len <= 0 || len > 180000) {
    http.end();
    return false;
  }

  uint8_t *pngData = static_cast<uint8_t *>(malloc(static_cast<size_t>(len)));
  if (!pngData) {
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();
  size_t total = 0;
  const uint32_t startMs = millis();
  while (http.connected() && total < static_cast<size_t>(len) && (millis() - startMs) < 7000UL) {
    const size_t avail = stream->available();
    if (avail == 0) {
      delay(1);
      continue;
    }
    const size_t need = static_cast<size_t>(len) - total;
    const size_t chunk = avail < need ? avail : need;
    const int got = stream->readBytes(pngData + total, chunk);
    if (got > 0) total += static_cast<size_t>(got);
  }
  http.end();
  if (total != static_cast<size_t>(len)) {
    free(pngData);
    return false;
  }

  const bool ok = decodePngToTilePixels(pngData, len, outPixels);
  if (ok) {
    saveTileToCache(z, tileX, tileY, pngData, static_cast<size_t>(len));
  }
  free(pngData);
  return ok;
#endif
}

void updateTileCache() {
  if (!gTileFetchEnabled || !gState.gpsFix) return;
  const uint32_t now = millis();
  if (now - gLastTileFetchMs < kTileFetchPeriodMs) return;
  gLastTileFetchMs = now;

  int32_t centerX = 0;
  int32_t centerY = 0;
  float fracX = 0.0f;
  float fracY = 0.0f;
  if (!computeTileCoords(gState.latE7, gState.lonE7, gMapZoomActive, centerX, centerY, fracX, fracY)) return;

  gCenterTileX = centerX;
  gCenterTileY = centerY;
  gCenterTileFracX = fracX;
  gCenterTileFracY = fracY;

  for (int oy = -1; oy <= 1; ++oy) {
    for (int ox = -1; ox <= 1; ++ox) {
      const int idx = tileIndexFromOffset(ox, oy);
      CachedTile &slot = gTiles[idx];
      const int32_t tx = centerX + ox;
      const int32_t ty = centerY + oy;
      if (slot.valid && slot.z == gMapZoomActive && slot.x == tx && slot.y == ty) continue;
      if (fetchAndDecodeTile(gMapZoomActive, tx, ty, slot.pixels)) {
        slot.z = gMapZoomActive;
        slot.x = tx;
        slot.y = ty;
        slot.valid = true;
        gTileAnyReady = true;
        Serial.print("DISPLAY_TILE_OK z=");
        Serial.print(gMapZoomActive);
        Serial.print(" x=");
        Serial.print(tx);
        Serial.print(" y=");
        Serial.println(ty);
        gUiDirty = true;
      } else {
        slot.valid = false;
        Serial.print("DISPLAY_TILE_FAIL z=");
        Serial.print(gMapZoomActive);
        Serial.print(" x=");
        Serial.print(tx);
        Serial.print(" y=");
        Serial.println(ty);
      }
      return; // fetch one tile per cycle to keep UI responsive
    }
  }
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
  gState.latE7 = static_cast<int32_t>(frame.payload[18]) |
                 (static_cast<int32_t>(frame.payload[19]) << 8) |
                 (static_cast<int32_t>(frame.payload[20]) << 16) |
                 (static_cast<int32_t>(frame.payload[21]) << 24);
  gState.lonE7 = static_cast<int32_t>(frame.payload[22]) |
                 (static_cast<int32_t>(frame.payload[23]) << 8) |
                 (static_cast<int32_t>(frame.payload[24]) << 16) |
                 (static_cast<int32_t>(frame.payload[25]) << 24);
  gState.headingCdeg = static_cast<uint16_t>(frame.payload[26]) |
                       (static_cast<uint16_t>(frame.payload[27]) << 8);
  gState.gpsFix = frame.payload[28] != 0;
  gState.gpsSats = frame.payload[29];
  gMapZoomActive = frame.payload[30] <= 19 ? frame.payload[30] : 15;

  if (gState.gpsFix) {
    float x = 0.0f;
    float y = 0.0f;
    mapMetersFromLatLonE7(gState.latE7, gState.lonE7, x, y);
    const uint32_t now = millis();
    if ((now - gLastTrailSampleMs) > 500UL) {
      gLastTrailSampleMs = now;
      appendTrailPoint(x, y);
    }
  }
  gLastStatusRxMs = millis();
  gUiDirty = true;
}

void pollMaster() {
  while (Serial1.available() > 0) {
    const uint8_t b = static_cast<uint8_t>(Serial1.read());
    Frame frame;
    if (!gDecoder.feed(b, frame)) continue;
    if (frame.type == MSG_HEARTBEAT) {
      gLastMasterHeartbeatMs = millis();
      ++gHbRxCount;
    } else if (frame.type == MSG_STATUS_SNAPSHOT) {
      applyStatusSnapshot(frame);
      ++gStatusRxCount;
    }
  }
}

void debugIfDue() {
  if (!kEnableSerialDebug) return;
  const uint32_t now = millis();
  if (now - gLastDebugMs < 1000) return;
  gLastDebugMs = now;
  Serial.print("DISPLAY_LINK rxPin=");
  Serial.print(kMasterUartRxPin);
  Serial.print(" txPin=");
  Serial.print(kMasterUartTxPin);
  Serial.print(" hbTx=");
  Serial.print(gHbTxCount);
  Serial.print(" hbRx=");
  Serial.print(gHbRxCount);
  Serial.print(" statusRx=");
  Serial.println(gStatusRxCount);
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
  const uint8_t speedShown = static_cast<uint8_t>(gSmoothSpeed + 0.5f);
  const uint8_t powerAskShown = static_cast<uint8_t>(gSmoothPowerAsked + 0.5f);
  const int8_t powerUsedShown = static_cast<int8_t>(gSmoothPowerUsed);

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
  snprintf(speedBuf, sizeof(speedBuf), "%u", static_cast<unsigned>(speedShown));
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

  int askH = map(powerAskShown, 0, 100, 0, 118);
  int usedAbs = powerUsedShown < 0 ? -powerUsedShown : powerUsedShown;
  int usedH = map(usedAbs, 0, 100, 0, 118);
  gGfx->drawRect(356, 118, 32, 122, RGB565_WHITE);
  gGfx->drawRect(408, 118, 32, 122, RGB565_WHITE);
  gGfx->fillRect(358, 120, 28, 118, RGB565_BLACK);
  gGfx->fillRect(410, 120, 28, 118, RGB565_BLACK);
  gGfx->fillRect(358, 238 - askH, 28, askH, RGB565_CYAN);
  gGfx->fillRect(410, 238 - usedH, 28, usedH, (powerUsedShown < 0) ? RGB565_GREEN : 0xFD20);
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

void drawLinkTestScreen() {
  const uint32_t now = millis();
  const uint32_t hbAge = now - gLastMasterHeartbeatMs;
  const uint32_t stAge = now - gLastStatusRxMs;
  const bool hbOk = hbAge <= kNodeOfflineTimeoutMs;
  const bool stOk = stAge <= 1500;

  gGfx->fillScreen(RGB565_BLACK);
  gGfx->setTextColor(RGB565_WHITE);
  gGfx->setTextSize(3);
  gGfx->setCursor(20, 20);
  gGfx->print("LINK TEST");

  gGfx->setTextSize(3);
  gGfx->setCursor(20, 80);
  gGfx->print("MASTER HB:");
  gGfx->setTextColor(hbOk ? RGB565_GREEN : RGB565_RED);
  gGfx->setCursor(280, 80);
  gGfx->print(hbOk ? "OK" : "NO");

  gGfx->setTextColor(RGB565_WHITE);
  gGfx->setCursor(20, 130);
  gGfx->print("STATUS:");
  gGfx->setTextColor(stOk ? RGB565_GREEN : RGB565_RED);
  gGfx->setCursor(280, 130);
  gGfx->print(stOk ? "OK" : "NO");

  gGfx->setTextColor(RGB565_WHITE);
  gGfx->setTextSize(2);
  gGfx->setCursor(20, 190);
  gGfx->print("HB_RX: ");
  gGfx->print(gHbRxCount);
  gGfx->setCursor(20, 220);
  gGfx->print("HB_TX: ");
  gGfx->print(gHbTxCount);
  gGfx->setCursor(20, 250);
  gGfx->print("ST_RX: ");
  gGfx->print(gStatusRxCount);
  gGfx->setCursor(260, 190);
  gGfx->print("HB AGE");
  gGfx->setCursor(260, 212);
  gGfx->print(hbAge);
  gGfx->print("ms");
  gGfx->setCursor(260, 242);
  gGfx->print("ST AGE");
  gGfx->setCursor(260, 264);
  gGfx->print(stAge);
  gGfx->print("ms");

  gGfx->flush();
}

void drawMapScreen() {
  const int W = gGfx->width();
  const int H = gGfx->height();
  const int topBarH = 40;
  const int bottomBarH = 34;
  const int mapTop = topBarH + 4;
  const int mapBottom = H - bottomBarH - 4;
  const int mapHeight = mapBottom - mapTop;
  const uint32_t now = millis();

  const uint8_t speedShown = static_cast<uint8_t>(gSmoothSpeed + 0.5f);
  const uint8_t powerAskShown = static_cast<uint8_t>(gSmoothPowerAsked + 0.5f);
  const int8_t powerUsedShown = static_cast<int8_t>(gSmoothPowerUsed);
  const bool hbOk = (now - gLastMasterHeartbeatMs) <= kNodeOfflineTimeoutMs;
  const bool stOk = (now - gLastStatusRxMs) <= 1500;

  gGfx->fillScreen(0x0020);
  gGfx->fillRect(0, mapTop, W, mapHeight, 0x0008);

  const int spacing = 40;
  float mapX = 0.0f;
  float mapY = 0.0f;
  if (gState.gpsFix) {
    mapMetersFromLatLonE7(gState.latE7, gState.lonE7, mapX, mapY);
  }
  const float mapPxPerMeter = 0.55f;

  const int cx = W / 2;
  const int cy = mapTop + mapHeight / 2;
  bool drewTiles = false;
  if (gTileAnyReady) {
    const int originX = cx - static_cast<int>(gCenterTileFracX * kTileSizePx);
    const int originY = cy - static_cast<int>(gCenterTileFracY * kTileSizePx);
    for (int oy = -1; oy <= 1; ++oy) {
      for (int ox = -1; ox <= 1; ++ox) {
        const CachedTile &slot = gTiles[tileIndexFromOffset(ox, oy)];
        if (!slot.valid || slot.z != gMapZoomActive || slot.pixels == nullptr) continue;
        const int drawX = originX + ox * kTileSizePx;
        const int drawY = originY + oy * kTileSizePx;
        gGfx->draw16bitRGBBitmap(drawX, drawY, slot.pixels, kTileSizePx, kTileSizePx);
        drewTiles = true;
      }
    }
  }

  if (!drewTiles) {
    const float fallbackAnim = static_cast<float>((now / 35UL + speedShown * 3U) % spacing);
    float offsetX = fmodf(-mapX * mapPxPerMeter, static_cast<float>(spacing));
    float offsetY = fmodf(-mapY * mapPxPerMeter, static_cast<float>(spacing));
    if (!gState.gpsFix) {
      offsetX = fallbackAnim;
      offsetY = fallbackAnim * 0.6f;
    }
    if (offsetX < 0.0f) offsetX += spacing;
    if (offsetY < 0.0f) offsetY += spacing;
    for (int x = -spacing; x < W + spacing; x += spacing) {
      gGfx->drawLine(static_cast<int>(x + offsetX), mapTop, static_cast<int>(x + offsetX), mapBottom, 0x11CA);
    }
    for (int y = mapTop - spacing; y < mapBottom + spacing; y += spacing) {
      gGfx->drawLine(0, static_cast<int>(y + offsetY), W, static_cast<int>(y + offsetY), 0x0946);
    }
  }

  gGfx->drawLine(cx - 60, cy, cx + 60, cy, 0x5AEB);
  gGfx->drawLine(cx, cy - 60, cx, cy + 60, 0x5AEB);

  if (gTrailCount >= 2) {
    int prevX = cx;
    int prevY = cy;
    for (size_t i = 0; i < gTrailCount; ++i) {
      const size_t idx = (gTrailHead + i) % kMapTrailCapacity;
      const int px = cx + static_cast<int>((gTrailX[idx] - mapX) * mapPxPerMeter);
      const int py = cy - static_cast<int>((gTrailY[idx] - mapY) * mapPxPerMeter);
      if (i > 0) {
        gGfx->drawLine(prevX, prevY, px, py, RGB565_CYAN);
      }
      prevX = px;
      prevY = py;
    }
  }

  const float headingRad = radiansf(static_cast<float>(gState.headingCdeg) / 100.0f);
  const int noseX = cx + static_cast<int>(sinf(headingRad) * 15.0f);
  const int noseY = cy - static_cast<int>(cosf(headingRad) * 15.0f);
  const int leftX = cx + static_cast<int>(sinf(headingRad + 2.5f) * 9.0f);
  const int leftY = cy - static_cast<int>(cosf(headingRad + 2.5f) * 9.0f);
  const int rightX = cx + static_cast<int>(sinf(headingRad - 2.5f) * 9.0f);
  const int rightY = cy - static_cast<int>(cosf(headingRad - 2.5f) * 9.0f);
  gGfx->fillTriangle(noseX, noseY, leftX, leftY, rightX, rightY, RGB565_WHITE);
  gGfx->drawTriangle(noseX, noseY, leftX, leftY, rightX, rightY, RGB565_CYAN);

  gGfx->fillRect(0, 0, W, topBarH, 0x18C3);
  gGfx->setTextColor(RGB565_WHITE);
  gGfx->setTextSize(2);
  gGfx->setCursor(8, 10);
  gGfx->print("MAP");
  gGfx->setCursor(70, 10);
  gGfx->print(speedShown);
  gGfx->print("km/t");
  gGfx->setCursor(210, 10);
  gGfx->print("SOC ");
  gGfx->print(gState.socPct);
  gGfx->print("%");
  gGfx->setCursor(330, 10);
  gGfx->print("G ");
  gGfx->print(gearLabel(gState.gear));
  gGfx->setTextSize(1);
  gGfx->setTextColor(hbOk ? RGB565_GREEN : RGB565_RED);
  gGfx->setCursor(8, 30);
  gGfx->print("HB");
  gGfx->setTextColor(stOk ? RGB565_GREEN : RGB565_RED);
  gGfx->setCursor(32, 30);
  gGfx->print("ST");
  gGfx->setTextColor(gState.inputOnline ? RGB565_GREEN : RGB565_RED);
  gGfx->setCursor(56, 30);
  gGfx->print("IN");
  gGfx->setTextColor(gState.gpsFix ? RGB565_GREEN : RGB565_RED);
  gGfx->setCursor(80, 30);
  gGfx->print("GPS");
  gGfx->setTextColor(drewTiles ? RGB565_GREEN : RGB565_YELLOW);
  gGfx->setCursor(116, 30);
  gGfx->print("TILE");
  gGfx->setTextColor(RGB565_WHITE);
  gGfx->setCursor(162, 30);
  gGfx->print("Z");
  gGfx->print(gMapZoomActive);

  gGfx->fillRect(0, H - bottomBarH, W, bottomBarH, 0x18C3);
  gGfx->setTextColor(RGB565_WHITE);
  gGfx->setTextSize(2);
  gGfx->setCursor(8, H - 24);
  gGfx->print("Trip ");
  gGfx->print(gState.tripTotalTenthsKm / 10);
  gGfx->print(".");
  gGfx->print(gState.tripTotalTenthsKm % 10);
  gGfx->setCursor(210, H - 24);
  gGfx->print("Ask ");
  gGfx->print(static_cast<int>(powerAskShown));
  gGfx->print("%");
  gGfx->setCursor(320, H - 24);
  gGfx->print("Use ");
  gGfx->print(static_cast<int>(powerUsedShown));
  gGfx->print("%");
  gGfx->setTextSize(1);
  gGfx->setCursor(8, H - 8);
  if (gState.gpsFix) {
    gGfx->print("SAT ");
    gGfx->print(gState.gpsSats);
    gGfx->print(" HDG ");
    gGfx->print(static_cast<float>(gState.headingCdeg) / 100.0f, 0);
    gGfx->print("deg");
  } else {
    gGfx->print("NO GPS");
  }
  gGfx->setCursor(430, H - 8);
  gGfx->print("(c)OSM");

  gGfx->flush();
}

} // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("DISPLAY_BOOT_EARLY");
  Serial1.begin(115200, SERIAL_8N1, kMasterUartRxPin, kMasterUartTxPin);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(kBacklightPin, OUTPUT);
  digitalWrite(kBacklightPin, kBacklightOnLevel);

  if (!gGfx->begin()) {
    Serial.println("DISPLAY_GFX_BEGIN_FAIL");
    while (true) delay(1000);
  }
  gGfx->setRotation(kScreenRotation);
  ensureTileBuffers();
  gTileFsReady = LittleFS.begin(false);
  if (!gTileFsReady) {
    Serial.println("DISPLAY_LITTLEFS_FAIL");
  } else {
    Serial.println("DISPLAY_LITTLEFS_OK");
  }
#if ELLERT_DISPLAY_WIFI_LOCAL_AVAILABLE
  if (gTileFetchEnabled) {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.begin(kWifiSsid, kWifiPassword);
    Serial.print("DISPLAY_WIFI_CONNECT ssid=");
    Serial.println(kWifiSsid);
  }
#else
  Serial.println("DISPLAY_WIFI_DISABLED (no wifi_local.h)");
#endif

  gLastMasterHeartbeatMs = millis();
  gLastStatusRxMs = millis();
  gSmoothSpeed = static_cast<float>(gState.speedKmt);
  gSmoothPowerAsked = static_cast<float>(gState.powerAskedPct);
  gSmoothPowerUsed = static_cast<float>(gState.powerUsedPct);
  gLastMapAnimMs = millis();
  gLastBlinkPhase = ((millis() / 700UL) % 2UL) == 0;
  if (kShowLinkTestScreen) {
    drawLinkTestScreen();
  } else if (kShowMapScreen) {
    drawMapScreen();
  } else {
    drawDashboard();
  }

  Serial.println("DISPLAY_ESP32S3_AXS_CANVAS_BOOT");
}

void loop() {
  pollMaster();
  pollDisplayWifiStatus();
  updateTileCache();
  sendHeartbeatIfDue();
  debugIfDue();

  const uint32_t now = millis();
  if (kShowMapScreen && !kShowLinkTestScreen && (now - gLastMapAnimMs) >= kMapAnimPeriodMs) {
    gLastMapAnimMs = now;
    gUiDirty = true;
  }
  const bool blink = ((now / 700UL) % 2UL) == 0;
  if (blink != gLastBlinkPhase) {
    gLastBlinkPhase = blink;
    gUiDirty = true;
  }

  const float targetSpeed = static_cast<float>(gState.speedKmt);
  const float targetAsk = static_cast<float>(gState.powerAskedPct);
  const float targetUsed = static_cast<float>(gState.powerUsedPct);
  gSmoothSpeed += (targetSpeed - gSmoothSpeed) * kSmoothSpeedAlpha;
  gSmoothPowerAsked += (targetAsk - gSmoothPowerAsked) * kSmoothPowerAlpha;
  gSmoothPowerUsed += (targetUsed - gSmoothPowerUsed) * kSmoothPowerAlpha;

  const bool smoothingActive =
      (fabsf(gSmoothSpeed - targetSpeed) > 0.5f) ||
      (fabsf(gSmoothPowerAsked - targetAsk) > 0.5f) ||
      (fabsf(gSmoothPowerUsed - targetUsed) > 0.5f);
  if (smoothingActive) gUiDirty = true;

  if (gUiDirty && (now - gLastRenderMs) >= kRenderPeriodMs) {
    gLastRenderMs = now;
    if (kShowLinkTestScreen) {
      drawLinkTestScreen();
    } else if (kShowMapScreen) {
      drawMapScreen();
    } else {
      drawDashboard();
    }
    gUiDirty = false;
  }

  digitalWrite(LED_BUILTIN, (millis() - gLastMasterHeartbeatMs) <= kNodeOfflineTimeoutMs ? HIGH : LOW);
  delay(5);
}
