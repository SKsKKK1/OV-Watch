# OV-Watch 工程架构说明

> STM32F411CEU6 智能手表工程。本文整理整体分层、目录职责、核心结构体/变量、任务与数据流。

---

## 一、整体分层（自下而上）

```
┌─────────────────────────────────────────────────────────┐
│  User/GUI_App   UI 界面（20 个页面，LVGL 绘制）           │  ← 最上层，用户看到的
│  User/Tasks     13 个 FreeRTOS 任务（业务逻辑）           │
│  User/Func      功能中间层：HWDataAccess / PageManager    │  ← 枢纽层
├─────────────────────────────────────────────────────────┤
│  BSP            板级驱动：传感器 / LCD / 触摸 / 蓝牙 / 电源 │
│  SYSTEM         系统工具：delay（延时）/ sys              │
├─────────────────────────────────────────────────────────┤
│  Middlewares    中间件：FreeRTOS + LVGL                  │
│  Drivers        STM32 HAL 库 + CMSIS                    │
│  Core           CubeMX 生成：main.c / 启动 / 外设初始化   │  ← 最底层
└─────────────────────────────────────────────────────────┘
```

**核心思想**：上层（UI / 任务）只跟 `HWDataAccess` 这个「控制面板」打交道，不直接碰硬件；`HWDataAccess` 再把活儿分给 BSP 里的具体驱动。

---

## 二、目录职责

### Software/OV_Watch 内部

| 目录 | 是什么 | 关键文件 |
|---|---|---|
| **Core** | CubeMX 生成，芯片启动 + 外设初始化 | `main.c`（入口）、`freertos.c`（FreeRTOS 初始化）、`stm32f4xx_it.c`（中断）、`rtc.c` / `usart.c` / `spi.c` 等外设 |
| **Drivers** | ST 官方 HAL 库 + CMSIS | 不用改，拿来用 |
| **Middlewares** | 两个中间件 | `Third_Party/FreeRTOS`、`LVGL`（图形库） |
| **BSP** | 板级驱动（最该关心的） | 见下表 |
| **SYSTEM** | 延时 / 系统工具 | `delay.c`（`delay_us` / `delay_ms`） |
| **User** | 用户应用代码 | `Func` / `Tasks` / `GUI_App` |
| **MDK-ARM** | Keil 工程文件 | `OV_Watch.uvprojx`（新增 .c 文件要手动加入） |

### BSP（板级支持包）

| 子目录 | 硬件 | 引脚 |
|---|---|---|
| `IIC/` | 软件模拟 I2C 底层（`iic_hal.c`，带互斥锁） | — |
| `AHT21/` | 温湿度传感器 | PB13/PB14 |
| `EM7028/` | 心率传感器 | PB13/PB14 |
| `LSM303DLH/` | 电子罗盘（指南针） | PB13/PB14 |
| `MPU6050/` | 六轴 / 计步（含 DMP 算法库） | PB13/PB14 |
| `SPL06_001/` | 气压计 | PB13/PB14 |
| `TOUCH/` | 触摸屏 CST816 | PB4/PB6 |
| `BL24C02/` | EEPROM（存设置 / 步数） | PB11/PB12 |
| `LCD/` | 屏幕（ST7789 SPI） | — |
| `KT6328/` | 蓝牙芯片 KT6368A（AT 指令 + 透传） | PA8 供电 |
| `POWER/` | 电池电量采集 | ADC |
| `KEY/` | 实体按键 | — |
| `OWDG/` | 看门狗 | — |

### User（用户代码）

| 子目录 | 职责 | 关键文件 |
|---|---|---|
| `Func/` | 功能中间层 | `HWDataAccess`、`PageManager`、`pubsub`、`StrCalculate` |
| `Tasks/` | 13 个任务 | 见任务清单 |
| `GUI_App/` | UI | `ui.c/h`、`Screens/`（20 页）、`Fonts/`、`IMGs/` |

---

## 三、启动流程

