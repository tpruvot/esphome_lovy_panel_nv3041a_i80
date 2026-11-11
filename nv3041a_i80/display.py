import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display
from esphome.const import CONF_ID, CONF_RESET_PIN, CONF_LAMBDA
from esphome import pins

CODEOWNERS = ["@tpruvot"]

DOMAIN = "nv3041a_i80"

# SC07 NV3041A 480x272 with Touchscreen Hynitron CST3240

nv3041a_i80_ns = cg.esphome_ns.namespace(DOMAIN)
NV3041ADisplay = nv3041a_i80_ns.class_(
    "NV3041ADisplay", cg.PollingComponent, display.DisplayBuffer
)

CONFIG_SCHEMA = display.FULL_DISPLAY_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(NV3041ADisplay),
    cv.Optional(CONF_RESET_PIN): pins.internal_gpio_output_pin_schema,
}).extend(cv.polling_component_schema("1000ms"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await display.register_display(var, config)

    if CONF_RESET_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(pin))

    # lambda in YAML
    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))

    # Flags ESP-IDF
    cg.add_build_flag("-DUSE_ESP_LCD")
