#include <Arduino.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>

#include "protocol_v1.h"
#if __has_include("wifi_local.h")
#include "wifi_local.h"
#define ELLERT_WIFI_LOCAL_AVAILABLE 1
#else
#define ELLERT_WIFI_LOCAL_AVAILABLE 0
#endif

#ifndef ELLERT_BOOT_GPS_LAT_DEG
#define ELLERT_BOOT_GPS_LAT_DEG 55.527450f
#endif

#ifndef ELLERT_BOOT_GPS_LON_DEG
#define ELLERT_BOOT_GPS_LON_DEG 8.470522f
#endif

#ifndef ELLERT_BOOT_GPS_HEADING_DEG
#define ELLERT_BOOT_GPS_HEADING_DEG 0.0f
#endif

#ifndef ELLERT_BOOT_GPS_SATS
#define ELLERT_BOOT_GPS_SATS 9
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

namespace {
using namespace EllertProtocolV1;

// -------- Transport --------
HardwareSerial gInputUart(1);
HardwareSerial gDisplayUart(2);
Decoder gInputDecoder;
Decoder gDisplayDecoder;
uint8_t gSeq = 0;

constexpr uint32_t kBaud = 115200;
// Temporary wiring-friendly mapping for bring-up:
// - Display node on labeled UART2 pins (GPIO16/17, header 27/28)
// - Input node on alternate pins (GPIO18/19, header 30/31)
constexpr int kInputRxPin = 18;
constexpr int kInputTxPin = 19;
constexpr int kDisplayRxPin = 16;
constexpr int kDisplayTxPin = 17;

// -------- Master IO --------
constexpr int PIN_DRL = 2;
constexpr int PIN_FRONT_NEAR = 4;
constexpr int PIN_FRONT_HIGH = 5;
constexpr int PIN_IND_LEFT = 21;
constexpr int PIN_IND_RIGHT = 22;
constexpr int PIN_BRAKE_LIGHT = 23;
constexpr int PIN_HORN = 25;
constexpr int PIN_SPRINKLER = 26;
constexpr int PIN_WIPER_INT = 27;
constexpr int PIN_WIPER_NORMAL = 14;
constexpr int PIN_WIPER_FAST = 13;
constexpr int PIN_VENT_LOW = 12;
constexpr int PIN_VENT_MID = 15;
constexpr int PIN_VENT_HIGH = 33;

constexpr int PIN_IGNITION = 32;
constexpr int PIN_BRAKE_PEDAL = 35;
constexpr int PIN_PEDAL_ADC = 34;

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

enum EventType : uint8_t {
  EVENT_NONE = 0,
  EVENT_PRESS = 1,
  EVENT_RELEASE = 2
};

struct RuntimeState {
  bool inputOnline = false;
  bool displayOnline = false;
  bool hazardEnabled = false;
  bool indLeftRequested = false;
  bool indRightRequested = false;
  bool indicatorBlinkOn = false;
  uint8_t lastCommand = 0xFF;
  uint16_t inputMask = 0;
  uint32_t lastInputHeartbeatMs = 0;
  uint32_t lastDisplayHeartbeatMs = 0;
  uint32_t lastHeartbeatTxMs = 0;
  uint32_t lastStatusTxMs = 0;
  uint32_t lastIndicatorToggleMs = 0;
  uint32_t lastDebugTxMs = 0;
} gState;

uint16_t gTripTotalTenthsKm = 0;
uint16_t gTripSinceChargeTenthsKm = 0;
uint32_t gLastTripUpdateMs = 0;

struct GpsState {
  int32_t latE7 = 0;
  int32_t lonE7 = 0;
  uint16_t headingCdeg = 0;
  uint8_t sats = 0;
  bool fix = false;
  uint32_t lastUpdateMs = 0;
} gGps;

char gSerialLine[128];
size_t gSerialLineLen = 0;
uint32_t gLastWifiStatusMs = 0;
uint32_t gLastTimeStatusMs = 0;
bool gTimeConfigured = false;
uint8_t gMapZoom = 17;

void writeFrame(HardwareSerial &port, uint8_t type, const uint8_t *payload, uint8_t len) {
  uint8_t buffer[kMaxPayloadLen + kFrameOverhead];
  const size_t n = encodeFrame(type, gSeq++, payload, len, buffer, sizeof(buffer));
  if (n > 0) {
    port.write(buffer, n);
  }
}

void setOutput(int pin, bool on) {
  digitalWrite(pin, on ? HIGH : LOW);
}

float clampf(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

int32_t degToE7(float deg) {
  return static_cast<int32_t>(lroundf(deg * 10000000.0f));
}

float normalizeHeading(float deg) {
  while (deg < 0.0f) deg += 360.0f;
  while (deg >= 360.0f) deg -= 360.0f;
  return deg;
}

bool parseNmeaLatLon(const char *value, const char hemi, bool latField, float &outDeg) {
  if (!value || !value[0]) return false;
  const float raw = strtof(value, nullptr);
  if (!isfinite(raw) || raw <= 0.0f) return false;
  const float div = latField ? 100.0f : 100.0f;
  const float degrees = floorf(raw / div);
  const float minutes = raw - (degrees * div);
  float decimal = degrees + (minutes / 60.0f);
  if (hemi == 'S' || hemi == 'W') decimal = -decimal;
  outDeg = decimal;
  return isfinite(outDeg);
}

void updateGpsFix(float latDeg, float lonDeg, float headingDeg) {
  latDeg = clampf(latDeg, -90.0f, 90.0f);
  lonDeg = clampf(lonDeg, -180.0f, 180.0f);
  const float normHeading = normalizeHeading(headingDeg);
  gGps.latE7 = degToE7(latDeg);
  gGps.lonE7 = degToE7(lonDeg);
  gGps.headingCdeg = static_cast<uint16_t>(lroundf(normHeading * 100.0f));
  gGps.fix = true;
  gGps.lastUpdateMs = millis();
}

bool handleGpsNmeaRmc(char *line) {
  // $GPRMC,time,status,lat,N,lon,E,speed,course,...
  char *save = nullptr;
  char *token = strtok_r(line, ",", &save);
  uint8_t field = 0;
  char status = 'V';
  char latStr[20] = {0};
  char lonStr[20] = {0};
  char latHem = 'N';
  char lonHem = 'E';
  char courseStr[16] = {0};

  while (token) {
    if (field == 2 && token[0]) status = token[0];
    if (field == 3) strncpy(latStr, token, sizeof(latStr) - 1);
    if (field == 4 && token[0]) latHem = token[0];
    if (field == 5) strncpy(lonStr, token, sizeof(lonStr) - 1);
    if (field == 6 && token[0]) lonHem = token[0];
    if (field == 8) strncpy(courseStr, token, sizeof(courseStr) - 1);
    token = strtok_r(nullptr, ",", &save);
    ++field;
  }

  if (status != 'A') {
    gGps.fix = false;
    return false;
  }

  float latDeg = 0.0f;
  float lonDeg = 0.0f;
  if (!parseNmeaLatLon(latStr, latHem, true, latDeg)) return false;
  if (!parseNmeaLatLon(lonStr, lonHem, false, lonDeg)) return false;
  const float headingDeg = courseStr[0] ? strtof(courseStr, nullptr) : 0.0f;
  updateGpsFix(latDeg, lonDeg, headingDeg);
  return true;
}

bool handleConsoleGpsCommand(char *line) {
  // Format: GPS <lat> <lon> [headingDeg] [sats]
  char *save = nullptr;
  char *cmd = strtok_r(line, " ", &save);
  if (!cmd || strcmp(cmd, "GPS") != 0) return false;
  char *latTok = strtok_r(nullptr, " ", &save);
  char *lonTok = strtok_r(nullptr, " ", &save);
  if (!latTok || !lonTok) return false;

  const float latDeg = strtof(latTok, nullptr);
  const float lonDeg = strtof(lonTok, nullptr);
  float headingDeg = 0.0f;
  if (char *headingTok = strtok_r(nullptr, " ", &save)) {
    headingDeg = strtof(headingTok, nullptr);
  }
  if (char *satTok = strtok_r(nullptr, " ", &save)) {
    const long sats = strtol(satTok, nullptr, 10);
    gGps.sats = static_cast<uint8_t>(constrain(sats, 0L, 99L));
  }
  updateGpsFix(latDeg, lonDeg, headingDeg);
  Serial.print("GPS_SET lat=");
  Serial.print(latDeg, 6);
  Serial.print(" lon=");
  Serial.print(lonDeg, 6);
  Serial.print(" heading=");
  Serial.println(normalizeHeading(headingDeg), 1);
  return true;
}

bool handleConsoleMapZoomCommand(char *line) {
  // Format: MAPZOOM <0..19>
  char *save = nullptr;
  char *cmd = strtok_r(line, " ", &save);
  if (!cmd || strcmp(cmd, "MAPZOOM") != 0) return false;
  char *zoomTok = strtok_r(nullptr, " ", &save);
  if (!zoomTok) return false;
  const long zoom = strtol(zoomTok, nullptr, 10);
  if (zoom < 0 || zoom > 19) {
    Serial.println("MAPZOOM_FAIL range 0..19");
    return true;
  }
  gMapZoom = static_cast<uint8_t>(zoom);
  Serial.print("MAPZOOM_SET ");
  Serial.println(gMapZoom);
  return true;
}

void printGpsHelp() {
  Serial.println("Console usage:");
  Serial.println("  GPS <lat> <lon> [headingDeg] [sats]");
  Serial.println("  GPS?  (print last fix)");
  Serial.println("  MAPZOOM <0..19>");
  Serial.println("  NTP?  (print local time sync status)");
  Serial.println("  TILETEST <z> <x> <y>  (download one OSM tile and print result)");
  Serial.println("  NMEA RMC lines are accepted on USB serial");
}

void initTimeIfConfigured() {
#if ELLERT_WIFI_LOCAL_AVAILABLE
  // Europe/Copenhagen with DST rules.
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org", "time.google.com");
  gTimeConfigured = true;
  Serial.println("NTP_INIT");
#endif
}

void printNtpStatus() {
  if (!gTimeConfigured) {
    Serial.println("NTP_DISABLED");
    return;
  }
  time_t now = time(nullptr);
  if (now < 1700000000) {
    Serial.print("NTP_WAIT epoch=");
    Serial.println(static_cast<unsigned long>(now));
    return;
  }

  struct tm tmNow;
  if (!localtime_r(&now, &tmNow)) {
    Serial.println("NTP_ERR localtime");
    return;
  }

  char ts[32];
  strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %Z", &tmNow);
  Serial.print("NTP_OK ");
  Serial.print(ts);
  Serial.print(" epoch=");
  Serial.println(static_cast<unsigned long>(now));
}

bool fetchTilePrototype(uint8_t z, uint32_t x, uint32_t y) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("TILETEST_FAIL wifi_not_connected");
    return false;
  }
  if (z > 19) {
    Serial.println("TILETEST_FAIL bad_zoom");
    return false;
  }
  const uint32_t maxCoord = (1UL << z);
  if (x >= maxCoord || y >= maxCoord) {
    Serial.println("TILETEST_FAIL bad_xy_for_zoom");
    return false;
  }

