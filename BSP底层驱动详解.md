# OV-Watch BSP 底层驱动详解

> 适用范围：`Software/OV_Watch/BSP/` 目录全部 14 个模块。
> 主控：STM32F411（Cortex-M4）。本文按"文件 → 函数"逐项说明每个函数**做什么、怎么做到、实现原理**。

---

## 0. 总览：BSP 在固件中的位置与整体结构

BSP（板级支持包）是固件中所有硬件外设驱动的集合，向上被 RTOS 任务（UI、传感器采集、蓝牙通信等）调用，向下直接操作 STM32F411 的寄存器/HAL 库。全项目共享的关键资源是**软件模拟 I2C 总线**（`iic_hal.c`），AHT21、EM7028、LSM303、MPU6050、SPL06 五个传感器**共用 PB13(SDA)/PB14(SCL) 同一组引脚**，由 RTOS 互斥锁串行化访问，避免多任务同时拉总线。

| 总线/外设 | 连接的模块 | 说明 |
| --- | --- | --- |
| 软件 I2C（PB13/PB14，互斥锁） | AHT21、EM7028、LSM303DLH、MPU6050、SPL06-001 | 五个传感器共用，公共底层在 `IIC/iic_hal.c` |
| 软件 I2C（PA11/PA12） | BL24C02（EEPROM） | 独立总线 |
| 软件 I2C（PB4/PB6） | CST816（触摸） | 独立总线 |
| SPI1 | ST7789 LCD（240×280） | 显示 + TIM3 CH3 PWM 背光 + DMA 加速 |
| USART1 | KT6328（蓝牙 BLE） | AT 指令 / `$`文本帧透传 |
| GPIO/EXTI | KEY（PA5/PA4）、POWER（PA1/PA2/PA3）、WDOG（PB1/PB2）、MPU6050 INT（PB12） | 按键中断、充电中断、看门狗、运动中断 |
| ADC1 | 电池电压（PA1） | 8 次采样平均 + 查表换算电量 |

目录结构：

```
BSP/
├── IIC/          软件 I2C 公共底层（所有 I2C 器件共用）
├── AHT21/        温湿度传感器
├── BL24C02/      EEPROM 存储 + 设置保存（DataSave）
├── EM7028/       心率传感器 + 心率算法 + 环形队列 + 算法库(.lib)
├── LSM303DLH/    加速度计 + 磁力计（罗盘/翻腕检测）
├── MPU6050/      六轴传感器 + eMPL 官方 DMP 库
├── SPL06_001/    气压计（海拔）
├── TOUCH/        电容触摸屏 CST816
├── LCD/          ST7789 屏幕（初始化/绘图/字库/图片）
├── KT6328/       蓝牙 BLE 芯片
├── KEY/          按键
├── POWER/        电源管理与电量检测
└── OWDG/         外部看门狗
```

---

## 1. IIC —— 软件 I2C 底层（`iic_hal.c` / `iic_hal.h`）

这是全项目 I2C 器件的地基：用 GPIO 位翻转软件模拟 I2C 时序，不依赖 STM32 硬件 I2C 外设，因此 SDA/SCL 可以接到任意 GPIO。所有上层传感器驱动都调用这里的原语。

**`iic_bus_t`（结构体）**：描述一条 I2C 总线——SDA/SCL 的 GPIO 端口与引脚。每个器件模块实例化一份（如 `AHT_bus`、`BL_bus`），写驱动时只需换结构体，代码即可复用。

- **`IIC_Bus_Lock` / `IIC_Bus_Unlock`（静态）**：RTOS 互斥锁。因为五个传感器共用 PB13/PB14，多个任务会并发操作同一组 GPIO，锁保证一次只有一个任务占用总线。读操作会发"重复起始信号"（两次 `IICStart` 但只一次 `IICStop`），若盲目加锁会把自己锁死，所以用 `osMutexGetOwner` 判断"锁是否已经是我持有的"，是则直接跳过加锁，避免死锁。
- **`SDA_Input_Mode` / `SDA_Output_Mode`**：把 SDA 引脚在"上拉输入"和"开漏输出"两种模式间切换。原理是 I2C 半双工：发送数据时 SDA 是输出，接收数据（含从机应答）时从机拉 SDA，主机必须把引脚切成输入才能读到电平；开漏输出 + 外部上拉电阻实现"线与"逻辑。
- **`SDA_Output` / `SCL_Output`**：通过 GPIO 的 `BSRR` 寄存器写引脚电平（低 16 位置 1、高 16 位清零，单条指令原子操作，不打断正在进行的时序）。SCL 永远是输出。
- **`SDA_Input`**：读 SDA 引脚当前电平（1/0）。
- **`IICStart`**：产生起始条件——SCL 为高期间 SDA 由高变低。流程：加锁 → SDA=1、SCL=1（空闲）→ SDA=0 → SCL=0，配合 `delay_us` 保证建立/保持时间满足 I2C 协议。
- **`IICStop`**：产生停止条件——SCL 为高期间 SDA 由低变高，最后解锁总线。
- **`IICWaitAck`**：等待从机应答：SDA 切输入，SCL 拉高，若读到低电平说明从机应答（ACK）；若持续为高（NACK）或超时（重试 5 次）则发停止信号并返回 `ERROR`。原理：I2C 规定应答位期间从机把 SDA 拉低。
- **`IICSendAck` / `IICSendNotAck`**：主机主动发应答/非应答。连续读多个字节时，前 `length-1` 个字节发 ACK 表示"继续发"，最后一个字节发 NACK 通知从机"别再发了"，之后跟停止条件。
- **`IICSendByte`**：按 I2C 协议**高位先行**逐位发送：先拉低 SCL，把数据位放到 SDA，再拉高 SCL（从机在高电平期间采样），8 次循环完成一字节。
- **`IICReceiveByte`**：逐位接收：SDA 切输入，SCL 拉低→拉高，在 SCL 高电平期间读 SDA，先收最高位，8 位拼成一字节。
- **`IIC_Write_One_Byte`**：写单寄存器，完整事务为：START → 器件地址(写方向) → ACK → 寄存器地址 → ACK → 数据 → ACK → STOP。任何一步无应答即终止。
- **`IIC_Write_Multi_Byte`**：同上，但在寄存器地址之后连续发送 `length` 个数据字节（每字节都等 ACK）。用于 EEPROM 页写或向同一寄存器连续灌数据。
- **`IIC_Read_One_Byte`**：读单寄存器：START → 器件地址(写) → 寄存器地址 → **重复 START** → 器件地址(读) → 读一字节 → 主机发 NACK → STOP。重复起始是 I2C 读的标准手法：先写寄存器指针、再切读方向。
- **`IIC_Read_Multi_Byte`**：读多字节，读回过程中前 N-1 字节发 ACK，最后发 NACK + STOP。
- **`IICInit`**：把 SDA/SCL 初始化为推挽输出 + 上拉，作为软件 I2C 的初始状态。

