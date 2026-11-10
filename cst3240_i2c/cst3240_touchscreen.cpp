#include "cst3240_touchscreen.hpp"
#include "esphome/core/log.h"

namespace esphome {
namespace cst3240_i2c {

static const char *const TAG = "CST3240";

#define MAX_TOUCH_POINTS 4

void CST3240Touchscreen::setup() {
  ESP_LOGCONFIG(TAG, "Init CST3240...");
  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
  }
  this->check_init_();
}

void CST3240Touchscreen::dump_config() {
  ESP_LOGCONFIG(TAG, "Touchscreen CST3240 I2C");
  LOG_I2C_DEVICE(this);
  if (this->interrupt_pin_ != nullptr) {
    LOG_PIN("  TP_INT: ", this->interrupt_pin_);
  }
}

bool CST3240Touchscreen::write_reg16_(uint16_t reg) {
  uint16_t sw = (reg << 8) | (reg >> 8); // Swap endianness requis par le CST3240
  return this->write(reinterpret_cast<uint8_t *>(&sw), 2) == i2c::ERROR_OK;
}

bool CST3240Touchscreen::read_reg16_(uint16_t reg, uint8_t *data, size_t length) {
  uint16_t sw = (reg << 8) | (reg >> 8);
  if (this->write(reinterpret_cast<uint8_t *>(&sw), 2) != i2c::ERROR_OK) {
    return false;
  }
  return this->read(data, length) == i2c::ERROR_OK;
}

bool CST3240Touchscreen::check_init_() {
  if (this->inited_) return true;
  bool ok = this->write_reg16_(0xD109);
  if (ok) {
    this->inited_ = true;
    ESP_LOGI(TAG, "Touchscreen ready");
  }
  return this->inited_;
}

void CST3240Touchscreen::update_touches() {
  if (!this->check_init_()) return;

  if (this->interrupt_pin_ != nullptr && this->interrupt_pin_->digital_read()) {
    // if no INT triggered, no I2C read
    if (!this->touches_.empty()) {
      for (auto &tp : this->touches_) {
        ESP_LOGI(TAG, "%d: CLICK at %d/%d", tp.second.id, tp.second.x, tp.second.y);
        tp.second.state = 0;
      }
      this->send_touches_();
      this->touches_.clear();
    }
    return;
  }

  uint8_t data[(MAX_TOUCH_POINTS * 7)] = {0};

  if (!this->read_reg16_(0xD000, data, sizeof(data))) {
    return;
  }

  bool released = (data[0] & 0x04) == 0;
  if (released) {
    if (!this->touches_.empty()) {
      for (auto &tp : this->touches_) {
        ESP_LOGI(TAG, "%d: click at %d/%d", tp.second.id, tp.second.x, tp.second.y);
        tp.second.state = 0; // hmm not 1 nor 2.. no TOUCH_UP/RELEASE ?
      }
      this->send_touches_();
      this->touches_.clear();
    }
    return;
  }

  uint8_t touches = 0;
  this->touches_.clear();

  for (uint8_t i = 0; i < MAX_TOUCH_POINTS; i++) {
    uint8_t offset = i * 7;

    // bit 0x04 EVT_PRESS (else EVT_RELEASE)
    if (data[offset] & 0x04) {
      uint8_t id = (data[offset + 5] & 0x0F) - 1;
      if (id >= MAX_TOUCH_POINTS) continue; // (0x0F-1 if empty)
      uint16_t x = (data[offset + 1] << 4) | ((data[offset + 3] >> 4) & 0x0F);
      uint16_t y = (data[offset + 2] << 4) | (data[offset + 3] & 0x0F);

      touchscreen::TouchPoint tp;
      tp.id = id;
      tp.x = x;
      tp.y = y;
      // 2: MOVE, 1: DOWN (new one)
      tp.state = (this->touches_.count(id) > 0) ? 2 : 1;

      this->touches_[id] = tp;
      ESP_LOGD(TAG, "%d: moved at %d/%d", id, x, y);
      touches++;
    }
  }
  if (!this->touches_.empty()) {
    this->send_touches_();
    ESP_LOGD(TAG, "sent %d touches...", touches);
  }
}

}  // namespace cst3240_i2c
}  // namespace esphome
