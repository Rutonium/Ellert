#include "RemoteInterfaces.h"

#include "protocol_v1.h"

namespace {
using namespace EllertProtocolV1;

Decoder gInputDecoder;
Decoder gDisplayDecoder;

uint8_t gMasterSeq = 0;
uint8_t gLastInputSeq = 0;
uint32_t gLastInputHeartbeatMs = 0;
uint32_t gLastDisplayHeartbeatMs = 0;
uint32_t gLastHeartbeatTxMs = 0;
uint32_t gLastStatusTxMs = 0;

uint8_t gInputFlags = 0;
uint8_t gInputRaw[8] = {0};
uint16_t gInputButtonMask = 0;
uint8_t gLastEventCommand = 0xFF;
uint8_t gLastEventType = 0;
uint8_t gLastEventSeq = 0xFF;

const char* commandName(uint8_t commandId) {
    switch (commandId) {
        case 0: return "LIGHTS_OFF";
        case 1: return "LIGHTS_PARK";
        case 2: return "LIGHTS_LOW";
        case 3: return "LIGHTS_HIGH";
        case 4: return "IND_LEFT";
        case 5: return "IND_RIGHT";
        case 6: return "HAZARD";
        case 7: return "HORN";
        case 8: return "WIPER_INT";
        case 9: return "WIPER_LOW";
        case 10: return "WIPER_HIGH";
        case 11: return "WASHER";
        case 12: return "FAN_LOW";
        case 13: return "FAN_MID";
        case 14: return "FAN_HIGH";
        case 15: return "DEMIST";
        default: return "UNKNOWN";
    }
}

void writeFrame(HardwareSerial& port, uint8_t type, const uint8_t* payload, uint8_t len) {
    uint8_t buffer[kMaxPayloadLen + kFrameOverhead];
    const size_t frameLen = encodeFrame(type, gMasterSeq++, payload, len, buffer, sizeof(buffer));
    if (frameLen > 0) {
        port.write(buffer, frameLen);
    }
}

void handleInputFrame(const Frame& frame) {
    if (frame.type == MSG_HEARTBEAT) {
        gLastInputHeartbeatMs = millis();
        return;
    }

    if (frame.type == MSG_INPUT_STATE) {
        gLastInputHeartbeatMs = millis();
        gLastInputSeq = frame.seq;
        if (frame.len >= 1) gInputFlags = frame.payload[0];
        if (frame.len >= 3) {
            gInputButtonMask = static_cast<uint16_t>(frame.payload[1]) |
                               (static_cast<uint16_t>(frame.payload[2]) << 8);
        }

        const uint8_t maxCopy = sizeof(gInputRaw);
        for (uint8_t i = 0; i < maxCopy; ++i) {
            gInputRaw[i] = (i < frame.len) ? frame.payload[i] : 0;
        }

        if (frame.len >= 5) {
            const uint8_t eventCommand = frame.payload[3];
            const uint8_t eventType = frame.payload[4];
            if (eventType != 0 && frame.seq != gLastEventSeq) {
                gLastEventSeq = frame.seq;
                gLastEventCommand = eventCommand;
                gLastEventType = eventType;

                Serial.print("INPUT_EVENT|cmd=");
                Serial.print(commandName(eventCommand));
                Serial.print("|type=");
                Serial.print(eventType == 1 ? "PRESS" : "RELEASE");
                Serial.print("|mask=0x");
                Serial.println(gInputButtonMask, HEX);
            }
        }
    }
}

void handleDisplayFrame(const Frame& frame) {
    if (frame.type == MSG_HEARTBEAT) {
        gLastDisplayHeartbeatMs = millis();
    }
}

void pollPort(HardwareSerial& port, Decoder& decoder, bool isInputPort) {
    while (port.available() > 0) {
        const uint8_t b = static_cast<uint8_t>(port.read());
        Frame frame;
        if (decoder.feed(b, frame)) {
            if (isInputPort) {
                handleInputFrame(frame);
            } else {
                handleDisplayFrame(frame);
            }
        }
    }
}

void sendHeartbeatIfDue() {
    const uint32_t now = millis();
    if (now - gLastHeartbeatTxMs < kHeartbeatMs) return;
    gLastHeartbeatTxMs = now;

    const uint8_t payload[2] = {
        static_cast<uint8_t>(RemoteInterfaces::inputCpuOnline() ? 1 : 0),
        static_cast<uint8_t>(RemoteInterfaces::displayCpuOnline() ? 1 : 0)
    };
    writeFrame(Serial1, MSG_HEARTBEAT, payload, sizeof(payload));
    writeFrame(Serial2, MSG_HEARTBEAT, payload, sizeof(payload));
}

void sendStatusIfDue() {
    const uint32_t now = millis();
    if (now - gLastStatusTxMs < kMasterStatusMs) return;
    gLastStatusTxMs = now;

    uint8_t payload[8];
    payload[0] = static_cast<uint8_t>(now & 0xFF);
    payload[1] = static_cast<uint8_t>((now >> 8) & 0xFF);
    payload[2] = static_cast<uint8_t>((now >> 16) & 0xFF);
    payload[3] = static_cast<uint8_t>((now >> 24) & 0xFF);
    payload[4] = static_cast<uint8_t>(gInputFlags);
    payload[5] = static_cast<uint8_t>(gLastInputSeq);
    payload[6] = static_cast<uint8_t>(RemoteInterfaces::inputCpuOnline() ? 1 : 0);
    payload[7] = static_cast<uint8_t>(RemoteInterfaces::displayCpuOnline() ? 1 : 0);

    writeFrame(Serial2, MSG_STATUS_SNAPSHOT, payload, sizeof(payload));
}

} // namespace

void RemoteInterfaces::initialize() {
    Serial1.begin(115200); // Input CPU link
    Serial2.begin(115200); // Display CPU link

    gInputDecoder.reset();
    gDisplayDecoder.reset();

    gLastInputHeartbeatMs = 0;
    gLastDisplayHeartbeatMs = 0;
    gLastHeartbeatTxMs = 0;
    gLastStatusTxMs = 0;
    gMasterSeq = 0;
}

void RemoteInterfaces::update() {
    pollPort(Serial1, gInputDecoder, true);
    pollPort(Serial2, gDisplayDecoder, false);
    sendHeartbeatIfDue();
    sendStatusIfDue();
}

bool RemoteInterfaces::inputCpuOnline() {
    const uint32_t now = millis();
    return (now - gLastInputHeartbeatMs) <= kNodeOfflineTimeoutMs;
}

bool RemoteInterfaces::displayCpuOnline() {
    const uint32_t now = millis();
    return (now - gLastDisplayHeartbeatMs) <= kNodeOfflineTimeoutMs;
}
