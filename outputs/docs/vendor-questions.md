# 发给 USB-CAN 卖家的技术问题草稿

下面是供用户自行发送的文字，本次没有向卖家发送消息。

我购买的双通道隔离 Type-C USB-CAN 要接 RK3506B（实机 `uname -m` 为 `armv7l`），系统为 Linux 6.1 + Buildroot。实机 `lsusb` 已识别 `0471:1200`；Windows 设备名为 `USBCAN-UC12`。当前 RK 没有对应的真实 `can` 或 `tty` 接口，只有既有 `vcan0/vcan1`。现有 Linux 驱动包只有 usbcan_libusb_x86/x64_2018_10_15，两份 libusbcan.so 都不是 ARM 架构。请提供：

1. 与我所购型号/硬件版本/固件匹配的 ARM32 SDK 或可交叉编译的完整库源码；说明 hard-float/soft-float ABI、libc 类型/最低版本、libusb 和 C++ 运行库依赖。
2. 是否原生支持 SocketCAN？若支持，请给出 Linux 驱动名、支持内核、源码/构建方法和实际 VID:PID；若只有 VCI API，请明确说明。
3. CAN1/CAN2 的 H、L、现场隔离地端子顺序、两通道是否彼此隔离、是否需要外部供电、USB 最大功耗和终端开关说明。
4. VCI API 的 500k 位时序、normal/单次发送设置、错误计数和 bus-off 恢复方法，以及该设备双通道的索引定义。

用户态 libc/浮点 ABI 尚需在 RK 实机确认。`0471:1200`、`armv7l` 已由实机截图确认；目前没有真实 CAN 收发测试结果。
