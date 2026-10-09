ESPHome Modbus TCP Server 组件
这是一个 ESPHome 自定义外部组件，用于将 ESPHome 中的传感器数值通过 Modbus TCP 协议暴露为保持寄存器。
当前实现支持功能码 0x03（Read Holding Registers），并将 float32 类型的数据拆分为两个 16 位寄存器（大端 / ABCD 字节序）。

文件说明
本仓库仅包含以下三个文件，构成完整的 ESPHome 外部组件：

text
.
├── __init__.py
├── modbus_tcp_server.h
└── modbus_tcp_server.cpp
__init__.py：ESPHome 组件配置入口，定义 YAML 配置项。

modbus_tcp_server.h：组件类声明。

modbus_tcp_server.cpp：组件具体实现，包含 socket 通信与 Modbus 报文处理。

功能特性
基于 ESP-IDF 的 lwip sockets 实现，仅支持 ESP32 平台。

支持 Modbus TCP 功能码 0x03。

将单个 float32 传感器值拆分为两个保持寄存器（高字在前，低字在后）。

默认端口 502，可配置。

非阻塞 socket，单客户端连接。

集成 ESPHome 日志系统。

安装
方式一：通过 GitHub 引用（推荐）
在你的 ESPHome 配置中加入 external_components，指向本仓库：

yaml
external_components:
  - source:
      type: git
      url: https://github.com/你的用户名/你的仓库名
    components: [modbus_tcp_server]
方式二：本地文件
将三个文件放入你的 ESPHome 配置目录下的 components/modbus_tcp_server/ 文件夹中，然后引用：

yaml
external_components:
  - source:
      type: local
      path: components
    components: [modbus_tcp_server]
配置示例
yaml
# 定义一个传感器，例如 BH1750 光照传感器
sensor:
  - platform: bh1750
    name: "环境光照度"
    id: bh1750_illuminance
    address: 0x23
    update_interval: 5s
    unit_of_measurement: "lx"
    accuracy_decimals: 1

# 启用 Modbus TCP Server
modbus_tcp_server:
  id: mb_server
  port: 502
  sensor_id: bh1750_illuminance
配置参数
参数	类型	必填	默认值	说明
id	ID	否	自动生成	组件 ID
port	端口号	是	502	Modbus TCP 监听端口
sensor_id	传感器 ID	是	-	要暴露的 ESPHome 传感器 ID，其状态应为 float
Modbus 寄存器映射
组件将传感器当前的 float32 数值按大端字节序拆分为两个连续的 16 位保持寄存器：

寄存器地址（协议地址）	传统地址	内容
0	40001	float32 高 16 位
1	40002	float32 低 16 位
其他	-	返回 0
注意：Modbus 协议地址从 0 开始，传统地址从 40001 开始。

读取示例（功能码 0x03）
起始地址：0

寄存器数量：2

单元标识符（Unit ID）：任意（组件会回显请求中的 Unit ID）

数据解析
读取到的两个寄存器需要按大端浮点数解析：

python
import struct

high_word = 0x1234  # 示例
low_word  = 0x5678

lux = struct.unpack(">f", struct.pack(">HH", high_word, low_word))[0]
print(lux)
Python 读取示例（pymodbus）
python
from pymodbus.client import ModbusTcpClient
import struct

HOST = "192.168.1.100"
PORT = 502
UNIT_ID = 1

client = ModbusTcpClient(HOST, port=PORT)
client.connect()

rr = client.read_holding_registers(address=0, count=2, slave=UNIT_ID)
# 若 pymodbus 版本较新，请将 slave 改为 device_id

if rr.isError():
    raise RuntimeError(rr)

lux = struct.unpack(">f", struct.pack(">HH", rr.registers[0], rr.registers[1]))[0]
print(f"光照度: {lux:.2f} lx")

client.close()
限制与注意事项
仅支持 ESP32 平台（依赖 lwip/sockets.h）。

仅支持 Modbus 功能码 0x03。

一次仅维护一个客户端连接。

读取超过 2 个寄存器时，未定义寄存器返回 0。

不包含 Modbus 认证或加密，请仅在受信任的局域网中使用。

需要 ESPHome 版本支持 external_components（建议 2023.1 及以上）。

许可证
建议使用 MIT License。请根据你的需要添加 LICENSE 文件。

致谢
ESPHome

Modbus 协议