---

## 2. AHT21 —— 温湿度传感器（`AHT21.c` / `AHT21.h`）

AHT21 是数字温湿度芯片（I2C 7 位地址 0x38，写 0x70 / 读 0x71），挂 PB13/PB14，模块内实例化 `AHT_bus`。

- **`AHT_Read_Status`**：读状态寄存器（发 0x71 读一字节）。bit7=忙标志、bit3=校准使能、低 4 位表示工作模式，上层靠它判断芯片是否可测量。
- **`AHT_Read_Cal_Enable`**：检查校准位——`(val & 0x68) == 0x08` 表示芯片已进入 NOR 模式且已校准，返回 1。原理：AHT21 出厂带校准系数，未校准的芯片测量值无意义。
- **`AHT_Reset`**：发软复位命令（0x70 + 0xBA），把芯片恢复到上电状态。
- **`AHT_Init`**：初始化：开 GPIOB 时钟 → `IICInit` → 延时 40ms（上电稳定）→ 若读到的状态不是已校准模式，则发 0xBE 初始化命令（带 0x08、0x00 两个参数）使芯片进入测量模式。
- **`AHT_Read`**：一次完整的温湿度测量：先发触发命令（0xAC, 0x33, 0x00），延时 80ms 等转换完成，然后最多 5 次轮询状态寄存器的忙位（bit7），忙则每 5ms 重试；就绪后连续读 6 字节原始数据，按数据手册拆包：**湿度 = 20bit 原始值 / 2^20 × 100%**，**温度 = 20bit 原始值 / 2^20 × 200 − 50℃**。代码用定点移位实现（`RetuData * 1000 >> 20`），避免浮点开销；湿度/温度共用第 3 字节的低 4 位（湿度取 20 位，温度取低 12 位接后续两字节），所以拆包时先拼湿度再拼温度。

---

## 3. BL24C02 + DataSave —— EEPROM 存储与设置保存

### `BL24C02.c` —— EEPROM 驱动
BL24C02 是 2Kbit（256 字节）EEPROM，I2C 器件地址 0x50，挂 PA11/PA12（`BL_bus`）。

- **`BL24C02_Write`**：从 `addr` 起连续写 `length` 字节，直接复用底层 `IIC_Write_Multi_Byte`（地址 0x50、写寄存器地址即 EEPROM 内部地址）。EEPROM 按页写，单页通常 8 字节，跨页连续写需分页，这里交由调用方控制长度。
- **`BL24C02_Read`**：从 `addr` 起连续读 `length` 字节，复用 `IIC_Read_Multi_Byte`。
- **`BL24C02_Init`**：开 GPIOA 时钟 + `IICInit`。

### `DataSave.c` —— 数据持久化层
在 EEPROM 之上封装"校验 + 设置读写"，布局见文件头注释：`[0x00][0x01]` 存 0x55/0xAA 签名，`[0x10]` 存手腕设置，`[0x11]` 存 UI 应用使能，`[0x20]` 存上次保存日期、`[0x21]` 存当日步数。

- **`EEPROM_Init`**：调 `BL24C02_Init`，打通 I2C 和 EEPROM。
- **`EEPROM_Check`**：自检：读地址 0~1，若等于 0x55/0xAA 说明 EEPROM 正常返回 0；否则先写入签名再回读验证——若写后能读回正确值说明芯片正常（只是从未初始化过），仍返回 0；写读都不对则返回 1 表示芯片损坏。原理：用已知模式的"写→读→比对"证明存储介质真实可用。
- **`SettingSave` / `SettingGet`**：保存/读取用户设置。只允许操作 `addr > 1` 的区域（保护 0~1 的签名区），且每次都先做 `EEPROM_Check` 确认 EEPROM 健康再读写，失败返回 1 而非静默出错。

