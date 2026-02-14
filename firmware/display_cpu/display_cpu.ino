#include <Arduino.h>
#include <MCUFRIEND_kbv.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

#include "protocol_v1.h"

namespace {
using namespace EllertProtocolV1;

uint8_t gSeq = 0;
uint32_t gLastHeartbeatTxMs = 0;
uint32_t gLastMasterHeartbeatMs = 0;
uint32_t gLastStatusRxMs = 0;
uint32_t gLastRenderMs = 0;

Decoder gDecoder;
MCUFRIEND_kbv gTft;

const uint8_t kDisplayRotation = 1; // Horizontal landscape
const bool kDemoMode = true;

struct DashboardState {
    uint8_t speedMph;
    uint8_t socPct;
    int8_t powerUsedPct;
    uint8_t powerAskedPct;
    uint8_t indicatorBits;
    uint8_t lightBits;
    uint8_t wiperMode;
    uint8_t gear;
    uint16_t tripTotalTenthsMi;
    uint16_t tripSinceChargeTenthsMi;
    bool inputOnline;
    bool displayOnlineFromMaster;
    bool ready;
    uint8_t lastCommand;
    uint16_t inputMask;
    int8_t outsideTempC;
};

DashboardState gState = {0};
bool gStaticDrawn = false;

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

    const uint8_t payload[1] = {1}; // Display CPU alive
    sendFrame(MSG_HEARTBEAT, payload, sizeof(payload));
}

void applyStatusSnapshot(const Frame& frame) {
    if (frame.len < kStatusPayloadLen) return;

    gState.speedMph = frame.payload[0];
    gState.socPct = frame.payload[1];
    gState.powerUsedPct = static_cast<int8_t>(frame.payload[2]);
    gState.powerAskedPct = frame.payload[3];
    gState.indicatorBits = frame.payload[4];
    gState.lightBits = frame.payload[5];
    gState.wiperMode = frame.payload[6];
    gState.gear = frame.payload[7];
    gState.tripTotalTenthsMi = static_cast<uint16_t>(frame.payload[8]) |
                               (static_cast<uint16_t>(frame.payload[9]) << 8);
    gState.tripSinceChargeTenthsMi = static_cast<uint16_t>(frame.payload[10]) |
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
    while (Serial.available() > 0) {
        const uint8_t b = static_cast<uint8_t>(Serial.read());
        Frame frame;
        if (!gDecoder.feed(b, frame)) continue;

        if (frame.type == MSG_HEARTBEAT) {
            gLastMasterHeartbeatMs = millis();
        } else if (frame.type == MSG_STATUS_SNAPSHOT) {
            applyStatusSnapshot(frame);
        }
    }
}

bool masterOnline() {
    return (millis() - gLastMasterHeartbeatMs) <= kNodeOfflineTimeoutMs;
}

const uint16_t C_BG = 0x0000;
const uint16_t C_PANEL = 0x18C3;
const uint16_t C_TEXT = 0xFFFF;
const uint16_t C_ACCENT = 0x07FF;
const uint16_t C_WARN = 0xFD20;
const uint16_t C_GOOD = 0x07E0;
const uint16_t C_BAD = 0xF800;
const uint16_t C_IND_YELLOW = 0xFFE0;

int textWidthPx(const char* text, uint8_t textSize) {
    int width = 0;
    while (*text) {
        ++width;
        ++text;
    }
    return width * 6 * textSize;
}

void drawCenteredTextInRect(int x, int y, int w, int h, const char* text, uint8_t size, uint16_t fg, uint16_t bg) {
    const int tw = textWidthPx(text, size);
    const int tx = x + ((w - tw) / 2);
    const int ty = y + ((h - (8 * size)) / 2);
    gTft.setTextColor(fg, bg);
    gTft.setTextSize(size);
    gTft.setCursor(tx, ty);
    gTft.print(text);
}

void drawCenteredGfxTextInRect(int x, int y, int w, int h, const char* text, const GFXfont* font, uint16_t fg) {
    int16_t x1, y1;
    uint16_t tw, th;
    gTft.setFont(font);
    gTft.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);

    const int tx = x + ((w - static_cast<int>(tw)) / 2) - x1;
    const int ty = y + ((h - static_cast<int>(th)) / 2) - y1;

    gTft.setTextColor(fg);
    gTft.setCursor(tx, ty);
    gTft.print(text);
    gTft.setFont(nullptr);
}

bool indicatorBlinkOn() {
    // 0.7 second ON, 0.7 second OFF
    return ((millis() / 700UL) % 2UL) == 0;
}

const char* gearLabel(uint8_t gear) {
    switch (gear) {
        case GEAR_P: return "P";
        case GEAR_R: return "R";
        case GEAR_N: return "N";
        case GEAR_D: return "D";
        default: return "-";
    }
}

