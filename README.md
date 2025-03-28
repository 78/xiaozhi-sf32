# 小智聊天机器人
本示例通过互联网连上小智机器人的服务器，实现了一个有语音和显示的聊天机器人，互联网底层基于蓝牙PAN协议。

使用办法：
1. 需要在手机上启用通过手机蓝牙上网的功能
2. 将板子开机后连上蓝牙（名称默认为sifli-pan）
3. 根据屏幕显示，连接小智正常后，通过按下按钮开始跟小智说话，说完后放开按键。
4. 添加新设备到管理后台：对话时会显示：“请登陆到控制面板添加设备，输入验证码“xxxxxx””
- 第一步：先创建智能体，确认提交后
- 第二步：添加新设备，输入提示语中告知6位数字的设备 ID，然后点击“添加设备”按钮，设备添加后将自动激活，并显示在“设备管理”列表页面上,之后重启设备开始对话
- 后台地址：https://xiaozhi.me

```{note}
- em-lb525板子测试通过。
```

## 工具链
参考[上手指南](https://docs.sifli.com/projects/sdk/v2.3/sf32lb52x/quickstart/get-started.html)安装工具链，本项目暂时只支持Keil 5.32编译，GCC编译还在调试中。

## 获取代码
因为仓库包含了子模块，请使用`--recursive`参数克隆仓库，参考如下命令：
```shell
git clone --recursive https://github.com/78/xiaozhi-sf2.git
```


## 编译和烧写
1. 打开env
1. `cd`到`sdk`目录，执行`set_env`设置编译环境
1. `cd`到`app\project`目录，执行`scons --board=em-lb525 -j8`编译项目代码
1. 编译生成的镜像文件保存在`build-em-lb525_hcpu`目录下，在编译的目录下执行`build_em-lb525_hcpu\uart_download.bat`烧写镜像文件，具体方法参考[上手指南](https://docs.sifli.com/projects/sdk/v2.3/sf32lb52x/quickstart/get-started.html)