---

## 4. EM7028 —— 心率传感器与心率算法

### `em70x8.c` —— EM7028 芯片驱动
EM7028 是心率和血氧（PPG）传感器（I2C 地址 0x24，ID=0x36），挂 PB13/PB14（`EM7028_bus`）。

- **`EM7028_ReadOneReg` / `EM7028_WriteOneReg`**：单寄存器读写封装（复用 IIC 底层单字节读写）。
- **`EM7028_Get_ID`**：读 0x00 寄存器，正常应返回 0x36，用于确认芯片在位。
- **`EM7028_hrs_init`**：初始化：开时钟 → `IICInit` → 轮询 ID（最多 5 次×100ms）→ 依次写配置：HRS_CFG=0x00（先关测量）、HRS2_DATA_OFFSET=0（无暗电流偏移）、HRS2_GAIN_CTRL=0x7F（增益 1）、HRS1_CTRL=0x47（HRS1 增益 1、量程 8、采样时钟 2.62144MHz、16bit 分辨率）、INT_CTRL=0x00（LED 电流 2.5mA）。
- **`EM7028_hrs_Enable`**：HRS_CFG=0x08，打开 HRS1 通道——点亮 LED1（红光），红光+IR 传感器使能，PPG 数据写入 HRS_DATA0。开始测量前同样先确认 ID。
- **`EM7028_hrs_DisEnable`**：HRS_CFG=0x00 关测量，熄灭 LED 省电。
- **`EM7028_Get_HRS1`**：读 HRS1_DATA0 高/低两字节（0x29/0x28）合成 16 位 PPG 原始幅值。

### `HrAlgorythm.c` —— 心率计算算法
- **`HR_AlgoInit`**：初始化三条队列（原始数据、时间戳、心率结果）。
- **`Hr_Ave_Filter`**：对数组求平均（滑动平均滤波，平滑心率抖动）。注意该函数实际未 `return` 平均值，返回值未定义，属实现瑕疵；调用处仅用其副作用。
- **`HR_Calculate`**：核心算法——**滑动窗口峰值检测 + 峰间隔换算心率**。把当前采样值和时刻压入 7 格环形队列 `datas`/`times`；当队列中点 `data[3]` 同时大于/等于前后相邻值（局部极大值）且与前一个峰的时间差 > 425ms（即心率上限约 141bpm，滤掉高频噪声尖峰）时，判定为一个心跳峰，记录峰时刻，用 `60000 / 峰间隔(ms)` 得瞬时心率，压入 `HR_List`；当队列攒满 7 个心率值后取平均输出。原理：PPG 信号中每次心跳对应一个反射光强低谷（幅度峰值），峰与峰之间的时间间隔即心跳周期。

### `user_Queue.c` —— 环形队列
- **`initQueue`**：front=0、rear=-1、size=0 初始化。
- **`isQueueEmpty` / `isQueueFull`**：按 `size` 判断空/满（容量 7）。
- **`enqueue`**：入队，`rear = (rear+1) % QUEUE_SIZE` 取模循环，满则打印提示并拒绝。
- **`dequeue`**：出队，`front = (front+1) % QUEUE_SIZE`，返回队头元素。取模实现环形复用，避免搬移数据。

### 二进制算法库
`HeartRatelib.lib`（心率）、`libBp.lib`（血压）、`libBpm.lib`（血氧）是厂商预编译的算法库，由 `HrAlgorythm.c` 之外的应用层（如 User 目录的算法调用）链接使用，源码不可见，功能是把 PPG 原始数据进一步算出心率/血氧/血压。

---

## 5. KEY —— 按键（`key.c` / `key.h`）

- **`Key_Port_Init`**：初始化两个按键引脚：KEY1（PA5）配**下降沿中断 + 上拉**（按下接地产生下降沿）；KEY2（PA4，唤醒键）配**上升沿中断 + 无上拉**（按键接高电平，按下产生上升沿）。使能 EXTI9_5 与 EXTI4 两个中断线，优先级 5。中断回调 `Key_Interrupt_Callback` 在别处实现（用于从睡眠中唤醒系统）。
- **`KeyScan`**：软件消抖 + 状态机扫描。`static key_up/key_down` 记忆"未按下/已按下"状态；`mode` 非 0 时强制复位状态（支持连续扫描模式）。检测到 KEY1 为低或 KEY2 为高时 `osDelay(3)` 延时消抖，确认后记录按键号并置 `key_up=0`（防止重复触发）；当按键释放（KEY1 高且 KEY2 低）且此前有按下记录时，返回按键值并复位状态。原理：先用延时过滤抖动毛刺，再用"按下→释放"双边沿锁存，保证一次按下只返回一次键值。

---

## 6. KT6328 —— 蓝牙 BLE 芯片（`KT6328.c` / `KT6328.h`）

- **`KT6328_GPIO_Init`**：初始化 PA8 为推挽输出并拉低，作为蓝牙模组的控制/使能引脚（上电默认关闭）。
- **`KT6328_SendATCmd`**：通过 USART1 发送一条 AT 指令，**自动补 `\r\n`** 结尾（AT 协议要求回车换行结束）。
- **`KT6328_SendText`**：发送 BLE 透传文本帧：`$指令码,参数\r\n`（无参数时发 `$指令码\r\n`），由蓝牙模组原样转发到手机端 App。`sprintf` 组帧后经串口发出。
- **`KT6328_Enable` / `KT6328_Disable`**：PA8 置高/置低，控制蓝牙模组使能（低功耗时关闭模组）。头文件定义了与手机端的文本帧指令码（PING/BAT/VOL/LED/TIME/REQ），这是项目自定义的双向通信协议。

