# RTOS 与 UI/LVGL 应用层详解

> 聚焦「应用层」——任务文件、UI 页面、页面管理、LVGL 移植。
> 不含 FreeRTOS / LVGL 官方库源码（那些是拿来用的第三方代码）。

---

# 第一部分：RTOS 应用层（`User/Tasks/`）

## 1. FreeRTOS 初始化

### `Core/Src/freertos.c`

| 函数 | 作用 |
|---|---|
| `MX_FREERTOS_Init()` | FreeRTOS 初始化入口，内部调用 `User_Tasks_Init()` 创建所有任务，再创建一个空的 `defaultTask` |
| `StartDefaultTask()` | CubeMX 生成的默认任务，只是闪一下灯（PC13），无实际业务 |
| `vApplicationIdleHook()` | 空闲钩子（空实现） |
| `vApplicationTickHook()` | 每次系统 tick 中断调用 → 转调 `TaskTickHook()`（在 user_TasksInit.c 里，累加 LVGL tick 和空闲计时） |

### `Core/Inc/FreeRTOSConfig.h`

关键配置：`configUSE_MUTEXES=1`（启用互斥锁）、`configUSE_RECURSIVE_MUTEXES=1`、`configSUPPORT_DYNAMIC_ALLOCATION=1`（支持动态创建对象）。

---

## 2. 任务总控（`user_TasksInit.c`）

### 函数

| 函数 | 作用 |
|---|---|
| `User_Tasks_Init()` | 创建 13 个任务 + 6 个消息队列 + 1 个事件标志 |
| `TaskTickHook()` | 每 1ms 由 tick 中断调用：累加 LVGL tick、空闲计时 `IdleTimerCount`、心率计时 `user_HR_timecount` |
| `LvHandlerTask()` | 跑 `lv_task_handler()`，驱动整个 UI 刷新 |
| `WDOGFeedTask()` | 每 100ms 喂看门狗 |

### 创建的消息队列 / 事件标志

| 对象 | 类型 | 用途 |
|---|---|---|
| `Key_MessageQueue` | 队列 | 按键事件（key1/key2） |
| `Idle_MessageQueue` | 队列 | 通知熄屏 |
| `Stop_MessageQueue` | 队列 | 通知进入休眠 |
| `IdleBreak_MessageQueue` | 队列 | 用户有操作，唤醒亮屏 |
| `HomeUpdata_MessageQueue` | 队列 | 通知首页刷新数据 |
| `DataSave_MessageQueue` | 队列 | 通知保存到 EEPROM |
| `HardIntEventHandle` | 事件标志 | 硬件中断事件（充电 `HARDINT_EVENT_CHARG`、串口 `HARDINT_EVENT_UART`） |

### 13 个任务的属性（栈 × 优先级）

见「任务清单」章节。

---

## 3. 各任务详解

### `user_HardwareInitTask.c` — 硬件初始化任务

| 函数 | 作用 |
|---|---|
| `HardwareInitTask()` | 最高优先级任务，一次性初始化所有硬件：电源、按键、5 个传感器、EEPROM、蓝牙（配 AT 指令）、触摸屏、LCD、LVGL、UI。完成后 `vTaskDelete(NULL)` 删除自己 |

初始化顺序（代码里按此执行）：
RTC 唤醒定时器 → USART DMA 接收 → PWM → 电源 → 按键 → AHT21 → LSM303 → SPL06 → MPU6050 → EM7028 → EEPROM → 蓝牙（`AT+BMLSWOV` 设名 → `AT+CR00` 关回传 → `AT+CZ` 复位 → 默认关闭）→ 触摸屏 → LCD → LVGL → UI。

### `user_KeyTask.c` — 按键扫描任务

| 函数 | 作用 |
|---|---|
| `KeyTask()` | 每 1ms 扫描两个物理按键，分发事件 |

