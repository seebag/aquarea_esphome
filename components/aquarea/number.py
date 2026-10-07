import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    UNIT_CELSIUS,
    ICON_THERMOMETER,
    ICON_GAUGE,
    CONF_INITIAL_VALUE,
)
from . import AquareaComponent, aquarea_ns, CONF_AQUAREA_ID

HeatOutsideTempLowNumber = aquarea_ns.class_("HeatOutsideTempLowNumber", number.Number)
HeatOutsideTempHighNumber = aquarea_ns.class_("HeatOutsideTempHighNumber", number.Number)
HeatWaterTempLowNumber = aquarea_ns.class_("HeatWaterTempLowNumber", number.Number)
HeatWaterTempHighNumber = aquarea_ns.class_("HeatWaterTempHighNumber", number.Number)
HeatOffOutsideTempNumber = aquarea_ns.class_("HeatOffOutsideTempNumber", number.Number)
HeaterOnOutsideTempNumber = aquarea_ns.class_("HeaterOnOutsideTempNumber", number.Number)
CoolSetpointTempNumber = aquarea_ns.class_("CoolSetpointTempNumber", number.Number)
TankSetpointTempNumber = aquarea_ns.class_("TankSetpointTempNumber", number.Number)
ShiftSetpointTempNumber = aquarea_ns.class_("ShiftSetpointTempNumber", number.Number)
PumpSpeedNumber = aquarea_ns.class_("PumpSpeedNumber", number.Number)
# Installer-menu registers
BoosterDelayNumber = aquarea_ns.class_("BoosterDelayNumber", number.Number)
SettingsContNumber = aquarea_ns.class_("SettingsContNumber", number.Number)
SettingsRawNumber = aquarea_ns.class_("SettingsRawNumber", number.Number)
HeatIntervalNumber = aquarea_ns.class_("HeatIntervalNumber", number.Number)
TankIntervalNumber = aquarea_ns.class_("TankIntervalNumber", number.Number)
SterilTempNumber = aquarea_ns.class_("SterilTempNumber", number.Number)
SterilTimeNumber = aquarea_ns.class_("SterilTimeNumber", number.Number)

CONF_HeatOutsideTempLow = ("heat_out_temp_low", -20, 5, 1)
CONF_HeatOutsideTempHigh = ("heat_out_temp_high", -5, 15, 1)
CONF_HeatWaterTempLow = ("heat_water_temp_low", 25, 55, 1)
CONF_HeatWaterTempHigh = ("heat_water_temp_high", 25, 45, 1)
CONF_HeatOffOutsideTemp = ("heat_off_out_temp", 0, 20, 1)
CONF_HeaterOnOutsideTemp = ("heater_on_out_temp", -15, 0, 1)
CONF_CoolSetpointTemp = ("cool_setpoint_temp", 13, 20, 1)
CONF_TankSetpointTemp = ("tank_setpoint_temp", 40, 70, 1)
CONF_ShiftSetpointTemp = ("shift_setpoint_temp", -5, 5, 1)
CONF_PumpSpeed = ("pump_speed", 1, 7, 1)
CONF_BoosterDelay = ("booster_delay", 20, 95, 5)     # 195, minutes
CONF_SettingsCont = ("settings_cont", 0, 255, 1)     # 198, raw byte
CONF_SettingsRaw  = ("settings_raw", 0, 255, 1)      # 192, raw byte
CONF_HeatInterval = ("heat_interval", 1, 20, 1)      # 193, 1 = 30 min ... 20 = 10 h
CONF_TankInterval = ("tank_interval", 5, 95, 5)      # 194, minutes
CONF_SterilTemp   = ("steril_temp", 40, 75, 1)       # 196, degrees
CONF_SterilTime   = ("steril_time", 5, 60, 5)        # 197, minutes

TYPES = [
    CONF_HeatOutsideTempLow,
    CONF_HeatOutsideTempHigh,
    CONF_HeatWaterTempLow,
    CONF_HeatWaterTempHigh,
    CONF_HeatOffOutsideTemp,
    CONF_HeaterOnOutsideTemp,
    CONF_CoolSetpointTemp,
    CONF_TankSetpointTemp,
    CONF_ShiftSetpointTemp,
    CONF_PumpSpeed,
    CONF_BoosterDelay,
    CONF_SettingsCont,
    CONF_SettingsRaw,
    CONF_HeatInterval,
    CONF_TankInterval,
    CONF_SterilTemp,
    CONF_SterilTime
]

