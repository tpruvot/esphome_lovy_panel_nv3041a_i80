import esphome.codegen as cg

CODEOWNERS = ["@tpruvot"]
DEPENDENCIES = ["i2c"]
DOMAIN = "cst3240_i2c"

cst3240_i2c_ns = cg.esphome_ns.namespace(DOMAIN)
