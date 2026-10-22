# 温度传感器实践作业 —— 程序流程图（Mermaid / ProcessOn）

> 用法：在 ProcessOn 中新建图形 → 顶部「插入」→「Mermaid」→ 把下面任意一个 ```mermaid 代码块内的内容整段粘贴进去 → 确定。
> 如果 ProcessOn 提示语法错误，把第一行的 `flowchart TD` 改成 `graph TD` 再试（老版本 mermaid 只认 graph）。

---

## 图 1　51 单片机主程序流程（main 函数）

对应报告「5.软件设计 →（1）51 单片机程序」的主流程段与源文件 `Listings\main.c`。

```mermaid
flowchart TD
    A["开始"] --> B["上电延时 delay_20ms"]
    B --> C["读取一次温度 ReadTemperature<br/>丢弃上电默认值 +85℃"]
    C --> D["初始化 LCD1602 Init_lcd"]
    D --> E["显示标题 T Monitor"]
    E --> F["进入主循环 while 1"]
    F --> G["读取温度 ReadTemperature"]
    G --> H["温度换算 covert1<br/>补码转整数并拆分显示位"]
    H --> I["LCD 显示温度值"]
    I --> J["电机控制 motor<br/>按温度输出 PWM 调速"]
    J --> F
```

---

## 图 2　DS18B20 温度读取子程序流程（ReadTemperature）

对应报告「DS18B20 驱动」段。注意温度转换与读取是**两次独立的总线访问**。

```mermaid
flowchart TD
    A["进入 ReadTemperature"] --> B["复位 Init_DS18B20<br/>DQ 拉低约 480μs 后释放<br/>检测器件 15~60μs 应答脉冲"]
    B --> C["写命令 0xCC 跳过 ROM"]
    C --> D["写命令 0x44 启动温度转换"]
    D --> E["延时等待转换完成"]
    E --> F["再次复位 Init_DS18B20"]
    F --> G["写命令 0xCC 跳过 ROM"]
    G --> H["写命令 0xBE 读暂存器"]
    H --> I["延时"]
    I --> J["按位读回温度低字节 t0"]
    J --> K["按位读回温度高字节 t1"]
    K --> L["返回"]
```

---

## 图 3　温度数据处理子程序流程（covert1）

对应报告「温度数据处理 covert1()」段。

```mermaid
flowchart TD
    A["进入 covert1"] --> B{"高字节 t1 大于 0x07 ?"}
    B -->|是| C["判定为负温度<br/>符号位填 - 即 0x2D"]
    C --> D["t1、t0 取反加一<br/>得到温度绝对值"]
    B -->|否| E["判定为正温度<br/>符号位填 + 即 0x2B"]
    D --> F["合并为整数部分<br/>t1 左移 4 位再或上 t0 的高 4 位"]
    E --> F
    F --> G["赋值给 temperature"]
    G --> H["拆分百位、十位、个位<br/>并转换为 ASCII 码"]
    H --> I["百位为 0 时消隐"]
    I --> J["写入显示缓冲区 TempBuffer1"]
    J --> K["返回"]
```

---

## 图 4　电机控制子程序流程（motor，PWM 调速）

对应报告「电机控制 motor(tmp)」段。正温度按 25℃／50℃ 分三档，负温度反向转动。

```mermaid
flowchart TD
    A["进入 motor<br/>参数 tmp 为整数温度值"] --> B{"符号位是 + ?"}
    B -->|是| C{"温度值低于 25 ?"}
    C -->|是| D["停机<br/>转向位 D=0，PWM 输出 0"]
    C -->|否| E{"温度值高于 50 ?"}
    E -->|是| F["全速运行<br/>转向位 D=0，PWM 恒为高电平"]
    E -->|否| G["PWM 调速<br/>高电平延时 5×tmp 个延时单位<br/>低电平延时 255-5×tmp 个延时单位<br/>占空比随温度线性增大"]
    B -->|否| H["反向运行<br/>转向位 D=1<br/>按温度输出反向 PWM"]
    D --> I["返回"]
    F --> I
    G --> I
    H --> I
```

---

## 图 5　STM32 主程序与主循环状态机

对应报告「（2）STM32 程序」的「主循环结构」段。三个状态循环切换，共同完成一次 750ms 量级的温度转换与读取。

```mermaid
flowchart TD
    A["开始"] --> B["关闭 JTAG 复用功能<br/>释放 PB3、PB4 为普通 IO"]
    B --> C["使能 GPIO 时钟<br/>初始化 LCD1602 与电机控制口"]
    C --> D["LCD 显示 T Monitor 与 Motor:Stop"]
    D --> E["预启动一次转换<br/>丢弃上电默认值 +85℃"]
    E --> F["主循环：判断 ds_state"]

    F -->|状态 0| G["启动温度转换<br/>ds_timer=0，ds_state=1"]
    F -->|状态 1| H["ds_timer 自增"]
    H --> I{"ds_timer 达到 10 ?"}
    I -->|否| J["延时 10ms"]
    I -->|是| K["ds_state=2"]
    K --> J
    F -->|状态 2| L["读取温度并按策略处理<br/>见下方图 6"]
    L --> M["ds_state=0"]
    M --> J
    G --> J
    J --> F
```

---

## 图 6　STM32 温度处理与电机回差控制（状态 2 展开）

对应报告「控制策略」与「故障处理」两段。图中 30.0℃／28.0℃ 之间的「保持原状态」即回差（滞环）区间。

```mermaid
flowchart TD
    A["状态 2：读取温度"] --> B["temp = DS18B20_ReadTempResult"]
    B --> C{"读回值等于 0x7FFF ?"}
    C -->|是| D["判定通信失败<br/>LCD 显示 Sensor Error<br/>电机停机 MOT_STOP"]
    C -->|否| E{"温度与上次相同 ?"}
    E -->|否| F["格式化并刷新 LCD 温度显示"]
    E -->|是| G["跳过刷新"]
    F --> H{"温度高于 30.0℃ ?"}
    G --> H
    H -->|是| I["MOT_FORWARD 启动电机"]
    H -->|否| J{"温度低于 28.0℃ ?"}
    J -->|是| K["MOT_STOP 停机"]
    J -->|否| L["保持原状态<br/>28~30℃ 回差区间"]
    I --> M["更新 LCD 电机状态显示"]
    K --> M
    L --> M
    M --> N["ds_state=0，返回主循环"]
    D --> N
```