def ensure_within_range(value, config):
    if value < config[1] or value > config[2]:
        raise cv.Invalid(f"Value: {value} out supported range for {config[0]} \
range is {config[1]}:{config[2]}")
    return value

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_AQUAREA_ID): cv.use_id(AquareaComponent),
            cv.Optional(CONF_HeatOutsideTempLow[0]): number.number_schema(
                HeatOutsideTempLowNumber,
                icon=ICON_THERMOMETER,
                unit_of_measurement=UNIT_CELSIUS,
            ).extend({
                cv.Optional(CONF_INITIAL_VALUE): cv.All(cv.float_, lambda v: ensure_within_range(v, CONF_HeatOutsideTempLow)),
            }),
            cv.Optional(CONF_HeatOutsideTempHigh[0]): number.number_schema(
                HeatOutsideTempHighNumber,
                icon=ICON_THERMOMETER,
                unit_of_measurement=UNIT_CELSIUS,
            ).extend({
                cv.Optional(CONF_INITIAL_VALUE): cv.All(cv.float_, lambda v: ensure_within_range(v, CONF_HeatOutsideTempHigh)),
            }),
            cv.Optional(CONF_HeatWaterTempLow[0]): number.number_schema(
                HeatWaterTempLowNumber,
                icon=ICON_THERMOMETER,
                unit_of_measurement=UNIT_CELSIUS,
            ).extend({
                cv.Optional(CONF_INITIAL_VALUE): cv.All(cv.float_, lambda v: ensure_within_range(v, CONF_HeatWaterTempLow)),
            }),
            cv.Optional(CONF_HeatWaterTempHigh[0]): number.number_schema(
                HeatWaterTempHighNumber,
                icon=ICON_THERMOMETER,
                unit_of_measurement=UNIT_CELSIUS,
            ).extend({
                cv.Optional(CONF_INITIAL_VALUE): cv.All(cv.float_, lambda v: ensure_within_range(v, CONF_HeatWaterTempHigh)),
            }),
            cv.Optional(CONF_HeatOffOutsideTemp[0]): number.number_schema(
                HeatOffOutsideTempNumber,
                icon=ICON_THERMOMETER,
                unit_of_measurement=UNIT_CELSIUS,
            ).extend({
                cv.Optional(CONF_INITIAL_VALUE): cv.All(cv.float_, lambda v: ensure_within_range(v, CONF_HeatOffOutsideTemp)),
            }),
            cv.Optional(CONF_HeaterOnOutsideTemp[0]): number.number_schema(
                HeaterOnOutsideTempNumber,
                icon=ICON_THERMOMETER,
                unit_of_measurement=UNIT_CELSIUS,
            ).extend({
                cv.Optional(CONF_INITIAL_VALUE): cv.All(cv.float_, lambda v: ensure_within_range(v, CONF_HeaterOnOutsideTemp)),
            }),
            cv.Optional(CONF_CoolSetpointTemp[0]): number.number_schema(
                CoolSetpointTempNumber,
                icon=ICON_THERMOMETER,
                unit_of_measurement=UNIT_CELSIUS,
            ).extend({
                cv.Optional(CONF_INITIAL_VALUE): cv.All(cv.float_, lambda v: ensure_within_range(v, CONF_CoolSetpointTemp)),
            }),
            cv.Optional(CONF_TankSetpointTemp[0]): number.number_schema(
                TankSetpointTempNumber,
                icon=ICON_THERMOMETER,
                unit_of_measurement=UNIT_CELSIUS,
            ).extend({
                cv.Optional(CONF_INITIAL_VALUE): cv.All(cv.float_, lambda v: ensure_within_range(v, CONF_TankSetpointTemp)),
            }),
            cv.Optional(CONF_ShiftSetpointTemp[0]): number.number_schema(
                ShiftSetpointTempNumber,
                icon=ICON_THERMOMETER,
                unit_of_measurement=UNIT_CELSIUS,
            ).extend({
                cv.Optional(CONF_INITIAL_VALUE): cv.All(cv.float_, lambda v: ensure_within_range(v, CONF_ShiftSetpointTemp)),
            }),
            cv.Optional(CONF_PumpSpeed[0]): number.number_schema(
                PumpSpeedNumber,
                icon=ICON_GAUGE,
            ).extend({
                cv.Optional(CONF_INITIAL_VALUE): cv.All(cv.float_, lambda v: ensure_within_range(v, CONF_PumpSpeed)),
            }),
            # No CONF_INITIAL_VALUE on the installer-menu registers: their
            # current value cannot be read back, so writing one at boot would
            # overwrite settings with a guess.
            cv.Optional(CONF_BoosterDelay[0]): number.number_schema(
                BoosterDelayNumber,
                icon=ICON_GAUGE,
            ),
            cv.Optional(CONF_SettingsCont[0]): number.number_schema(
                SettingsContNumber,
                icon=ICON_GAUGE,
            ),
            cv.Optional(CONF_SettingsRaw[0]): number.number_schema(
                SettingsRawNumber,
                icon=ICON_GAUGE,
            ),
            cv.Optional(CONF_HeatInterval[0]): number.number_schema(
                HeatIntervalNumber,
                icon=ICON_GAUGE,
            ),
            cv.Optional(CONF_TankInterval[0]): number.number_schema(
                TankIntervalNumber,
                icon=ICON_GAUGE,
            ),
            cv.Optional(CONF_SterilTemp[0]): number.number_schema(
                SterilTempNumber,
                icon=ICON_THERMOMETER,
                unit_of_measurement=UNIT_CELSIUS,
            ),
            cv.Optional(CONF_SterilTime[0]): number.number_schema(
                SterilTimeNumber,
                icon=ICON_GAUGE,
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def setup_conf(config, key, hub):
    if number_config := config.get(key[0]):
        nr = await number.new_number(number_config, min_value=key[1], max_value=key[2], step=key[3])
        cg.add(getattr(hub, f"set_{key[0]}_number")(nr))
        await cg.register_parented(nr, config[CONF_AQUAREA_ID])
        # initial_value is declared Optional, so only write it when given
        if CONF_INITIAL_VALUE in number_config:
            cg.add(nr.make_call().set_value(number_config[CONF_INITIAL_VALUE]).perform())

async def to_code(config):
    hub = await cg.get_variable(config[CONF_AQUAREA_ID])
    for key in TYPES:
        await setup_conf(config, key, hub)
