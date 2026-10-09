import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PORT
from esphome.components import sensor

CODEOWNERS = ["@you"]
DEPENDENCIES = ["wifi"]

modbus_tcp_server_ns = cg.esphome_ns.namespace("modbus_tcp_server")
ModbusTCPServer = modbus_tcp_server_ns.class_("ModbusTCPServer", cg.Component)

CONF_SENSOR_ID = "sensor_id"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ModbusTCPServer),
    cv.Required(CONF_PORT): cv.port,
    cv.Required(CONF_SENSOR_ID): cv.use_id(sensor.Sensor),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_port(config[CONF_PORT]))
    sens = await cg.get_variable(config[CONF_SENSOR_ID])
    cg.add(var.set_sensor(sens))