按键逻辑：
- **key1** → 发 `Key_MessageQueue`（返回上一页）+ `IdleBreak`（唤醒）
- **key2** → 在首页时发 `Stop_MessageQueue`（休眠）；其他页发 `Key_MessageQueue`（回首页）

### `user_ScrRenewTask.c` — 屏幕刷新任务

| 函数 | 作用 |
|---|---|
| `ScrRenewTask()` | 收到 `Key_MessageQueue` 后执行页面返回 |

- key1 → `Page_Back()`（返回上一页）
- key2 → `Page_Back_Bottom()`（回首页）
- 返回菜单页时，顺手让心率/指南针传感器睡眠省电。

### `user_RunModeTasks.c` — 电源状态管理（两个任务）

| 函数 | 作用 |
|---|---|
| `IdleEnterTask()` | 空闲检测：收到 `Idle_MessageQueue` 调暗背光，收到 `IdleBreak` 恢复背光 |
| `StopEnterTask()` | 休眠/唤醒：收到 `Stop_MessageQueue` 后关闭外设 → 进 STOP 低功耗 → 唤醒后恢复 |

**关键变量** `IdleTimerCount`（uint32_t）：空闲计时器，每 1ms 在 `TaskTickHook` 累加，用户按键/触摸时清零。

> ⚠️ 注意：`IdleEnterTask` 里两段"空闲超过阈值自动熄屏/休眠"的 if 代码当前被**注释掉了**（调试屏幕常亮用），恢复后才会真正自动熄屏。

### `user_SensUpdateTask.c` — 传感器数据（三个任务）

| 函数 | 作用 |
|---|---|
| `MPUCheckTask()` | 每 300ms 检查 MPU6050 姿态，判断抬腕/放腕，放腕时发休眠消息 |
| `HRDataUpdateTask()` | 每 50ms 测心率（心率页或 `g_hr_measure=1` 时），算心率写 `HWInterface.HR_meter.HrRate` |
| `SensorDataUpdateTask()` | 每 500ms 更新首页（电量/步数/温湿度）、环境页、指南针页数据 |

**关键变量** `g_hr_measure`（uint8_t）：心率测量开关，睡眠页的心率开关控制它。

### `user_BleCommTask.c` — 蓝牙通信任务

| 函数 | 作用 |
|---|---|
| `BleCommTask()` | 蓝牙帧收发主循环：收 `$GET/$END/$PING` 等指令，做采集/发送计时 |
| `BLE_CollectSensorData()` | 采集所有传感器 → 填入 `g_sensor` 快照（每 30s） |
| `BLE_SendSensorData()` | 组帧 `$DATA,T=...,H=...,HR=...,...` 发送 |
| `BLE_SyncSleepFlag()` | 睡眠标志同步：只更新快照 `g_sensor.sleep`，不立即发送 |
| `BLE_DispatchCmd()` | 命令分派（按帧头 `PING/GET/END/VOL/LED`） |
| `BLE_HandlePing/Get/End/Volume/Led()` | 各指令的处理函数 |

**关键变量**：
- `g_ble_on`（蓝牙开关）、`g_send_enable`（周期发送使能）
- `g_sleep_flag`（睡眠三态）、`g_sleep_time`（睡眠时长）
- `g_sensor`（传感器数据快照，类型 `SensorData_t`）

**结构体** `SensorData_t`（数据快照）：
```c
typedef struct {
    float    temp;    // 温度 ℃
    float    humi;    // 湿度 %
    uint8_t  hr;      // 心率
    uint8_t  spo2;    // 血氧（暂 0）
    uint16_t step;    // 步数
    float    pres;    // 气压 hPa
    uint8_t  sleep;   // 睡眠标志 0/1/2
    float    stime;   // 睡眠时长 小时
} SensorData_t;
```

### `user_DataSaveTask.c` — 数据保存任务

| 函数 | 作用 |
|---|---|
| `DataSaveTask()` | 收到 `DataSave_MessageQueue` 后，把设置和步数写入 EEPROM |