---

## 7. LCD —— ST7789 屏幕驱动

### `lcd_init.c` —— 底层时序与初始化
ST7789 为 240×280 TFT 屏，SPI1 接口；控制脚 RES(PB7)、CS(PB8)、DC(PB9)，背光由 TIM3 CH3 PWM 驱动（BLK=PB0 亦在头文件中定义）。

- **`LCD_GPIO_Init`**：把 RES/CS/DC 三个脚配为推挽输出并置高（CS 高=未选中、RES 高=不复位）。
- **`LCD_Writ_Bus`**：向 SPI1 发送 1 字节（`HAL_SPI_Transmit`）。保留了软件 SPI 位翻转代码（可切换），实际用的是硬件 SPI。
- **`LCD_WR_DATA8` / `LCD_WR_DATA`**：写 8 位/16 位数据。16 位 RGB565 颜色拆成高字节+低字节两次发送（或一次 2 字节发送）。
- **`LCD_WR_REG`**：写命令：DC 拉低（命令模式）→ 发字节 → DC 拉高（回到数据模式）。ST7789 用 DC 区分命令/数据。
- **`LCD_Address_Set`**：设置显示窗口：写 0x2A 命令设置列地址（X 范围）、0x2B 设置行地址（Y 范围）、0x2C 命令表示"开始写显存"，之后的 SPI 数据按窗口顺序填充。
- **`LCD_Set_Light`**：调 TIM3 CH3 比较值，`dc/100 × PWM_PERIOD` 控制背光占空比（亮度），限幅 5%~100%。
- **`LCD_Close_Light` / `LCD_Open_Light`**：PWM 比较值置 0（关背光）/ 启动 PWM 输出（开背光）。
- **`LCD_ST7789_SleepIn` / `LCD_ST7789_SleepOut`**：发 0x10 / 0x11 命令进入/退出睡眠模式（各延时 100ms 等稳定）。
- **`LCD_Init`**：完整初始化序列：GPIO → 拉低 CS → RES 拉低 100ms 再拉高 100ms（硬件复位）→ 0x11 退出睡眠 → 0x36 设置扫描方向（按 `USE_HORIZONTAL` 选 0x00/0xC0/0x70/0xA0）→ 0x3A=0x05 像素格式 16bit → 0xB2/0xB7/0xBB/0xC0/0xC2/0xC3/0xC4/0xC6/0xD0 写电源时序与充电泵参数 → 0xE0/0xE1 写正负 Gamma 校正曲线 → 0x21 开反色 → 0x29 开显示。这些寄存器值来自 ST7789 数据手册的推荐配置。

### `lcd.c` —— 绘图与文字
- **`LCD_Fill`**：在指定矩形区域填单一颜色：`LCD_Address_Set` 定窗口后逐点写颜色（注意 `OFFSET_Y=20`：屏上 20 行是状态区/圆孔区，逻辑坐标自动偏移）。
- **`LCD_Color_Fill`**：批量填充一帧像素数组，用 `HAL_SPI_Transmit_DMA` **DMA 搬运**（不占 CPU），适合 LVGL 等整块刷新加速。
- **`LCD_Set_Flush_Complete_Callback`**：注册"刷新完成"回调（供 LVGL 通知）。
- **`HAL_SPI_TxCpltCallback`**：SPI DMA 完成中断：等待 `BSY` 标志清空（确保最后一位真正移出移位寄存器）后调用回调。这是 DMA 与 GUI 库之间的同步握手。
- **`LCD_DrawPoint`**：单点画点：窗口设为 (x,y)-(x,y)，写一个颜色。
- **`LCD_DrawLine`**：**Bresenham 直线算法**：取 X/Y 增量，选增量大的轴为步进方向，用误差累计（xerr/yerr）决定另一轴何时进 1，逐点画。
- **`LCD_DrawRectangle`**：四条直线围成矩形。
- **`Draw_Circle`**：**中点圆算法**：a 从 0 递增、b 从 r 递减，用 `a²+b²>r²` 判别式控制，八分对称一次画 8 个点。
- **`LCD_ShowChinese` / `12x12`/`16x16`/`24x24`/`32x32`**：汉字显示。按汉字内码（2 字节）在 `lcdfont.h` 的点阵字库表中**顺序查找**（`Index[0]==s[0] && Index[1]==s[1]`），命中后按行扫描字模字节的每一位：mode=0（非叠加）每一位写前景色或背景色（整字块覆盖）；mode=1（叠加）只在为 1 的位画前景点（透明背景）。s 每处理完一个汉字前进 2 字节（GB2312 双字节编码），x 前进一个字号宽度。
- **`LCD_ShowChar`**：ASCII 字符显示：`num-' '` 得到在字库中的偏移，查 `ascii_1206/1608/2412/3216` 对应字号字体，逐位写点，原理同汉字。
- **`LCD_ShowString`**：循环调用 `LCD_ShowChar`，每个字符 x 前进 `sizey/2`。
- **`mypow`**：整数幂函数（循环累乘），供数字拆位用。
- **`LCD_ShowIntNum`**：按十进制拆位显示整数，高位为 0 显示空格（`enshow` 标志：首个非零位出现前不显示 0），实现"前导零隐藏"。
- **`LCD_ShowFloatNum1`**：显示保留两位小数的浮点：`num*100` 变整数后拆位，在倒数第 2 位前插入小数点字符。
- **`LCD_ShowPicture`**：显示 `pic.h` 中预存的 RGB565 图片数组（每像素 2 字节，低字节在前），设窗口后逐个字节送显。

