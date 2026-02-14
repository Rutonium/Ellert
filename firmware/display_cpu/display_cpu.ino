#include <Arduino.h>

#include "protocol_v1.h"

namespace {
using namespace EllertProtocolV1;

uint8_t gSeq = 0;
uint32_t gLastHeartbeatTxMs = 0;
uint32_t gLastMasterHeartbeatMs = 0;
uint32_t gLastStatusRxMs = 0;

Decoder gDecoder;

uint32_t gMasterUptimeMs = 0;
uint8_t gInputFlags = 0;
uint8_t gInputSeq = 0;
bool gInputOnline = false;
bool gDisplayOnlineFromMaster = false;

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
    if (frame.len < 8) return;

    gMasterUptimeMs = static_cast<uint32_t>(frame.payload[0]) |
                      (static_cast<uint32_t>(frame.payload[1]) << 8) |
                      (static_cast<uint32_t>(frame.payload[2]) << 16) |
                      (static_cast<uint32_t>(frame.payload[3]) << 24);

    gInputFlags = frame.payload[4];
    gInputSeq = frame.payload[5];
    gInputOnline = frame.payload[6] != 0;
    gDisplayOnlineFromMaster = frame.payload[7] != 0;
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

void renderPlaceholder() {
    // Placeholder rendering: LED reflects master online/offline.
    // Replace this with actual TFT rendering on the display controller.
    const bool hasRecentStatus = (millis() - gLastStatusRxMs) <= (kMasterStatusMs * 4);
    const bool ledOn = masterOnline() && hasRecentStatus && gDisplayOnlineFromMaster;
    digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);

    (void)gMasterUptimeMs;
    (void)gInputFlags;
    (void)gInputSeq;
    (void)gInputOnline;
}

} // namespace

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    pollMaster();
    sendHeartbeatIfDue();
    renderPlaceholder();
}
