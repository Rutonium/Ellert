#ifndef ELLERT_PROTOCOL_V1_H
#define ELLERT_PROTOCOL_V1_H

#include <stdint.h>
#include <stddef.h>

namespace EllertProtocolV1 {

static const uint8_t kProtocolVersion = 1;
static const uint8_t kSof = 0xAA;
static const uint8_t kMaxPayloadLen = 48;
static const uint8_t kFrameOverhead = 6; // sof, version, type, seq, len, crc8
static const uint32_t kHeartbeatMs = 500;
static const uint32_t kMasterStatusMs = 200;
static const uint32_t kNodeOfflineTimeoutMs = 1500;
static const uint8_t kStatusPayloadLen = 18;

enum GearCode : uint8_t {
    GEAR_P = 0,
    GEAR_R = 1,
    GEAR_N = 2,
    GEAR_D = 3,
    GEAR_UNKNOWN = 255
};

enum StatusIndicatorBits : uint8_t {
    IND_LEFT = 1 << 0,
    IND_RIGHT = 1 << 1,
    IND_HAZARD = 1 << 2
};

enum StatusLightBits : uint8_t {
    LIGHT_DRL = 1 << 0,
    LIGHT_NEAR = 1 << 1,
    LIGHT_HIGH = 1 << 2,
    LIGHT_BRAKE = 1 << 3
};

enum MessageType : uint8_t {
    MSG_HEARTBEAT = 0x01,
    MSG_INPUT_STATE = 0x10,
    MSG_STATUS_SNAPSHOT = 0x20,
    MSG_ACK = 0x7F
};

struct FrameHeader {
    uint8_t sof;
    uint8_t version;
    uint8_t type;
    uint8_t seq;
    uint8_t len;
};

struct Frame {
    uint8_t type;
    uint8_t seq;
    uint8_t len;
    uint8_t payload[kMaxPayloadLen];
};

inline uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 0x80) {
                crc = static_cast<uint8_t>((crc << 1) ^ 0x07);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

inline size_t encodeFrame(
    uint8_t type,
    uint8_t seq,
    const uint8_t* payload,
    uint8_t len,
    uint8_t* outBuffer,
    size_t outSize) {
    if (len > kMaxPayloadLen) return 0;
    const size_t frameLen = static_cast<size_t>(len) + kFrameOverhead;
    if (outSize < frameLen) return 0;

    outBuffer[0] = kSof;
    outBuffer[1] = kProtocolVersion;
    outBuffer[2] = type;
    outBuffer[3] = seq;
    outBuffer[4] = len;
    for (uint8_t i = 0; i < len; ++i) {
        outBuffer[5 + i] = payload[i];
    }
    outBuffer[5 + len] = crc8(&outBuffer[1], static_cast<size_t>(4 + len));
    return frameLen;
}

class Decoder {
public:
    Decoder() { reset(); }

    void reset() {
        state_ = 0;
        expectedPayloadLen_ = 0;
        payloadIndex_ = 0;
        checksum_ = 0;
        frame_.type = 0;
        frame_.seq = 0;
        frame_.len = 0;
    }

    bool feed(uint8_t byte, Frame& outFrame) {
        switch (state_) {
            case 0: // SOF
                if (byte == kSof) {
                    state_ = 1;
                    checksum_ = 0;
                }
                return false;

            case 1: // version
                if (byte != kProtocolVersion) {
                    reset();
                    return false;
                }
                checksum_ = crc8Step(checksum_, byte);
                state_ = 2;
                return false;

            case 2: // type
                frame_.type = byte;
                checksum_ = crc8Step(checksum_, byte);
                state_ = 3;
                return false;

            case 3: // seq
                frame_.seq = byte;
                checksum_ = crc8Step(checksum_, byte);
                state_ = 4;
                return false;

            case 4: // len
                if (byte > kMaxPayloadLen) {
                    reset();
                    return false;
                }
                frame_.len = byte;
                expectedPayloadLen_ = byte;
                payloadIndex_ = 0;
                checksum_ = crc8Step(checksum_, byte);
                state_ = expectedPayloadLen_ == 0 ? 6 : 5;
                return false;

            case 5: // payload
                frame_.payload[payloadIndex_++] = byte;
                checksum_ = crc8Step(checksum_, byte);
                if (payloadIndex_ >= expectedPayloadLen_) {
                    state_ = 6;
                }
                return false;

            case 6: // crc
                if (byte == checksum_) {
                    outFrame = frame_;
                    reset();
                    return true;
                }
                reset();
                return false;

            default:
                reset();
                return false;
        }
    }

private:
    static uint8_t crc8Step(uint8_t crc, uint8_t data) {
        crc ^= data;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 0x80) {
                crc = static_cast<uint8_t>((crc << 1) ^ 0x07);
            } else {
                crc <<= 1;
            }
        }
        return crc;
    }

    uint8_t state_;
    uint8_t expectedPayloadLen_;
    uint8_t payloadIndex_;
    uint8_t checksum_;
    Frame frame_;
};

} // namespace EllertProtocolV1

#endif