### `lcdfont.h` / `pic.h`（数据文件）
`lcdfont.h` 是点阵字库：ASCII 四种字号（6×12/8×16/12×24/16×32）+ 汉字 12/16/24/32 号字模（`tfont12/16/24/32` 结构体数组，每项含内码 `Index[2]` 与字模 `Msk[]`）；`pic.h` 是图片数据数组（RGB565）。两者都是"查表显示"的数据源，不包含逻辑。

---

## 8. LSM303DLH —— 加速度计 + 磁力计（`LSM303.c` / `LSM303.h`）

LSM303DLH 片内有两个器件：加速度计（从地址 0x19）与磁力计+温度（0x1E），共用 PB13/PB14 的 `LSM303_bus`。**注意该芯片不支持连续读**（实测不可靠），所以多字节读取用"单字节循环读"实现（文件头注释有明确说明）。

- **`LSM303_ReadOneReg` / `LSM303_WriteOneReg`**：按寄存器地址自动选择从机——`RegAddr > 0x19` 视为加速度计，否则视为磁力计（两器件寄存器地址段不同，以此区分）。
- **`LSM303_ReadMultiReg`**：连续读 N 个寄存器，内部退化为 N 次 `IIC_Read_One_Byte`（逐字节重发地址），规避芯片不支持突发读的问题。
- **`LSM303_Temp_ReadOneReg`**：专门读磁力计侧的寄存器（温度计在 0x1E 器件内）。
- **`LSM303DLH_Init`**：初始化并带回读校验：加速度计写 CTRL_REG4_A=0x10（满量程 ±4g）→ CTRL_REG1_A=0x2F（低功耗、10Hz 输出）→ 回读 0x20 确认，不符则重试最多 3 次；磁力计写 CRA_REG_M=0x10（15Hz、关闭温度计）→ CRB_REG_M=0x80（±4.0Gauss、XY 灵敏度 450LSB/Gauss）→ MR_REG_M=0x00（连续转换模式）→ 回读 0x02 确认。任一校验失败返回 1。原理：写关键寄存器后回读比对，确保芯片真的接收了配置。
- **`LSM303DLH_Sleep` / `Wakeup`**：睡眠时磁力计写 MR=0x03（睡眠模式）、加速度计写 CTRL_REG1_A=0x0F（关 ODR 使能位）；唤醒写回 0x00/0x2F。
- **`LSM303_ReadAcceleration`**：读 6 字节（X_L X_H Y_L Y_H Z_L Z_H），高低字节拼接成 16 位有符号原始值；真实加速度 = 原始值/灵敏度(LSB/g)。
- **`LSM303_ReadMagnetic`**：读磁力输出寄存器（顺序 X_H X_L Z_H Z_L Y_H Y_L），按顺序拼出 X/Y/Z。真实磁场 = 原始值/灵敏度。
- **`LSM303_ReadTemperature`**：读 TEMP_OUT_H/L，拼接后右移 4 位，按约 8 LSB/℃ 换算（`temp/3+0.5` 为近似）。
- **`LSM303DLH_CalculationZAxisAngle` / `CalculationXAxisAngle`**：**表盘倾角计算**：先对三轴加速度归一化（除以矢量模长 A），再求某轴与重力方向夹角：Z 轴角度 = 90° − atan(√(fx²+fy²)/fz)，X 轴角度类似（向上为正/向下为负），返回值×100 提高精度。原理：静止时加速度计测的就是重力矢量，其方向即手表姿态；翻腕检测（X 轴倾角）依赖此函数。
- **`Azimuth_Calculate`**：**电子罗盘航向角**：先由加速度算 pitch/roll，再把磁力计三轴投影到水平面（倾斜补偿：Hy=Ym·cos(roll)+Xm·sin(roll)sin(pitch)−Zm·cos(pitch)sin(roll)，Hx=Xm·cos(pitch)+Zm·sin(pitch)），最后 `atan2(Hy,Hx)` 得方位角。原理：不补偿倾斜时，磁场矢量含重力分量，表不平时方位会偏；补偿后才是水平面内的地磁北向角。

---

## 9. MPU6050 —— 六轴传感器

### `mpu6050.c` —— 项目精简驱动
MPU6050 内含三轴陀螺仪+三轴加速度计，I2C 地址 0x68，挂 PB13/PB14（`MPU_bus`），INT 脚 PB12。

