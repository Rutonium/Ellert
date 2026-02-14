#ifndef PIN_MAP_H
#define PIN_MAP_H

#include <Arduino.h>

// Centralized, mutable pin map with EEPROM persistence.
class PinMap {
public:
    enum class AnalogInputId : uint8_t {
        Accelerator = 0,
        Count
    };

    enum class DigitalInputId : uint8_t {
        IgnitionOn = 0,
        IgnitionAcc,
        IgnitionStart,
        BrakePedal,
        Handbrake,
        GearPark,
        GearReverse,
        GearNeutral,
        GearDrive,
        IndStalkLeft,
        IndStalkRight,
        LightsStalkParking,
        LightsStalkHead,
        HighBeamStalk,
        HazardSwitch,
        WiperStalkLow,
        WiperStalkHigh,
        WiperStalkInt,
        WasherSwitch,
        HornButton,
        DemisterSwitch,
        TripResetButton,
        HeatElem1Sw,
        HeatElem2Sw,
        FanLow,
        FanMid,
        FanHigh,
        AcOffAll,
        AcVentOnly,
        AcHeat1,
        AcHeat2,
        WiperStartUser,
        WiperStopUser,
        SprinklerUser,
        LightsNormalUser,
        LightsHighUser,
        LightsOffUser,
        Count
    };

    enum class OutputId : uint8_t {
        MainContactor = 0,
        BrakeLightsRelay,
        HeadlightsRelay,
        HighBeamsRelay,
        ParkingLightsRelay,
        IndLeftRelay,
        IndRightRelay,
        HornRelay,
        ReverseLightRelay,
        PBrakeIndicator,
        InteriorLightRelay,
        ChargingRelay,
        WiperIntRelay,
        WiperLowRelay,
        WiperHighRelay,
        WasherPumpRelay,
        HeaterElem1Relay,
        HeaterElem2Relay,
        HeaterFanRelay,
        DemisterRelay,
        Count
    };

    static constexpr int kAnalogCount = static_cast<int>(AnalogInputId::Count);
    static constexpr int kDigitalCount = static_cast<int>(DigitalInputId::Count);
    static constexpr int kOutputCount = static_cast<int>(OutputId::Count);

    static void load();
    static void save();
    static void resetDefaults();
    static void markDirty();
    static bool consumeDirty();

    static int analogInput(AnalogInputId id);
    static int digitalInput(DigitalInputId id);
    static int output(OutputId id);

    static void setAnalogInput(AnalogInputId id, int pin);
    static void setDigitalInput(DigitalInputId id, int pin);
    static void setOutput(OutputId id, int pin);

    static int analogInputByIndex(int index);
    static int digitalInputByIndex(int index);
    static int outputByIndex(int index);

    static int analogInputCount();
    static int digitalInputCount();
    static int outputCount();

    static bool validateNoConflicts();
    static bool wouldConflict(bool isOutput, int index, int proposedPin);
    static bool wouldConflictAnalog(int index, int proposedPin);
    static bool isReservedPin(int pin);
    static bool isAnalogCapablePin(int pin);
    static bool isPwmCapablePin(int pin);
    static bool isPwmRequiredOutput(OutputId id);
    static bool isCriticalOutput(OutputId id);
    static const char* digitalInputLabel(DigitalInputId id);
    static const char* outputLabel(OutputId id);

private:
    static void applyDefaults();
};

#endif // PIN_MAP_H