  const String url = String("https://tile.openstreetmap.org/") + z + "/" + x + "/" + y + ".png";
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setUserAgent("Ellert-Prototype/0.1 (local dev test)");

  if (!http.begin(client, url)) {
    Serial.println("TILETEST_FAIL begin");
    return false;
  }

  Serial.print("TILETEST_GET ");
  Serial.println(url);
  const int code = http.GET();
  const int len = http.getSize();
  Serial.print("TILETEST_HTTP code=");
  Serial.print(code);
  Serial.print(" len=");
  Serial.println(len);

  bool ok = false;
  if (code == HTTP_CODE_OK) {
    WiFiClient *stream = http.getStreamPtr();
    uint8_t buffer[256];
    size_t total = 0;
    const uint32_t startMs = millis();
    while (http.connected() && (millis() - startMs) < 5000UL) {
      const size_t avail = stream->available();
      if (avail == 0) {
        if (len > 0 && total >= static_cast<size_t>(len)) break;
        delay(1);
        continue;
      }
      const size_t chunk = avail > sizeof(buffer) ? sizeof(buffer) : avail;
      const int got = stream->readBytes(buffer, chunk);
      if (got > 0) {
        total += static_cast<size_t>(got);
      }
      if (len > 0 && total >= static_cast<size_t>(len)) break;
    }
    Serial.print("TILETEST_BYTES ");
    Serial.println(static_cast<unsigned long>(total));
    ok = total > 0;
  }

