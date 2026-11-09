import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_LAMBDA

CODEOWNERS = ["@tpruvot"]

# SC07 NV3041A 480x272 with Touchscreen Hynitron CST3240

DOMAIN = "lovy_panel_nv3041a_i80"

ns = cg.esphome_ns.namespace("lovy_panel_nv3041a_i80")
Panel_NV3041A_I80 = ns.class_("Panel_NV3041A_I80", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Panel_NV3041A_I80),
    cv.Optional(CONF_LAMBDA): cv.lambda_,

}).extend(cv.polling_component_schema("1s")).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    # cg.add_library("LovyanGFX", None)
    await cg.register_component(var, config)

    if CONF_LAMBDA in config:
        lambda_expr = await cg.process_lambda(
            config[CONF_LAMBDA],
            [(Panel_NV3041A_I80.operator("ptr"), "it")],
            return_type=cg.void
        )
        cg.add(var.set_writer(lambda_expr))
