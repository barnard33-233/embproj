# embproj

## 简介

这是一个 stm32f4xx 的嵌入式工程项目。使用的软件为 Keil 5。是嵌入式的结业实验。

## 大纲

### 功能设计

输入: 按键

输出: 数码管 (显示速度数值) 蜂鸣器 (可变速的放歌)

功能:

- 平时会在数码管显示当前速度和暂停状况
- 按下 A 键暂停，B 键播放
- 按下 \# 进入 receive 模式，接收数字，再次按下 \# 提交并变速。receive 模式下接收到的数值会显示在数码管
- receive 模式下按下非数字且非 \# 键会取消当前的输入
- 按下 \* 会在串口输出当前状态的调试信息

### 安全设计

#### 冷启动与热启动

#### 看门狗

#### 输入滤波与配置刷新

#### 输出刷新与配置刷新

#### 数据单元备份与校验

#### 睡眠躲避干扰

#### 随机延时与随机功耗

#### 随机乱序执行

#### 前序代码执行检查
