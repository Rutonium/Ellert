#include "PinMap.h"
#include "PinDefinitions.h"
#include "PinConstraints.h"
#include "PinRules.h"
#include "EEPROM.h"

namespace {
    constexpr uint32_t kMagic = 0x50494E4D; // 'PINM'
    constexpr uint16_t kVersion = 1;

    constexpr int kAnalogCount = PinMap::kAnalogCount;
    constexpr int kDigitalCount = PinMap::kDigitalCount;
    constexpr int kOutputCount = PinMap::kOutputCount;

    struct PinMapData {
        uint32_t magic;
        uint16_t version;
        uint16_t checksum;
        int16_t analogPins[kAnalogCount];
        int16_t digitalPins[kDigitalCount];
        int16_t outputPins[kOutputCount];
    };

    PinMapData g_data;
    bool g_dirty = false;

    uint16_t computeChecksum(const PinMapData& data) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
        const size_t len = sizeof(PinMapData) - sizeof(uint16_t); // exclude checksum field
        uint16_t sum = 0;
        for (size_t i = 0; i < len; ++i) {
            sum = static_cast<uint16_t>(sum + bytes[i]);
        }
        return sum;
    }
}

void PinMap::applyDefaults() {
    g_data.magic = kMagic;
    g_data.version = kVersion;

    g_data.analogPins[static_cast<int>(AnalogInputId::Accelerator)] = PIN_ACCELERATOR;

    g_data.digitalPins[static_cast<int>(DigitalInputId::IgnitionOn)] = PIN_IGNITION_ON;
    g_data.digitalPins[static_cast<int>(DigitalInputId::IgnitionAcc)] = PIN_IGNITION_ACC;
    g_data.digitalPins[static_cast<int>(DigitalInputId::IgnitionStart)] = PIN_IGNITION_START;
    g_data.digitalPins[static_cast<int>(DigitalInputId::BrakePedal)] = PIN_BRAKE_PEDAL;
    g_data.digitalPins[static_cast<int>(DigitalInputId::Handbrake)] = PIN_HANDBRAKE;
    g_data.digitalPins[static_cast<int>(DigitalInputId::GearPark)] = PIN_GEAR_PARK;
    g_data.digitalPins[static_cast<int>(DigitalInputId::GearReverse)] = PIN_GEAR_REVERSE;
    g_data.digitalPins[static_cast<int>(DigitalInputId::GearNeutral)] = PIN_GEAR_NEUTRAL;
    g_data.digitalPins[static_cast<int>(DigitalInputId::GearDrive)] = PIN_GEAR_DRIVE;
    g_data.digitalPins[static_cast<int>(DigitalInputId::IndStalkLeft)] = PIN_IND_STALK_LEFT;
    g_data.digitalPins[static_cast<int>(DigitalInputId::IndStalkRight)] = PIN_IND_STALK_RIGHT;
    g_data.digitalPins[static_cast<int>(DigitalInputId::LightsStalkParking)] = PIN_LIGHTS_STALK_PARKING;
    g_data.digitalPins[static_cast<int>(DigitalInputId::LightsStalkHead)] = PIN_LIGHTS_STALK_HEAD;
    g_data.digitalPins[static_cast<int>(DigitalInputId::HighBeamStalk)] = PIN_HIGH_BEAM_STALK;
    g_data.digitalPins[static_cast<int>(DigitalInputId::HazardSwitch)] = PIN_HAZARD_SWITCH;
    g_data.digitalPins[static_cast<int>(DigitalInputId::WiperStalkLow)] = PIN_WIPER_STALK_LOW;
    g_data.digitalPins[static_cast<int>(DigitalInputId::WiperStalkHigh)] = PIN_WIPER_STALK_HIGH;
    g_data.digitalPins[static_cast<int>(DigitalInputId::WiperStalkInt)] = PIN_WIPER_STALK_INT;
    g_data.digitalPins[static_cast<int>(DigitalInputId::WasherSwitch)] = PIN_WASHER_SWITCH;
    g_data.digitalPins[static_cast<int>(DigitalInputId::HornButton)] = PIN_HORN_BUTTON;
    g_data.digitalPins[static_cast<int>(DigitalInputId::DemisterSwitch)] = PIN_DEMISTER_SWITCH;
    g_data.digitalPins[static_cast<int>(DigitalInputId::TripResetButton)] = PIN_TRIP_RESET_BUTTON;
    g_data.digitalPins[static_cast<int>(DigitalInputId::HeatElem1Sw)] = PIN_HEAT_ELEM_1_SW;
    g_data.digitalPins[static_cast<int>(DigitalInputId::HeatElem2Sw)] = PIN_HEAT_ELEM_2_SW;
    g_data.digitalPins[static_cast<int>(DigitalInputId::FanLow)] = PIN_FAN_LOW;
    g_data.digitalPins[static_cast<int>(DigitalInputId::FanMid)] = PIN_FAN_MID;
    g_data.digitalPins[static_cast<int>(DigitalInputId::FanHigh)] = PIN_FAN_HIGH;
    g_data.digitalPins[static_cast<int>(DigitalInputId::AcOffAll)] = PIN_AC_OFF_ALL;
    g_data.digitalPins[static_cast<int>(DigitalInputId::AcVentOnly)] = PIN_AC_VENT_ONLY;
    g_data.digitalPins[static_cast<int>(DigitalInputId::AcHeat1)] = PIN_AC_HEAT_1;
    g_data.digitalPins[static_cast<int>(DigitalInputId::AcHeat2)] = PIN_AC_HEAT_2;
    g_data.digitalPins[static_cast<int>(DigitalInputId::WiperStartUser)] = PIN_WIPER_START_USER;
    g_data.digitalPins[static_cast<int>(DigitalInputId::WiperStopUser)] = PIN_WIPER_STOP_USER;
    g_data.digitalPins[static_cast<int>(DigitalInputId::SprinklerUser)] = PIN_SPRINKLER_USER;
    g_data.digitalPins[static_cast<int>(DigitalInputId::LightsNormalUser)] = PIN_LIGHTS_NORMAL_USER;
    g_data.digitalPins[static_cast<int>(DigitalInputId::LightsHighUser)] = PIN_LIGHTS_HIGH_USER;
    g_data.digitalPins[static_cast<int>(DigitalInputId::LightsOffUser)] = PIN_LIGHTS_OFF_USER;

    g_data.outputPins[static_cast<int>(OutputId::MainContactor)] = PIN_MAIN_CONTACTOR;
    g_data.outputPins[static_cast<int>(OutputId::BrakeLightsRelay)] = PIN_BRAKE_LIGHTS_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::HeadlightsRelay)] = PIN_HEADLIGHTS_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::HighBeamsRelay)] = PIN_HIGH_BEAMS_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::ParkingLightsRelay)] = PIN_PARKING_LIGHTS_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::IndLeftRelay)] = PIN_IND_LEFT_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::IndRightRelay)] = PIN_IND_RIGHT_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::HornRelay)] = PIN_HORN_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::ReverseLightRelay)] = PIN_REVERSE_LIGHT_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::PBrakeIndicator)] = PIN_P_BRAKE_INDICATOR;
    g_data.outputPins[static_cast<int>(OutputId::InteriorLightRelay)] = PIN_INTERIOR_LIGHT_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::ChargingRelay)] = PIN_CHARGING_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::WiperIntRelay)] = PIN_WIPER_INT_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::WiperLowRelay)] = PIN_WIPER_LOW_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::WiperHighRelay)] = PIN_WIPER_HIGH_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::WasherPumpRelay)] = PIN_WASHER_PUMP_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::HeaterElem1Relay)] = PIN_HEATER_ELEM_1_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::HeaterElem2Relay)] = PIN_HEATER_ELEM_2_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::HeaterFanRelay)] = PIN_HEATER_FAN_RELAY;
    g_data.outputPins[static_cast<int>(OutputId::DemisterRelay)] = PIN_DEMISTER_RELAY;

    g_data.checksum = computeChecksum(g_data);
}

