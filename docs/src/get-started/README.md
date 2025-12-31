---
title: 快速入门
icon: lightbulb
---

这里是xiaozhi-sf32的快速入门指南。
## 硬件支持列表

- [SF32LB52-DevKit-ULP（黄山派）](SF32LB52-DevKit-ULP/README.md)
- [SF32LB52-DevKit-LCD开发板](SF32LB52-DevKit-LCD/README.md)
- [SF32LB52-DevKit-Nano开发板](SF32LB52-DevKit-Nano/README.md)
- [小汤圆直插版（立创训练营）](SF32LB52-XTY-AI-THT/README.md)



## 固件下载及烧录

固件烧录支持 ui界面烧录 以及 命令行烧录，这里更加推荐ui界面烧录，烧录简单易上手
* 需要注意的是，1.4.0以及之后的固件版本暂不支持终端命令烧录，可暂时先使用ui界面烧录，后续会恢复支持

- [使用ui界面烧录固件](./sftool_gui.md)
- [使用终端命令烧录固件](./sftool_cmd.md)

## 蓝牙使用注意事项

在连接板子设备之前，请打开手机的蓝牙网络共享功能！！！

### Android蓝牙使用注意事项

以下是Android手机的蓝牙设置界面，通过打开个人热点共享中的蓝牙共享网络功能。

![](image/2025-05-14-17-41-19.png) 
![](image/2025-05-14-17-41-29.png)
![](image/2025-05-14-17-41-37.png)


### iOS蓝牙使用注意事项

iOS同样需要打开蓝牙共享网络功能，以下是参考步骤

![](image/2025-05-14-17-45-34.png)
![](image/2025-05-14-17-45-39.png)
![](image/2025-05-14-17-45-45.png)

⚠ 注意 如果iOS在蓝牙列表未看见sifli-pan 设备，请尝试重启手机。

## 开始使用

正确烧录固件后，开发板初始化界面如下:

![](image/xiaozhi_ready.png){width=50%}

### 激活设备

烧录固件之后，确保蓝牙共享网络已打开，这时，手机就可以连接蓝牙 sifli-pan 设备了。 以下是Android手机连接状态示例图: 

![](image/2025-05-14-17-46-39.png){width=30%}

⚠ 注意：一般情况下，Android连接成功后，连接的蓝牙设备会显示正在向设备共享网络（iOS不会显示）,我们可以以此确定是否成功开启蓝牙网络共享

⚠  连接上sifli-pan设备后，开发板会有连接画面提示，此时按下对话按键（参考对应硬件支持查看对话按键），xiaozhi则会提示需要登录到控制面板，填设备码。

| ![](image/xiaozhi_ready.png){width=50%} | ![](image/xiaozhi_pan_connect.png){width=50%}  |
|-------------------------------|-------------------------------|

| ![](image/xiaozhi_connect.png){width=50%} | ![](image/control.png){width=50%}|
|-------------------------------|-------------------------------|

⚠  这个时候，打开浏览器，输入网址：<https://xiaozhi.me>。浏览器用手机或者电脑都可以。 进入小智 AI 的网页后，点击控制台，用手机号登录。

新建智能体填写，最后添加设备码。

可以这个时候拔掉开发板上的数据线再接入就可以正常使用了。

![](image/2025-05-14-17-49-06.png)
![](image/2025-05-14-17-49-12.png)
![](image/2025-05-14-17-49-18.png)
![](image/2025-05-14-17-49-24.png)

## 界面提示含义
### 出现下方UI提示均是pan断开的情况

| ![](image/no_pan.png){width=50%} | ![](image/pan_disconnect.png){width=50%} | ![](image/no_pan3.png){width=50%} |
| --- | --- | --- |
### 异常情况：
1. 对应手机的显示可能是未打开蓝牙共享直接连接sifli-pan设备
2. 蓝牙共享网络关闭
3. 蓝牙已断开

解决方案：打开蓝牙共享网络重新连接设备

下图为蓝牙连接成功但未开启蓝牙共享网络：

![](image/2025-05-14-17-50-33.png){width=30%}

## 唤醒 & 重连

### 唤醒

长时间未对话小智会进入休眠，此时需要按下唤醒键进行唤醒（参考对应硬件支持查看唤醒键）

![](image/sleep.png){width=30%}

### 重连

支持重连操作：若无主动删除手机匹配列表下的sifli-pan设备，当按下唤醒键也可进行蓝牙重连（参考对应硬件支持查看唤醒键）

| ![](image/pan_rec.png){width=30%} | ![](image/pan_rec_sucf.png){width=30%}

## 电池曲线
### 获取电池曲线
程序中默认提供的曲线表(位于各板子目录下，以黄山派为例：boards/sf32lb52-lchspi-ulp_base/battery_table.c)可能与您实际使用的电池不匹配，从而导致电量显示不准确。为确保电量显示的准确性，我们推荐您使用官方默认电池：

**购买链接**: [淘宝官方旗舰店 - SiFli官方同款电池](https://item.taobao.com/item.htm?abbucket=12&id=938718221597&mi_id=0000tb_9vrJ-SsxMUIsW-1kfO28IuJD11JqF__CKtcmsCTQ&ns=1&skuId=5834126861696&spm=a21n57.1.hoverItem.6&utparam=%7B%22aplus_abtest%22%3A%22fb56882eb25a9781979c75e66efb6a72%22%7D&xxc=taobaoSearch)

或者如果您使用的是第三方电池或电池容量与官方电池不同，为保证电量显示的准确性，您需要获取相应的电池曲线：
1. **联系电池供应商**: 向电池商家索取该型号电池的放电/充电曲线数据
2. **自行测试获取曲线**: 若具备相关条件，可自行测试并生成对应的曲线表

### 替换曲线表
获取到合适的电池曲线后，请按以下步骤替换默认曲线表：

1. 找到电池配置文件 `battery_table.c`(位于板子目录下)
2. 替换 `discharge_curve_table` 和 `charging_curve_table` 数组
3. 确保电压值按从高到低顺序排列
4. 更新表大小参数
5. 重新编译并烧录固件

```c
// 替换为新的曲线表
const battery_lookup_point_t charging_curve_table[] ={
    // 从供应商获取的放电曲线数据
    {100, 41998},
    {99, 41864},
    // ...其他数据点
    {0, 35000}
};

const battery_lookup_point_t discharge_curve_table[] ={
    // 从供应商获取的放电曲线数据
    {100, 41998},
    {99, 41864},
    // ...其他数据点
    {0, 35000}
};
```

更多信息可以参考：
(https://docs.sifli.com/projects/sdk/latest/sf32lb52x/middleware/battery_calculator.html)

