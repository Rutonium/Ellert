#include "SerialDiagnostics.h"

namespace {
    constexpr unsigned long kPublishMs = 250;
}

void SerialDiagnostics::initialize() {
    Serial.println("DIAG: serial diagnostics enabled");
}

void SerialDiagnostics::update() {
    static unsigned long lastPublish = 0;
    const unsigned long now = millis();
    if (now - lastPublish < kPublishMs) return;
    lastPublish = now;

    // Protocol:
    // DIAG|D|v0,v1,...,v65|A|a0,a1,...,a11
    Serial.print("DIAG|D|");
    for (int pin = 0; pin <= 65; ++pin) {
        if (pin > 0) Serial.print(",");
        Serial.print(digitalRead(pin) == HIGH ? 1 : 0);
    }
    Serial.print("|A|");
    for (int i = 0; i < 12; ++i) {
        if (i > 0) Serial.print(",");
        Serial.print(analogRead(A0 + i));
    }
    Serial.println();
}