- **`MPU_INT_Pin_Init`**：PB12 配下降沿中断（上拉），开 EXTI15_10，用于运动检测/数据就绪唤醒。
- **`MPU_Motion_Init`**：配置运动检测：加速度阈值寄存器=0x01、时长=0x01（阈值×2mg、时长 ms 级）、中断脚低电平有效（0x90）、开中断源（0x40）。这是"手腕抬起亮屏"类功能的硬件触发源。
- **`MPU_Bus_Init`**：I2C 初始化。
- **`MPU_Init`**：完整初始化流程：软复位（PWR_MGMT1=0x80）→ 延时 100ms → 唤醒（0x00）→ 陀螺仪 ±2000dps → 加速度 ±8g → 采样率 50Hz → 关中断/关 I2C 主机模式/关 FIFO → 中断低有效 → 读 0x75 校验 ID==0x68 → 进入低功耗（PWR_MGMT1=0x28：内部 8MHz、cycle 模式、关温度；PWR_MGMT2=0x87：只开加速度计、关陀螺仪、唤醒频率 20Hz）→ 重设 50Hz → 配运动检测 → 配 INT 脚。校验失败返回 1。
- **`MPU_Sleep` / `MPU_Wakeup`**：PWR_MGMT1 写 0x48（sleep=1）/ 0x28（低功耗 cycle 模式）。
- **`MPU_Read_Status`**：读中断状态寄存器 0x3A，供应用查询触发原因。
- **`MPU_Set_Gyro_Fsr` / `MPU_Set_Accel_Fsr`**：量程配置：`fsr<<3` 写入配置寄存器（fsr=0~3 对应 ±250/500/1000/2000dps 与 ±2/4/8/16g）。
- **`MPU_Set_LPF`**：数字低通滤波：按期望频率选档（≥188→1 … ≥10→5，否则 6），写入 CONFIG 寄存器。原理：低通滤掉高频振动噪声，保留人体运动频段。
- **`MPU_Set_Rate`**：采样率 = 1000/(分频+1)（SMPLRT 寄存器），并自动把 LPF 设为 rate/2（采样定理）。
- **`MPU_Get_Temperature`**：读温度寄存器，`36.53 + raw/340`（MPU6050 的换算公式），返回×100。
- **`MPU_Get_Gyroscope` / `MPU_Get_Accelerometer`**：连续读 6 字节，拼出 X/Y/Z 三轴 16 位原始值。
- **`MPU_Write_Len` / `MPU_Write_Byte` / `MPU_Read_Byte` / `MPU_Read_Len`**：I2C 寄存器级读写封装（起始/地址/ACK/数据/停止），与 `iic_hal` 的时序原语一致，专为 MPU 器件封装。
- **`MPU_Write_Multi_Byte` / `MPU_Read_Multi_Byte`**：复用通用 IIC 多字节读写（供 eMPL 库及上层 FIFO 操作调用）。
- **`MPU_Get_Angles`**：由加速度计算 roll/pitch：pitch = −atan(ax/√(ay²+az²))，roll = atan(ay/az)。只依赖重力矢量，无需陀螺积分，静止时准确。
- **`MPU_isHorizontal`**：调用 `MPU_Get_Angles`，若 |roll|、|pitch| 均 ≤0.5° 判定为水平（用于找平基准）。

### `eMPL/` —— InvenSense 官方运动库
`inv_mpu.c` + `inv_mpu_dmp_motion_driver.c` 是官方 eMPL（Embedded Motion Library）精简移植版，实现 DMP（Digital Motion Processor，MPU 内部硬件姿态协处理器）的固件加载与数据读出。项目在文件尾部追加了 `mpu_dmp_init` 与 `mpu_dmp_get_data` 两个入口，把 DMP 输出的四元数换算成 pitch/roll/yaw 供 UI 使用。

`inv_mpu.c` 主要函数：
- **`mpu_init`**：底层初始化——时钟选择、电源管理、量程、采样率、FIFO 复位等，是库的入口。
- **`mpu_set_sensors` / `mpu_get_power_state`**：按位使能/关闭陀螺、加速度、温度等传感器电源。
- **`mpu_set_gyro_fsr` / `mpu_set_accel_fsr` / `mpu_set_lpf` / `mpu_set_sample_rate`**：量程、低通、采样率配置（对应寄存器直写）。
- **`mpu_configure_fifo` / `mpu_get_fifo_config` / `mpu_read_fifo`**：配置 FIFO 采集哪些传感器、读取 FIFO 并校验，返回陀螺/加速度/时间戳。DMP 数据也经由 FIFO 流出。
- **`mpu_get_gyro_reg` / `mpu_get_accel_reg` / `mpu_get_temperature`**：直接读寄存器原始值（不走 FIFO）。
- **`mpu_run_self_test` / `gyro_self_test` / `accel_self_test` / `get_st_biases`**：上电自检——写入自检使能寄存器触发片内自检，比较自检前后偏差得到零偏，返回三轴零偏用于校准。
- **`mpu_write_mem` / `mpu_read_mem` / `mpu_load_firmware`**：通过 MPU 的 I2C 寄存器接口读写其内部 1KB RAM（bank/address 分页），用于烧录 DMP 固件。
- **`mpu_set_dmp_state` / `mpu_get_dmp_state`**：DMP 使能开关（USER_CTRL 的 FIFO_RESET/DMP_EN 位）。
- **`mpu_lp_motion_interrupt`**：低功耗运动中断配置（阈值+时长），睡眠时靠它唤醒。
- **`mpu_set_bypass` / `mpu_set_int_level` / `mpu_set_int_latched`**：I2C 旁路（直通外部传感器）、中断极性、锁存模式。
- **`mpu_dmp_init`（项目添加）**：加载 DMP 固件 → 配方向、FIFO 速率 → 使能四元数输出 → 设置采样率与中断。
- **`mpu_dmp_get_data`（项目添加）**：从 FIFO 读四元数 q0~q3，用标准四元数→欧拉角公式转出 pitch/roll/yaw（返回 ×10 或 ×100 的定点值）。