**EEPROM 存储布局**：
| 地址 | 内容 |
|---|---|
| 0x00/0x01 | 校验标志 0x55/0xAA |
| 0x10 | 抬腕唤醒设置 |
| 0x11 | APP 设置开关 |
| 0x20 | 上次保存日期 |
| 0x21/0x22 | 当天步数（高/低字节） |

跨天逻辑：读到 EEPROM 里的日期和今天不同 → 步数清零重新记。

### `user_ChargCheckTask.c` — 充电检测任务

| 函数 | 作用 |
|---|---|
| `ChargPageEnterTask()` | 用事件标志（门铃）等充电事件，插电跳充电页、拔电退出充电页 |

---

## 4. 任务清单（栈/优先级速查）

| 任务 | 栈大小 | 优先级 | 周期 |
|---|---|---|---|
| HardwareInitTask | 1280 | High3（最高） | 一次性 |
| LvHandlerTask | 3072 | Low | 动态 |
| WDOGFeedTask | 128 | High2 | 100ms |
| IdleEnterTask | 128 | High | 10ms |
| StopEnterTask | 2048 | High1 | 100ms |
| KeyTask | 128 | Normal | 1ms |
| ScrRenewTask | 1280 | Low1 | 10ms |
| SensorDataTask | 640 | Low1 | 500ms |
| HRDataTask | 640 | Low1 | 50ms |
| ChargPageEnterTask | 1280 | Low1 | 事件触发 |
| BleCommTask | 3072 | Low1 | 1000ms |
| MPUCheckTask | 384 | Low2 | 300ms |
| DataSaveTask | 640 | Low2 | 事件触发 |

---

# 第二部分：UI/LVGL 应用层

## 5. LVGL 移植（`Middlewares/LVGL/GUI/porting/`）

### `lv_port_disp.c` — 显示移植

| 函数 | 作用 |
|---|---|
| `lv_port_disp_init()` | 注册显示驱动：设分辨率、分配绘制缓冲、绑定 `disp_flush` 回调 |
| `disp_flush()` | LVGL 画好一块区域后，调 `LCD_Color_Fill()` 把数据刷到屏幕 |
| `lcd_flush_ready_callback()` | 屏幕刷完的回调，通知 LVGL 可继续画 |

缓冲方式：双缓冲（`BUFFER_METHOD=2`），各 1/10 屏大小。

### `lv_port_indev.c` — 输入（触摸）移植

| 函数 | 作用 |
|---|---|
| `lv_port_indev_init()` | 注册触摸输入设备，绑定 `touchpad_read` 回调 |
| `touchpad_read()` | LVGL 读取触摸状态和坐标 |
| `touchpad_is_pressed()` | 读 CST816 判断是否按下 |
| `touchpad_get_xy()` | 读触摸坐标 |

---

## 6. UI 入口与辅助

### `User/GUI_App/ui.c`

| 函数 | 作用 |
|---|---|
| `ui_init()` | 初始化默认主题、调 `Pages_init()` 加载首页、创建 1 秒定时器 |

### `User/GUI_App/ui.h`

声明所有字体（`LV_FONT_DECLARE`）+ `ui_init()`。各页面的 `Page_t` 在各自页面的 `.h` 里声明。

### `User/GUI_App/ui_helpers.c`（SquareLine 自动生成）

一批通用控件操作辅助函数，如：
- `_ui_slider_set_property()` / `_ui_bar_set_property()` — 设滑块/进度条值
- `_ui_label_set_property()` — 设文字
- `_ui_flag_modify()` / `_ui_state_modify()` — 加/删/切换控件 flag 或 state
- `_ui_anim_callback_set/get_xxx()` — 动画回调（位置/尺寸/透明度/缩放/角度）
- `_ui_slider_set_text_value()` — 把滑块值同步显示到文字标签

---

