#include <Arduino.h>
#define LGFX_USE_V1
#define LGFX_USE_QSPI
#include <LovyanGFX.hpp>

#include "board_config.h"
#include "Panel_AXS15231B_local.hpp"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

namespace {

class LGFX_JC3248_Lab : public lgfx::LGFX_Device {
  lgfx::Bus_SPI _bus;
  lgfx::Panel_AXS15231B _panel;
  lgfx::Light_PWM _light;

 public:
  LGFX_JC3248_Lab() {
    {
      auto cfg = _bus.config();
      cfg.spi_mode = 0;
      cfg.freq_write = 20000000;
      cfg.freq_read = 16000000;
      cfg.spi_host = SPI3_HOST;
      cfg.spi_3wire = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = kLcdSckPin;
      cfg.pin_io0 = kLcdD0Pin;
      cfg.pin_io1 = kLcdD1Pin;
      cfg.pin_io2 = kLcdD2Pin;
      cfg.pin_io3 = kLcdD3Pin;
      cfg.use_lock = true;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();
      cfg.pin_cs = kLcdCsPin;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.panel_width = kScreenWidth;
      cfg.panel_height = kScreenHeight;
      cfg.memory_width = kScreenWidth;
      cfg.memory_height = kScreenHeight;
      cfg.offset_x = 1;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.readable = true;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }

    {
      auto cfg = _light.config();
      cfg.pin_bl = kBacklightPin;
      cfg.invert = (kBacklightOnLevel == LOW);
      _light.config(cfg);
      _panel.setLight(&_light);
    }

    setPanel(&_panel);
  }
};

LGFX_JC3248_Lab gTft;

constexpr int kW = 480;
constexpr int kH = 320;

enum class GateMode : uint8_t {
  A_SOLID = 0,
  B_CHECKER = 1,
  C_SOAK = 2,
};

constexpr GateMode kGateMode = GateMode::B_CHECKER;

void drawSolidSequence() {
  gTft.fillScreen(TFT_BLACK);
  delay(600);
  gTft.fillScreen(TFT_WHITE);
  delay(600);
  gTft.fillScreen(TFT_RED);
  delay(600);
  gTft.fillScreen(TFT_GREEN);
  delay(600);
  gTft.fillScreen(TFT_BLUE);
  delay(600);
}

void drawCheckerStatic() {
  constexpr int cell = 16;
  gTft.fillScreen(TFT_BLACK);
  for (int y = 0; y < kH; y += cell) {
    for (int x = 1; x < kW; x += cell) {
      const bool odd = ((((x - 1) / cell) + (y / cell)) & 1) != 0;
      const int cw = (x + cell > kW) ? (kW - x) : cell;
      gTft.fillRect(x, y, cw, cell, odd ? TFT_DARKGREY : TFT_NAVY);
    }
  }
  gTft.drawRect(5, 4, kW - 9, kH - 8, TFT_WHITE);
  gTft.drawFastVLine(1, 0, kH, TFT_GREEN);
  gTft.setTextColor(TFT_CYAN, TFT_BLACK);
  gTft.setTextSize(2);
  gTft.setCursor(12, 10);
  gTft.print("LGFX LAB GATE B");
  gTft.setCursor(12, 34);
  gTft.print("CHECKER STATIC");
}

void drawSoakFrame(uint32_t tick) {
  gTft.fillScreen(TFT_BLACK);
  const int x = 10 + static_cast<int>((tick * 7) % (kW - 40));
  gTft.fillRect(x, 100, 30, 120, TFT_CYAN);
  gTft.drawRect(1, 1, kW - 2, kH - 2, TFT_WHITE);
  gTft.setTextColor(TFT_YELLOW, TFT_BLACK);
  gTft.setTextSize(2);
  gTft.setCursor(12, 10);
  gTft.print("LGFX LAB GATE C");
  gTft.setCursor(12, 34);
  gTft.print("SOAK RUNNING");
}

} // namespace

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay(150);
  Serial.println("LGFX_LAB_BOOT");

  gTft.init();
  gTft.setRotation(kScreenRotation);

  if (kGateMode == GateMode::A_SOLID) {
    drawSolidSequence();
    gTft.setTextColor(TFT_WHITE, TFT_BLACK);
    gTft.setTextSize(2);
    gTft.setCursor(10, 10);
    gTft.print("LGFX LAB GATE A DONE");
  } else if (kGateMode == GateMode::B_CHECKER) {
    drawCheckerStatic();
  }
}

void loop() {
  static uint32_t tick = 0;

  if (kGateMode == GateMode::C_SOAK) {
    drawSoakFrame(tick++);
    digitalWrite(LED_BUILTIN, (tick & 1U) ? HIGH : LOW);
    delay(120);
    return;
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
}
