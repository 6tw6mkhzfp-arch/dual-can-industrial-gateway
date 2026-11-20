# RK3506B CAN 接入与测试

## 2026-11-17 用户态 USB 原型

实机已确认 `0471:1200` 没有生成真实 SocketCAN 或 tty 接口，厂商 Linux 库只有 x86/x64。现已通过 Windows USBPcap + CANTest 2.69 抓取 CAN2、500 kbit/s 的初始化、接收和发送数据，初步协议记录见 `usbcan_protocol.md`，实验性 ARM 用户态接收程序见 `usbcan_probe.c`。

该实现直接使用 libusb 接口 0：Interrupt `0x01/0x81` 完成通道初始化，Bulk `0x02/0x82` 传输 19 字节 CAN 记录。目前覆盖物理 CAN2、500 kbit/s、标准数据帧收发；尚未覆盖 CAN1、扩展帧、RTR、错误状态和 bus-off。

状态：ARMv7 静态程序已在 RK3506B 实机运行。已持续接收 STM32 的 `0x100/F103` 心跳，并实测 RK 发送 `0x123` 后收到 STM32 原样返回的 `0x321`。常驻版安装在 `/userdata/usbcan_gateway/usbcan_gateway`，启动脚本为 `/etc/init.d/S95usbcan-gateway`，日志为 `/var/log/usbcan_gateway.log`。默认只接收；手动运行时添加 `--test` 才发送测试请求。

服务管理：

```sh
/etc/init.d/S95usbcan-gateway status
/etc/init.d/S95usbcan-gateway restart
tail -f /var/log/usbcan_gateway.log
```

## CAN 状态 HMI（保留原工业监控界面）

`can-status-hmi` 是独立的 Qt/Wayland 全屏状态页，只读取
`/var/log/usbcan_gateway.log`，不替换、不修改原有 `systemui`、
`controlcenter` 或工业监控程序。页面显示网关在线状态、STM32 心跳计数、
最新 CAN 帧和日志更新时间；点击“返回原工业监控界面”即可退出。

在 Windows 本目录启动临时文件服务器：

```powershell
python -m http.server 8000
```

RK 端安装（脚本会先备份旧版本，并执行无界面自检）：

```sh
sh install-can-status-hmi.sh
```

首次只做人工测试，不配置开机启动：

```sh
export XDG_RUNTIME_DIR=/run/user/0
export QT_QPA_PLATFORM=wayland
/userdata/usbcan_gateway/can-status-hmi
```

若 Wayland socket 名称不是默认值，先执行 `ls /run/user/0/wayland-*`，
再按实际名称设置 `WAYLAND_DISPLAY`。回滚 HMI 二进制：

```sh
test -f /userdata/usbcan_gateway/can-status-hmi.bak && \
    mv /userdata/usbcan_gateway/can-status-hmi.bak \
       /userdata/usbcan_gateway/can-status-hmi
```

## 已检查的驱动包限制

用户提供的 Linux 包只有 x86/x86-64 的 libusbcan.so，使用 VCI_* API，并未提供 ARM 库或 SocketCAN 驱动。下面的 CAN 配置命令只有在实机出现对应真实 CAN netdev 后才能执行，不能用复制现有库代替驱动适配。详见 [核查报告](../../docs/review-2026-09-06.md)。

先将同目录 collect-diagnostics.sh 放到 RK，用 `sh collect-diagnostics.sh > rk-usb-before.txt 2>&1` 采集；USB-CAN 插底板 USB-A 后再采集为 rk-usb-after.txt。脚本为只读，不发帧；目标板尚未执行。

两次采集必须在同一次开机中完成，第一次不插 USB-CAN，第二次插上后等待约 5 秒再执行。脚本还记录 USB 接口绑定的驱动和 ttyUSB/ttyACM 节点；请保留两份完整原始输出，不只截取 `lsusb` 行。CAN1/CAN2 端子在此步骤保持悬空。若 USB 设备出现但没有 CAN netdev，先根据接口驱动和实际型号判断路线，不执行下方 `ip link set ... type can` 命令。

## 1. 识别硬件与驱动

插入 USB-CAN 前后对比以下输出并保存：

```sh
uname -a
lsusb
dmesg | tail -n 100
ip -details -statistics link show
ls /sys/class/net
```

`lsusb` 需要 usbutils；网络配置使用完整 iproute2，不能假设 BusyBox ip 包含 CAN 参数；收发命令需要 can-utils 的 candump/cansend。Buildroot 需启用对应软件包并重新生成固件。