## 7. 页面管理（`User/Func/PageManager.c`）

| 结构体 | 字段 | 作用 |
|---|---|---|
| `Page_t` | `init` / `deinit` / `page_obj` | 一个页面的「身份证」：初始化函数、反初始化函数、页面对象指针 |
| `PageStack_t` | `pages[MAX_DEPTH]` / `top` | 页面栈：最多 6 层，`top` 是栈顶 |

| 函数 | 作用 |
|---|---|
| `Pages_init()` | 开机加载首页 |
| `Page_Load(&Page_X)` | 跳转新页面（压栈，先 deinit 当前页） |
| `Page_Back()` | 返回上一页（弹栈） |
| `Page_Back_Bottom()` | 直接回到底部 |
| `Page_Get_NowPage()` | 拿当前页面指针（任务判断「在哪个页」） |

---

## 8. 页面通用模式

每个 `ui_XxxPage.c` 都遵循同一套结构：

```
① Page_t Page_Xxx = {ui_XxxPage_screen_init, ui_XxxPage_screen_deinit, &ui_XxxPage};  // 注册
② void ui_XxxPage_screen_init(void)      // 创建所有控件（按钮/滑块/标签...）
③ void ui_XxxPage_screen_deinit(void)    // 清理（删控件/清状态/关传感器）
④ xxx_event_cb(lv_event_t * e)          // 事件回调（按钮点击/滑块变化/开关切换）
⑤ xxx_timer_cb(lv_timer_t * timer)      // （可选）定时刷新数据
```

- 带**定时刷新**的页面：首页、心率、环境、指南针、血氧、充电
- 带**事件回调**的页面：几乎所有（按钮/滑块/开关交互）

---

## 9. 页面清单（20+ 个 Page）

| 页面 | Page_t 名 | 主要函数 | 说明 |
|---|---|---|---|
| 首页 | `Page_Home` | `HomePage_timer_cb` + 多按钮事件 | 时间/步数/电量 + NFC/BLE/电源/设置/亮度滑块 |
| 电源页 | `Page_Power` | （在 HomePage.c） | 亮度调节 |
| 菜单页 | `Page_Menu` | 14 个 `ui_event_MenuXxxPanel` | 各功能入口（含睡眠入口） |
| 心率页 | `Page_HR` | `HRPage_timer_cb` | 定时刷新心率 |
| 血氧页 | `Page_SPO2` | `SPO2Page_timer_cb` | 定时刷新（暂占位） |
| 环境页 | `Page_Env` | `EnvPage_timer_cb` | 温湿度 |
| 指南针 | `Page_Compass` | `EcompassPage_timer_cb` | 方位角 + 海拔 |
| 睡眠页 | `Page_Sleep` | `ui_SleepSlider_event_cb` / `ui_SleepHRSwitch_event_cb` | 三挡滑块 + 心率开关 |
| 计算器 | `Page_Computer` | `ui_CompageBtnM_event_cb` 等 | 计算器 |
| 2048 | `Page_Game_2048` | `move_up/down/left/right` 等 | 2048 游戏 |
| 记忆游戏 | `Page_GameMem` | `Game_Mem_Init` 等 | 翻牌记忆游戏 |
| 游戏选择 | `Page_GameSelect` | 2 个 panel 事件 | 选 2048 / 记忆游戏 |
| 计时器 | `Page_Timer` | `LabelRefresh_timer` + 按钮事件 | 秒表 |
| 日历 | `Page_Calender` | — | 日历显示 |
| 设置页 | `Page_Set` | 6 个 `ui_event_xxx` | 亮度时间/抬腕/日期/密码入口 |
| 日期时间设置 | `Page_DateTimeSet` / `Page_DateSet` / `Page_TimeSet` | 多按钮事件 | 改日期/时间/APP 开关 |
| 关机设置 | `Page_LOffTimeSet` / `Page_TOffTimeSet` | OK 按钮事件 | 设息屏时间/休眠时间 |
| NFC 卡 | `Page_NFCCard` | `ui_event_WriteBtn1` / `ChooseBtn1` | NFC 门禁卡模拟 |
| 关于页 | `Page_About` | — | 关于信息 |
| 充电页 | `Page_Charg` | `ChargPage_timer_cb` | 充电动画 |
| 消息框 | （ui_Megboxes.c） | `ui_mbox_create` / `ui_mbox_close` | 弹窗提示 |