  http.end();
  return ok;
}

void handleConsoleLine(char *line) {
  if (!line || !line[0]) return;
  if (strcmp(line, "GPS?") == 0) {
    Serial.print("GPS fix=");
    Serial.print(gGps.fix ? "1" : "0");
    Serial.print(" latE7=");
    Serial.print(gGps.latE7);
    Serial.print(" lonE7=");
    Serial.print(gGps.lonE7);
    Serial.print(" heading=");
    Serial.print(static_cast<float>(gGps.headingCdeg) / 100.0f, 1);
    Serial.print(" sats=");
    Serial.println(gGps.sats);
    return;
  }
  if (strcmp(line, "GPSHELP") == 0) {
    printGpsHelp();
    return;
  }
  if (strcmp(line, "NTP?") == 0) {
    printNtpStatus();
    return;
  }

  char local[128];
  strncpy(local, line, sizeof(local) - 1);
  local[sizeof(local) - 1] = '\0';
  if (handleConsoleGpsCommand(local)) return;

  char zoomLocal[128];
  strncpy(zoomLocal, line, sizeof(zoomLocal) - 1);
  zoomLocal[sizeof(zoomLocal) - 1] = '\0';
  if (handleConsoleMapZoomCommand(zoomLocal)) return;

  char tileLocal[128];
  strncpy(tileLocal, line, sizeof(tileLocal) - 1);
  tileLocal[sizeof(tileLocal) - 1] = '\0';
  char *save = nullptr;
  char *cmd = strtok_r(tileLocal, " ", &save);
  if (cmd && strcmp(cmd, "TILETEST") == 0) {
    char *zTok = strtok_r(nullptr, " ", &save);
    char *xTok = strtok_r(nullptr, " ", &save);
    char *yTok = strtok_r(nullptr, " ", &save);
    if (!zTok || !xTok || !yTok) {
      Serial.println("TILETEST_FAIL usage TILETEST <z> <x> <y>");
      return;
    }
    const long z = strtol(zTok, nullptr, 10);
    const long x = strtol(xTok, nullptr, 10);
    const long y = strtol(yTok, nullptr, 10);
    if (z < 0 || z > 19 || x < 0 || y < 0) {
      Serial.println("TILETEST_FAIL bad_args");
      return;
    }
    const bool ok = fetchTilePrototype(static_cast<uint8_t>(z), static_cast<uint32_t>(x),
                                       static_cast<uint32_t>(y));
    Serial.println(ok ? "TILETEST_OK" : "TILETEST_FAIL");
    return;
  }

  if (line[0] == '$') {
    char nmea[128];
    strncpy(nmea, line, sizeof(nmea) - 1);
    nmea[sizeof(nmea) - 1] = '\0';
    if (strstr(nmea, "RMC") != nullptr && handleGpsNmeaRmc(nmea)) {
      Serial.print("GPS_RMC latE7=");
      Serial.print(gGps.latE7);
      Serial.print(" lonE7=");
      Serial.println(gGps.lonE7);
      return;
    }
  }
}

