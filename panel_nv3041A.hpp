#pragma once
#define LGFX_USE_V1
#define LGFX_AUTODETECT

#include "esphome/core/component.h"
#include <LovyanGFX.hpp>
#include <functional>

#define PIX_MAX 480
#define PIX_MIN 272

#define TP_INT GPIO_NUM_42

namespace lovy_panel_nv3041a_i80 {

class Panel_NV3041A_I80 : public esphome::PollingComponent, public lgfx::LGFX_Device {

  lgfx::Panel_NV3041A _panel_instance;
  lgfx::Bus_Parallel8 _bus_instance;
  // lgfx::Touch_CST226 _touch_instance;// CST3240 not yet in LovyanGFX
  // PR: https://github.com/lovyan03/LovyanGFX/pull/1006/changes
  lgfx::Touch_CST3240 _touch_instance;

  volatile bool touch_triggered_{false};
  lgfx::v1::touch_point_t points[3];

  static void IRAM_ATTR on_touch_isr(void* arg) {
    auto* instance = static_cast<Panel_NV3041A_I80*>(arg);
    instance->touch_triggered_ = true;
  }

  std::function<void(Panel_NV3041A_I80*)> writer_f_{nullptr};

public:

  bool touched = false;
  lgfx::v1::touch_point_t tp;

  //lgfx::Font_TTF font_roboto;
  //lgfx::Font_TTF font_icons;

  Panel_NV3041A_I80() : esphome::PollingComponent(100)
  {
    {
      auto cfg = _bus_instance.config();

      cfg.port = 0;              // LCD_CAM ESP32-S3 (I2S_NUM_0)
      cfg.freq_write = 16000000; // 16 MHz

      cfg.pin_wr = GPIO_NUM_21;  // WR
      cfg.pin_rd = -1;           // RD always on
      cfg.pin_rs = GPIO_NUM_47;  // RS / DC

      cfg.pin_d0 = GPIO_NUM_39; cfg.pin_d1 = GPIO_NUM_40;
      cfg.pin_d2 = GPIO_NUM_41; cfg.pin_d3 = GPIO_NUM_14;
      cfg.pin_d4 = GPIO_NUM_13; cfg.pin_d5 = GPIO_NUM_12;
      cfg.pin_d6 = GPIO_NUM_11; cfg.pin_d7 = GPIO_NUM_10;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = -1;          // CSX Active Low
      cfg.pin_rst          = GPIO_NUM_38; // TP_RST shared
      //cfg.pin_bl         = GPIO_NUM_15; // Backlight in ESPHome
      //cfg.pin_te         = GPIO_NUM_48; // Frame synch interrupt
      cfg.invert           = true;

      cfg.panel_width      = PIX_MAX;
      cfg.panel_height     = PIX_MIN;
      cfg.memory_width     = PIX_MAX;
      cfg.memory_height    = PIX_MIN;
      cfg.bus_shared       = false;

      _panel_instance.config(cfg);
    }

    {
      auto cfg = _touch_instance.config();
      cfg.x_min      = 0;
      cfg.x_max      = PIX_MAX;
      cfg.y_min      = 0;
      cfg.y_max      = PIX_MIN;
      cfg.bus_shared = false;

      cfg.pin_sda    = GPIO_NUM_9;   // TP_SDA
      cfg.pin_scl    = GPIO_NUM_3;   // TP_SCL
      cfg.pin_int    = TP_INT;
      cfg.pin_rst    = -1;           // TP_RST shared with display

      cfg.freq       = 400000;       // I2C Fast Mode : 400 kHz
      cfg.i2c_addr   = 0x5A;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }

  void setup() override {

    init();
    setRotation(0); // Landscape
    fillScreen(TFT_BLACK);

    // touch screen interrupt
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL << TP_INT);
    io_conf.mode = GPIO_MODE_INPUT;             // Mode Entrée
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;    // Pull-Up resistor
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(TP_INT, on_touch_isr, (void*)this);

    //uint_fast32_t touch_id = _touch_instance.getChipID();

    //todo: font_roboto.loadFont("/spiffs/roboto.ttf");
    //font_icons.loadFont("/spiffs/icons_16.ttf");
  }

  // link the lambda
  void set_writer(std::function<void(Panel_NV3041A_I80*)> writer_f) {
    this->writer_f_ = std::move(writer_f);
  }

  void update() override {
    // lambda 'it'
    if (this->writer_f_ != nullptr) {
      this->writer_f_(this);
    }
  }

  void sleep(bool withTouch = false) {
    // We wake up backlight with touch, avoid touch screen deep sleep
    if (withTouch)
      _touch_instance.sleep();
    _panel_instance.setSleep(1);
  }

  void wakeup(bool fullReset = false) {
    if (!fullReset) {
      _panel_instance.setSleep(0);
    } else {
      _panel_instance.init(fullReset);
      _touch_instance.wakeup();
    }
  }

  void loop() override {
    if (this->touch_triggered_) {
      this->touch_triggered_ = false;
      //uint_fast8_t res = _touch_instance.getTouchRaw(&points[0], 3);
      uint_fast8_t res = getTouch(&points[0], 1);
      if (res == 0) { // touch released if 0
        tp = points[0];
        this->touched = true;
        ESP_LOGD("CST3240", "touch at %d/%d %d:%d", tp.x, tp.y, tp.id, tp.size);
        this->update();
        this->touched = false;
      }
    }
  }

};

// sprite buffer to reduce flickering
extern lgfx::LGFX_Sprite canvas;

} // namespace lovy_panel_nv3041a_i80
