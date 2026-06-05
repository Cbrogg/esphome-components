import esphome.automation as automation
import esphome.codegen as cg
from esphome.components import esp32_ble, number, select, sensor, text_sensor
from esphome.components.esp32 import add_idf_sdkconfig_option
from esphome.components.esp32_ble import CONF_ADVERTISING, CONF_BLE_ID
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.const import (
    CONF_COMMAND,
    CONF_DURATION,
    CONF_ID,
    CONF_INDEX,
    CONF_RAW,
    CONF_REVERSED,
    CONF_VARIANT,
    PLATFORM_ESP32,
)
from esphome.core import CORE, ID, TimePeriod, TimePeriodMilliseconds
from esphome.helpers import fnv1_hash

DIAGNOSTIC_POLL_INTERVAL = TimePeriodMilliseconds(milliseconds=5000)

from esphome.const import (
    CONF_DISABLED_BY_DEFAULT,
    CONF_ENTITY_CATEGORY,
    CONF_FORCE_UPDATE,
    CONF_ICON,
    CONF_MODE,
    CONF_NAME,
    CONF_UPDATE_INTERVAL,
    CONF_ACCURACY_DECIMALS,
    CONF_STATE_CLASS,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_TOTAL_INCREASING,
)

from .const import (
    CONF_BLE_ADV_ENCODING,
    CONF_BLE_ADV_FORCED_ID,
    CONF_BLE_ADV_MAX_DURATION,
    CONF_BLE_ADV_SEQ_DURATION,
    CONF_BLE_ADV_SHOW_CONFIG,
)

AUTO_LOAD = ["esp32_ble", "sensor", "text_sensor", "number", "select"]
DEPENDENCIES = ["esp32"]
MULTI_CONF = True

CONF_ARG0 = "arg0"
CONF_ARG1 = "arg1"
CONF_ARG2 = "arg2"
CONF_ARG3 = "arg3"
CONF_HANDLER_ID = "ble_adv_handler_id"

ble_adv_controller_ns = cg.esphome_ns.namespace("ble_adv_controller")
BleAdvHandler = ble_adv_controller_ns.class_("BleAdvHandler", cg.Component)
BleAdvController = ble_adv_controller_ns.class_("BleAdvController", cg.Component)
BleAdvEntity = ble_adv_controller_ns.class_(
    "BleAdvEntity",
    cg.Component,
    cg.Parented.template(BleAdvController),
)

PairAction = ble_adv_controller_ns.class_(
    "PairAction", automation.Action, cg.Parented.template(BleAdvController)
)
UnpairAction = ble_adv_controller_ns.class_(
    "UnpairAction", automation.Action, cg.Parented.template(BleAdvController)
)
CommandAction = ble_adv_controller_ns.class_(
    "CommandAction", automation.Action, cg.Parented.template(BleAdvController)
)
RawInjectAction = ble_adv_controller_ns.class_(
    "RawInjectAction", automation.Action, cg.Parented.template(BleAdvController)
)
RawDecodeAction = ble_adv_controller_ns.class_(
    "RawDecodeAction", automation.Action, cg.Parented.template(BleAdvController)
)