void pollUsbConsole() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      gSerialLine[gSerialLineLen] = '\0';
      handleConsoleLine(gSerialLine);
      gSerialLineLen = 0;
      continue;
    }
    if (gSerialLineLen + 1 >= sizeof(gSerialLine)) {
      gSerialLineLen = 0;
      continue;
    }
    gSerialLine[gSerialLineLen++] = c;
  }
}

void initWifiIfConfigured() {
#if ELLERT_WIFI_LOCAL_AVAILABLE
  Serial.print("WIFI_CONNECT ssid=");
  Serial.println(kWifiSsid);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(kWifiSsid, kWifiPassword);
#else
  Serial.println("WIFI_DISABLED (no wifi_local.h)");
#endif
}

void pollWifiStatus() {
#if ELLERT_WIFI_LOCAL_AVAILABLE
  const uint32_t now = millis();
  if (now - gLastWifiStatusMs < 5000UL) return;
  gLastWifiStatusMs = now;

  const wl_status_t st = WiFi.status();
  if (st == WL_CONNECTED) {
    Serial.print("WIFI_OK ip=");
    Serial.println(WiFi.localIP());
    return;
  }

  Serial.print("WIFI_WAIT status=");
  Serial.println(static_cast<int>(st));
  if (st == WL_DISCONNECTED || st == WL_CONNECT_FAILED || st == WL_CONNECTION_LOST) {
    WiFi.disconnect(false, false);
    WiFi.begin(kWifiSsid, kWifiPassword);
  }
#endif
}

