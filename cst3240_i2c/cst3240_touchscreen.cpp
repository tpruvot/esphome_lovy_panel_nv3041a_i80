#include "cst3240_touchscreen.hpp"
#include "esphome/core/log.h"

namespace esphome {
namespace cst3240_i2c {

static const char *const TAG = "CST3240";

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
  // Le troisième argument optionnel de 'write' dans ESPHome stable permet de forcer ou non un signal de stop.
  // Pour faire un "Repeated Start", on utilise l'API standard d'ESPHome :
  //if (this->write(reinterpret_cast<uint8_t *>(&sw), 2, false) != i2c::ERROR_OK) {
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
    ESP_LOGI(TAG, "CST3240 ready");
  }
  return this->inited_;
}

void CST3240Touchscreen::update_touches() {
  if (!this->check_init_()) return;

  if (this->interrupt_pin_ != nullptr && this->interrupt_pin_->digital_read()) {
    // Avoid loops if no int triggered
    return;
  }

  uint8_t data[28] = {0}; // (4 points * 7 octets)

  if (!this->read_reg16_(0xD000, data, sizeof(data))) {
    return;
  }

  // Si le bit de pression (TOUCH_EVT_PRESS = 0x04) n'est pas actif sur le premier point, on sort
  if (!(data[0] & 0x04)) {
    return;
  }

  this->touches_.clear();

  for (uint8_t i = 0; i < 4; i++) {
    uint8_t offset = i * 7;

    // Si l'état de l'événement est valide (PRESS)
    if (data[offset] & 0x04) {
      uint8_t id = (data[offset + 5] & 0x0F) - 1;
      uint16_t x = (data[offset + 1] << 4) | ((data[offset + 3] >> 4) & 0x0F);
      uint16_t y = (data[offset + 2] << 4) | (data[offset + 3] & 0x0F);

      touchscreen::TouchPoint tp;
      tp.id = id;
      tp.x = x;
      tp.y = y;

      this->touches_[id] = tp;
    }
  }
  if (!this->touches_.empty())
    this->send_touches_();
}

}  // namespace cst3240_i2c
}  // namespace esphome
