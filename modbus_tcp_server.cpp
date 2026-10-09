#include "modbus_tcp_server.h"

#ifdef USE_ESP32
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#endif

namespace esphome {
namespace modbus_tcp_server {

static const char *TAG = "modbus_tcp_server";

#ifdef USE_ESP32

void ModbusTCPServer::setup() {
  this->server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (this->server_fd_ < 0) {
    ESP_LOGE(TAG, "Failed to create socket: errno=%d", errno);
    return;
  }

  int opt = 1;
  setsockopt(this->server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(this->port_);

  if (bind(this->server_fd_, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    ESP_LOGE(TAG, "Failed to bind port %d: errno=%d", this->port_, errno);
    close(this->server_fd_);
    this->server_fd_ = -1;
    return;
  }

  if (listen(this->server_fd_, 4) < 0) {
    ESP_LOGE(TAG, "Failed to listen: errno=%d", errno);
    close(this->server_fd_);
    this->server_fd_ = -1;
    return;
  }

  int flags = fcntl(this->server_fd_, F_GETFL, 0);
  fcntl(this->server_fd_, F_SETFL, flags | O_NONBLOCK);

  ESP_LOGI(TAG, "Modbus TCP server started on port %d", this->port_);
}

void ModbusTCPServer::loop() {
  if (this->server_fd_ < 0) return;

  if (this->client_fd_ < 0) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    this->client_fd_ = accept(this->server_fd_, (struct sockaddr *) &client_addr, &client_len);
    if (this->client_fd_ >= 0) {
      int flags = fcntl(this->client_fd_, F_GETFL, 0);
      fcntl(this->client_fd_, F_SETFL, flags | O_NONBLOCK);
      ESP_LOGI(TAG, "Client connected");
    }
    return;
  }

  uint8_t buffer[260];
  int len = recv(this->client_fd_, buffer, sizeof(buffer), 0);
  if (len <= 0) {
    if (len == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
      ESP_LOGI(TAG, "Client disconnected");
      close(this->client_fd_);
      this->client_fd_ = -1;
    }
    return;
  }

  if (len < 12) return;

  uint8_t unit_id = buffer[6];
  uint8_t function_code = buffer[7];

  if (function_code != 0x03) {
    ESP_LOGW(TAG, "Unsupported function code: 0x%02X", function_code);
    return;
  }

  uint16_t start_address = (buffer[8] << 8) | buffer[9];
  uint16_t register_count = (buffer[10] << 8) | buffer[11];

  ESP_LOGD(TAG, "Read: unit=%d start=%d count=%d", unit_id, start_address, register_count);

  // 获取光照度值（float32）
  float lux_value = 0.0f;
  if (this->sensor_ != nullptr && !std::isnan(this->sensor_->state)) {
    lux_value = this->sensor_->state;
  }

  // 将 float32 拆成两个 16 位寄存器（大端 / ABCD 字节序）
  uint32_t lux_bits;
  memcpy(&lux_bits, &lux_value, sizeof(lux_bits));
  uint16_t high_word = (lux_bits >> 16) & 0xFFFF;  // 寄存器 0
  uint16_t low_word  = lux_bits & 0xFFFF;          // 寄存器 1

  uint8_t response[260];
  response[0] = buffer[0];
  response[1] = buffer[1];
  response[2] = 0x00;
  response[3] = 0x00;
  uint16_t resp_length = 3 + (register_count * 2);
  response[4] = (resp_length >> 8) & 0xFF;
  response[5] = resp_length & 0xFF;
  response[6] = unit_id;
  response[7] = function_code;
  response[8] = register_count * 2;

  for (uint16_t i = 0; i < register_count; i++) {
    uint16_t value = 0;
    uint16_t reg_addr = start_address + i;

    if (reg_addr == 0) {
      value = high_word;
    } else if (reg_addr == 1) {
      value = low_word;
    } else {
      value = 0;
    }

    response[9 + i * 2] = (value >> 8) & 0xFF;
    response[10 + i * 2] = value & 0xFF;
  }

  send(this->client_fd_, response, 9 + register_count * 2, 0);
  ESP_LOGD(TAG, "Response sent: %d registers, lux=%.2f", register_count, lux_value);
}

#else
void ModbusTCPServer::setup() {}
void ModbusTCPServer::loop() {}
#endif

}  // namespace modbus_tcp_server
}  // namespace esphome