```
main.c
  → HAL_Init()                        // 复位外设、初始化 Flash/SysTick
  → SystemClock_Config()              // 配系统时钟（HSI→PLL，100MHz）
  → MX_GPIO/DMA/ADC/RTC/SPI/TIM/USART_Init()  // 外设初始化
  → osKernelInitialize()              // FreeRTOS 内核初始化
  → MX_FREERTOS_Init()
        └→ User_Tasks_Init()          // 创建 13 个任务 + 消息队列
  → osKernelStart()                   // 启动调度器（此后由 FreeRTOS 接管）

调度器启动后：
  HardwareInitTask（最高优先级）先跑：
    初始化电源/按键/5个传感器/EEPROM/蓝牙/触摸/LCD
    → lv_init() + lv_port_disp/indev_init() + ui_init()
    → 完成后 vTaskDelete(NULL) 删除自己
```

---

## 四、核心枢纽：HWDataAccess（硬件中间层）

文件：`User/Func/Src/HWDataAccess.c`、`User/Func/Inc/HWDataAccess.h`

全局变量 `HWInterface`（类型 `HW_InterfaceTypeDef`）内含 9 个子接口：

```c
HW_InterfaceTypeDef HWInterface = {
    .RealTimeClock,   // RTC 时钟
    .BLE,             // 蓝牙
    .Power,           // 电池/电源
    .LCD,             // 屏幕背光
    .IMU,             // MPU6050 计步
    .AHT21,           // 温湿度
    .Barometer,       // 气压计
    .Ecompass,        // 电子罗盘
    .HR_meter,        // 心率/血氧 EM7028
};
```

每个子接口里有两类东西：

1. **函数指针** —— 指向 BSP 里真正干活的函数。例：`HWInterface.AHT21.GetHumiTemp` 指向 `AHT_Read()`。
2. **数据字段** —— 缓存读到的数值。例：`HWInterface.IMU.Steps`、`HWInterface.AHT21.temperature`。

**举例**：UI 要读温度，写 `HWInterface.AHT21.GetHumiTemp(&humi, &temp)`，而不是直接调 `AHT_Read()`。

**关键开关** `HW_USE_HARDWARE`（`HWDataAccess.h:34`）：

- 真机 = 1，走真实硬件
- PC 仿真 = 0，走 `#if HW_USE_HARDWARE` 的 `#else` 分支返回固定假数据

---

## 五、页面管理：PageManager（页面栈）

文件：`User/Func/Src/PageManager.c`、`User/Func/Inc/PageManager.h`

`Page_t` 结构体是每个页面的「身份证」：

```c
typedef struct {
    void (*init)(void);    // 初始化函数
    void (*deinit)(void);  // 反初始化（退出时清理）
    lv_obj_t **page_obj;   // 页面对象指针
} Page_t;
```

用**栈**管理页面切换（最多 6 层 `MAX_DEPTH`）：

| 函数 | 作用 |
|---|---|
| `Pages_init()` | 开机加载首页 |
| `Page_Load(&Page_X)` | 跳转到新页面（压栈） |
| `Page_Back()` | 返回上一页（弹栈） |
| `Page_Back_Bottom()` | 直接回到底部 |
| `Page_Get_NowPage()` | 拿到当前页面（任务判断「现在在哪个页」常用它） |

**关键点**：每个页面有 `init` / `deinit`，进入时 `init`、退出时 `deinit` 清理状态。

---

## 六、发布订阅：pubsub

文件：`User/Func/Src/pubsub.c`、`User/Func/Inc/pubsub.h`

轻量「消息广播」机制，目前主要给键盘 / 触摸事件用：

- `PubSub_Publisher_t`（发布者）维护订阅者链表
- `Publisher_subscribe()` 订阅，`Publisher_publish()` 广播
- 典型场景：仿真器按下 Backspace → 发布消息 → 订阅者执行 `Page_Back()`

---

## 七、任务清单（13 个 FreeRTOS 任务，`user_TasksInit.c` 创建）

