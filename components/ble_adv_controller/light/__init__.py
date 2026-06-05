import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import (
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_CONSTANT_BRIGHTNESS,
    CONF_DEFAULT_TRANSITION_LENGTH,
    CONF_MIN_BRIGHTNESS,
    CONF_OUTPUT_ID,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
)

from .. import BleAdvController, ble_adv_controller_ns
from ..const import (
    CONF_BLE_ADV_CONTROLLER_ID,
    CONF_BLE_ADV_SECONDARY,
    CONF_BLE_ADV_SPLIT_DIM_CCT,
)

CODEOWNERS = ["@esphome/core"]
DEPENDENCIES = ["light"]

BleAdvLight = ble_adv_controller_ns.class_(
    "BleAdvLight", light.LightOutput, cg.Parented.template(BleAdvController)
)
BleAdvSecLight = ble_adv_controller_ns.class_(
    "BleAdvSecLight", light.LightOutput, cg.Parented.template(BleAdvController)
)

PARENT_SCHEMA = cv.Schema(
    {cv.Required(CONF_BLE_ADV_CONTROLLER_ID): cv.use_id(BleAdvController)}
)

CONFIG_SCHEMA = cv.All(
    cv.Any(
        light.light_schema(
            BleAdvLight,
            light.LightType.RGB,
            default_restore_mode="RESTORE_DEFAULT_OFF",
        )
        .extend(
            {
                cv.Optional(
                    CONF_COLD_WHITE_COLOR_TEMPERATURE, default="167 mireds"
                ): cv.color_temperature,
                cv.Optional(
                    CONF_WARM_WHITE_COLOR_TEMPERATURE, default="333 mireds"
                ): cv.color_temperature,
                cv.Optional(CONF_CONSTANT_BRIGHTNESS, default=False): cv.boolean,
                cv.Optional(CONF_MIN_BRIGHTNESS, default="1%"): cv.percentage,
                cv.Optional(CONF_BLE_ADV_SPLIT_DIM_CCT, default=False): cv.boolean,
                cv.Optional(
                    CONF_DEFAULT_TRANSITION_LENGTH, default="0s"
                ): cv.positive_time_period_milliseconds,
            }
        )
        .extend(PARENT_SCHEMA)
        .extend(cv.COMPONENT_SCHEMA),
        light.light_schema(
            BleAdvSecLight,
            light.LightType.BINARY,
            default_restore_mode="RESTORE_DEFAULT_OFF",
        )
        .extend({cv.Required(CONF_BLE_ADV_SECONDARY): cv.one_of(True)})
        .extend(PARENT_SCHEMA)
        .extend(cv.COMPONENT_SCHEMA),
    ),
    cv.has_none_or_all_keys(
        [CONF_COLD_WHITE_COLOR_TEMPERATURE, CONF_WARM_WHITE_COLOR_TEMPERATURE]
    ),
    light.validate_color_temperature_channels,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_parented(var, config[CONF_BLE_ADV_CONTROLLER_ID])
    await light.register_light(var, config)
    if CONF_BLE_ADV_SECONDARY in config:
        cg.add(var.set_traits())
        return
    cg.add(
        var.set_traits(
            config[CONF_COLD_WHITE_COLOR_TEMPERATURE],
            config[CONF_WARM_WHITE_COLOR_TEMPERATURE],
        )
    )
    cg.add(var.set_constant_brightness(config[CONF_CONSTANT_BRIGHTNESS]))
    cg.add(var.set_split_dim_cct(config[CONF_BLE_ADV_SPLIT_DIM_CCT]))
    cg.add(var.set_min_brightness(config[CONF_MIN_BRIGHTNESS]))
