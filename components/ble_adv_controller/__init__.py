import esphome.automation as automation
import esphome.codegen as cg
from esphome.components import esp32_ble, number, select, sensor, text_sensor
from esphome.components.esp32 import add_idf_sdkconfig_option
from esphome.components.esp32_ble import CONF_BLE_ID
import esphome.config_validation as cv
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
from esphome.core import ID, TimePeriod

from esphome.const import CONF_ENTITY_CATEGORY, CONF_NAME, ENTITY_CATEGORY_CONFIG, ENTITY_CATEGORY_DIAGNOSTIC

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
BleAdvLastPacketTextSensor = ble_adv_controller_ns.class_(
    "BleAdvLastPacketTextSensor",
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
BleAdvForcedIdSensor = ble_adv_controller_ns.class_(
    "BleAdvForcedIdSensor",
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
    cg.add_define("USE_ESP32_BLE_ADVERTISING")
    cg.add_define("USE_ESP32_BLE_UUID")
    add_idf_sdkconfig_option("CONFIG_BT_ENABLED", True)
    add_idf_sdkconfig_option("CONFIG_BT_BLE_42_FEATURES_SUPPORTED", True)
    return _handler


async def _register_diagnostic_entities(parent, config):
    from esphome.components import sensor, text_sensor

    base = config[CONF_ID].id
    update_interval = cg.update_interval("5s")

    protocol = cg.new_Pvariable(
        ID(f"{base}_protocol_text", BleAdvProtocolTextSensor), BleAdvProtocolTextSensor
    )
    await cg.register_parented(protocol, parent)
    await cg.register_component(protocol, {})
    cg.add(protocol.set_update_interval(update_interval))
    await text_sensor.register_text_sensor(
        protocol,
        {
            CONF_NAME: f"{base} Protocol",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        },
    )

    last_packet = cg.new_Pvariable(
        ID(f"{base}_last_packet", BleAdvLastPacketTextSensor), BleAdvLastPacketTextSensor
    )
    await cg.register_parented(last_packet, parent)
    await cg.register_component(last_packet, {})
    cg.add(last_packet.set_update_interval(update_interval))
    await text_sensor.register_text_sensor(
        last_packet,
        {
            CONF_NAME: f"{base} Last Packet",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        },
    )

    tx_count = cg.new_Pvariable(
        ID(f"{base}_tx_count", BleAdvTxCountSensor), BleAdvTxCountSensor
    )
    await cg.register_parented(tx_count, parent)
    await cg.register_component(tx_count, {})
    cg.add(tx_count.set_update_interval(update_interval))
    await sensor.register_sensor(
        tx_count,
        {
            CONF_NAME: f"{base} TX Counter",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        },
    )

    forced_id = cg.new_Pvariable(
        ID(f"{base}_forced_id_diag", BleAdvForcedIdSensor), BleAdvForcedIdSensor
    )
    await cg.register_parented(forced_id, parent)
    await cg.register_component(forced_id, {})
    cg.add(forced_id.set_update_interval(update_interval))
    await sensor.register_sensor(
        forced_id,
        {
            CONF_NAME: f"{base} Forced ID",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        },
    )

    queue_len = cg.new_Pvariable(
        ID(f"{base}_queue_len", BleAdvQueueLengthSensor), BleAdvQueueLengthSensor
    )
    await cg.register_parented(queue_len, parent)
    await cg.register_component(queue_len, {})
    cg.add(queue_len.set_update_interval(update_interval))
    await sensor.register_sensor(
        queue_len,
        {
            CONF_NAME: f"{base} Queue Length",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        },
    )


async def _register_config_entities(parent, config):
    from esphome.components import number, select

    base = config[CONF_ID].id
    encoding = config[CONF_BLE_ADV_ENCODING]
    variants = [*ENCODINGS[encoding]["variants"], "all"]

    duration = cg.new_Pvariable(
        ID(f"{base}_duration", BleAdvDurationNumber), BleAdvDurationNumber
    )
    await cg.register_parented(duration, parent)
    await cg.register_component(duration, {})
    await number.register_number(
        duration,
        {
            CONF_NAME: f"{base} Duration",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            "unit_of_measurement": "ms",
            "mode": number.NUMBER_MODE_BOX,
        },
    )
    cg.add(duration.set_internal_min(100))
    cg.add(duration.set_internal_max(500))

    max_duration = cg.new_Pvariable(
        ID(f"{base}_max_duration", BleAdvMaxDurationNumber), BleAdvMaxDurationNumber
    )
    await cg.register_parented(max_duration, parent)
    await cg.register_component(max_duration, {})
    await number.register_number(
        max_duration,
        {
            CONF_NAME: f"{base} Max Duration",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            "unit_of_measurement": "ms",
            "mode": number.NUMBER_MODE_BOX,
        },
    )
    cg.add(max_duration.set_internal_min(300))
    cg.add(max_duration.set_internal_max(10000))

    index = cg.new_Pvariable(ID(f"{base}_index", BleAdvIndexNumber), BleAdvIndexNumber)
    await cg.register_parented(index, parent)
    await cg.register_component(index, {})
    await number.register_number(
        index,
        {
            CONF_NAME: f"{base} Index",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            "mode": number.NUMBER_MODE_BOX,
        },
    )
    cg.add(index.set_internal_min(0))
    cg.add(index.set_internal_max(255))

    forced_id = cg.new_Pvariable(
        ID(f"{base}_forced_id_cfg", BleAdvForcedIdNumber), BleAdvForcedIdNumber
    )
    await cg.register_parented(forced_id, parent)
    await cg.register_component(forced_id, {})
    await number.register_number(
        forced_id,
        {
            CONF_NAME: f"{base} Forced ID Config",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            "mode": number.NUMBER_MODE_BOX,
        },
    )
    cg.add(forced_id.set_internal_min(0))
    cg.add(forced_id.set_internal_max(0xFFFFFFFF))

    encoding_select = cg.new_Pvariable(
        ID(f"{base}_encoding", BleAdvEncodingSelect), BleAdvEncodingSelect
    )
    await cg.register_parented(encoding_select, parent)
    await cg.register_component(encoding_select, {})
    await select.register_select(
        encoding_select,
        {
            CONF_NAME: f"{base} Encoding",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            "options": list(ENCODINGS.keys()),
        },
    )

    variant_select = cg.new_Pvariable(
        ID(f"{base}_variant", BleAdvVariantSelect), BleAdvVariantSelect
    )
    await cg.register_parented(variant_select, parent)
    await cg.register_component(variant_select, {})
    await select.register_select(
        variant_select,
        {
            CONF_NAME: f"{base} Variant",
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            "options": variants,
        },
    )


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

    await _register_diagnostic_entities(var, config)
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