void PinMap::resetDefaults() {
    applyDefaults();
}

void PinMap::load() {
    EEPROM.get(0, g_data);
    if (g_data.magic != kMagic || g_data.version != kVersion) {
        applyDefaults();
        save();
        return;
    }
    uint16_t checksum = computeChecksum(g_data);
    if (checksum != g_data.checksum) {
        applyDefaults();
        save();
    }
}

void PinMap::save() {
    g_data.checksum = computeChecksum(g_data);
    EEPROM.put(0, g_data);
}

int PinMap::analogInput(AnalogInputId id) {
    return g_data.analogPins[static_cast<int>(id)];
}

int PinMap::digitalInput(DigitalInputId id) {
    return g_data.digitalPins[static_cast<int>(id)];
}

int PinMap::output(OutputId id) {
    return g_data.outputPins[static_cast<int>(id)];
}

void PinMap::setAnalogInput(AnalogInputId id, int pin) {
    g_data.analogPins[static_cast<int>(id)] = static_cast<int16_t>(pin);
    g_dirty = true;
}

void PinMap::setDigitalInput(DigitalInputId id, int pin) {
    g_data.digitalPins[static_cast<int>(id)] = static_cast<int16_t>(pin);
    g_dirty = true;
}

void PinMap::setOutput(OutputId id, int pin) {
    g_data.outputPins[static_cast<int>(id)] = static_cast<int16_t>(pin);
    g_dirty = true;
}

