#pragma once
#include "esphome/core/component.h"
#include "esphome/components/display/display_buffer.h"
#include "esp_lcd_panel_io.h"
#include "driver/gpio.h"
#include <functional>
#include <vector>

namespace esphome {
namespace nv3041a_i80 {

class NV3041ADisplay : public display::DisplayBuffer {
 public:
  void setup() override;
  void update() override;

  void test_setup(); // temporary init test

  void sleep() { }
  void wakeup() { }

  float get_setup_priority() const override { return setup_priority::PROCESSOR; }
  display::DisplayType get_display_type() override { return display::DISPLAY_TYPE_COLOR; }

  int get_width_internal() override { return 480; }
  int get_height_internal() override { return 272; }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  void flush_buffer_to_lcd_();

  esp_lcd_i80_bus_handle_t io_bus_{nullptr};
  esp_lcd_panel_io_handle_t panel_io_{nullptr};

  size_t buffer_size_{0};
};

}  // namespace nv3041a_i80
}  // namespace esphome