void drawStaticFrame() {
    gTft.fillScreen(C_BG);

    gTft.fillRoundRect(6, 6, 468, 54, 8, C_PANEL);     // Top status band
    gTft.fillRoundRect(6, 66, 142, 208, 8, C_PANEL);   // Left SOC panel
    gTft.fillRoundRect(154, 66, 172, 208, 8, C_PANEL); // Center speed panel
    gTft.fillRoundRect(332, 66, 142, 208, 8, C_PANEL); // Right power panel
    gTft.fillRoundRect(6, 280, 468, 34, 8, C_PANEL);   // Bottom strip

    gTft.setTextColor(C_TEXT, C_PANEL);
    gTft.setTextSize(2);
    gTft.setCursor(16, 24);
    gTft.print("IND");
    gTft.setCursor(190, 24);
    gTft.print("STATUS");
    gTft.setCursor(380, 24);
    gTft.print("LIGHTS");

    drawCenteredTextInRect(154, 76, 172, 20, "SPEED", 2, C_TEXT, C_PANEL);
    drawCenteredTextInRect(332, 76, 142, 20, "POWER", 2, C_TEXT, C_PANEL);
}

void drawSpeedIndicators() {
    // Place indicators on each side of SPEED title.
    const int leftCx = 178;
    const int rightCx = 302;
    const int cy = 86;
    const int r = 12;

    gTft.fillRect(leftCx - 16, cy - 16, 32, 32, C_PANEL);
    gTft.fillRect(rightCx - 16, cy - 16, 32, 32, C_PANEL);

    const bool blink = indicatorBlinkOn();
    if ((gState.indicatorBits & IND_LEFT) && blink) {
        gTft.fillCircle(leftCx, cy, r, C_IND_YELLOW);
    }
    if ((gState.indicatorBits & IND_RIGHT) && blink) {
        gTft.fillCircle(rightCx, cy, r, C_IND_YELLOW);
    }

    gTft.setTextSize(2);
    gTft.setTextColor(C_TEXT, C_PANEL);
    gTft.setCursor(leftCx - 6, cy - 8);
    gTft.print("<");
    gTft.setCursor(rightCx - 6, cy - 8);
    gTft.print(">");
}

void drawBatteryPanel() {
    const int x = 26;
    const int y = 100;
    const int w = 56;
    const int h = 120;
    gTft.fillRect(14, 96, 124, 170, C_PANEL);

    gTft.drawRect(x, y, w, h, C_TEXT);
    gTft.fillRect(x + 18, y - 8, 20, 6, C_TEXT);

    int fillH = map(gState.socPct, 0, 100, 0, h - 4);
    if (fillH < 0) fillH = 0;
    if (fillH > h - 4) fillH = h - 4;
    const uint16_t fillColor = gState.socPct >= 35 ? C_GOOD : C_BAD;
    gTft.fillRect(x + 2, y + 2, w - 4, h - 4, C_BG);
    gTft.fillRect(x + 2, y + h - 2 - fillH, w - 4, fillH, fillColor);

    gTft.setTextColor(C_TEXT, C_PANEL);
    gTft.setTextSize(2);
    gTft.setCursor(88, 132);
    gTft.print(gState.socPct);
    gTft.print("%");

    gTft.setCursor(88, 164);
    gTft.print((gState.socPct * 2));
    gTft.print("km");
}

void drawSpeedPanel() {
    gTft.fillRect(164, 102, 152, 156, C_PANEL);

    char speedBuf[5];
    snprintf(speedBuf, sizeof(speedBuf), "%u", gState.speedMph);
    drawCenteredGfxTextInRect(164, 112, 152, 108, speedBuf, &FreeSansBold24pt7b, C_TEXT);

    gTft.setTextSize(2);
    gTft.setCursor(214, 230);
    gTft.print("km/t");

    // Gear indicator: lower-right inside speed panel.
    gTft.setTextColor(C_TEXT, C_PANEL);
    gTft.setTextSize(4);
    gTft.setCursor(276, 224);
    gTft.print(gearLabel(gState.gear));
}

void drawPowerPanel() {
    gTft.fillRect(342, 102, 122, 156, C_PANEL);

    const int askH = map(gState.powerAskedPct, 0, 100, 0, 120);
    int usedPct = gState.powerUsedPct;
    if (usedPct < 0) usedPct = -usedPct;
    const int usedH = map(usedPct, 0, 100, 0, 120);

    gTft.drawRect(356, 118, 32, 122, C_TEXT);
    gTft.drawRect(408, 118, 32, 122, C_TEXT);
    gTft.fillRect(358, 120, 28, 118, C_BG);
    gTft.fillRect(410, 120, 28, 118, C_BG);

    gTft.fillRect(358, 238 - askH, 28, askH, C_ACCENT);
    const uint16_t usedColor = gState.powerUsedPct < 0 ? C_GOOD : C_WARN;
    gTft.fillRect(410, 238 - usedH, 28, usedH, usedColor);

    gTft.setTextColor(C_TEXT, C_PANEL);
    gTft.setTextSize(1);
    gTft.setCursor(356, 246);
    gTft.print("ASK");
    gTft.setCursor(410, 246);
    gTft.print("USED");
}