void pollTimeStatus() {
#if ELLERT_WIFI_LOCAL_AVAILABLE
  if (!gTimeConfigured) return;
  if (WiFi.status() != WL_CONNECTED) return;
  const uint32_t nowMs = millis();
  if (nowMs - gLastTimeStatusMs < 30000UL) return;
  gLastTimeStatusMs = nowMs;
  printNtpStatus();
#endif
}

void applyWiperMode(uint8_t mode) {
  setOutput(PIN_WIPER_INT, mode == 1);
  setOutput(PIN_WIPER_NORMAL, mode == 2);
  setOutput(PIN_WIPER_FAST, mode == 3);
}

void applyFanMode(uint8_t mode) {
  setOutput(PIN_VENT_LOW, mode == 1);
  setOutput(PIN_VENT_MID, mode == 2);
  setOutput(PIN_VENT_HIGH, mode == 3);
}

void applyInputCommand(uint8_t cmd, uint8_t eventType) {
  if (eventType != EVENT_PRESS && eventType != EVENT_RELEASE) {
    return;
  }

  const bool pressed = (eventType == EVENT_PRESS);
  if (pressed) {
    gState.lastCommand = cmd;
  }

  switch (cmd) {
    case CMD_LIGHTS_OFF:
      if (pressed) {
        setOutput(PIN_DRL, false);
        setOutput(PIN_FRONT_NEAR, false);
        setOutput(PIN_FRONT_HIGH, false);
      }
      break;
    case CMD_LIGHTS_PARK:
      if (pressed) {
        setOutput(PIN_DRL, true);
        setOutput(PIN_FRONT_NEAR, false);
        setOutput(PIN_FRONT_HIGH, false);
      }
      break;
    case CMD_LIGHTS_LOW:
      if (pressed) {
        setOutput(PIN_DRL, true);
        setOutput(PIN_FRONT_NEAR, true);
        setOutput(PIN_FRONT_HIGH, false);
      }
      break;
    case CMD_LIGHTS_HIGH:
      if (pressed) {
        setOutput(PIN_DRL, true);
        setOutput(PIN_FRONT_NEAR, true);
        setOutput(PIN_FRONT_HIGH, true);
      }
      break;
    case CMD_IND_LEFT:
      if (pressed) {
        gState.indLeftRequested = !gState.indLeftRequested;
        if (gState.indLeftRequested) {
          gState.indRightRequested = false;
          gState.hazardEnabled = false;
        }
      }
      break;
    case CMD_IND_RIGHT:
      if (pressed) {
        gState.indRightRequested = !gState.indRightRequested;
        if (gState.indRightRequested) {
          gState.indLeftRequested = false;
          gState.hazardEnabled = false;
        }
      }
      break;
    case CMD_HAZARD:
      if (pressed) {
        gState.hazardEnabled = !gState.hazardEnabled;
        if (gState.hazardEnabled) {
          gState.indLeftRequested = false;
          gState.indRightRequested = false;
        }
      }
      break;
    case CMD_HORN:
      setOutput(PIN_HORN, pressed);
      break;
    case CMD_WIPER_INT:
      if (pressed) applyWiperMode(1);
      break;
    case CMD_WIPER_LOW:
      if (pressed) applyWiperMode(2);
      break;
    case CMD_WIPER_HIGH:
      if (pressed) applyWiperMode(3);
      break;
    case CMD_WASHER:
      setOutput(PIN_SPRINKLER, pressed);
      break;
    case CMD_FAN_LOW:
      if (pressed) {
        applyFanMode(1);
        if (gMapZoom > 3) --gMapZoom;
      }
      break;
    case CMD_FAN_MID:
      if (pressed) {
        applyFanMode(2);
      }
      break;
    case CMD_FAN_HIGH:
      if (pressed) {
        applyFanMode(3);
        if (gMapZoom < 19) ++gMapZoom;
      }
      break;
    case CMD_DEMIST:
      // Placeholder output behavior until dedicated demist output is assigned.
      if (pressed) {
        setOutput(PIN_FRONT_NEAR, true);
      }
      break;
    default:
      break;
  }
}