BleAdvProtocolTextSensor = ble_adv_controller_ns.class_(
    "BleAdvProtocolTextSensor",
    text_sensor.TextSensor,
    cg.PollingComponent,
    cg.Parented.template(BleAdvController),
)
BleAdvVariantTextSensor = ble_adv_controller_ns.class_(
    "BleAdvVariantTextSensor",
    text_sensor.TextSensor,
    cg.PollingComponent,
    cg.Parented.template(BleAdvController),
)
BleAdvLastPacketTextSensor = ble_adv_controller_ns.class_(
    "BleAdvLastPacketTextSensor",
    text_sensor.TextSensor,
    cg.PollingComponent,
    cg.Parented.template(BleAdvController),
)
BleAdvForcedIdTextSensor = ble_adv_controller_ns.class_(
    "BleAdvForcedIdTextSensor",
    text_sensor.TextSensor,
    cg.PollingComponent,
    cg.Parented.template(BleAdvController),
)
BleAdvTxCountSensor = ble_adv_controller_ns.class_(
    "BleAdvTxCountSensor",
    sensor.Sensor,
    cg.PollingComponent,
    cg.Parented.template(BleAdvController),
)
BleAdvQueueLengthSensor = ble_adv_controller_ns.class_(
    "BleAdvQueueLengthSensor",
    sensor.Sensor,
    cg.PollingComponent,
    cg.Parented.template(BleAdvController),
)
BleAdvDurationNumber = ble_adv_controller_ns.class_(
    "BleAdvDurationNumber", number.Number, cg.Parented.template(BleAdvController)
)
BleAdvMaxDurationNumber = ble_adv_controller_ns.class_(
    "BleAdvMaxDurationNumber", number.Number, cg.Parented.template(BleAdvController)
)
BleAdvIndexNumber = ble_adv_controller_ns.class_(
    "BleAdvIndexNumber", number.Number, cg.Parented.template(BleAdvController)
)
BleAdvForcedIdNumber = ble_adv_controller_ns.class_(
    "BleAdvForcedIdNumber", number.Number, cg.Parented.template(BleAdvController)
)
BleAdvEncodingSelect = ble_adv_controller_ns.class_(
    "BleAdvEncodingSelect", select.Select, cg.Parented.template(BleAdvController)
)
BleAdvVariantSelect = ble_adv_controller_ns.class_(
    "BleAdvVariantSelect", select.Select, cg.Parented.template(BleAdvController)
)

ENCODINGS = {
    "fanlamp_pro": {
        "variants": {"v1": 0xFFFFFF, "v2": 0xFFFFFFFF, "v3": 0xFFFFFFFF},
        "default_variant": "v3",
        "default_id": 0,
    },
    "lampsmart_pro": {
        "variants": {"v1": 0xFFFFFF, "v2": 0xFFFFFFFF, "v3": 0xFFFFFFFF},
        "default_variant": "v3",
        "default_id": 0,
    },
    "zhijia": {
        "variants": {"v0": 0xFFFF, "v1": 0xFFFFFF, "v2": 0xFFFFFF},
        "default_variant": "v2",
        "default_id": 0xC630B8,
    },
    "remote": {
        "variants": {"v1": 0xFFFFFF, "v3": 0xFFFFFFFF},
        "default_variant": "v3",
        "default_id": 0,
    },
    "other": {
        "variants": {
            "v1a": 0xFFFFFF,
            "v1b": 0xFFFFFF,
            "v2": 0xFFFFFFFF,
            "v3": 0xFFFFFFFF,
        },
        "default_variant": "v1b",
        "default_id": 0,
    },
}


def _controller_schema(encoding, definition):
    variants = [*definition["variants"], "all"]
    return cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BleAdvController),
            cv.GenerateID(CONF_HANDLER_ID): cv.declare_id(BleAdvHandler),
            cv.GenerateID(CONF_BLE_ID): cv.use_id(esp32_ble.ESP32BLE),
            cv.Required(CONF_BLE_ADV_ENCODING): cv.one_of(encoding),
            cv.Optional(
                CONF_VARIANT, default=definition["default_variant"]
            ): cv.one_of(*variants),
            cv.Optional(
                CONF_BLE_ADV_FORCED_ID, default=definition["default_id"]
            ): cv.hex_uint32_t,
            cv.Optional(CONF_INDEX, default=0): cv.int_range(min=0, max=255),
            cv.Optional(CONF_DURATION, default="200ms"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=TimePeriod(milliseconds=100), max=TimePeriod(milliseconds=500)),
            ),
            cv.Optional(CONF_BLE_ADV_MAX_DURATION, default="3s"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=TimePeriod(milliseconds=300), max=TimePeriod(milliseconds=10000)),
            ),
            cv.Optional(CONF_BLE_ADV_SEQ_DURATION, default="100ms"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=TimePeriod(milliseconds=1), max=TimePeriod(milliseconds=150)),
            ),
            cv.Optional(CONF_REVERSED, default=False): cv.boolean,
            cv.Optional(CONF_BLE_ADV_SHOW_CONFIG, default=True): cv.boolean,
            cv.Optional(CONF_NAME): cv.string,
        }
    ).extend(cv.COMPONENT_SCHEMA)