| 任务 | 职责 | 优先级 | 周期 |
|---|---|---|---|
| `HardwareInitTask` | 初始化所有硬件 + LVGL，跑完自删 | 最高 | 一次性 |
| `LvHandlerTask` | 跑 LVGL（`lv_task_handler`） | 低 | 动态 |
| `WDOGFeedTask` | 喂看门狗 | 高 | 100ms |
| `IdleEnterTask` | 息屏 / 休眠 | 高 | — |
| `StopEnterTask` | 关机 | 高 | — |
| `KeyTask` | 按键处理 | 中 | — |
| `ScrRenewTask` | 屏幕刷新 | 低 | — |
| `SensorDataTask` | 更新首页 / 环境 / 指南针数据 | 低 | 500ms |
| `HRDataTask` | 测心率 | 低 | 50ms |
| `ChargPageEnterTask` | 充电页进入 | 低 | — |
| `BleCommTask` | 蓝牙收发 + 采集发送 | 低 | 1000ms |
| `MPUCheckTask` | 抬腕检测 | 低 | 300ms |
| `DataSaveTask` | 数据存 EEPROM | 低 | — |

**消息队列**（任务间通信）：`Key_MessageQueue`、`Idle_MessageQueue`、`Stop_MessageQueue`、`IdleBreak_MessageQueue`、`HomeUpdata_MessageQueue`、`DataSave_MessageQueue`。

---

## 八、传感器与 I2C 总线

```
PB13/PB14（软件 I2C）──────── 5 个传感器共用
   ├─ AHT21（温湿度）   ├─ EM7028（心率）   ├─ LSM303（罗盘）
   ├─ MPU6050（计步）   └─ SPL06（气压）

PB4/PB6    ── 触摸屏 CST816（独立）
PB11/PB12  ── EEPROM BL24C02（独立）
```

底层统一走 `iic_hal.c` 的 `IICStart` / `IICStop` / `IIC_Read_One_Byte` 等函数。

**互斥锁**：5 个传感器 + 多个任务并发访问 PB13/PB14，`iic_hal.c` 在 `IICStart`（上锁）/ `IICStop`（解锁）加了互斥锁，保证同一时刻只有一个任务在总线上通信。读操作有 repeated start（2 次 `IICStart`、1 次 `IICStop`），用 `osMutexGetOwner` 判断重入避免死锁。

---

## 九、UI 页面清单（20 个，`Screens/` 下）

首页、菜单、心率、血氧、环境、指南针、睡眠、计算器、2048、记忆游戏、游戏选择、计时器、日历、设置、关于、充电、日期时间设置、NFC 卡、关机、消息框。

每个页面 `ui_XxxPage.c` 里有 `Page_t Page_Xxx` 变量 + `screen_init` / `screen_deinit` 两个函数。

---

## 十、数据流举例

### 心率

```
① EM7028 硬件（LED 照皮肤采样）
      ↓ 每 50ms
② HRDataTask（user_SensUpdateTask.c）
      调 EM7028 读寄存器 → HR_Calculate() 算心率 → 写 HWInterface.HR_meter.HrRate
      ↓
③ 两个消费者同时读这个缓存：
   ├─ UI 心率页 → 显示数字
   └─ BLE 采集（每 30s）→ 填进 g_sensor.hr 快照
          ↓ 每 60s 发送
④ KT6328 蓝牙芯片 → 音箱收到 $DATA,HR=xx
```

温湿度、气压、步数类似，生产者是 `SensorDataTask`（500ms）和各自的 BSP 驱动。

---

## 十一、关键变量速查

| 变量 | 位置 | 作用 |
|---|---|---|
| `HWInterface` | HWDataAccess.c | 全局硬件控制面板（枢纽） |
| `PageStack` | PageManager.c | 页面栈 |
| `g_ble_on` | user_BleCommTask.c | 蓝牙开关 |
| `g_send_enable` | user_BleCommTask.c | 周期发送使能 |
| `g_sleep_flag` | user_BleCommTask.c | 睡眠三态（0 清醒 / 1 准备入睡 / 2 睡眠） |
| `g_hr_measure` | user_SensUpdateTask.c | 心率测量开关 |
| `g_sensor` | user_BleCommTask.c | 传感器数据快照（发蓝牙用） |
| `iic_mutex` | iic_hal.c | I2C 总线互斥锁 |