内核需要 CONFIG_CAN、CONFIG_CAN_RAW；硬件 netdev 驱动通常依赖 CONFIG_CAN_DEV，以及适配器实际对应的 USB CAN 驱动。CONFIG_CAN_VCAN 只服务虚拟测试。若驱动为模块，才按已确认型号 `modprobe <驱动名>`；内建驱动不需要 modprobe。不要仅因 USB 口为 Type-C 就猜 gs_usb。

- 出现两个 CAN netdev：检查 `/sys/class/net/<接口>/device/driver` 和 dmesg，核对 USB 路径；接口不一定叫 can0/can1。
- 只有 ttyUSB/ttyACM：查厂商是否明确支持 SLCAN，再选择 slcand/对应内核支持；串口设备不自动等于 SLCAN。
- 只有厂商 SDK：确认 ARM Linux ABI、Linux 6.1 支持和是否暴露 SocketCAN；Windows 软件能运行不证明 RK 支持。此时暂缓以下命令。

若接口名是 can0/can1，可辅助检查：

```sh
readlink -f /sys/class/net/can0/device
readlink -f /sys/class/net/can0/device/driver
readlink -f /sys/class/net/can1/device
```

逐个接物理端口并观察 MCU 心跳来自哪个接口，记录 CAN1/CAN2 映射；不要仅按枚举顺序认定。现有 vcan0/vcan1 不属于这两个真实接口。

## 2. 单通道与 STM32（以下 can0 按实际替换）

STM32 先按 firmware/stm32/README.md 烧录并接好。两端经典 CAN、500000 bit/s、normal 模式；关闭自测试/回环/只听模式。若用过 FD 模式，先按驱动能力关闭 FD；第一阶段不发送 FD 帧。

```sh
ip link set dev can0 down
ip link set dev can0 type can bitrate 500000 restart-ms 0
ip link set dev can0 up
ip -details -statistics link show dev can0
```

确认输出位速率、模式和状态；若存在 LOOPBACK/LISTEN-ONLY，关闭相应模式后再继续，不同驱动对 ctrlmode 支持不同，配置报错不能忽略。restart-ms 0 用于保留故障证据，不自动掩盖 bus-off。

终端 A 先运行（工作目录自行建立 logs）：

```sh
mkdir -p logs
candump -t a -e -x can0 | tee logs/can0.txt
```

终端 B 发送一次：

```sh
cansend can0 123#1122334455667788
```

预期：看到 STM32 每秒发送 ID 100、DLC=8，前 4 字节为小端递增计数，后 4 字节为 46 31 30 33（F103）；请求 123 应收到 ID 321、同 DLC 和数据。显示格式由 can-utils 版本决定。123 本身可能是本地回显，不能作为实收证据。

100 次递增请求（两次之间 1 秒）：

```sh
i=0
while [ "$i" -lt 100 ]; do
    payload=$(printf '%02X00000000000000' "$i")
    cansend can0 "123#$payload" || break
    i=$((i + 1))
    sleep 1
done
ip -details -statistics link show dev can0
```

在日志中核对 100 个唯一请求与 100 个 321 回复一一匹配；再持续抓取 10 分钟心跳并检查计数连续性。`candump -e` 的错误解释不代表默认接收所有错误帧，可另开终端：

```sh
candump -t a -e 'can0,0:0,#FFFFFFFF' | tee logs/can0-errors-and-data.txt
```

完成后 `ip link set dev can0 down`，整体断电，将总线移到物理 CAN2，按映射的另一 netdev 重复。不要带电移动端子。

## 3. USB 两通道直接互测

全断电移走 STM32 和其终端，按 wiring.md 第三种拓扑连接，两通道各开一个终端。两个接口均按上述步骤配置 500k 并启动。分别抓 `candump can0` 和 `candump can1`。

```sh
cansend can0 201#01020304
cansend can1 202#A1A2A3A4
```

要求 can1 收到 201，can0 收到 202，并核对接收计数增加；同接口发送回显不计。这只验证两物理通道，不证明两个独立现场总线间转发成功。

## 4. 故障处理

无 netdev：先解决驱动/USB host/数据线；不要改位速率碰运气。接口 UP 但无回复：查 MCU 已运行、PCLK1 和位时序、H/L、地、RS、终端、normal 模式。只有自身发包或错误计数增长：优先查远端 ACK 和时序。

出现 bus-off 时先保存 dmesg、接口统计和 MCU 的 can_test_error；停止发包，断电修复接线。修复后重新上电并配置接口；也可在确认物理问题已修复后使用 `ip link set dev can0 type can restart`（仅驱动支持且处于相应状态时）。不要靠不断重启掩盖接线错误。
