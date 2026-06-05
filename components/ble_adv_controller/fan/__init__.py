import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import fan
from esphome.const import CONF_ID

from .. import BleAdvController, BleAdvEntity, ble_adv_controller_ns
from ..const import (
    CONF_BLE_ADV_CONTROLLER_ID,
    CONF_BLE_ADV_DIRECTION_SUPPORTED,
    CONF_BLE_ADV_ENCODING,
    CONF_BLE_ADV_FORCED_REFRESH_ON_START,
    CONF_BLE_ADV_OSCILLATION_SUPPORTED,
    CONF_BLE_ADV_SPEED_COUNT,
)

FAN_DIR_OSC_ENCODINGS = {"fanlamp_pro", "lampsmart_pro", "remote", "other"}

BleAdvFan = ble_adv_controller_ns.class_("BleAdvFan", fan.Fan, BleAdvEntity)

CONFIG_SCHEMA = (
    fan.fan_schema(BleAdvFan, default_restore_mode="RESTORE_DEFAULT_OFF")
    .extend(
        {
            cv.Required(CONF_BLE_ADV_CONTROLLER_ID): cv.use_id(BleAdvController),
            cv.Optional(CONF_BLE_ADV_SPEED_COUNT, default=6): cv.one_of(0, 3, 6),
            cv.Optional(
                CONF_BLE_ADV_DIRECTION_SUPPORTED, default=True
            ): cv.boolean,
            cv.Optional(
                CONF_BLE_ADV_OSCILLATION_SUPPORTED, default=False
            ): cv.boolean,
            cv.Optional(
                CONF_BLE_ADV_FORCED_REFRESH_ON_START, default=True
            ): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


def _final_validate_fan(config):
    controller_id = config[CONF_BLE_ADV_CONTROLLER_ID]
    fconf = fv.full_config.get()
    controller_path = fconf.get_path_for_id(controller_id)[:-1]
    controller = fconf.get_config_for_path(controller_path)
    encoding = controller[CONF_BLE_ADV_ENCODING]
    if encoding not in FAN_DIR_OSC_ENCODINGS:
        if config[CONF_BLE_ADV_DIRECTION_SUPPORTED]:
            raise cv.Invalid(
                f"use_direction is not supported for encoding '{encoding}'",
                path=[CONF_BLE_ADV_DIRECTION_SUPPORTED],
            )
        if config[CONF_BLE_ADV_OSCILLATION_SUPPORTED]:
            raise cv.Invalid(
                f"use_oscillation is not supported for encoding '{encoding}'",
                path=[CONF_BLE_ADV_OSCILLATION_SUPPORTED],
            )
    return config


FINAL_VALIDATE_SCHEMA = _final_validate_fan


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_parented(var, config[CONF_BLE_ADV_CONTROLLER_ID])
    await cg.register_component(var, config)
    await fan.register_fan(var, config)
    cg.add(var.set_speed_count(config[CONF_BLE_ADV_SPEED_COUNT]))
    cg.add(
        var.set_direction_supported(config[CONF_BLE_ADV_DIRECTION_SUPPORTED])
    )
    cg.add(
        var.set_oscillation_supported(config[CONF_BLE_ADV_OSCILLATION_SUPPORTED])
    )
    cg.add(
        var.set_forced_refresh_on_start(
            config[CONF_BLE_ADV_FORCED_REFRESH_ON_START]
        )
    )