void drawTopStatus() {
    gTft.fillRect(12, 12, 456, 42, C_PANEL);

    // Time in top-left (demo clock based on uptime).
    const uint32_t totalMin = (millis() / 60000UL) + (12UL * 60UL + 34UL);
    const uint8_t hh = static_cast<uint8_t>((totalMin / 60UL) % 24UL);
    const uint8_t mm = static_cast<uint8_t>(totalMin % 60UL);
    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u", hh, mm);

    gTft.setTextSize(2);
    gTft.setTextColor(C_TEXT, C_PANEL);
    gTft.setCursor(18, 22);
    gTft.print(timeBuf);

    gTft.setTextColor(gState.ready ? C_GOOD : C_WARN, C_PANEL);
    gTft.setCursor(186, 22);
    gTft.print(gState.ready ? "READY" : "WAIT");

    gTft.setTextColor(C_TEXT, C_PANEL);
    gTft.setCursor(268, 22);
    gTft.print("IN:");
    gTft.print(gState.inputOnline ? "OK" : "NO");

    gTft.setCursor(334, 22);
    gTft.print("L:");
    gTft.print((gState.lightBits & LIGHT_DRL) ? "D" : "-");
    gTft.print((gState.lightBits & LIGHT_NEAR) ? "N" : "-");
    gTft.print((gState.lightBits & LIGHT_HIGH) ? "H" : "-");
    gTft.print((gState.lightBits & LIGHT_BRAKE) ? "B" : "-");

    gTft.setCursor(430, 22);
    gTft.print(gState.outsideTempC);
    gTft.print("C");
}

void drawBottomStrip() {
    gTft.fillRect(12, 286, 456, 22, C_PANEL);

    gTft.setTextColor(C_TEXT, C_PANEL);
    gTft.setTextSize(2);
    gTft.setCursor(18, 290);
    gTft.print("Trip:");
    gTft.print(gState.tripTotalTenthsMi / 10);
    gTft.print(".");
    gTft.print(gState.tripTotalTenthsMi % 10);
    gTft.print("km");

    gTft.setCursor(186, 290);
    gTft.print("SinceChg:");
    gTft.print(gState.tripSinceChargeTenthsMi / 10);
    gTft.print(".");
    gTft.print(gState.tripSinceChargeTenthsMi % 10);
    gTft.print("km");

    gTft.setCursor(390, 290);
    gTft.print(" ");
}

void renderDashboard() {
    const bool hasRecentStatus = (millis() - gLastStatusRxMs) <= (kMasterStatusMs * 5);
    const bool online = masterOnline() && hasRecentStatus && gState.displayOnlineFromMaster;
    digitalWrite(LED_BUILTIN, online ? HIGH : LOW);

    if (!gStaticDrawn) {
        drawStaticFrame();
        gStaticDrawn = true;
    }

    if (!online) {
        gTft.fillRect(166, 140, 148, 60, C_PANEL);
        gTft.setTextColor(C_BAD, C_PANEL);
        gTft.setTextSize(2);
        gTft.setCursor(182, 164);
        gTft.print("NO DATA");
        return;
    }

    drawTopStatus();
    drawSpeedIndicators();
    drawBatteryPanel();
    drawSpeedPanel();
    drawPowerPanel();
    drawBottomStrip();
}

} // namespace

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    uint16_t id = gTft.readID();
    if (id == 0xD3D3 || id == 0x0000) id = 0x9486;
    gTft.begin(id);
    gTft.setRotation(kDisplayRotation);
    gTft.fillScreen(C_BG);

    if (kDemoMode) {
        gState.speedMph = 0;
        gState.socPct = 34;
        gState.powerUsedPct = 32;
        gState.powerAskedPct = 46;
        gState.indicatorBits = IND_LEFT;
        gState.lightBits = static_cast<uint8_t>(LIGHT_DRL);
        gState.wiperMode = 0;
        gState.gear = GEAR_D;
        gState.tripTotalTenthsMi = 3450;       // 345.0 km
        gState.tripSinceChargeTenthsMi = 220;  // 22.0 km
        gState.inputOnline = true;
        gState.displayOnlineFromMaster = true;
        gState.ready = true;
        gState.lastCommand = 0;
        gState.inputMask = 0;
        gState.outsideTempC = 20;
        gLastMasterHeartbeatMs = millis();
        gLastStatusRxMs = millis();
    }
}

void loop() {
    if (!kDemoMode) {
        pollMaster();
        sendHeartbeatIfDue();
    }
    const uint32_t now = millis();
    if (now - gLastRenderMs >= 120) {
        gLastRenderMs = now;
        renderDashboard();
    }
}
