# 真实 CAN 测试记录（未填写＝未验证）

## 2026-10-21 截图判读

用户提供的终端提示符为 `p@p-virtual-machine`，`lsusb` 包含 VMware Virtual Mouse 和虚拟 USB root hub；`dmesg` 含 `ens160`。这些命令运行于 Ubuntu 虚拟机，不能作为 RK3506B 的 USB-CAN 枚举证据。截图中曾执行 `sh collect-diagnostics.sh > rk-usb-before.txt 2>&1`，但文件内容未提供，且执行环境也是虚拟机。RK 实机 USB 枚举、驱动及真实 CAN 收发仍待测。

随后用户截图显示 Windows 可与 `192.168.31.126:22` 建立 SSH 连接，`root` 登录连续三次被拒；不能据此确认该 IP 的设备身份，也不能假设出厂 `root/root` 凭据适用于当前自制固件。下一步需核对路由器设备身份，优先通过板载 CH343 调试串口确认系统与登录状态。

## 2026-10-22 RK 串口实机观察

用户通过 CH343 COM5、1500000 bps 进入 `root@MiWiFi-R4-srv:/#`。`uname -m` 为 `armv7l`；`cat /proc/device-tree/model` 为 `ATK-DLRK3506B Board`（输出末尾无换行，紧接 shell 提示符）。因此已确认该串口为 RK 板。截图中 `lsusb` 列出 `1a86:8091`、两个 `1d6b:0002` 和 `0bda:b733`，未见预期 USB-CAN 枚举；不能据此认定 USB-CAN 已接入或驱动可用。下一步需在确认同一根 USB 数据线连接底板 USB-A 与 USB-CAN Type-C 后，对比 USB 插拔日志。

台架接线阶段，用户报告在 HW-977 模块 `3.3V` 与 `GND` 两针测得直流 **3.26V**；这在 TI 对 SN65HVD230 给出的 3.0–3.6V 工作范围内。该数值为用户实测口述，尚未保存仪表照片；CANH-CANL 终端电阻、STM32 固件和真实收发仍未验证。

用户随后报告全断电、2kΩ 档 HW-977 `CANH` 与 `CANL` 之间显示 `.120`，折算约 **120Ω**，可视为模块端已有一只终端的台架证据；未保存仪表照片。USB-CAN 端子/开关仍需实物核对和断电测阻，真实收发未做。

用户报告 USB-CAN CAN1 终端开关置 ON、断电测 `L1-H1` 为约 **116Ω**；与一只 120Ω 终端相符。尚未将两端接成总线，也未测整条总线的约 60Ω 或进行收发。

用户已将黄色芯线接 `H1→CANH`、黑色芯线接 `L1→CANL`，并确认 `G1→Nucleo GND` 第三根参考线已接。首次整条总线读数为 70Ω；重测在 200Ω 档读到 **57.8Ω**，落在两只约 120Ω 终端并联的预期范围。此为断电电阻与接线口述验证，不代表 STM32 固件运行或真实 CAN 收发。

Windows 端后续用可传数据的线连接 USB-CAN 后，PnP 显示 `USBCAN-UC12`，硬件 ID `USB\\VID_0471&PID_1200`，问题代码 28（未安装驱动）。这确认适配器在 Windows 上可完成 USB 枚举，但不证明 Windows 驱动可用，更不证明 RK 已枚举或能收发 CAN。下一步用这根已证实有数据连接的线回接 RK，重查 `lsusb` 与热插拔日志。

用户安装厂商 Windows 驱动后，Windows PnP 对同一硬件 ID `USB\\VID_0471&PID_1200` 显示设备名 `ZLG USBCAN`、状态 `OK`、问题代码 0。Windows USB 驱动识别已验证；上位机打开通道、CAN 收发和 RK 端驱动仍未验证。

换用同一根已在 Windows 验证的数据线后，RK 串口截图的 `lsusb` 出现 `Bus 002 Device 004: ID 0471:1200`；`dmesg` 出现 `usb 2-1.1: new full-speed USB device number 4 using dwc2`。因此 RK 的 USB 物理连接与枚举已验证。尚未检查是否有 SocketCAN netdev、tty 或厂商驱动绑定；真实 CAN 收发仍未测试。