def _validate_controller(config):
    encoding = config[CONF_BLE_ADV_ENCODING]
    variant = config[CONF_VARIANT]
    forced_id = config[CONF_BLE_ADV_FORCED_ID]
    if variant != "all":
        max_id = ENCODINGS[encoding]["variants"][variant]
        if forced_id > max_id:
            raise cv.Invalid(
                f"forced_id 0x{forced_id:X} exceeds the maximum 0x{max_id:X} "
                f"for {encoding}/{variant}"
            )
    if config[CONF_BLE_ADV_MAX_DURATION] < config[CONF_DURATION]:
        raise cv.Invalid("max_duration must be greater than or equal to duration")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Any(
        *[
            _controller_schema(encoding, definition)
            for encoding, definition in ENCODINGS.items()
        ]
    ),
    _validate_controller,
    cv.only_on([PLATFORM_ESP32]),
)


def _final_validate_ble_adv(config):
    fconf = fv.full_config.get()
    ble_config = fconf.get("esp32_ble")
    if ble_config is not None and ble_config.get(CONF_ADVERTISING, False):
        raise cv.Invalid(
            "esp32_ble.advertising must be false when using ble_adv_controller; "
            "the component exclusively owns BLE GAP advertising"
        )
    return config


FINAL_VALIDATE_SCHEMA = _final_validate_ble_adv

_handler = None


async def _get_handler(config):
    global _handler
    if _handler is not None:
        return _handler
    _handler = cg.new_Pvariable(config[CONF_HANDLER_ID])
    await cg.register_component(_handler, {})
    ble_parent = await cg.get_variable(config[CONF_BLE_ID])
    cg.add(_handler.set_ble_parent(ble_parent))
    esp32_ble.register_gap_event_handler(ble_parent, _handler)
    cg.add_define("USE_ESP32_BLE_UUID")
    add_idf_sdkconfig_option("CONFIG_BT_ENABLED", True)
    add_idf_sdkconfig_option("CONFIG_BT_BLE_42_FEATURES_SUPPORTED", True)
    return _handler


def _entity_config(entity_id, name, *, entity_category, **extra):
    return {
        CONF_ID: entity_id,
        CONF_NAME: name,
        CONF_ENTITY_CATEGORY: entity_category,
        CONF_DISABLED_BY_DEFAULT: False,
        **extra,
    }


def _entity_label(config, suffix):
    label = config.get(CONF_NAME)
    if label:
        return f"{label} {suffix}"
    return suffix


async def _register_polling_text_sensor(
    parent, entity_id, name, *, icon=None, disabled_by_default=False
):
    CORE.component_ids.add(entity_id.id)
    entity_config = _entity_config(
        entity_id,
        name,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        **{
            CONF_UPDATE_INTERVAL: DIAGNOSTIC_POLL_INTERVAL,
            CONF_DISABLED_BY_DEFAULT: disabled_by_default,
        },
    )
    if icon is not None:
        entity_config[CONF_ICON] = icon
    var = await text_sensor.new_text_sensor(entity_config)
    await cg.register_parented(var, parent)
    await cg.register_component(var, entity_config)


async def _register_polling_sensor(
    parent,
    entity_id,
    name,
    *,
    icon=None,
    accuracy_decimals=0,
    state_class=None,
    disabled_by_default=False,
):
    CORE.component_ids.add(entity_id.id)
    entity_config = _entity_config(
        entity_id,
        name,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        **{
            CONF_UPDATE_INTERVAL: DIAGNOSTIC_POLL_INTERVAL,
            CONF_FORCE_UPDATE: False,
            CONF_ACCURACY_DECIMALS: accuracy_decimals,
            CONF_DISABLED_BY_DEFAULT: disabled_by_default,
        },
    )
    if icon is not None:
        entity_config[CONF_ICON] = icon
    if state_class is not None:
        entity_config[CONF_STATE_CLASS] = state_class
    var = await sensor.new_sensor(entity_config)
    await cg.register_parented(var, parent)
    await cg.register_component(var, entity_config)


