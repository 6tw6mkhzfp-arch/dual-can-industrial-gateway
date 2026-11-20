# 双 CAN 工业边缘网关 + HMI

基于 RK3506B、USB 双通道 CAN、STM32 NUCLEO-F103RB 和 5.5 英寸 MIPI
屏构建的工业监控原型。项目已经完成实机 CAN 通信、传感器采集、全屏 HMI、
故障联动和长期运行所需的日志轮换。

## 已实现功能

- USB-CAN `0471:1200` 的 ARMv7 用户态双通道通信
- 经典 CAN 500 kbit/s 收发和设备掉线自动重连
- STM32 周期发送心跳、温湿度、运行状态和报警帧
- DHT22 真实温湿度采集
- 光敏传感器环境亮度采集
- Qt/Wayland 全屏工业 HMI
- CAN 报警或离线时自动启动风扇
- 正常状态绿色指示灯
- 服务开机自启动、看门式重启和容量受限的日志轮换

## 目录

- [`outputs/firmware/stm32`](outputs/firmware/stm32)：STM32 固件与构建脚本
- [`outputs/software/rk3506`](outputs/software/rk3506)：RK3506 网关、HMI 源码和启动脚本
- [`outputs/docs`](outputs/docs)：接线、协议及设计文档
- [`outputs/tests`](outputs/tests)：测试记录和来源清单

## 主要文档

- [CAN 应用层协议](outputs/docs/can-application-protocol.md)
- [硬件与接线](outputs/docs/wiring.md)
- [RK3506 软件说明](outputs/software/rk3506/README.md)
- [STM32 固件说明](outputs/firmware/stm32/README.md)
- [测试记录](outputs/tests/record.md)

## 当前定位

该仓库是经过实机验证的功能原型，不是已认证的工业量产产品。用于现场部署前，
仍需完成绝缘机箱、端子化布线、电源保护、隔离 CAN/ESD、EMC 和长期可靠性验证。

MQ-2、蜂鸣器和 microSD 未纳入最终配置：MQ-2 需要额外的 5V 到 3.3V
模拟分压保护；现有蜂鸣器模块声音异常；RK3506 板无 microSD 卡槽。
