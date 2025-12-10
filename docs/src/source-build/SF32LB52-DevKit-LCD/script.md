---
title: 使用脚本编译
---

使用 ENV 工具进行编译比较繁琐，在 SiFli-SDK 2.4.0 版本之后，我们提供了一种更简单的方式来进行编译，并且对于 Linux 和 macOS 以及 Windows 都适用。

## 准备工作

### 克隆源码

该步骤的前置准备是你安装了 git 工具，如何安装不多赘述。

目前源码托管在 GitHub 上，仓库链接为：<https://github.com/78/xiaozhi-sf32>

::: details 使用 gitee 加速

如果在国内访问 GitHub 可能比较困难，那么可以尝试使用 gitee 镜像仓库：<https://gitee.com/SiFli/xiaozhi-sf32>。下述的所有 github 的链接都可以替换为 gitee 的链接。

但是需要注意的是，gitee 中的`子模块`存在一些问题，所以 clone 的时候不需要添加`--recursive`参数，但是需要 clone 下来之后修改仓库根目录下的`.gitmodules` 文件，将

```
[submodule "sdk"]
	path = sdk
	url = ../../OpenSiFli/SiFli-SDK.git
```

修改为

```
[submodule "sdk"]
    path = sdk
    url = https://gitee.com/SiFli/sifli-sdk.git
```

然后手动执行 submoudle 初始化

```bash
git submodule update --init --recursive
```

:::

使用以下命令克隆源码：

```bash
git clone --recursive https://github.com/78/xiaozhi-sf32.git
```

::: tip

需要注意的是，`--recursive` 参数是必须的，因为我们使用了子模块来管理依赖库。如果你忘记添加这个参数 clone 的话，请使用下面的命令重新拉取子模块 (submodule)：

```bash
git submodule update --init --recursive
```

:::

### 安装工具

`xiaozhi` 工程默认将 SiFli-SDK 作为一个子模块来引入。编译之前我们需要先安装 SiFli-SDK 工具，如果你之前已经安装过 SiFli-SDK 工具，可以跳过此步骤。

切换到 `sdk` 目录下，执行以下命令：

::: tabs#os

@tab windows

```powershell
cd sdk
.\install.ps1
```

@tab macOS/Linux

```bash
cd sdk
./install.sh
```

:::

::: tabs#os

::: tip

如果你遇到了网络等问题，或者不知道该怎么运行脚本的话，可以参考 [SiFli-SDK 安装文档](https://docs.sifli.com/projects/sdk/latest/sf32lb52x/quickstart/install/script/index.html)。

:::

### 设置环境变量

安装完成之后，我们还需要设置环境变量，同样切换到 `sdk` 目录下，执行以下命令：

::: tabs#os

@tab windows

```powershell
cd sdk
.\export.ps1
```

@tab macOS/Linux

```bash
cd sdk
. ./export.sh
```

:::

## 编译工程

我们的工程在`app/project`目录下，因此进入该目录下，执行以下命令：

```bash
cd app/project
scons --board=sf32lb52-lcd_n16r8 --board_search_path="../boards" -j8
```

需要注意的是，这一步必须要在设置环境变量完成之后才能进行。

::: tip
在 `scons` 命令中，`--board` 参数指定了编译的目标板子，`--board_search_path`参数指定了编译的外置板子的路径，`-j8` 参数指定了编译的线程数，可以根据自己的电脑性能进行调整。
:::

编译成功之后会出现以下提示：

![](image/2025-06-06-12-43-58.png)

## 下载程序

保持开发板与电脑的USB连接，运行以下脚本下载程序到开发板：

::: tabs#os

@tab windows

```powershell
.\build_sf32lb52-lcd_n16r8_hcpu\uart_download.bat
```

@tab macOS/Linux

```bash
./build_sf32lb52-lcd_n16r8_hcpu/uart_download.sh
```

:::