`inv_mpu_dmp_motion_driver.c` 主要函数：
- **`dmp_load_motion_driver_firmware`**：把内嵌的 DMP 固件字节数组经 `mpu_load_firmware` 写入 MPU 内部 RAM（DMP 是 MPU 内部运行固件的小处理器）。
- **`dmp_set_orientation`**：写方向矩阵到 DMP 内存，告诉固件芯片在板上的安装朝向。
- **`dmp_set_gyro_bias` / `dmp_set_accel_bias`**：把自检得到的零偏写入 DMP。
- **`dmp_set_fifo_rate` / `dmp_get_fifo_rate`**：DMP 输出频率。
- **`dmp_enable_feature` / `dmp_get_enabled_features`**：按位使能 DMP 功能：四元数、6 轴低功耗四元数、手势、计步、tap 等（如 `DMP_FEATURE_6X_LP_QUAT` 用于省电姿态）。
- **`dmp_read_fifo`**：读 DMP 输出：四元数（原始 32 位定标）、陀螺、加速度、手势码，按使能的功能拆包。
- **`dmp_set_tap_thresh/axes/count/time`**：轻敲检测阈值、轴、次数、时间窗（敲击手势）。
- **`dmp_set_shake_reject_thresh/time/timeout`**：晃动抑制（防止走路时误触发）。
- **`dmp_get/set_pedometer_step_count` / `dmp_get/set_pedometer_walk_time`**：计步器（硬件计步，省电）。
- **`dmp_set_interrupt_mode` / `decode_gesture`**：中断模式与手势解码。
- **`dmp_register_tap_cb` / `dmp_register_android_orient_cb`**：注册手势/方向变化的回调。

`dmpKey.h` / `dmpmap.h`：DMP 固件使用的内存映射宏（各功能在 DMP RAM 中的地址偏移），库内部使用，不直接调用。

---

## 10. OWDG —— 外部看门狗（`WDOG.c` / `WDOG.h`）

板上外部看门狗芯片（低电平使能、脉冲喂狗），PB1 为使能脚、PB2 为喂狗脚。

- **`WDOG_Port_Init`**：PB1/PB2 配推挽输出（PB1 低速、PB2 高速）。
- **`WDOG_Enable`**：PB1 拉低——使能看门狗（低有效）。
- **`WDOG_Disnable`**：PB1 拉高——禁用（调试/OTA 时用）。
- **`WDOG_Feed`**：PB2 翻转（Toggle）产生脉冲喂狗。原理：外部看门狗要求在超时窗口内收到脉冲沿，否则复位整板；喂狗任务周期调用此函数即可防复位。注意它没有喂狗超时检查（纯硬件机制），喂狗是否及时取决于调用方调度。

---

## 11. POWER —— 电源与电量（`power.c` / `power.h`）

- **`Power_Pins_Init`**：PA3（电源使能）配推挽输出并默认拉低（关机态）；PA2（充电检测）配上升/下降沿中断（EXTI2）——充电器插入/拔出产生中断唤醒系统。
- **`Power_Enable` / `Power_DisEnable`**：PA3 置高/置低，控制外部 MOSFET 开关，给外设（屏、传感器等）上电/断电。**断电后总线上的器件被隔离，也是低功耗的关键动作**。
- **`ChargeCheck`**：读 PA2 电平，1 表示正在充电。
- **`BatCheck`**：单次 ADC 采样：`HAL_ADC_Start` → `PollForConversion` → `GetValue` → `Stop`，`电压 = ADC值 × 2 × 3.3 / 4096`。×2 是分压电阻的倍率（电池经 1:1 分压进 ADC），3.3/4096 是 12 位 ADC 的满量程换算。
- **`BatCheck_8times`**：连采 8 次求和右移 3 位取平均再换算，消除采样噪声与电源纹波。
- **`PowerCalculate`**：把电压映射成电量百分比：4.2V→100%、4.06→90%、3.98→80%、3.92→70%、3.87→60%、3.82→50%、3.79→40%、3.77→30%、3.74→20%、3.68→10%、3.45→5%（分段查表，近似锂电放电曲线）。充电时先扣减内阻压降 `0.128V × 1A`（INTERNAL_RES × CHARGING_CUR），避免充电电压虚高导致电量误判。
- **`Power_Init`**：引脚初始化 + 上电。

---

## 12. SPL06-001 —— 气压计（`SPL06_001.c` / `SPL06_001.h`）

SPL06-001 是高精度气压/温度传感器（I2C 地址 0x76，ID=0x10），挂 PB13/PB14（`SPL_bus`）。

