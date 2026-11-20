# 芯路城新增下载资料核查（2026-10-22）

## 来源

- [用户提供的驱动 ZIP](https://testxinlu.oss-cn-beijing.aliyuncs.com/static/upload/file/warehouse/2026/06/17/1781690560853493.zip)，下载副本位于 `work/inspection/xinlu-driver-1781690560853493.zip`，85447654 字节，SHA256 `326803d0d0bbbaf41855f2df2ce56dc7624a4ce2b48095150da1ed180961e6ef`。
- [芯路城 USB 转双路 CAN 分析仪常见问题与上位机教程](https://www.xinlucity.com/?s=plugins/index/pluginsname/ask/pluginscontrol/index/pluginsaction/detail/id/78.html)，于 2026-10-22 读取页面文字。

## 内容和适用性

ZIP 文件列表共 7 项，其中一项为目录；文件为 `USBCAN_AllInOne_x86_x64_2.0.0.1.exe`、Windows 驱动安装视频、Win7 数字签名问题 PDF、两个 Windows 6.1 KB3033929 `.msu`、Win7 签名补丁安装程序。该包没有 ARM32 Linux 库、Linux 驱动源码或 SocketCAN 驱动。仅检查文件列表和哈希，未运行安装程序或补丁。

教程页列出 CANalyst、CANPro、CANTest、ZCANPRO 四种上位机软件，并描述 Windows 软件的设备选择和操作；该页未给出 RK3506B/Buildroot 的 ARM32 SDK 或 SocketCAN 接入方法。教程中的设备名称并不能证明当前 `USBCAN-UC12` 与任何 Linux 驱动兼容。

## 对本项目的结论

此 ZIP 可作为 Windows PC 安装驱动的候选资料，不能解决 RK 上 `0471:1200` 已 USB 枚举但没有真实 `can`/`tty` 接口的问题。仍需向卖家索取适配该设备固件的 ARM32 Linux SDK、完整可编译源码，或明确支持该 VID:PID 的 SocketCAN 驱动。

进一步检查卖家[客户资料页](https://www.xinlucity.com/?s=resourcedetail/index/id/160.html)的“二次开发”ZIP：其中 Linux RAR 的 SHA256 为 `6603a235d6fadef4c8421564ab4d1d77e4a9dacc18153e599ef143ce995ee9dc`，与用户原在 E 盘提供的 Linux RAR 完全相同，仍仅含 x86/x86-64 库。该页另有“实物图尺寸图”ZIP，其中实物图提供 G1/L1/H1/G2/L2/H2 端子丝印和两个 120R 开关位置，已用于接线文档；未运行任何包内程序。