void handleInputFrame(const Frame &frame) {
  if (frame.type == MSG_HEARTBEAT) {
    gState.lastInputHeartbeatMs = millis();
    return;
  }

  if (frame.type != MSG_INPUT_STATE) {
    return;
  }

  gState.lastInputHeartbeatMs = millis();
  if (frame.len >= 3) {
    gState.inputMask = static_cast<uint16_t>(frame.payload[1]) |
                       (static_cast<uint16_t>(frame.payload[2]) << 8);
  }
  if (frame.len >= 5) {
    applyInputCommand(frame.payload[3], frame.payload[4]);
  }
}

void handleDisplayFrame(const Frame &frame) {
  if (frame.type == MSG_HEARTBEAT) {
    gState.lastDisplayHeartbeatMs = millis();
  }
}

void pollPort(HardwareSerial &port, Decoder &decoder, bool inputPort) {
  while (port.available() > 0) {
    const uint8_t b = static_cast<uint8_t>(port.read());
    Frame frame;
    if (!decoder.feed(b, frame)) {
      continue;
    }
    if (inputPort) {
      handleInputFrame(frame);
    } else {
      handleDisplayFrame(frame);
    }
  }
}

void updateOnlineStates() {
  const uint32_t now = millis();
  gState.inputOnline = (now - gState.lastInputHeartbeatMs) <= kNodeOfflineTimeoutMs;
  gState.displayOnline = (now - gState.lastDisplayHeartbeatMs) <= kNodeOfflineTimeoutMs;
  if (!gState.inputOnline) {
    // Fail-safe: release momentary outputs if input node is lost.
    setOutput(PIN_HORN, false);
    setOutput(PIN_SPRINKLER, false);
    gState.hazardEnabled = false;
    gState.indLeftRequested = false;
    gState.indRightRequested = false;
  }
}

void updateIndicators() {
  const uint32_t now = millis();
  if (now - gState.lastIndicatorToggleMs >= 700) {
    gState.lastIndicatorToggleMs = now;
    gState.indicatorBlinkOn = !gState.indicatorBlinkOn;
  }

  const bool activeLeft = gState.hazardEnabled || gState.indLeftRequested;
  const bool activeRight = gState.hazardEnabled || gState.indRightRequested;

  setOutput(PIN_IND_LEFT, activeLeft && gState.indicatorBlinkOn);
  setOutput(PIN_IND_RIGHT, activeRight && gState.indicatorBlinkOn);
}

uint8_t readGearCode() {
  return GEAR_UNKNOWN;
}

void updateTripsAndBrake() {
  const bool brakePressed = digitalRead(PIN_BRAKE_PEDAL) == LOW;
  setOutput(PIN_BRAKE_LIGHT, brakePressed);

  const uint16_t pedalRaw = static_cast<uint16_t>(analogRead(PIN_PEDAL_ADC));
  const uint8_t speedKmt = map(pedalRaw, 0, 4095, 0, 120);

  const uint32_t now = millis();
  if (gLastTripUpdateMs == 0) {
    gLastTripUpdateMs = now;
    return;
  }

  const uint32_t dtMs = now - gLastTripUpdateMs;
  gLastTripUpdateMs = now;

  // tenths_km += km/h * dt_h * 10
  const uint32_t addTenths = (static_cast<uint32_t>(speedKmt) * dtMs) / 360000;
  gTripTotalTenthsKm = static_cast<uint16_t>(gTripTotalTenthsKm + addTenths);
  gTripSinceChargeTenthsKm = static_cast<uint16_t>(gTripSinceChargeTenthsKm + addTenths);
}

