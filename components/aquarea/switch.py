import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from . import AquareaComponent, aquarea_ns, CONF_AQUAREA_ID

ForceSwitch = aquarea_ns.class_("ForceSwitch", switch.Switch)
# Installer-menu switches
BoosterHeaterEnableSwitch = aquarea_ns.class_("BoosterHeaterEnableSwitch", switch.Switch)
SterilizationEnableSwitch = aquarea_ns.class_("SterilizationEnableSwitch", switch.Switch)

CONF_Force = "force"
CONF_BoosterHeaterEnable = "booster_heater_enable"
CONF_SterilizationEnable = "sterilization_enable"

TYPES = [
   CONF_Force,
   CONF_BoosterHeaterEnable,
   CONF_SterilizationEnable
]

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_AQUAREA_ID): cv.use_id(AquareaComponent),
            cv.Optional(CONF_Force): switch.switch_schema(ForceSwitch),
            cv.Optional(CONF_BoosterHeaterEnable): switch.switch_schema(BoosterHeaterEnableSwitch),
            cv.Optional(CONF_SterilizationEnable): switch.switch_schema(SterilizationEnableSwitch),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def setup_conf(config, key, hub):
    if switch_config := config.get(key):
        sw = await switch.new_switch(switch_config)
        cg.add(getattr(hub, f"set_{key}_switch")(sw))
        await cg.register_parented(sw, config[CONF_AQUAREA_ID])


async def to_code(config):
    hub = await cg.get_variable(config[CONF_AQUAREA_ID])
    for key in TYPES:
        await setup_conf(config, key, hub)
