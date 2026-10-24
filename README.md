# OV-Watch 智能手表

<div align="center">

![MCU](https://img.shields.io/badge/MCU-STM32F411CEU6-lightblue)
![UI](https://img.shields.io/badge/UI-LVGL%208.2-red)
![OS](https://img.shields.io/badge/OS-FreeRTOS-green)
![BLE](https://img.shields.io/badge/BLE-KT6368A-blueviolet)
![License](https://img.shields.io/badge/License-GPL--3.0-blue)

</div>

基于 **STM32F411CEU6** 的开源智能手表工程，运行 **FreeRTOS + LVGL 8.2**。
本仓库在 [No-Chicken/OV-Watch](https://github.com/No-Chicken/OV-Watch) 的基础上二次开发，保留原版全部功能，并在其上补齐了 **手表 ↔ 智能音箱的 BLE 双向通信** 链路：手表把温湿度、心率、步数、气压、睡眠状态以文本帧上报，同时响应音箱的请求、暂停与心跳指令。

> 详细的分层设计、任务与数据流见 [ARCHITECTURE.md](./ARCHITECTURE.md)，驱动实现见 [BSP底层驱动详解.md](./BSP底层驱动详解.md)。

---

## 目录

- [一、项目简介](#一项目简介)
- [二、功能特性](#二功能特性)
- [三、硬件规格](#三硬件规格)
- [四、软件框架](#四软件框架)
- [五、目录结构与文件职责](#五目录结构与文件职责)
- [六、蓝牙通信协议](#六蓝牙通信协议)
- [七、编译与烧录](#七编译与烧录)
- [八、文档索引](#八文档索引)
- [九、版本与开发记录](#九版本与开发记录)
- [十、许可证与致谢](#十许可证与致谢)

---

## 一、项目简介

OV-Watch 是一块可以戴在手上的开源智能手表：STM32F411 主控 + ST7789 屏幕 + 六轴/温湿度/气压/罗盘/心率五路传感器，跑 FreeRTOS 做任务调度、LVGL 做界面绘制。

本仓库的重点是**蓝牙**。原版把 KT6368A 当作一个可开关的透传模块；本版本把它接进了完整的业务链路：

```
音箱 (SmartSpeaker)                  手表 (OV-Watch)
   │  $GET,DATA ───────────────────▶  │  立即回一帧 + 启动周期上报
   │  ◀─────────────────── $DATA,...  │  温湿度/心率/步数/气压/睡眠
   │  $END ────────────────────────▶  │  暂停上报
   │  $PING,7 ─────────────────────▶  │
   │  ◀──────────────────── $PING,7   │  心跳回显
```

工程按「上层不碰硬件」的思路分层：UI 和任务只调用 `HWInterface` 这个硬件控制面板，`HWInterface` 再落到 BSP 的具体驱动；新增一路传感器或换一块屏，只需要改 BSP 和 `HWDataAccess`，上层代码不动。

---

## 二、功能特性

### 2.1 手表本体

| 功能 | 说明 |
|---|---|
| 表盘 / 菜单 | 首页表盘 + 九宫格菜单，页面栈管理，支持返回上一页 |
| 心率 | EM7028 PPG 采样 + 自研峰值检测算法（官方库太慢，已替换） |
| 环境 | AHT21 温湿度 |
| 气压 / 海拔 | SPL06-001 |
| 指南针 | LSM303DLH 电子罗盘 |
| 计步 / 抬腕亮屏 | MPU6050 + DMP，RTC 定时检测抬腕 |
| 睡眠页 | 可手动切换「清醒 / 准备入睡 / 睡眠」三态，用于 BLE 联调 |
| 计算器 | 双栈表达式求值，支持小数与优先级 |
| 小游戏 | 2048、记忆翻牌 |
| 实用工具 | 计时器、日历、日期时间设置、NFC 卡、关于、充电页、关机页 |
| 低功耗 | 运行 / STOP 睡眠（RTC + 抬腕唤醒）/ 关机三态；不用蓝牙时 PA8 断电 |

### 2.2 蓝牙双向通信（本项目核心）

- **物理层**：USART1 + DMA 接收 + 串口 IDLE 中断判定帧结束，`BleCommTask` 负责半包拼接与帧解析
- **上行**（手表 → 音箱）：`$DATA` 传感器数据帧
- **下行**（音箱 → 手表）：`$GET,DATA`、`$END`、`$PING`、`$VOL`、`$LED`
- **节奏**：开启蓝牙后每 60 s 采集一次；收到 `$GET,DATA` 立即回一帧，之后每 120 s 周期上报；收到 `$END` 暂停
- **休眠协同**：串口收到数据会向 `IdleBreak_MessageQueue` 投递消息，避免息屏打断通信
- **广播名**：`BMLSWOV`
- **低功耗**：`KT6328_Enable/Disable` 通过 PA8 控制蓝牙模块电源，关蓝牙即断电

---

## 三、硬件规格

| 类别 | 器件 | 说明 |
|---|---|---|
| 主控 | **STM32F411CEU6** | Cortex-M4，100 MHz，512 KB Flash，128 KB SRAM，UFQFPN-48 |
| 显示 | **ST7789** | SPI 接口 LCD，240 × 280 |
| 触摸 | **CST816** | 电容触摸，独立 I2C 控制 |
| 姿态 / 计步 | **MPU6050** | 六轴 IMU + DMP 运动处理库 |
| 温湿度 | **AHT21** | 数字温湿度传感器 |
| 气压 | **SPL06-001** | 气压 / 温度，可用于海拔估算 |
| 电子罗盘 | **LSM303DLH** | 三轴磁力计 + 加速度计 |
| 心率 | **EM7028** | PPG 光电心率传感器 |
| 存储 | **BL24C02** | 2 Kbit EEPROM，保存设置、步数等掉电数据 |
| 蓝牙 | **KT6368A** | BLE 5.1 双模透传模块，USART1 + AT 指令配置 |
| 电源 | **TPS63020 / TP4056 / BL1551B** | 升降压 DCDC 3.3 V、单节锂电充电管理、蓝牙电源负载开关 |
| 其他 | 功能按键 KEY1、硬件看门狗、RTC 32.768 kHz 晶振 | — |
| 结构 | Core 主板 + Back 背板 | 双层堆叠，过孔对齐贴装 |

传感器共用一条**软件模拟 I2C 总线**（`BSP/IIC/iic_hal.c`，带互斥锁）；屏幕走 SPI；蓝牙走 USART1。具体引脚分配见 [OV_watch原理图文字版](./OV_watch原理图文字版)。

---

## 四、软件框架

### 4.1 分层结构

```
┌──────────────────────────────────────────────────────────────┐
│ User/GUI_App    20 个 LVGL 页面（用户看到的界面）              │
│ User/Tasks      13 个 FreeRTOS 任务（业务逻辑）                │
│ User/Func       HWDataAccess / PageManager / pubsub / 计算器   │  ← 枢纽层
├──────────────────────────────────────────────────────────────┤
│ BSP             传感器 / LCD / 触摸 / 蓝牙 / 电源 / 按键驱动    │
│ SYSTEM          delay / sys 系统工具                          │
├──────────────────────────────────────────────────────────────┤
│ Middlewares     FreeRTOS + LVGL 8.2                          │
│ Drivers         STM32 HAL + CMSIS                            │
│ Core            CubeMX 生成：main.c / 时钟 / 外设初始化        │  ← 最底层
└──────────────────────────────────────────────────────────────┘
```

核心思想：**UI / 任务 → `HWInterface` → BSP 驱动**。UI 想读温度写 `HWInterface.AHT21.GetHumiTemp(&humi, &temp)`，不直接调用 `AHT_Read()`；`HW_USE_HARDWARE` 置 0 时同一套 UI 代码可在 PC 上跑仿真，由 `#else` 分支返回假数据。

### 4.2 启动流程

```
main.c
  → HAL_Init() / SystemClock_Config()           // 时钟与外设基础
  → MX_GPIO/DMA/ADC/RTC/SPI/TIM/USART_Init()    // 各外设初始化
  → osKernelInitialize() / MX_FREERTOS_Init()
        └→ User_Tasks_Init()                    // 创建 13 个任务 + 消息队列
  → osKernelStart()
        └→ HardwareInitTask（最高优先级）
              初始化电源/按键/5 个传感器/EEPROM/蓝牙/触摸/LCD
              → lv_init() + 显示/输入移植 + ui_init()
              → vTaskDelete(NULL) 自删
```

### 4.3 FreeRTOS 任务（13 个）

| 任务 | 职责 | 优先级 | 周期 |
|---|---|---|---|
| `HardwareInitTask` | 初始化全部硬件与 LVGL，跑完自删 | 最高 | 一次性 |
| `LvHandlerTask` | 驱动 LVGL 刷新 | 低 | 动态 |
| `WDOGFeedTask` | 喂看门狗 | 高 | 100 ms |
| `IdleEnterTask` | 息屏 / 进入 STOP 休眠 | 高 | 事件 |
| `StopEnterTask` | 关机 | 高 | 事件 |
| `KeyTask` | 按键扫描与处理 | 中 | 事件 |
| `ScrRenewTask` | 页面刷新 | 低 | 事件 |
| `SensorDataUpdateTask` | 刷新首页 / 环境 / 罗盘数据 | 低 | 500 ms |
| `HRDataUpdateTask` | 心率采样与计算 | 低 | 50 ms |
| `MPUCheckTask` | 抬腕检测 | 低 | 300 ms |
| `ChargPageEnterTask` | 充电检测与充电页 | 低 | 事件 |
| `BleCommTask` | 蓝牙收发 + 采集上报 | 低 | 1000 ms 轮询 |
| `DataSaveTask` | 数据写入 EEPROM | 低 | 事件 |

任务间通过 6 个消息队列通信：`Key`、`Idle`、`Stop`、`IdleBreak`、`HomeUpdata`、`DataSave`。

### 4.4 心率数据流示例

```
EM7028 采样（50 ms）
   → HRDataUpdateTask：读寄存器 → HR_Calculate() → 写 HWInterface.HR_meter.HrRate
   → 两个消费者：
        ├─ UI 心率页：显示数字
        └─ BleCommTask：每 60 s 采集成快照 → 每 120 s 发 $DATA,HR=..
```

---

## 五、目录结构与文件职责

### 5.1 仓库顶层

```
OV-Watch/
├── Software/
│   ├── OV_Watch/          # 手表 APP 主工程（Keil MDK）
│   └── IAP_F411/          # BootLoader / OTA 工程
├── lv_sim_vscode_win/     # LVGL PC 仿真工程（VSCode + CMake）
├── ARCHITECTURE.md        # 架构详解
├── BSP底层驱动详解.md      # BSP 驱动逐文件讲解
├── RTOS_UI_详解.md         # FreeRTOS 与 UI 机制详解
├── 蓝牙BLE                 # 蓝牙串口收数流水线详解
├── KT6368A的AT指令.txt     # 蓝牙 AT 指令速查
├── # KT6368A 蓝牙芯片开发参考手册.txt
├── # OV-Watch 手表端双向通信实现清单（最小改动原厂固件）.txt
├── OV_watch原理图文字版     # 原理图网络与引脚说明
├── README.md / README_Eng.md
└── LICENSE                # GPL-3.0
```

### 5.2 `Software/OV_Watch/`（手表 APP）

| 文件 / 目录 | 职责 |
|---|---|
| `User/version.h` | 固件版本号与版本说明（当前 v2.4.5） |
| `Core/Src/main.c` | 程序入口：HAL 初始化、时钟配置、外设初始化、启动内核 |
| `Core/Src/freertos.c` | CubeMX 生成的 FreeRTOS 初始化，调用 `User_Tasks_Init()` |
| `Core/Src/stm32f4xx_it.c` | 中断向量：SysTick、USART1（DMA/IDLE）、EXTI 等 |
| `Core/Src/rtc.c` | RTC 时钟与唤醒定时 |
| `Core/Src/usart.c` | USART1 初始化 + DMA 接收（蓝牙透传数据） |
| `Core/Src/spi.c` | SPI 初始化（LCD） |
| `Core/Src/adc.c` | 电池电压采集 |
| `Core/Src/tim.c` | HAL 时基定时器 |
| `Core/Src/dma.c` / `gpio.c` | DMA 控制器 / GPIO 初始化 |
| `Core/Inc/*.h` | 上述模块的头文件与 `main.h` 引脚宏 |
| `Drivers/` | ST HAL 库与 CMSIS（无需改动） |
| `Middlewares/Third_Party/FreeRTOS` | FreeRTOS 内核与移植层 |
| `Middlewares/LVGL/GUI` | LVGL 8.2 图形库 |
| `SYSTEM/delay.c/.h`、`sys.h` | 微秒/毫秒延时、系统工具 |
| `MDK-ARM/OV_Watch.uvprojx` | Keil 工程文件（新增 `.c` 需手动加入） |
| `MDK-ARM/startup_stm32f411xe.s` | 启动汇编文件 |

### 5.3 `BSP/`（板级驱动）

| 模块 | 文件 | 职责 |
|---|---|---|
| `IIC/` | `iic_hal.c/.h` | 软件模拟 I2C 底层时序 + 总线互斥锁（5 个传感器共用） |
| `AHT21/` | `AHT21.c/.h` | 温湿度采集与校准计算 |
| `EM7028/` | `em70x8.c/.h`、`HrAlgorythm.c/.h`、`user_Queue.c/.h` | 心率 PPG 采样、峰值检测算法、采样队列 |
| `LSM303DLH/` | `LSM303.c/.h` | 电子罗盘数据读取与航向角计算 |
| `MPU6050/` | `mpu6050.c/.h`、`eMPL/*` | 六轴数据读取、DMP 计步、抬腕检测 |
| `SPL06_001/` | `SPL06_001.c/.h` | 气压/温度采集，气压→高度换算 |
| `TOUCH/` | `CST816.c/.h` | 电容触摸坐标与手势 |
| `BL24C02/` | `BL24C02.c/.h`、`DataSave.c/.h` | EEPROM 读写与设置项持久化 |
| `LCD/` | `lcd.c/.h`、`lcd_init.c/.h`、`lcdfont.h`、`pic.h` | ST7789 初始化、刷屏、字库与图片数据 |
| `KT6328/` | `KT6328.c/.h` | 蓝牙模块上电/AT 初始化/文本帧收发（`$命令,参数\r\n`） |
| `POWER/` | `power.c/.h` | 电量百分比、充电状态、电源使能 |
| `KEY/` | `key.c/.h` | 按键按下/松开事件 |
| `OWDG/` | `WDOG.c/.h` | 独立看门狗初始化与喂狗 |

### 5.4 `User/Func/`（功能中间层）

| 文件 | 职责 |
|---|---|
| `HWDataAccess.c/.h` | 全局 `HWInterface` 硬件控制面板，含 9 个子接口（时钟/蓝牙/电源/屏幕/IMU/温湿度/气压/罗盘/心率）；`HW_USE_HARDWARE` 切换真机与仿真 |
| `PageManager.c/.h` | 页面栈（最多 6 层）：`Page_Load` / `Page_Back` / `Page_Back_Bottom`，每个页面有 `init` / `deinit` |
| `pubsub.c/.h` | 轻量发布订阅，键盘/触摸事件广播（如仿真器 Backspace → `Page_Back()`） |
| `StrCalculate.c/.h` | 计算器表达式解析：数字栈 + 运算符栈，处理优先级与小数 |

### 5.5 `User/Tasks/`（业务任务）

| 文件 | 任务 | 职责 |
|---|---|---|
| `user_TasksInit.c/.h` | — | 创建全部任务与消息队列；`LvHandlerTask`、`WDOGFeedTask` 也定义在此 |
| `user_HardwareInitTask.c` | `HardwareInitTask` | 硬件与 LVGL 一次性初始化，完成后自删 |
| `user_KeyTask.c` | `KeyTask` | 按键扫描、消抖、事件分发 |
| `user_ScrRenewTask.c` | `ScrRenewTask` | 页面切换与刷新 |
| `user_SensUpdateTask.c` | `HRDataUpdateTask`、`SensorDataUpdateTask`、`MPUCheckTask` | 心率、环境/首页数据、抬腕检测 |
| `user_RunModeTasks.c` | `IdleEnterTask`、`StopEnterTask` | 息屏休眠、关机 |
| `user_ChargCheckTask.c` | `ChargPageEnterTask` | 充电检测与充电页跳转 |
| `user_DataSaveTask.c` | `DataSaveTask` | 异步写 EEPROM |
| `user_BleCommTask.c` | `BleCommTask` | **蓝牙收发、传感器采集与 `$DATA` 上报**（本项目核心） |

### 5.6 `User/GUI_App/`（界面）

| 文件 / 目录 | 职责 |
|---|---|
| `ui.c` / `ui.h` | `ui_init()` 入口、LVGL 主定时器、页面注册 |
| `ui_helpers.c/.h` | SquareLine 生成的通用控件与动画辅助函数 |
| `Screens/Src/*.c`、`Screens/Inc/*.h` | 20 个页面：首页、菜单、心率、血氧、环境、指南针、睡眠、计算器、2048、记忆游戏、游戏选择、计时器、日历、设置、关于、充电、日期时间、NFC 卡、关机、消息框 |
| `Fonts/` | 中文字库（Cuyuan 系列）与图标字库（iconfont） |
| `IMGs/` | 图片资源（如罗盘指针） |

### 5.7 `Software/IAP_F411/`（BootLoader）

| 文件 | 职责 |
|---|---|
| `Core/Src/main.c` | BootLoader 入口：判断 APP 有效性并跳转到 `0x08008000` |
| `Ymodem/ymodem.c/.h` | Ymodem 传输协议（串口/蓝牙 OTA 升级） |
| `Ymodem/flash_if.c/.h` | Flash 擦除与写入接口 |
| `Ymodem/menu.c/.h` | 串口升级菜单交互 |
| `BSP/` | BootLoader 用到的按键、LCD、蓝牙、电源驱动 |
| `MDK-ARM/` | BootLoader 的 Keil 工程 |

### 5.8 `lv_sim_vscode_win/`（PC 仿真）

| 文件 / 目录 | 职责 |
|---|---|
| `main.c` | 仿真器入口，创建 SDL 窗口并驱动 LVGL |
| `lv_conf.h` / `lv_drv_conf.h` | LVGL 与驱动配置（仿真时 `HW_USE_HARDWARE = 0`） |
| `user_test/` | 被仿真的手表 UI 代码副本 |
| `lvgl/` / `lv_drivers/` | LVGL 8.2 与 PC 驱动（上游子模块内容） |
| `CMakeLists.txt` / `Makefile` | 构建脚本 |

---

## 六、蓝牙通信协议

### 6.1 物理链路

```
音箱 ── BLE ──▶ KT6368A（透传）── UART ──▶ USART1 ── DMA ──▶ BleCommTask
                                              IDLE 中断标记一帧结束
```

`BSP/KT6328/KT6328.c` 负责 AT 初始化与文本帧封装；`user_BleCommTask.c` 负责粘包/半包处理、指令分派与数据组帧。

### 6.2 帧格式

```
$指令码[,参数1,参数2,...]\r\n
```

以 `$` 为帧头、`\r\n` 为帧尾，ASCII 文本，字段间用逗号分隔。

### 6.3 指令表

| 方向 | 指令 | 参数 | 作用 |
|---|---|---|---|
| 音箱 → 手表 | `$GET,DATA` | — | 请求传感器数据：立即回一帧，并启动 120 s 周期上报 |
| 音箱 → 手表 | `$END` | — | 暂停周期上报 |
| 音箱 → 手表 | `$PING` | `序号`（可选） | 心跳，手表原样回显 `$PING,序号` |
| 音箱 → 手表 | `$VOL` | `0~100` | 音量同步（预留，待接 UI） |
| 音箱 → 手表 | `$LED` | `R,G,B` | 灯控（预留） |
| 手表 → 音箱 | `$DATA` | 见 6.4 | 传感器数据帧 |
| 手表 → 音箱 | `$PING` | `序号` | 心跳回显 |

### 6.4 `$DATA` 字段

| 字段 | 含义 | 单位 / 取值 |
|---|---|---|
| `T` | 温度 | ℃，1 位小数，无效时为 0 |
| `H` | 湿度 | %，整数，无效时为 0 |
| `HR` | 心率 | 次/分，未测到为 0 |
| `SPO2` | 血氧 | %，当前固定 0（功能未实现） |
| `STEP` | 步数 | 步 |
| `P` | 气压 | hPa，1 位小数，超出 300~1100 视为无效 |
| `SLEEP` | 睡眠状态 | 0 清醒 / 1 准备入睡 / 2 睡眠 |
| `ST` | 睡眠时长 | 小时，1 位小数 |

示例：

```
$DATA,T=26.5,H=48,HR=72,SPO2=0,STEP=3560,P=1013.2,SLEEP=0,ST=0.0
```

### 6.5 交互时序

```
音箱                                  手表
 │  连接（广播名 BMLSWOV）              │
 │  $GET,DATA ───────────────────────▶ │ g_send_enable=1，立即发一帧
 │  ◀─────────────────────── $DATA,... │
 │                                     │ 每 120 s 周期发送
 │  ◀─────────────────────── $DATA,... │
 │  $END ────────────────────────────▶ │ g_send_enable=0，停止上报
```

采集（60 s）与发送（120 s）解耦：即使不发送，`BleCommTask` 仍持续更新数据快照，避免 `$GET,DATA` 时读到过期数据。

---

## 七、编译与烧录

### 7.1 手表 APP（Keil MDK）

1. 用 **Keil MDK 5** 打开 `Software/OV_Watch/MDK-ARM/OV_Watch.uvprojx`
2. 选择目标 `OV_Watch`，编译（F7），下载（F8）
3. 新增 `.c` 文件时需在 Keil 工程中手动加入对应分组

### 7.2 BootLoader 与 APP 分区

| 区域 | 起始地址 | 说明 |
|---|---|---|
| BootLoader | `0x08000000` | `Software/IAP_F411`，支持 Ymodem / 蓝牙 OTA |
| APP | `0x08008000` | `Software/OV_Watch`，需在 Keil 中设置 IROM1 起始地址与向量表偏移 |

### 7.3 LVGL 仿真（无硬件）

`lv_sim_vscode_win` 是用 SDL2 把屏幕"搬"到 PC 上的工程：改好 `lv_conf.h` / `lv_drv_conf.h` 的路径后可直接用 VSCode + CMake 编译运行。仿真时把 `HW_USE_HARDWARE` 置 0，`HWDataAccess` 会返回固定假数据，UI 逻辑照常调试。

依赖版本：**LVGL 8.2**、**FreeRTOS**、**STM32 HAL**、**SDL2**（仿真）。

---

## 八、文档索引

| 文档 | 内容 |
|---|---|
| [ARCHITECTURE.md](./ARCHITECTURE.md) | 分层设计、目录职责、启动流程、页面栈、发布订阅、任务与消息队列、I2C 总线、关键变量 |
| [BSP底层驱动详解.md](./BSP底层驱动详解.md) | 每个 BSP 驱动模块的接口、时序与调用关系 |
| [RTOS_UI_详解.md](./RTOS_UI_详解.md) | FreeRTOS 任务机制与 LVGL 页面组织方式 |
| [蓝牙BLE](./蓝牙BLE) | 从串口字节到业务帧的完整收数流水线（DMA + IDLE 中断） |
| [KT6368A的AT指令.txt](./KT6368A的AT指令.txt) | 蓝牙模块 AT 指令速查 |
| [# KT6368A 蓝牙芯片开发参考手册.txt](./%23%20KT6368A%20蓝牙芯片开发参考手册.txt) | 芯片开发要点与调试笔记 |
| [OV_watch原理图文字版](./OV_watch原理图文字版) | Core + Back 双层板的网络与引脚说明 |
| [README_Eng.md](./README_Eng.md) | 上游英文说明（图片链接指向上游仓库，仅作参考） |

---

## 九、版本与开发记录

上游基线：**v2.4.5**（LCD 改用同步 DMA 刷屏）。本仓库在其上完成的工作：

| 提交 | 内容 |
|---|---|
| 新增 KT6368A 蓝牙 AT 初始化配置 | 模块上电、AT 指令、透传通道打通 |
| 新增 BLE 双向通信框架、添加中文注释及屏幕常亮调试改动 | `$命令` 帧收发的骨架 |
| 新增 BLE 传感器数据发送功能 | 温湿度/心率/步数/气压采集与 `$DATA` 组帧 |
| 修复开机即休眠和 BLE 任务栈溢出 | 事件标志误判、任务栈扩容 |
| 阶段 2：修改蓝牙广播名称为 BMLSWOV | 便于音箱端过滤设备 |

---

## 十、许可证与致谢

- 本项目基于 [No-Chicken/OV-Watch](https://github.com/No-Chicken/OV-Watch) 二次开发，遵循 **GPL-3.0**，见 [LICENSE](./LICENSE)
- [LVGL](https://github.com/lvgl/lvgl) — MIT
- [FreeRTOS](https://www.freertos.org/) — MIT
- STM32 HAL / CMSIS — BSD-3-Clause

如果你在自己的项目里使用了本仓库的代码，请保留原作者的版权声明，并按 GPL-3.0 开源你的修改。
