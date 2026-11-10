#pragma once

//#include "esphome/core/gpio.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace cst3240_i2c {

static constexpr uint8_t MAX_TOUCH_POINTS = 4;

class CST3240Touchscreen : public touchscreen::Touchscreen, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;

  void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }

  touchscreen::TouchPoint last_tp{};

 protected:
  void update_touches() override;

  bool write_reg16_(uint16_t reg);
  bool read_reg16_(uint16_t reg, uint8_t *data, size_t length);
  bool check_init_();

  bool inited_{false};

  uint16_t chip_id_{};
  uint16_t project_id_{};
  uint8_t fw_ver_major_{};
  uint8_t fw_ver_minor_{};
  uint16_t fw_build_{};

  InternalGPIOPin *interrupt_pin_{nullptr};
};

}  // namespace cst3240_i2c
}  // namespace esphome

