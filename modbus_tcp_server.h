#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace modbus_tcp_server {

class ModbusTCPServer : public Component {
 public:
  void set_port(uint16_t port) { this->port_ = port; }
  void set_sensor(sensor::Sensor *sensor) { this->sensor_ = sensor; }
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

 protected:
  uint16_t port_{502};
  int server_fd_{-1};
  int client_fd_{-1};
  sensor::Sensor *sensor_{nullptr};

  uint16_t read_holding_register(uint16_t address);
  void handle_client();
};

}  // namespace modbus_tcp_server
}  // namespace esphome