async def _register_diagnostic_entities(parent, config, *, show_config):
    base = config[CONF_ID].id

    await _register_polling_text_sensor(
        parent,
        ID(f"{base}_protocol_text", type=BleAdvProtocolTextSensor),
        _entity_label(config, "Protocol"),
        icon="mdi:bluetooth-connect",
    )
    await _register_polling_text_sensor(
        parent,
        ID(f"{base}_variant_text", type=BleAdvVariantTextSensor),
        _entity_label(config, "Variant"),
        icon="mdi:tag-outline",
    )
    if not show_config:
        await _register_polling_text_sensor(
            parent,
            ID(f"{base}_forced_id_diag", type=BleAdvForcedIdTextSensor),
            _entity_label(config, "Device ID"),
            icon="mdi:identifier",
        )
    await _register_polling_sensor(
        parent,
        ID(f"{base}_tx_count", type=BleAdvTxCountSensor),
        _entity_label(config, "Transmissions"),
        icon="mdi:counter",
        state_class=STATE_CLASS_TOTAL_INCREASING,
    )
    await _register_polling_sensor(
        parent,
        ID(f"{base}_queue_len", type=BleAdvQueueLengthSensor),
        _entity_label(config, "Command Queue"),
        icon="mdi:format-list-numbered",
        disabled_by_default=True,
    )
    await _register_polling_text_sensor(
        parent,
        ID(f"{base}_last_packet", type=BleAdvLastPacketTextSensor),
        _entity_label(config, "Last Packet"),
        icon="mdi:radio-tower",
        disabled_by_default=True,
    )


async def _register_config_number(
    parent, entity_id, name, *, min_value, max_value, step=1, unit=None, icon=None
):
    entity_config = _entity_config(
        entity_id,
        name,
        entity_category=ENTITY_CATEGORY_CONFIG,
        **{CONF_MODE: number.NumberMode.NUMBER_MODE_BOX},
    )
    if unit is not None:
        entity_config["unit_of_measurement"] = unit
    if icon is not None:
        entity_config[CONF_ICON] = icon
    var = cg.new_Pvariable(entity_id)
    await cg.register_parented(var, parent)
    await number.register_number(
        var, entity_config, min_value=min_value, max_value=max_value, step=step
    )
    return var


async def _register_config_select(parent, entity_id, name, options, *, icon=None):
    entity_config = _entity_config(
        entity_id, name, entity_category=ENTITY_CATEGORY_CONFIG
    )
    if icon is not None:
        entity_config[CONF_ICON] = icon
    var = cg.new_Pvariable(entity_id)
    await cg.register_parented(var, parent)
    await select.register_select(var, entity_config, options=options)
    return var


async def _register_config_entities(parent, config):
    base = config[CONF_ID].id
    encoding = config[CONF_BLE_ADV_ENCODING]
    variants = [*ENCODINGS[encoding]["variants"], "all"]
    forced_id_value = (
        config[CONF_BLE_ADV_FORCED_ID]
        if config[CONF_BLE_ADV_FORCED_ID] != 0
        else fnv1_hash(config[CONF_ID].id)
    )

    duration = await _register_config_number(
        parent,
        ID(f"{base}_duration", type=BleAdvDurationNumber),
        _entity_label(config, "Advertising Duration"),
        min_value=100,
        max_value=500,
        unit="ms",
        icon="mdi:timer-outline",
    )
    cg.add(duration.publish_state(config[CONF_DURATION].total_milliseconds))

    max_duration = await _register_config_number(
        parent,
        ID(f"{base}_max_duration", type=BleAdvMaxDurationNumber),
        _entity_label(config, "Max Advertising Duration"),
        min_value=300,
        max_value=10000,
        unit="ms",
        icon="mdi:timer-sand",
    )
    cg.add(
        max_duration.publish_state(config[CONF_BLE_ADV_MAX_DURATION].total_milliseconds)
    )

    index = await _register_config_number(
        parent,
        ID(f"{base}_index", type=BleAdvIndexNumber),
        _entity_label(config, "Device Index"),
        min_value=0,
        max_value=255,
        icon="mdi:numeric",
    )
    cg.add(index.publish_state(config[CONF_INDEX]))

    forced_id = await _register_config_number(
        parent,
        ID(f"{base}_forced_id_cfg", type=BleAdvForcedIdNumber),
        _entity_label(config, "Device ID"),
        min_value=0,
        max_value=0xFFFFFFFF,
        icon="mdi:identifier",
    )
    cg.add(forced_id.publish_state(forced_id_value))

    encoding_select = await _register_config_select(
        parent,
        ID(f"{base}_encoding", type=BleAdvEncodingSelect),
        _entity_label(config, "Encoding"),
        list(ENCODINGS.keys()),
        icon="mdi:bluetooth-connect",
    )
    cg.add(encoding_select.publish_state(encoding))

    variant_select = await _register_config_select(
        parent,
        ID(f"{base}_variant", type=BleAdvVariantSelect),
        _entity_label(config, "Variant"),
        variants,
        icon="mdi:tag-outline",
    )
    cg.add(variant_select.publish_state(config[CONF_VARIANT]))


