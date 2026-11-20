# NUCLEO-F103RB 最小经典 CAN 测试

`bench_main.c` 与 `can_test.c/.h` 已通过 `build-bench.ps1` 编译，生成 `build/can_bench.bin`，供 NUCLEO-F103RB 台架测试使用。ST-LINK 固件从 V2J28M18 升级至 V2J46M33 后，STM32CubeProgrammer 已识别设备 ID 0x410 和 128 KB Flash。已将原始 128 KB Flash 保存为 `build/original-flash.bin`，随后烧录 `can_bench.bin`，工具报告 `Download verified successfully` 并软件复位。实际 CAN 收发仍待测试。

最新台架固件将绿色 LD2 每 2 秒切换一次亮灭（完整亮灭周期 4 秒），便于区分原程序；初始化错误时每 100 毫秒切换一次。

运行构建：在 PowerShell 执行 `& 'D:\Codex\dual-can-industrial-gateway\outputs\firmware\stm32\build-bench.ps1'`。脚本使用本机 STM32CubeIDE 1.19.0 与 STM32CubeF1 v1.8.7。

本板使用板载 ST-LINK 时，CN2 的两只跳帽均须装上。CN2 两只跳帽都取下时，ST-LINK 改用于外接 SWD 目标。确认跳帽和供电后再连接 STM32CubeProgrammer；成功识别 STM32F103RB 之后才能烧录。

## 工程配置

1. 新建与实际 NUCLEO-F103RB 匹配的 STM32F103RBTx 工程，保留 SWD；使用板上 ST-LINK 烧录。不要启用目标 STM32 的 USB 外设；ST-LINK 是独立芯片，其 USB 可以用于调试。
2. 用 HSI 8MHz，经 HSI/2 ×16 得到 SYSCLK 64MHz；AHB /1，APB1 /2 得到 PCLK1=32MHz，APB2 /1。让 Cube 生成相应 Flash latency 和时钟初始化。HSI 精度受温度影响，此配置仅用于台架起步；后续用核实过的外部时钟提高裕量。
3. 启用 CAN，默认 PA11=CAN_RX、PA12=CAN_TX（不重映射）。CAN GPIO 和时钟由生成的 HAL_CAN_MspInit 配置；RX 为输入，TX 为复用推挽。开启 HAL_CAN_MODULE_ENABLED。
4. 初始时序设 Prescaler=4、BS1=13TQ、BS2=2TQ、SJW=1TQ、Normal、自动重发关闭、自动 bus-off 管理关闭。函数也会设置这些值；不要让另一段 CAN 初始化/中断消费同一 FIFO。
5. 计算：32MHz / [4 × (1+13+2)] = 500000 bit/s，采样点 87.5%。若改用 PCLK1=36MHz，可设置 Prescaler=4、BS1=15TQ、BS2=2TQ 得到 500k，但必须同时改源码与时钟检查；不可直接混用。
6. 将 can_test.c/.h 加入 Core/Src 与 Core/Inc。在 main.c 用户区加入头文件；完成 HAL_Init、SystemClock_Config、MX_GPIO_Init、MX_CAN_Init 后调用以下代码：

```c
if (CAN_Test_Init(&hcan) != HAL_OK) { Error_Handler(); }
while (1) {
    CAN_Test_Poll();
    HAL_Delay(1);
}
```

若生成句柄名不是 hcan，改成实际名称。CAN_Test_Init 会在启动前应用位时序、接收所有帧的过滤器并启动 CAN；不要在此前 HAL_CAN_Start。测试采用轮询，无须 CAN IRQ；HAL 时基仍需正常运行。工程需包含 stm32f1xx_hal_can.c 和其所需 HAL 源文件。

协议：每秒发标准数据帧 0x100（小端计数 + F103 标识）；只对标准数据帧 0x123 回复 0x321，复制 DLC/数据。远程帧、扩展帧不回复。回复依赖 MCU 实际接收请求，比 Linux 本地回显更有证明力。

调试器可观察 can_test_rx、can_test_queued、can_test_dropped、can_test_error。queued 仅表示进入发送邮箱，不表示已被对端接收；error 是 HAL 最近错误快照。最终成功判据必须来自 RK 抓包和控制器统计。关闭自动重发便于台架避免无 ACK 时持续重试；正式产品的重发与 bus-off 恢复策略需另行设计。
