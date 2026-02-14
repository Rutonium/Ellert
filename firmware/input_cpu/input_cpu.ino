#include <Arduino.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

#include "protocol_v1.h"

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

enum EventType : uint8_t {
    EVENT_NONE = 0,
    EVENT_PRESS = 1,
    EVENT_RELEASE = 2
};

struct ButtonDef {
    const char* label;
    uint8_t commandId;
    uint16_t color;
};

uint8_t gSeq = 0;
uint32_t gLastHeartbeatTxMs = 0;
uint32_t gLastInputTxMs = 0;
uint32_t gLastMasterHeartbeatMs = 0;
uint32_t gTouchChangedMs = 0;

Decoder gDecoder;

MCUFRIEND_kbv gTft;

// Typical 3.5" UNO shield resistive touch wiring.
const int XP = 8;
const int XM = A2;
const int YP = A3;
const int YM = 9;
TouchScreen gTouch(XP, YP, XM, YM, 300);

// TODO: Calibrate these values on your specific panel.
const int TS_MINX = 120;
const int TS_MAXX = 920;
const int TS_MINY = 70;
const int TS_MAXY = 900;
const int MINPRESSURE = 200;
const int MAXPRESSURE = 1000;
const uint8_t kScreenRotation = 0; // Portrait

const uint8_t kGridRows = 4;
const uint8_t kGridCols = 4;
const uint8_t kButtonCount = 16;

const ButtonDef kButtons[kButtonCount] = {
    {"L OFF", CMD_LIGHTS_OFF, 0x39E7},
    {"L PARK", CMD_LIGHTS_PARK, 0x39E7},
    {"L LOW", CMD_LIGHTS_LOW, 0x39E7},
    {"L HIGH", CMD_LIGHTS_HIGH, 0x39E7},
    {"IND L", CMD_IND_LEFT, 0x051D},
    {"IND R", CMD_IND_RIGHT, 0x051D},
    {"HAZ", CMD_HAZARD, 0xF800},
    {"HORN", CMD_HORN, 0xD8C0},
    {"W INT", CMD_WIPER_INT, 0x001F},
    {"W LOW", CMD_WIPER_LOW, 0x001F},
    {"W HIGH", CMD_WIPER_HIGH, 0x001F},
    {"WASH", CMD_WASHER, 0x001F},
    {"F LOW", CMD_FAN_LOW, 0x07E0},
    {"F MID", CMD_FAN_MID, 0x07E0},
    {"F HIGH", CMD_FAN_HIGH, 0x07E0},
    {"DEM", CMD_DEMIST, 0xFD20},
};

uint16_t gPressedMask = 0;
int8_t gStableTouchIndex = -1;
int8_t gRawTouchIndex = -1;