void sendHeartbeatIfDue() {
  const uint32_t now = millis();
  if (now - gState.lastHeartbeatTxMs < kHeartbeatMs) {
    return;
  }
  gState.lastHeartbeatTxMs = now;

  const uint8_t hbPayload[2] = {
      static_cast<uint8_t>(gState.inputOnline ? 1 : 0),
      static_cast<uint8_t>(gState.displayOnline ? 1 : 0)};
  writeFrame(gInputUart, MSG_HEARTBEAT, hbPayload, sizeof(hbPayload));
  writeFrame(gDisplayUart, MSG_HEARTBEAT, hbPayload, sizeof(hbPayload));
}

void sendStatusIfDue() {
  const uint32_t now = millis();
  if (now - gState.lastStatusTxMs < kMasterStatusMs) {
    return;
  }
  gState.lastStatusTxMs = now;

  const uint16_t pedalRaw = static_cast<uint16_t>(analogRead(PIN_PEDAL_ADC));
  const uint8_t powerAskedPct = map(pedalRaw, 0, 4095, 0, 100);
  int8_t powerUsedPct = static_cast<int8_t>(powerAskedPct);
  if (digitalRead(PIN_BRAKE_PEDAL) == LOW && powerAskedPct > 5) {
    powerUsedPct = -15;
  }

  const uint8_t speedKmt = map(pedalRaw, 0, 4095, 0, 120);

  uint8_t indicatorBits = 0;
  if (digitalRead(PIN_IND_LEFT) == HIGH) indicatorBits |= IND_LEFT;
  if (digitalRead(PIN_IND_RIGHT) == HIGH) indicatorBits |= IND_RIGHT;
  if ((indicatorBits & IND_LEFT) && (indicatorBits & IND_RIGHT)) indicatorBits |= IND_HAZARD;

  uint8_t lightBits = 0;
  if (digitalRead(PIN_DRL) == HIGH) lightBits |= LIGHT_DRL;
  if (digitalRead(PIN_FRONT_NEAR) == HIGH) lightBits |= LIGHT_NEAR;
  if (digitalRead(PIN_FRONT_HIGH) == HIGH) lightBits |= LIGHT_HIGH;
  if (digitalRead(PIN_BRAKE_LIGHT) == HIGH) lightBits |= LIGHT_BRAKE;

  uint8_t wiperMode = 0;
  if (digitalRead(PIN_WIPER_FAST) == HIGH) wiperMode = 3;
  else if (digitalRead(PIN_WIPER_NORMAL) == HIGH) wiperMode = 2;
  else if (digitalRead(PIN_WIPER_INT) == HIGH) wiperMode = 1;

  const bool ignitionOn = digitalRead(PIN_IGNITION) == LOW;

  uint8_t payload[kStatusPayloadLen];
  payload[0] = speedKmt;
  payload[1] = 34;  // SOC placeholder until BMS integration.
  payload[2] = static_cast<uint8_t>(powerUsedPct);
  payload[3] = powerAskedPct;
  payload[4] = indicatorBits;
  payload[5] = lightBits;
  payload[6] = wiperMode;
  payload[7] = readGearCode();
  payload[8] = static_cast<uint8_t>(gTripTotalTenthsKm & 0xFF);
  payload[9] = static_cast<uint8_t>((gTripTotalTenthsKm >> 8) & 0xFF);
  payload[10] = static_cast<uint8_t>(gTripSinceChargeTenthsKm & 0xFF);
  payload[11] = static_cast<uint8_t>((gTripSinceChargeTenthsKm >> 8) & 0xFF);
  payload[12] = static_cast<uint8_t>(gState.inputOnline ? 1 : 0);
  payload[13] = static_cast<uint8_t>(gState.displayOnline ? 1 : 0);
  payload[14] = static_cast<uint8_t>(ignitionOn ? 1 : 0);
  payload[15] = gState.lastCommand;
  payload[16] = static_cast<uint8_t>(gState.inputMask & 0xFF);
  payload[17] = static_cast<uint8_t>((gState.inputMask >> 8) & 0xFF);
  payload[18] = static_cast<uint8_t>(gGps.latE7 & 0xFF);
  payload[19] = static_cast<uint8_t>((gGps.latE7 >> 8) & 0xFF);
  payload[20] = static_cast<uint8_t>((gGps.latE7 >> 16) & 0xFF);
  payload[21] = static_cast<uint8_t>((gGps.latE7 >> 24) & 0xFF);
  payload[22] = static_cast<uint8_t>(gGps.lonE7 & 0xFF);
  payload[23] = static_cast<uint8_t>((gGps.lonE7 >> 8) & 0xFF);
  payload[24] = static_cast<uint8_t>((gGps.lonE7 >> 16) & 0xFF);
  payload[25] = static_cast<uint8_t>((gGps.lonE7 >> 24) & 0xFF);
  payload[26] = static_cast<uint8_t>(gGps.headingCdeg & 0xFF);
  payload[27] = static_cast<uint8_t>((gGps.headingCdeg >> 8) & 0xFF);
  payload[28] = static_cast<uint8_t>(gGps.fix ? 1 : 0);
  payload[29] = gGps.sats;
  payload[30] = gMapZoom;

  writeFrame(gDisplayUart, MSG_STATUS_SNAPSHOT, payload, sizeof(payload));
}

