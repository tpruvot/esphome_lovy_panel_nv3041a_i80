#include "nv3041a_i80.hpp"
#include "driver/gpio.h"
#include "esphome/core/log.h"

namespace esphome {
namespace nv3041a_i80 {

static const char *const TAG = "nv3041a_i80";

struct LcdCmd {
  uint8_t cmd;
  std::vector<uint8_t> data;
  uint32_t delay_ms;
};

uint8_t reflect_bits(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

void NV3041ADisplay::setup() {
  ESP_LOGI(TAG, "--- DEBUT DU TEST DE BIT-BANGING MANUEL ---");

  // 1. Reset matériel strict (GPIO 38)
  const gpio_num_t LCD_RESET_PIN = GPIO_NUM_38;
  gpio_config_t rst_cfg = {
    .pin_bit_mask = (1ULL << LCD_RESET_PIN),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&rst_cfg);

  gpio_set_level(LCD_RESET_PIN, 1); delay(10);
  gpio_set_level(LCD_RESET_PIN, 0); delay(50);
  gpio_set_level(LCD_RESET_PIN, 1); delay(200);

  // 2. Configuration minimale du bus (Fréquence très basse à 1 MHz pour éliminer tout parasite)
  esp_lcd_i80_bus_config_t bus_config = {
    .dc_gpio_num = 47,    // LCD_RS
    .wr_gpio_num = 21,    // LCD_WR
    .clk_src = LCD_CLK_SRC_DEFAULT,
    .data_gpio_nums =
       {39, 40, 41, 14, 13, 12, 11, 10},
       // {10, 11, 12, 13, 14, 41, 40, 39},
    .bus_width = 8,
    .max_transfer_bytes = 4096, // Configuration minimale
  };
  ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &this->io_bus_));

  esp_lcd_panel_io_i80_config_t io_config = {
    .cs_gpio_num = 1,             // CS virtuel
    .pclk_hz = 8 * 1000 * 1000,    // 1 MHz (sécurité maximale)
    .trans_queue_depth = 1,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(this->io_bus_, &io_config, &this->panel_io_));

  // 3. Séquence d'initialisation MINIMALE alternative (On bypasse le gros bloc usine)
  ESP_LOGI(TAG, "Envoi des commandes d'éveil minimales...");

  // Commande 0x11 : Exit Sleep (Sortie de veille)
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x11, NULL, 0);
  delay(150);
/*
  // Au moment d'envoyer les coordonnées et les pixels, on applique la réflexion :
  uint8_t colmod_param[] = { reflect_bits(0x05) };
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x3A, colmod_param, 1);

  uint8_t caset_data[] = { reflect_bits(0x00), reflect_bits(0x64), reflect_bits(0x01), reflect_bits(0x2C) };
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x2A, caset_data, 4);

  uint8_t raset_data[] = { reflect_bits(0x00), reflect_bits(0x64), reflect_bits(0x00), reflect_bits(0xC8) };
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x2B, raset_data, 4);

  // Envoi des pixels rouges avec réflexion des bits
  uint8_t red_byte1 = reflect_bits(0xF8);
  uint8_t red_byte2 = reflect_bits(0x00);
  uint8_t red_pixel_bytes[] = { red_byte1, red_byte2 };

  for (int i = 0; i < 20000; i++) {
    esp_lcd_panel_io_tx_param(this->panel_io_, 0x2C, red_pixel_bytes, 2);
    if (i % 500 == 0) { delay(1); }
  }
*/
  // ==========================================
  // OUVERTURE DE LA FENÊTRE SUR TOUTE LA GRAM (720x544)
  // ==========================================

  // Colonnes : 0 à 719 (0x0000 à 0x02CF)
  // Miroir de CASET (0x2A) = 0x54
  uint8_t caset_data[] = { reflect_bits(0x00), reflect_bits(0x00), reflect_bits(0x02), reflect_bits(0xCF) };
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x54, caset_data, 4);

  // Lignes : 0 à 543 (0x0000 à 0x021F)
  // Miroir de RASET (0x2B) = 0xD4
  uint8_t raset_data[] = { reflect_bits(0x00), reflect_bits(0x00), reflect_bits(0x02), reflect_bits(0x1F) };
  esp_lcd_panel_io_tx_param(this->panel_io_, 0xD4, raset_data, 4);

  // Miroir de RAMWR (0x2C) = 0x34
  ESP_LOGI(TAG, "Remplissage de la GRAM complète en Rouge...");

  uint8_t red_byte1 = reflect_bits(0xF8);
  uint8_t red_byte2 = reflect_bits(0x00);
  uint8_t red_pixel_bytes[] = { red_byte1, red_byte2 };

  // On inonde la GRAM complète (720 * 544 = 391 680 pixels) pour forcer l'allumage de la zone visible
  for (uint32_t i = 0; i < 391680; i++) {
    esp_lcd_panel_io_tx_param(this->panel_io_, 0x34, red_pixel_bytes, 2);

    // On nourrit le Watchdog régulièrement
    if (i % 1000 == 0) {
      delay(1);
    }
  }
