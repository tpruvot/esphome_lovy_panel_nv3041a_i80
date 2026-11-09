#include "cst3240_touchscreen.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cst3240_i2c {

static const char *const TAG = "cst3240_touchscreen";

void CST3240Touchscreen::setup() {
  ESP_LOGCONFIG(TAG, "Configuration du tactile CST3240...");

  // Équivalent de votre _write_reg16(0xD109) pour passer en mode normal
  uint16_t reg = 0xD109;
  uint16_t reg_swapped = (reg << 8) | (reg >> 8);
  this->write(reinterpret_cast<uint8_t*>(&reg_swapped), 2);
}

void CST3240Touchscreen::update_touches() {
  // Lecture de 28 octets à l'adresse 0xD000 (4 points * 7 octets)
  uint16_t reg = 0xD000;
  uint16_t reg_swapped = (reg << 8) | (reg >> 8);

  uint8_t data[28] = {0};

  // Équivalent de votre transactionWriteRead
  if (!this->write(reinterpret_cast<uint8_t*>(&reg_swapped), 2, false) ||
      !this->read(data, 28)) {
    return;
  }

  // Si pas d'événement PRESS (data[0] & 0x04)
  if (!(data[0] & 0x04)) {
    return;
  }

  // Parcourir les points détectés (votre boucle de PR)
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t offset = i * 7;

    // Extraction des coordonnées selon vos formules validées
    uint16_t x = (data[offset + 1] << 4) | ((data[offset + 3] >> 4) & 0x0F);
    uint16_t y = (data[offset + 2] << 4) | (data[offset + 3] & 0x0F);
    uint8_t id = (data[offset + 5] & 0x0F) - 1;

    // Envoi de l'état au sous-système Touchscreen d'ESPHome
    this->defer_touch_(id, touchscreen::TouchPoint{x, y, 1.0f, touchscreen::TouchPoint::State::TOUCHING});
  }

  // Acquittement de l'interruption (votre commande _write_reg16(0xD0AB))
  uint16_t ack_reg = 0xD0AB;
  uint16_t ack_swapped = (ack_reg << 8) | (ack_reg >> 8);
  this->write(reinterpret_cast<uint8_t*>(&ack_swapped), 2);
}

}  // namespace cst3240_i2c
}  // namespace esphome

