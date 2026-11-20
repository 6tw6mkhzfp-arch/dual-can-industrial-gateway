# 官方依据与适用边界

查询日期：2026-09-06；芯片资料不替代第三方模块或载板说明书。

- ST [NUCLEO-F103RB / UM1724](https://www.st.com/resource/en/user_manual/um1724-.pdf)：MB1136 接头表中 CN10-12=PA12、CN10-14=PA11、CN7-16=3V3、CN7-20=GND；供电与跳帽仍需匹配实物版本。
- ST [STM32F103RB 数据手册](https://www.st.com/resource/en/datasheet/stm32f103rb.pdf)：单 CAN 控制器与引脚功能。
- TI [SN65HVD230 数据手册](https://www.ti.com/lit/ds/symlink/sn65hvd230.pdf)：3.3V 收发器供电、RS 模式和总线电气要求。
- Linux [SocketCAN 文档](https://docs.kernel.org/networking/can.html)：CAN 网络接口、位速率、错误状态与本地回环；部署目标仍是用户的 Linux 6.1，应核对其实际配置。
- ST [STM32F1 HAL CAN 接口](https://github.com/STMicroelectronics/stm32f1xx-hal-driver/blob/master/Inc/stm32f1xx_hal_can.h)：HAL CAN 配置、发送及接收 API；需与工程自带 HAL 版本一致。
- ADI [DS1302 数据手册](https://www.analog.com/media/en/technical-documentation/data-sheets/DS1302.pdf)：三线时钟接口、供电和涓流充电条件。

更新：用户提供的底板原理图已确认 12V 输入和 USB-A 布线；Linux SDK 已核验为 x86/x86-64 VCI/libusb 库。参见 [本地资料核查](review-2026-09-06.md)与 [源文件哈希](../tests/source-manifest.json)。实物 USB-CAN 型号/端子、可用 ARM 驱动、RK 核心板 GPIO 电压域和继电器阈值仍待确认。