/*
  // Commande 0x3A : Interface Pixel Format -> On force le mode 16-bit RGB565 (0x05 ou 0x01 selon constructeurs)
  uint8_t colmod_param[] = {0x05};
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x3A, colmod_param, 1);

  // Commande 0x29 : Display ON (Allumage de la matrice)
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x29, NULL, 0);
  delay(50);

  // 4. Définition manuelle d'une zone d'écriture restreinte (Fenêtre au centre de l'écran)
  // Colonnes : 100 à 300 (0x0064 à 0x012C)
  uint8_t caset_data[] = {0x00, 0x64, 0x01, 0x2C};
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x2A, caset_data, 4);

  // Lignes : 100 à 200 (0x0064 à 0x00C8)
  uint8_t raset_data[] = {0x00, 0x64, 0x00, 0xC8};
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x2B, raset_data, 4);

  // 5. ENVOI MANUEL DE PIXELS ROUGES SANS DMA (Bit-Banging via tx_param)
  ESP_LOGI(TAG, "Tentative d'écriture manuelle d'un carré rouge...");

  // Commande 0x2C : RAM Write (Ouvrir la mémoire pour les pixels)
  // On va envoyer les octets de couleur un par un.
  // D'après la page 39 du manuel, un pixel rouge pur en RGB565 (R=31, G=0, B=0) s'écrit :
  // Byte 1 = 0xF8 (Rouge max) | Byte 2 = 0x00 (Vert/Bleu nul)
  uint8_t red_pixel_bytes[] = {0xF8, 0x00};

  // 200 col * 100 lig = 20 000 pixels
  for (int i = 0; i < 20000; i++) {
    esp_lcd_panel_io_tx_param(this->panel_io_, 0x2C, red_pixel_bytes, 2);
    // CORRECTION CRASH : Nourrir le Watchdog toutes les 500 itérations
    // pour éviter que FreeRTOS ne croie à un blocage matériel
    if (i % 500 == 0) {
      delay(1);
    }
  }

  // On remplit la fenêtre définie (200 colonnes * 100 lignes = 20 000 pixels)
  // On utilise esp_lcd_panel_io_tx_param de manière répétée et bloquante pour forcer l'écriture octet par octet
  for (int i = 0; i < 20000; i++) {
    // On envoie le pixel rouge directement sur le bus de données (Data phase)
    // En passant par tx_param avec une commande bidon ou vide, on s'assure d'écrire dans la RAM ouverte
    esp_lcd_panel_io_tx_param(this->panel_io_, 0x2C, red_pixel_bytes, 2);
  }
*/
  // 6. Allocation minimale pour éviter le crash d'ESPHome
  this->buffer_size_ = 32; // Taille bidon juste pour passer le setup
  this->buffer_ = (uint8_t *) heap_caps_malloc(this->buffer_size_, MALLOC_CAP_INTERNAL);

  ESP_LOGI(TAG, "--- FIN DU TEST BRUT ---");
}

