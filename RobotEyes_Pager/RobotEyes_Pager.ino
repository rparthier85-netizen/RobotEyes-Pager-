
#include <Arduino.h>
#include <LovyanGFX.hpp>

class PagerDisplay : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796 _panel;
  lgfx::Bus_SPI _bus;

public:
  PagerDisplay() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 35;
      cfg.pin_mosi = 34;
      cfg.pin_miso = 33;
      cfg.pin_dc = 37;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();
      cfg.pin_cs = 38;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.panel_width = 222;
      cfg.panel_height = 480;
      cfg.memory_width = 320;
      cfg.memory_height = 480;
      cfg.offset_x = 49;
      cfg.offset_y = 0;
      cfg.readable = false;
      cfg.invert = true;
      cfg.rgb_order = false;
      _panel.config(cfg);
    }

    setPanel(&_panel);
  }
};

PagerDisplay display;

void zeichneAugen(int blick) {
  display.fillScreen(TFT_BLACK);

  const int mitteY = display.height() / 2;
  const int mitteX = display.width() / 2;

  const int augeLinks = mitteX - 105;
  const int augeRechts = mitteX + 105;

  // Zwei weisse Augen
  display.fillEllipse(augeLinks, mitteY, 75, 88, TFT_WHITE);
  display.fillEllipse(augeRechts, mitteY, 75, 88, TFT_WHITE);

  // Zwei schwarze Pupillen
  display.fillCircle(augeLinks + blick, mitteY, 26, TFT_BLACK);
  display.fillCircle(augeRechts + blick, mitteY, 26, TFT_BLACK);
}

void setup() {
  pinMode(42, OUTPUT);
  digitalWrite(42, HIGH);

  display.init();
  display.setRotation(1);
  display.fillScreen(TFT_BLACK);
}

void loop() {
  for (int x = -30; x <= 30; x += 3) {
    zeichneAugen(x);
    delay(45);
  }

  delay(500);

  for (int x = 30; x >= -30; x -= 3) {
    zeichneAugen(x);
    delay(45);
  }

  delay(500);
}
