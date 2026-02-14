#ifndef ELLERT_PROTOCOL_V1_H
#define ELLERT_PROTOCOL_V1_H

#include <stdint.h>

namespace EllertProtocolV1 {

static const uint8_t kProtocolVersion = 1;
static const uint8_t kSof = 0xAA;

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

} // namespace EllertProtocolV1

#endif