// Laissez les fonctions update() et draw_absolute_pixel_internal() vides pour ce test
void NV3041ADisplay::update() {}
void NV3041ADisplay::draw_absolute_pixel_internal(int x, int y, Color color) {}
/*
void NV3041ADisplay::normal_setup() {
  ESP_LOGCONFIG(TAG, "Initialisation du bus i80 natif pour NV3041A...");

  const gpio_num_t LCD_RESET_PIN = GPIO_NUM_38;
  gpio_config_t rst_cfg = {
    .pin_bit_mask = (1ULL << LCD_RESET_PIN),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&rst_cfg);

  // Cycle de Reset matériel (Timings requis par New Vision)
  gpio_set_level(LCD_RESET_PIN, 1);
  delay(10);
  gpio_set_level(LCD_RESET_PIN, 0); // reinit
  delay(20);
  gpio_set_level(LCD_RESET_PIN, 1); // Retour au niveau haut
  delay(120); // Attente que le contrôleur interne du LCD stabilise ses tensions

  // bus matériel 8080 8 bits, GPIO de la carte ZX4D30CE08S
  esp_lcd_i80_bus_config_t bus_config = {
    .dc_gpio_num = 47,    // LCD_RS
    .wr_gpio_num = 21,    // LCD_WR
    .clk_src = LCD_CLK_SRC_DEFAULT,
    .data_gpio_nums = // DB0 à DB7
      {39, 40, 41, 14, 13, 12, 11, 10}, // (Driver waits Big Endian)
      // {10, 11, 12, 13, 14, 41, 40, 39},
    .bus_width = 8,
    .max_transfer_bytes = 480 * 272 * 2, // Taille complète d'une frame RGB565
    //.max_transfer_bytes = 480 * 40 * 2, // Transfert par tranches (Bands) less SRAM
  };
  ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &this->io_bus_));

  esp_lcd_panel_io_i80_config_t io_config = {
    .cs_gpio_num = 1, // EXT GPIO1 du bornier externe comme CS fictif pour le DMA (à modifier plus tard)
    //.pclk_hz = 16 * 1000 * 1000, // Fréquence d'horloge du bus parallèle
    .pclk_hz = 4 * 1000 * 1000,  // 8 MHz pour la stabilité globale
    .trans_queue_depth = 10,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
    .flags = {
      // only for colors or init data too ?
      // .reverse_color_bits = 1,
      // .swap_color_bytes = 1,
    }
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(this->io_bus_, &io_config, &this->panel_io_));

  // 2. Séquence d'initialisation brute issue du SDK constructeur PanelLan
  const std::vector<LcdCmd> init_sequence = {
    {0xFF, {0xA5}, 0}, {0xE7, {0x10}, 0}, {0x35, {0x01}, 0},
    {0x3A, {0x01}, 0}, // Format de couleur : 0x01 = RGB565 (16 bits)
    {0x40, {0x01}, 0},
    {0x41, {0x01}, 0}, // Configuration d'interface : 0x01 = Bus 8 bits parallel
    {0x55, {0x01}, 0}, {0x44, {0x15}, 0}, {0x45, {0x15}, 0}, {0x7D, {0x03}, 0},
    {0xC1, {0xAB}, 0}, {0xC2, {0x17}, 0}, {0xC3, {0x10}, 0}, {0xC6, {0x3A}, 0},
    {0xC7, {0x25}, 0}, {0xC8, {0x11}, 0}, {0x7A, {0x49}, 0}, {0x6F, {0x2F}, 0},
    {0x78, {0x4B}, 0}, {0x73, {0x08}, 0}, {0x74, {0x12}, 0}, {0xC9, {0x00}, 0},
    {0x67, {0x11}, 0},
    {0x51, {0x20}, 0}, {0x52, {0x7C}, 0}, {0x53, {0x1C}, 0}, {0x54, {0x77}, 0},
    {0x46, {0x0A}, 0}, {0x47, {0x2A}, 0}, {0x48, {0x0A}, 0}, {0x49, {0x1A}, 0},
    {0x56, {0x43}, 0}, {0x57, {0x42}, 0}, {0x58, {0x3C}, 0}, {0x59, {0x64}, 0},
    {0x5A, {0x41}, 0}, {0x5B, {0x3C}, 0}, {0x5C, {0x02}, 0}, {0x5D, {0x3C}, 0},
    {0x5E, {0x1F}, 0}, {0x60, {0x80}, 0}, {0x61, {0x3F}, 0}, {0x62, {0x21}, 0},
    {0x63, {0x07}, 0}, {0x64, {0xE0}, 0}, {0x65, {0x01}, 0},
    {0xCA, {0x20}, 0}, {0xCB, {0x52}, 0}, {0xCC, {0x10}, 0}, {0xCD, {0x42}, 0},
    {0xD0, {0x20}, 0}, {0xD1, {0x52}, 0}, {0xD2, {0x10}, 0}, {0xD3, {0x42}, 0},
    {0xD4, {0x0A}, 0}, {0xD5, {0x32}, 0}, {0xE5, {0x06}, 0}, {0xE6, {0x00}, 0},
    {0x6E, {0x14}, 0},
    {0x80, {0x04}, 0}, {0xA0, {0x00}, 0}, {0x81, {0x07}, 0}, {0xA1, {0x05}, 0},
    {0x82, {0x06}, 0}, {0xA2, {0x04}, 0}, {0x83, {0x39}, 0}, {0xA3, {0x39}, 0},
    {0x84, {0x3A}, 0}, {0xA4, {0x3A}, 0}, {0x85, {0x3F}, 0}, {0xA5, {0x3F}, 0},
    {0x86, {0x30}, 0}, {0xA6, {0x30}, 0}, {0x87, {0x3A}, 0}, {0xA7, {0x3A}, 0},
    {0x88, {0x08}, 0}, {0xA8, {0x08}, 0}, {0x89, {0x0F}, 0}, {0xA9, {0x0F}, 0},
    {0x8A, {0x17}, 0}, {0xAA, {0x17}, 0}, {0x8B, {0x10}, 0}, {0xAB, {0x10}, 0},
    {0x8C, {0x16}, 0}, {0xAC, {0x16}, 0}, {0x8D, {0x14}, 0}, {0xAD, {0x14}, 0},
    {0x8E, {0x11}, 0}, {0xAE, {0x11}, 0}, {0x8F, {0x14}, 0}, {0xAF, {0x14}, 0},
    {0x90, {0x06}, 0}, {0xB0, {0x06}, 0}, {0x91, {0x0F}, 0}, {0xB1, {0x0F}, 0},
    {0x92, {0x16}, 0}, {0xB2, {0x16}, 0},
    {0xFF, {0x00}, 0},
    {0x11, {}, 120}, // Sortie de veille (Sleep Out) + 120ms d'attente
    {0x29, {}, 100}  // Affichage Actif (Display On) + 100ms d'attente
  };
  ESP_LOGI(TAG, "Configuration des fenêtres de coordonnées (CASET/RASET)...");

  // Définition des colonnes : 0 à 479 (0x0000 à 0x01DF)
  uint8_t col_data[] = {0x00, 0x00, 0x01, 0xDF};
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x2A, col_data, 4);

  // Définition des lignes : 0 à 271 (0x0000 à 0x010F)
  uint8_t row_data[] = {0x00, 0x00, 0x01, 0x0F};
  esp_lcd_panel_io_tx_param(this->panel_io_, 0x2B, row_data, 4);

  for (const auto &line : init_sequence) {
    esp_lcd_panel_io_tx_param(this->panel_io_, line.cmd, line.data.data(), line.data.size());
    if (line.delay_ms > 0) {
      delay(line.delay_ms);
    }
  }

  this->buffer_size_ = this->get_width_internal() * this->get_height_internal() * 2;
  //this->init_internal_(this->buffer_size_);
  this->buffer_ = (uint8_t *) heap_caps_malloc(this->buffer_size_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (this->buffer_ != nullptr) {
    memset(this->buffer_, 0x00, this->buffer_size_);
    ESP_LOGI(TAG, "Mémoire tampon allouée avec succès en PSRAM (%d octets).", this->buffer_size_);
  } else {
    ESP_LOGE(TAG, "Échec critique d'allocation de la PSRAM ! Rebasculement en SRAM...");
    this->init_internal_(this->buffer_size_);
  }
  ESP_LOGI(TAG, "Driver NV3041A configured!");
}*/