随后 RK 实机截图的 `ip link` 和 `ip -details link show` 仅见 `lo`、`eth0`、`eth1`、`wlan0`、`wlan1`、`vcan0`、`vcan1`；`ls /sys/class/net` 同样无真实 CAN 接口。`ls /dev/ttyUSB* /dev/ttyACM*` 均报告文件不存在。结论：当前固件没有为此 USB-CAN 暴露 SocketCAN 或串口接口；已有 `vcan0/vcan1` 是虚拟接口，不是适配器两路 CAN。仍未排除厂商用户态 VCI/libusb 路线，但现有 Linux 包只含 x86/x86-64 库，无法直接运行在 RK 的 armv7l 上。

## 2026-11-17 NUCLEO-F103RB 与 Windows USB-CAN 实收

板载 ST-LINK/V2-1 固件由 V2J28M18 更新至 V2J46M33 后，STM32CubeProgrammer 识别目标为设备 ID `0x410`、128 KB Flash 的 STM32F101/F102/F103 Medium-density。原始 128 KB Flash 已备份为 `outputs/firmware/stm32/build/original-flash.bin`。台架固件 `can_bench.bin` 已写入、校验成功并复位运行；绿色 LD2 以亮 2 秒、灭 2 秒的节奏运行。

Windows CANTest 2.69 选择 `USBCAN2`、设备索引 0、通道 0、500 kbps、正常模式后，连续收到标准数据帧 ID `0x100`、DLC 8。截图中约每秒一帧，数据从 `FE 08 00 00 46 31 30 33` 递增至 `0D 09 00 00 46 31 30 33`；末四字节 `46 31 30 33` 为 ASCII `F103`，前四字节为小端心跳计数。这是 STM32→SN65HVD230→真实 CAN 总线→Windows USB-CAN 的远端实收证据。

随后 CANTest 正常发送标准数据帧 ID `0x123`、DLC 8、数据 `11 22 33 44 55 66 77 88`；接收列表紧接着出现 STM32 返回的标准数据帧 ID `0x321`，DLC 和八字节数据完全一致。至此 Windows→USB-CAN→CAN 总线→STM32→CAN 总线→USB-CAN→Windows 的双向实收测试通过。

全断电后将现场线由 CAN1 移至 CAN2，CAN1 终端关闭、CAN2 终端开启；用户报告 CAN2 的 `0x100` 心跳和 `0x123`→`0x321` 双向应答结果与 CAN1 相同、工作正常。CAN2 本次没有单独保存截图，因此该项作为用户现场确认记录；CAN1 有上述截图证据。

USB-CAN 接回 RK3506B 后，`lsusb` 再次显示 `Bus 002 Device 003: ID 0471:1200`。sysfs 路径为 `/sys/bus/usb/devices/2-1.1`，接口 `2-1.1:1.0` 暴露四个端点：`0x01` Interrupt OUT、16 字节；`0x81` Interrupt IN、16 字节；`0x02` Bulk OUT、64 字节；`0x82` Bulk IN、64 字节。端点存在不代表协议兼容；现有厂商 Linux 包仍只有封闭的 x86/x64 `libusbcan.so`，需要取得协议/ARM SDK或通过 USB 抓包实现 ARM 用户态兼容层。

## 2026-11-17 RK3506B 用户态 USB-CAN 实收

根据 Windows USBPcap 抓包实现并静态交叉编译 ARMv7/libusb 探测程序 `outputs/software/rk3506/usbcan_probe_armv7`。RK 实机成功完成 CAN2 关闭、初始化、滤波和启动命令，终端显示 `CAN2 started at 500 kbit/s`。随后持续收到标准数据帧 `id=100 dlc=8`，数据示例为 `b8 15 00 00 46 31 30 33` 至 `c3 15 00 00 46 31 30 33`，计数器连续递增，末四字节为 ASCII `F103`。至此 STM32→真实 CAN 总线→USB-CAN→RK3506B ARM 用户态接收链路实机通过。