---

# 第三部分：关键结构体汇总

| 结构体 | 位置 | 作用 |
|---|---|---|
| `Page_t` | PageManager.h | 页面身份证（init/deinit/page_obj） |
| `PageStack_t` | PageManager.h | 页面栈 |
| `HW_InterfaceTypeDef` | HWDataAccess.h | 硬件控制面板（含 9 个子接口） |
| `HW_RTC_InterfaceTypeDef` 等 9 个 | HWDataAccess.h | 每个硬件模块的接口（函数指针 + 数据字段） |
| `SensorData_t` | user_BleCommTask.c | BLE 传感器数据快照 |
| `iic_bus_t` | iic_hal.h | I2C 总线描述（端口 + 引脚） |
| `PubSub_Message_t` | pubsub.h | 发布订阅消息（id + data[256]） |
| `PubSub_Publisher_t` | pubsub.h | 发布者（订阅者链表） |
| `Stack_T` | ui_Game2048Page.c | 2048 游戏的栈（`Data[STACK_DEPTH]` + `Top_Point`） |

---

# 第四部分：任务与 UI 的关联

## 数据怎么从硬件流到屏幕

```
传感器/BSP → HWInterface 缓存字段（如 HWInterface.IMU.Steps）
              ↓
UI 页面的 timer_cb 定时读 HWInterface → 更新 lv_label/lv_arc 显示
```

## UI 事件怎么影响任务/硬件

```
用户点按钮/滑块 → 页面的 event_cb → 改全局变量 或 调 HWInterface 函数指针
  例：
  点蓝牙按钮   → ui_event_BLEButton → g_ble_on=1 + HWInterface.BLE.Enable()
  滑睡眠滑块   → ui_SleepSlider_event_cb → g_sleep_flag=x + BLE_SyncSleepFlag()
  滑亮度滑块   → ui_event_LightSlider → ui_LightSliderValue + HWInterface.LCD.SetLight()
  开心率开关   → ui_SleepHRSwitch_event_cb → g_hr_measure=1
```

## 跨层的全局变量（UI ↔ 任务共享）

| 变量 | 定义处 | 谁读 |
|---|---|---|
| `ui_LightSliderValue` | ui_HomePage.c | 任务（息屏/唤醒时设背光） |
| `ui_LTimeValue` / `ui_TTimeValue` | ui_OffTimePage.c | 空闲检测任务 |
| `ui_APPSy_EN` | ui_DateTimeSetPage.c | DataSaveTask（存 EEPROM） |
| `g_ble_on` / `g_sleep_flag` | user_BleCommTask.c | UI + BLE 任务 |
| `g_hr_measure` | user_SensUpdateTask.c | UI + 心率任务 |

---

## 快速定位指南

| 想改什么 | 去哪 |
|---|---|
| 加/改一个页面 | `User/GUI_App/Screens/Src/ui_XxxPage.c` + 菜单页加入口 |
| 改页面切换逻辑 | `User/Func/Src/PageManager.c` |
| 加一个任务 | `User/Tasks/Src/user_TasksInit.c` 注册 + 新建 .c 文件 |
| 改传感器读取 | `User/Func/Src/HWDataAccess.c`（中间层） + `BSP/xxx/` |
| 改蓝牙协议 | `User/Tasks/Src/user_BleCommTask.c` |
| 改电量/背光/休眠 | `user_RunModeTasks.c` + `BSP/POWER` + `BSP/LCD` |
