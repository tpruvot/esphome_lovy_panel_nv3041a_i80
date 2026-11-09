import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display
from esphome.const import CONF_ID, CONF_LAMBDA

CODEOWNERS = ["@tpruvot"]

DOMAIN = "nv3041a_i80"

# SC07 NV3041A 480x272 with Touchscreen Hynitron CST3240

nv3041a_i80_ns = cg.esphome_ns.namespace(DOMAIN)
NV3041ADisplay = nv3041a_i80_ns.class_(
    "NV3041ADisplay", cg.PollingComponent, display.DisplayBuffer
)

CONFIG_SCHEMA = display.FULL_DISPLAY_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(NV3041ADisplay),
}).extend(cv.polling_component_schema("1s"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await display.register_display(var, config)

    # lambda in YAML
    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))

    # Flags ESP-IDF
    cg.add_build_flag("-DUSE_ESP_LCD")