void PinMap::markDirty() {
    g_dirty = true;
}

bool PinMap::consumeDirty() {
    if (!g_dirty) return false;
    g_dirty = false;
    return true;
}

int PinMap::analogInputByIndex(int index) {
    if (index < 0 || index >= kAnalogCount) return -1;
    return g_data.analogPins[index];
}

int PinMap::digitalInputByIndex(int index) {
    if (index < 0 || index >= kDigitalCount) return -1;
    return g_data.digitalPins[index];
}

int PinMap::outputByIndex(int index) {
    if (index < 0 || index >= kOutputCount) return -1;
    return g_data.outputPins[index];
}

int PinMap::analogInputCount() {
    return kAnalogCount;
}

int PinMap::digitalInputCount() {
    return kDigitalCount;
}

int PinMap::outputCount() {
    return kOutputCount;
}

bool PinMap::validateNoConflicts() {
    bool ok = true;
    // Analog duplicates
    for (int i = 0; i < kAnalogCount; ++i) {
        const int pinI = g_data.analogPins[i];
        for (int j = i + 1; j < kAnalogCount; ++j) {
            if (pinI == g_data.analogPins[j]) ok = false;
        }
    }
    // Analog vs digital
    for (int i = 0; i < kAnalogCount; ++i) {
        const int pinI = g_data.analogPins[i];
        for (int j = 0; j < kDigitalCount; ++j) {
            if (pinI == g_data.digitalPins[j]) ok = false;
        }
        for (int j = 0; j < kOutputCount; ++j) {
            if (pinI == g_data.outputPins[j]) ok = false;
        }
    }
    for (int i = 0; i < kDigitalCount; ++i) {
        const int pinI = g_data.digitalPins[i];
        for (int j = i + 1; j < kDigitalCount; ++j) {
            if (pinI == g_data.digitalPins[j]) ok = false;
        }
    }
    for (int i = 0; i < kOutputCount; ++i) {
        const int pinI = g_data.outputPins[i];
        for (int j = i + 1; j < kOutputCount; ++j) {
            if (pinI == g_data.outputPins[j]) ok = false;
        }
    }
    for (int i = 0; i < kDigitalCount; ++i) {
        const int pinI = g_data.digitalPins[i];
        for (int j = 0; j < kOutputCount; ++j) {
            if (pinI == g_data.outputPins[j]) ok = false;
        }
    }
    return ok;
}

bool PinMap::wouldConflict(bool isOutput, int index, int proposedPin) {
    // Check conflicts with analog
    for (int i = 0; i < kAnalogCount; ++i) {
        if (g_data.analogPins[i] == proposedPin) return true;
    }

    if (isOutput) {
        for (int i = 0; i < kOutputCount; ++i) {
            if (i == index) continue;
            if (g_data.outputPins[i] == proposedPin) return true;
        }
        for (int i = 0; i < kDigitalCount; ++i) {
            if (g_data.digitalPins[i] == proposedPin) return true;
        }
    } else {
        for (int i = 0; i < kDigitalCount; ++i) {
            if (i == index) continue;
            if (g_data.digitalPins[i] == proposedPin) return true;
        }
        for (int i = 0; i < kOutputCount; ++i) {
            if (g_data.outputPins[i] == proposedPin) return true;
        }
    }
    return false;
}

bool PinMap::wouldConflictAnalog(int index, int proposedPin) {
    for (int i = 0; i < kAnalogCount; ++i) {
        if (i == index) continue;
        if (g_data.analogPins[i] == proposedPin) return true;
    }
    for (int i = 0; i < kDigitalCount; ++i) {
        if (g_data.digitalPins[i] == proposedPin) return true;
    }
    for (int i = 0; i < kOutputCount; ++i) {
        if (g_data.outputPins[i] == proposedPin) return true;
    }
    return false;
}

bool PinMap::isReservedPin(int pin) {
    if (PinRules::isReserved(pin)) return true;
    if (RESERVED_PINS_COUNT == 0) return false;
    for (int i = 0; i < RESERVED_PINS_COUNT; ++i) {
        if (RESERVED_PINS[i] == pin) return true;
    }
    return false;
}

bool PinMap::isAnalogCapablePin(int pin) {
    if (ALLOWED_ANALOG_PINS_COUNT == 0) return true;
    for (int i = 0; i < ALLOWED_ANALOG_PINS_COUNT; ++i) {
        if (ALLOWED_ANALOG_PINS[i] == pin) return true;
    }
    return false;
}

bool PinMap::isPwmCapablePin(int pin) {
    if (!PinRules::isPwmAllowed(pin)) return false;
    if (ALLOWED_PWM_PINS_COUNT == 0) return true;
    for (int i = 0; i < ALLOWED_PWM_PINS_COUNT; ++i) {
        if (ALLOWED_PWM_PINS[i] == pin) return true;
    }
    return false;
}