async def to_code(config):
    handler = await _get_handler(config)
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_handler(handler))
    cg.add(var.set_encoding(config[CONF_BLE_ADV_ENCODING]))
    cg.add(var.set_variant(config[CONF_VARIANT]))
    cg.add(var.set_min_duration(config[CONF_DURATION]))
    cg.add(var.set_max_duration(config[CONF_BLE_ADV_MAX_DURATION]))
    cg.add(var.set_sequence_duration(config[CONF_BLE_ADV_SEQ_DURATION]))
    cg.add(var.set_index(config[CONF_INDEX]))
    cg.add(var.set_reversed(config[CONF_REVERSED]))
    if config[CONF_BLE_ADV_FORCED_ID] != 0:
        cg.add(var.set_forced_id(config[CONF_BLE_ADV_FORCED_ID]))
    else:
        cg.add(var.set_forced_id(config[CONF_ID].id))

    await _register_diagnostic_entities(
        var, config, show_config=config[CONF_BLE_ADV_SHOW_CONFIG]
    )
    if config[CONF_BLE_ADV_SHOW_CONFIG]:
        await _register_config_entities(var, config)


CONTROLLER_ACTION_SCHEMA = automation.maybe_simple_id(
    {cv.Required(CONF_ID): cv.use_id(BleAdvController)}
)


async def _parented_action(config, action_id, template_arg, action_type):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "ble_adv_controller.pair",
    PairAction,
    CONTROLLER_ACTION_SCHEMA,
    synchronous=True,
)
async def pair_action_to_code(config, action_id, template_arg, args):
    return await _parented_action(config, action_id, template_arg, PairAction)


@automation.register_action(
    "ble_adv_controller.unpair",
    UnpairAction,
    CONTROLLER_ACTION_SCHEMA,
    synchronous=True,
)
async def unpair_action_to_code(config, action_id, template_arg, args):
    return await _parented_action(config, action_id, template_arg, UnpairAction)


@automation.register_action(
    "ble_adv_controller.command",
    CommandAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(BleAdvController),
            cv.Required(CONF_COMMAND): cv.templatable(cv.uint8_t),
            cv.Optional(CONF_ARG0, default=0): cv.templatable(cv.uint8_t),
            cv.Optional(CONF_ARG1, default=0): cv.templatable(cv.uint8_t),
            cv.Optional(CONF_ARG2, default=0): cv.templatable(cv.uint8_t),
            cv.Optional(CONF_ARG3, default=0): cv.templatable(cv.uint8_t),
        }
    ),
    synchronous=True,
)
async def command_action_to_code(config, action_id, template_arg, args):
    var = await _parented_action(config, action_id, template_arg, CommandAction)
    cg.add(var.set_command(await cg.templatable(config[CONF_COMMAND], args, cg.uint8)))
    for key in (CONF_ARG0, CONF_ARG1, CONF_ARG2, CONF_ARG3):
        value = await cg.templatable(config[key], args, cg.uint8)
        cg.add(getattr(var, f"set_{key}")(value))
    return var


RAW_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(BleAdvController),
        cv.Required(CONF_RAW): cv.templatable(cv.string),
    }
)


@automation.register_action(
    "ble_adv_controller.raw_inject",
    RawInjectAction,
    RAW_ACTION_SCHEMA,
    synchronous=True,
)
async def raw_inject_action_to_code(config, action_id, template_arg, args):
    var = await _parented_action(config, action_id, template_arg, RawInjectAction)
    cg.add(var.set_raw(await cg.templatable(config[CONF_RAW], args, cg.std_string)))
    return var


@automation.register_action(
    "ble_adv_controller.raw_decode",
    RawDecodeAction,
    RAW_ACTION_SCHEMA,
    synchronous=True,
)
async def raw_decode_action_to_code(config, action_id, template_arg, args):
    var = await _parented_action(config, action_id, template_arg, RawDecodeAction)
    cg.add(var.set_raw(await cg.templatable(config[CONF_RAW], args, cg.std_string)))
    return var