uint16_t darken565(uint16_t c) {
    uint8_t r = ((c >> 11) & 0x1F) / 2;
    uint8_t g = ((c >> 5) & 0x3F) / 2;
    uint8_t b = (c & 0x1F) / 2;
    return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

void drawButton(uint8_t index) {
    const int16_t w = gTft.width() / kGridCols;
    const int16_t h = gTft.height() / kGridRows;
    const int16_t row = static_cast<int16_t>(index / kGridCols);
    const int16_t col = static_cast<int16_t>(index % kGridCols);

    const int16_t x = col * w;
    const int16_t y = row * h;
    const bool pressed = (gPressedMask & (1u << index)) != 0;
    const uint16_t fillColor = pressed ? darken565(kButtons[index].color) : kButtons[index].color;

    gTft.fillRect(x + 2, y + 2, w - 4, h - 4, fillColor);
    gTft.drawRect(x + 1, y + 1, w - 2, h - 2, 0xFFFF);
    gTft.setTextColor(0xFFFF, fillColor);
    gTft.setTextSize(2);
    gTft.setCursor(x + 8, y + (h / 2) - 8);
    gTft.print(kButtons[index].label);
}

void drawGrid() {
    gTft.fillScreen(0x0000);
    for (uint8_t i = 0; i < kButtonCount; ++i) {
        drawButton(i);
    }
}

int8_t readTouchIndex() {
    TSPoint p = gTouch.getPoint();

    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    if (p.z < MINPRESSURE || p.z > MAXPRESSURE) {
        return -1;
    }

    const int16_t x = map(p.x, TS_MAXX, TS_MINX, 0, gTft.width());
    const int16_t y = map(p.y, TS_MINY, TS_MAXY, 0, gTft.height());
    if (x < 0 || y < 0 || x >= gTft.width() || y >= gTft.height()) {
        return -1;
    }

    const int16_t col = x / (gTft.width() / kGridCols);
    const int16_t row = y / (gTft.height() / kGridRows);
    const int16_t idx = row * kGridCols + col;
    if (idx < 0 || idx >= kButtonCount) {
        return -1;
    }
    return static_cast<int8_t>(idx);
}

void sendFrame(uint8_t type, const uint8_t* payload, uint8_t len) {
    uint8_t buffer[kMaxPayloadLen + kFrameOverhead];
    const size_t frameLen = encodeFrame(type, gSeq++, payload, len, buffer, sizeof(buffer));
    if (frameLen > 0) {
        Serial.write(buffer, frameLen);
    }
}

void sendHeartbeatIfDue() {
    const uint32_t now = millis();
    if (now - gLastHeartbeatTxMs < kHeartbeatMs) return;
    gLastHeartbeatTxMs = now;

    const uint8_t payload[1] = {1}; // Input CPU alive
    sendFrame(MSG_HEARTBEAT, payload, sizeof(payload));
}

void sendInputEvent(uint8_t commandId, uint8_t eventType) {
    uint8_t payload[5];
    payload[0] = 0; // flags
    payload[1] = static_cast<uint8_t>(gPressedMask & 0xFF);
    payload[2] = static_cast<uint8_t>((gPressedMask >> 8) & 0xFF);
    payload[3] = commandId;
    payload[4] = eventType;
    sendFrame(MSG_INPUT_STATE, payload, sizeof(payload));
}

void sendInputStateIfDue(uint8_t commandId, uint8_t eventType) {
    const uint32_t now = millis();
    if (now - gLastInputTxMs < 100) return;
    gLastInputTxMs = now;

    uint8_t payload[5];
    payload[0] = 0; // flags
    payload[1] = static_cast<uint8_t>(gPressedMask & 0xFF);
    payload[2] = static_cast<uint8_t>((gPressedMask >> 8) & 0xFF);
    payload[3] = commandId;
    payload[4] = eventType;
    sendFrame(MSG_INPUT_STATE, payload, sizeof(payload));
}

void pollMaster() {
    while (Serial.available() > 0) {
        const uint8_t b = static_cast<uint8_t>(Serial.read());
        Frame frame;
        if (!gDecoder.feed(b, frame)) continue;

        if (frame.type == MSG_HEARTBEAT) {
            gLastMasterHeartbeatMs = millis();
        }
    }
}

bool masterOnline() {
    return (millis() - gLastMasterHeartbeatMs) <= kNodeOfflineTimeoutMs;
}

void applyTouchStateMachine() {
    const int8_t touchNow = readTouchIndex();
    const uint32_t now = millis();

    if (touchNow != gRawTouchIndex) {
        gRawTouchIndex = touchNow;
        gTouchChangedMs = now;
        return;
    }

    if (now - gTouchChangedMs < 30) {
        return;
    }

    if (gStableTouchIndex == gRawTouchIndex) {
        return;
    }

    if (gStableTouchIndex >= 0) {
        const uint8_t oldIndex = static_cast<uint8_t>(gStableTouchIndex);
        gPressedMask &= static_cast<uint16_t>(~(1u << oldIndex));
        drawButton(oldIndex);
        sendInputEvent(kButtons[oldIndex].commandId, EVENT_RELEASE);
    }

    gStableTouchIndex = gRawTouchIndex;
    if (gStableTouchIndex >= 0) {
        const uint8_t newIndex = static_cast<uint8_t>(gStableTouchIndex);
        gPressedMask |= static_cast<uint16_t>(1u << newIndex);
        drawButton(newIndex);
        sendInputEvent(kButtons[newIndex].commandId, EVENT_PRESS);
    }
}

void updateStatusLed() {
    digitalWrite(LED_BUILTIN, masterOnline() ? HIGH : LOW);
}

} // namespace

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    uint16_t id = gTft.readID();
    if (id == 0xD3D3 || id == 0x0000) {
        id = 0x9486;
    }
    gTft.begin(id);
    gTft.setRotation(kScreenRotation);
    drawGrid();
}

void loop() {
    pollMaster();
    applyTouchStateMachine();
    sendHeartbeatIfDue();
    sendInputStateIfDue(0xFF, EVENT_NONE);
    updateStatusLed();
}