void sendDebugIfDue() {
  const uint32_t now = millis();
  if (now - gState.lastDebugTxMs < 1000) {
    return;
  }
  gState.lastDebugTxMs = now;

  const uint32_t inputAgeMs = now - gState.lastInputHeartbeatMs;
  const uint32_t displayAgeMs = now - gState.lastDisplayHeartbeatMs;

  Serial.print("LINK input=");
  Serial.print(gState.inputOnline ? "ON" : "OFF");
  Serial.print(" age=");
  Serial.print(inputAgeMs);
  Serial.print("ms | display=");
  Serial.print(gState.displayOnline ? "ON" : "OFF");
  Serial.print(" age=");
  Serial.print(displayAgeMs);
  Serial.print("ms | gps=");
  Serial.print(gGps.fix ? "FIX" : "NOFIX");
  Serial.print(" latE7=");
  Serial.print(gGps.latE7);
  Serial.print(" lonE7=");
  Serial.println(gGps.lonE7);
}

void initPins() {
  const int outputPins[] = {
      PIN_DRL,       PIN_FRONT_NEAR, PIN_FRONT_HIGH, PIN_IND_LEFT,  PIN_IND_RIGHT,
      PIN_BRAKE_LIGHT, PIN_HORN,     PIN_SPRINKLER,  PIN_WIPER_INT, PIN_WIPER_NORMAL,
      PIN_WIPER_FAST, PIN_VENT_LOW,  PIN_VENT_MID,   PIN_VENT_HIGH, LED_BUILTIN};
  for (size_t i = 0; i < sizeof(outputPins) / sizeof(outputPins[0]); ++i) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }

  pinMode(PIN_IGNITION, INPUT_PULLUP);
  pinMode(PIN_BRAKE_PEDAL, INPUT_PULLUP);
}

}  // namespace

void setup() {
  Serial.begin(kBaud);
  gInputUart.begin(kBaud, SERIAL_8N1, kInputRxPin, kInputTxPin);
  gDisplayUart.begin(kBaud, SERIAL_8N1, kDisplayRxPin, kDisplayTxPin);

  initPins();
  gInputDecoder.reset();
  gDisplayDecoder.reset();

  Serial.println("MASTER_ESP32_BOOT");
  printGpsHelp();
  gGps.sats = static_cast<uint8_t>(ELLERT_BOOT_GPS_SATS);
  updateGpsFix(ELLERT_BOOT_GPS_LAT_DEG, ELLERT_BOOT_GPS_LON_DEG, ELLERT_BOOT_GPS_HEADING_DEG);
  Serial.print("GPS_BOOT lat=");
  Serial.print(ELLERT_BOOT_GPS_LAT_DEG, 6);
  Serial.print(" lon=");
  Serial.print(ELLERT_BOOT_GPS_LON_DEG, 6);
  Serial.print(" sats=");
  Serial.println(gGps.sats);
  initWifiIfConfigured();
  initTimeIfConfigured();
}

void loop() {
  pollUsbConsole();
  pollWifiStatus();
  pollTimeStatus();
  pollPort(gInputUart, gInputDecoder, true);
  pollPort(gDisplayUart, gDisplayDecoder, false);

  updateOnlineStates();
  updateIndicators();
  updateTripsAndBrake();
  sendHeartbeatIfDue();
  sendStatusIfDue();
  sendDebugIfDue();

  digitalWrite(LED_BUILTIN, gState.inputOnline ? HIGH : LOW);
}