/*
void NV3041ADisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || x >= 480 || y < 0 || y >= 272 || this->buffer_ == nullptr) return;

  uint8_t r = color.red;
  uint8_t g = color.green;
  uint8_t b = color.blue;

  // 2. Alignement strict selon le diagramme Page 39 du NV3041A
  // Cycle 1 (Byte 1) : R4-R0 (5 bits) + G5-G3 (3 bits)
  uint8_t byte1 = (r & 0xF8) | ((g & 0xE0) >> 5);
  // Cycle 2 (Byte 2) : G2-G0 (3 bits) + B4-B0 (5 bits)
  uint8_t byte2 = ((g & 0x1C) << 3) | ((b & 0xF8) >> 3);

  // 3. Écriture dans le buffer d'octets ordonné pour le transfert DMA
  uint32_t index = (x + y * 480) * 2;
  this->buffer_[index]     = byte1; // Envoyé au 1er coup de WR
  this->buffer_[index + 1] = byte2; // Envoyé au 2eme coup de WR
}*/

/* without bus data inversion
void NV3041ADisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || x >= 480 || y < 0 || y >= 272 || this->buffer_ == nullptr) return;
  uint8_t r = color.red;
  uint8_t g = color.green;
  uint8_t b = color.blue;
  // Compression au format standard 16-bits RGB565 (5 bits R, 6 bits G, 5 bits B)
  uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  uint32_t index = (x + y * 480);
  // Invert endianness pour le bus parallèle 8 bits d'esp_lcd
  this->buffer_[index] = (rgb565 >> 8) | (rgb565 << 8);
}*/

/* to try later
void NV3041ADisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || x >= 480 || y < 0 || y >= 272 || this->buffer_ == nullptr) return;
  uint8_t r = color.red;
  uint8_t g = color.green;
  uint8_t b = color.blue;
  // Format standard RGB565 brut sans inversion logicielle
  uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  uint32_t index = (x + y * 480);
  this->buffer_[index] = rgb565;
}*/
/*
void NV3041ADisplay::update() {
  // 1. do_update_() efface le buffer interne et exécute votre code "lambda:" du YAML
  this->do_update_();
  this->flush_buffer_to_lcd_();
}*/

void NV3041ADisplay::flush_buffer_to_lcd_() {
  if (this->buffer_ == nullptr || this->panel_io_ == nullptr) return;
  // Memory Write 0x2C
  //esp_lcd_panel_io_tx_param(this->panel_io_, 0x2C, NULL, 0);
  // Transfert asynchrone du buffer complet via DMA direct
  esp_lcd_panel_io_tx_color(this->panel_io_, 0x2C, this->buffer_, this->buffer_size_);
}

}  // namespace nv3041a_i80
}  // namespace esphome