- **`SPL_ReadOneReg` / `SPL_WriteOneReg`**：寄存器读写封装。
- **`Get_Traw` / `Get_Praw`**：读 3 字节（B2/B1/B0）拼成 **24 位有符号补码**原始值（`Traw |= 0xFF000000` 做符号扩展）。
- **`get_c0` … `get_c30`（9 个函数）**：从校准系数寄存器区拼出各系数——c0/c1 是 12 位、c00/c10 是 20 位、其余 16 位，均做符号扩展。原理：SPL06 每片芯片出厂写入不同的校准系数，温度/压力补偿公式必须用这些系数，否则误差以 kPa 计。
- **`SPL_init`**：配置压力 2 倍过采样（PRS_CFG=0x01）、外部温度 1 倍（TMP_CFG=0x80）、连续测量模式（MEAS_CFG=0x07）→ 读出全部 9 个校准系数 → 回读 PRS_CFG/CFG_REG 校验，不一致返回 1。
- **`SPL_Sleep` / `SPL_Wakeup`**：MEAS_CFG 写 0x00（待机，省电）/ 0x07（连续测量）。
- **`SPL_GetID`**：读 0x0D（应为 0x10）。
- **`Temperature_Calculate`**：`Tcomp = c0×0.5 + c1×(Traw/KT)`，KT=524288（数据手册缩放常数）。
- **`Pressure_Calculate`**：按数据手册补偿公式：`Pcomp = c00 + Psc×(c10+Psc×(c20+Psc×c30)) + Tsc×c01 + Tsc×Psc×(c11+Psc×c21)`，其中 `Psc=Praw/KP`（KP=1572864）、`Tsc=Traw/KT`。原理：这是厂商给出的二阶多项式拟合，把原始 ADC 码修正为真实气压，温度交叉项补偿温度对气压测量的影响。
- **`Altitude_Calculate`**：**气压高度公式** `44330 × (1 − (P/101325)^0.1903)`（国际标准大气模型），用于楼层检测/海拔显示。

---

## 13. TOUCH —— 电容触摸屏 CST816（`CST816.c` / `CST816.h`）

CST816 是单点电容触摸芯片（I2C 地址 0x15），SDA=PB4、SCL=PB6，复位脚 PA15。

- **`CST816_GPIO_Init`**：复位脚（PA15）配推挽输出并置高；I2C 引脚初始化。
- **`CST816_Init`**：GPIO 初始化 + 配置自动睡眠时间 5 秒（无触摸 5s 后芯片自动低功耗）。
- **`CST816_IIC_ReadREG` / `CST816_IIC_WriteREG`**：寄存器读写（复用 IIC 单字节读写，地址 0x15）。
- **`CST816_RESET`**：硬件复位：RST 拉低 10ms → 拉高 100ms。触摸芯片初始化/唤醒都要走它。
- **`CST816_Get_XY_AXIS`**：连续读 4 字节（XposH/XposL/YposH/YposL），12 位坐标：`((H&0x0F)<<8)|L`（高位字节只用低 4 位），结果存入全局结构体 `CST816_Instance`。
- **`CST816_Get_FingerNum`**：读 0x02 触摸点数（0xFF 表示芯片睡眠）。
- **`CST816_Get_ChipID`**：读 0xA7，正常为 0xB5（可校验芯片在位）。
- **`CST816_Config_MotionMask`**：写 0xEC，使能连续左右/上下滑动、双击等连续动作（会增大响应时间）。
- **`CST816_Config_AutoSleepTime`**：写 0xF9 设置无触摸自动睡眠秒数。
- **`CST816_Sleep`**：写 0xE5=0x03 进入睡眠（无触摸唤醒，只能外部复位唤醒）。
- **`CST816_Wakeup`**：调用 `CST816_RESET` 硬件复位唤醒。
- **`CST816_Config_MotionSlAngle` / `NorScanPer` / `IrqPluseWidth` / `LpScanTH`**：分别配置手势滑动分区角度（0xEF）、正常扫描周期（0xEE，单位 10ms，限幅 1~30）、中断脉冲宽度（0xED，限幅 <200）、低功耗扫描唤醒门限（0xF5，越小越灵敏）。

---

## 14. 补充说明与注意事项

1. **共用总线**：AHT21/EM7028/LSM303/MPU6050/SPL06 全部挂在 PB13/PB14，`iic_hal` 的互斥锁是它们能共存的关键；任何模块新增 I2C 操作都应走 `IICStart/IICStop` 包裹的事务，否则可能与其他任务抢线。
2. **软件 I2C 时序**：各模块的 `delay_us` 决定时钟频率，未做总线速率归一化，改动延时会影响所有共用器件。
3. **已知小瑕疵**（整理时发现，不影响当前功能）：`Hr_Ave_Filter` 未 return 平均值；`AHT_Init` 中 `if(AHT_Read_Status()&&0x08!=0x08)` 的写法恒为假（位与优先级问题），实际校准判断依赖 `AHT_Read_Cal_Enable` 的逻辑；`PowerCalculate` 电压低于 3.45V 时 `power` 未初始化（返回随机值）。
4. **lcd.c 中的 `OFFSET_Y=20`**：所有绘图函数会自动加上 20 行的纵向偏移，对应表盘上方（摄像头/传感器开孔区）预留区，坐标换算在驱动层完成。
5. **eMPL 库为第三方源码**：`inv_mpu.c`/`inv_mpu_dmp_motion_driver.c` 来自 InvenSense，项目只在其尾部追加了 `mpu_dmp_init`/`mpu_dmp_get_data` 两个入口函数；DMP 固件二进制嵌入在 `inv_mpu_dmp_motion_driver.c` 的数组中。
