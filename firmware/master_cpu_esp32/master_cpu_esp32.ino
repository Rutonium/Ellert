#include <Arduino.h>

#include "protocol_v1.h"

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
constexpr int kInputRxPin = 16;
constexpr int kInputTxPin = 17;
constexpr int kDisplayRxPin = 18;
constexpr int kDisplayTxPin = 19;

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
} gState;

uint16_t gTripTotalTenthsKm = 0;
uint16_t gTripSinceChargeTenthsKm = 0;
uint32_t gLastTripUpdateMs = 0;

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
      if (pressed) applyFanMode(1);
      break;
    case CMD_FAN_MID:
      if (pressed) applyFanMode(2);
      break;
    case CMD_FAN_HIGH:
      if (pressed) applyFanMode(3);
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

  writeFrame(gDisplayUart, MSG_STATUS_SNAPSHOT, payload, sizeof(payload));
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
}

void loop() {
  pollPort(gInputUart, gInputDecoder, true);
  pollPort(gDisplayUart, gDisplayDecoder, false);

  updateOnlineStates();
  updateIndicators();
  updateTripsAndBrake();
  sendHeartbeatIfDue();
  sendStatusIfDue();

  digitalWrite(LED_BUILTIN, gState.inputOnline ? HIGH : LOW);
}
