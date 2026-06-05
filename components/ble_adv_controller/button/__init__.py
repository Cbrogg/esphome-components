import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import (
    CONF_ARGS,
    DEVICE_CLASS_IDENTIFY,
    ENTITY_CATEGORY_CONFIG,
)

from .. import BleAdvController, BleAdvEntity, ble_adv_controller_ns
from ..const import (
    CONF_BLE_ADV_CMD,
    CONF_BLE_ADV_COMMANDS,
    CONF_BLE_ADV_CONTROLLER_ID,
    CONF_BLE_ADV_NB_ARGS,
)

BleAdvButton = ble_adv_controller_ns.class_(
    "BleAdvButton", button.Button, BleAdvEntity
)


def validate_command(value):
    if value not in CONF_BLE_ADV_COMMANDS:
        raise cv.Invalid(
            f"{CONF_BLE_ADV_CMD} must be one of {', '.join(CONF_BLE_ADV_COMMANDS)}"
        )
    return value


def validate_args(config):
    expected = CONF_BLE_ADV_COMMANDS[config[CONF_BLE_ADV_CMD]][CONF_BLE_ADV_NB_ARGS]
    actual = len(config[CONF_ARGS])
    if actual != expected:
        raise cv.Invalid(
            f"{config[CONF_BLE_ADV_CMD]} requires {expected} arguments, got {actual}"
        )
    return config


CONFIG_SCHEMA = cv.All(
    button.button_schema(
        BleAdvButton,
        device_class=DEVICE_CLASS_IDENTIFY,
        entity_category=ENTITY_CATEGORY_CONFIG,
    )
    .extend(
        {
            cv.Required(CONF_BLE_ADV_CONTROLLER_ID): cv.use_id(BleAdvController),
            cv.Required(CONF_BLE_ADV_CMD): validate_command,
            cv.Optional(CONF_ARGS, default=[]): cv.ensure_list(cv.uint8_t),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    validate_args,
)


async def to_code(config):
    var = await button.new_button(config)
    await cg.register_parented(var, config[CONF_BLE_ADV_CONTROLLER_ID])
    await cg.register_component(var, config)
    command = CONF_BLE_ADV_COMMANDS[config[CONF_BLE_ADV_CMD]][CONF_BLE_ADV_CMD]
    cg.add(var.set_command(command))
    cg.add(var.set_args(config[CONF_ARGS]))