在同一 ARM 用户态程序中加入 Bulk OUT 发送后，RK 启动 CAN2 即发送标准数据帧 `id=123 dlc=8 data=11 22 33 44 55 66 77 88`；下一条接收记录为 STM32 返回的 `id=321 dlc=8 data=11 22 33 44 55 66 77 88`。因此 RK3506B→USB-CAN→CAN 总线→STM32→CAN 总线→USB-CAN→RK3506B 的双向真实收发测试通过。

## 2026-11-18 双通道协议与网关

Windows USBPcap 分别采集 CAN1、CAN2 的 500 kbit/s 正常模式初始化。CAN1
通道选择符确认为 `0x00`、滤波选择符 `0x40`；CAN2 分别为 `0x10` 和
`0x50`。STM32 总线移至 CAN1 后，CANTest 连续收到 ID `0x100` 心跳，抓包
确认接收描述符的 `0x1000` 位区分通道：清零为 CAN1，置位为 CAN2。

据此编译 ARMv7 静态双通道程序，SHA-256
`89BBB8E17F967813CC66967A8A51FA77D4467F6DC7F4E350E371A55E40AA8556`。
RK 临时运行及正式安装后均同时启动 CAN1/CAN2；当前接在 CAN1 的 STM32
心跳被连续记录为 `can=1`。原单通道程序备份为
`/userdata/usbcan_gateway/usbcan_gateway.single.backup`。

- 日期/操作者：待填
- RK 板型号、输入额定值/实测：待填
- 系统/内核/固件版本和校验值：待填
- USB-CAN 型号、VID:PID、驱动、USB 路径：待填（资料声明 0471:1200，不是实测）
- 接入路线（SocketCAN / VCI），SDK 架构与 ABI：待填
- 静态资料核查：见 docs/review-2026-09-06.md，不能代替真实测试
- CAN1/CAN2 对应 netdev（附映射证据）：待填
- Nucleo 版本、HAL/Cube 版本、编译结果、固件校验值：待填
- PCLK1/位时序/模式：待填
- 3V3 空载/接模块后电压：待填
- CAN1/CAN2 实验 H-L 断电电阻、终端设置：CAN1 整线 57.8Ω；CAN2 按相同双终端拓扑测试，用户确认正常
- 线长、参考地接法：待填
- 0x123 请求与 0x321 回复的原始抓包：2026-11-17 CANTest 截图已验证，数据 `11 22 33 44 55 66 77 88`
- 0x100 心跳、持续时间/实际帧数、漏帧：待填
- 起止 ip -details -statistics / 错误帧 / bus-off：待填
- CAN1↔CAN2 测试的两端日志：两个通道已分别与 STM32 双向测试；通道间直连测试仍待做
- 结论（通过/失败/阻塞）及问题：待填

不要以 vcan、本地发送回显、接口 UP、TX 队列入队成功或 USB 枚举成功代替远端实收证据。
# 2026-11-17 CAN 状态 HMI 构建记录

- 目标：在保留原工业监控界面的前提下，增加独立 CAN 状态页。
- 目标架构：ARM 32-bit ELF（ELF class 1，`e_machine=0x28`）。
- 二进制：`outputs/software/rk3506/can-status-hmi`
- 大小：38136 bytes
- SHA-256：`556164025EC41DB38C37714A1878DEB1EDAD8033AE0333E1BD6E53EDFFC1A3A0`
- 安全属性：只读 `/var/log/usbcan_gateway.log`；不修改原 `systemui`、
  `controlcenter` 或工业监控程序；默认不配置开机启动。
- 安装器：备份现有二进制到 `.bak`，新文件先以 `.new` 下载并执行
  `--self-test`，通过后才原子替换。
- 当前边界：已完成交叉编译和 ELF 头验证；因 Windows 未枚举 CH343/COM5，
  尚未在 RK 的 Wayland 画面上执行最终显示验收。