bool PinMap::isPwmRequiredOutput(OutputId id) {
    // No PWM-required outputs by default. Add cases here when needed.
    switch (id) {
        default:
            return false;
    }
}

bool PinMap::isCriticalOutput(OutputId id) {
    switch (id) {
        case OutputId::MainContactor:
        case OutputId::BrakeLightsRelay:
            return true;
        default:
            return false;
    }
}

const char* PinMap::digitalInputLabel(DigitalInputId id) {
    switch (id) {
        case DigitalInputId::IgnitionOn: return "Ignition ON";
        case DigitalInputId::IgnitionAcc: return "Ignition ACC";
        case DigitalInputId::IgnitionStart: return "Ignition START";
        case DigitalInputId::BrakePedal: return "Brake Pedal";
        case DigitalInputId::Handbrake: return "Handbrake";
        case DigitalInputId::GearPark: return "Gear Park";
        case DigitalInputId::GearReverse: return "Gear Reverse";
        case DigitalInputId::GearNeutral: return "Gear Neutral";
        case DigitalInputId::GearDrive: return "Gear Drive";
        case DigitalInputId::IndStalkLeft: return "Ind Left";
        case DigitalInputId::IndStalkRight: return "Ind Right";
        case DigitalInputId::LightsStalkParking: return "trykknap-panel Lights Parking";
        case DigitalInputId::LightsStalkHead: return "Lights Head";
        case DigitalInputId::HighBeamStalk: return "High Beam";
        case DigitalInputId::HazardSwitch: return "Hazard";
        case DigitalInputId::WiperStalkLow: return "trykknap-panel Wiper Low";
        case DigitalInputId::WiperStalkHigh: return "Wiper High";
        case DigitalInputId::WiperStalkInt: return "trykknap-panel Wiper Int";
        case DigitalInputId::WasherSwitch: return "Washer";
        case DigitalInputId::HornButton: return "trykknap-panel Horn";
        case DigitalInputId::DemisterSwitch: return "Demister";
        case DigitalInputId::TripResetButton: return "Trip Reset";
        case DigitalInputId::HeatElem1Sw: return "trykknap-panel Heat Elem 1 SW";
        case DigitalInputId::HeatElem2Sw: return "trykknap-panel Heat Elem 2 SW";
        case DigitalInputId::FanLow: return "trykknap-panel Fan Low";
        case DigitalInputId::FanMid: return "Fan Mid";
        case DigitalInputId::FanHigh: return "trykknap-panel Fan High";
        case DigitalInputId::AcOffAll: return "AC Off";
        case DigitalInputId::AcVentOnly: return "AC Vent";
        case DigitalInputId::AcHeat1: return "AC Heat 1";
        case DigitalInputId::AcHeat2: return "AC Heat 2";
        case DigitalInputId::WiperStartUser: return "Wiper Start";
        case DigitalInputId::WiperStopUser: return "Wiper Stop";
        case DigitalInputId::SprinklerUser: return "Sprinkler";
        case DigitalInputId::LightsNormalUser: return "Lights Normal";
        case DigitalInputId::LightsHighUser: return "Lights High";
        case DigitalInputId::LightsOffUser: return "Lights Off";
        case DigitalInputId::Count: break;
    }
    return "Unknown";
}

const char* PinMap::outputLabel(OutputId id) {
    switch (id) {
        case OutputId::MainContactor: return "Main Contactor";
        case OutputId::BrakeLightsRelay: return "Brake Lights";
        case OutputId::HeadlightsRelay: return "Headlights";
        case OutputId::HighBeamsRelay: return "High Beams";
        case OutputId::ParkingLightsRelay: return "Parking Lights";
        case OutputId::IndLeftRelay: return "Ind Left";
        case OutputId::IndRightRelay: return "Ind Right";
        case OutputId::HornRelay: return "Horn";
        case OutputId::ReverseLightRelay: return "Reverse Light";
        case OutputId::PBrakeIndicator: return "P-Brake Ind";
        case OutputId::InteriorLightRelay: return "Interior Light";
        case OutputId::ChargingRelay: return "Charging Relay";
        case OutputId::WiperIntRelay: return "Wiper Int";
        case OutputId::WiperLowRelay: return "Wiper Low";
        case OutputId::WiperHighRelay: return "Wiper High";
        case OutputId::WasherPumpRelay: return "Washer Pump";
        case OutputId::HeaterElem1Relay: return "Heater Elem 1";
        case OutputId::HeaterElem2Relay: return "Heater Elem 2";
        case OutputId::HeaterFanRelay: return "Heater Fan";
        case OutputId::DemisterRelay: return "Demister";
        case OutputId::Count: break;
    }
    return "Unknown";
}
