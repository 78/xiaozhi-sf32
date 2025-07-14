---
title: MCP (Model Context Protocol)
icon: fa-solid fa-phone
---

## MCP 简介

经过之前的学习，我们已经成功能与小智进行交流了。但是如果只是和小智简单的聊聊天，那就太无聊了。我们需要让小智能够理解我们的意图，并且根据我们的意图来执行相应的操作。小智的背后是各种各样的语言大模型，这样的模型对于处理自然语言非常擅长，也能完成一些简单的数学运算，但是他们并不知道如何与硬件交互。为了解决这个问题，我们需要一个中间层来将自然语言转换为硬件操作指令，这就是 MCP（Model Context Protocol，模型上下文协议）的作用。

具体来说，例如我有一个需求是查询当前的天气，那么我需要做的就是根据其他信息获取目前城市的地理名称，然后将这个名称传入某个天气的 API 中，获取到天气信息后再将其返回给用户。但是不同的天气 API 调用方式截然不同，而这部分逻辑无法被预训练到模型中，因此我们需要一个中间层来处理这些逻辑。MCP 就是这样一个中间层，它可以将自然语言转换为某种对应的 API 调用。而这个 API 调用的具体逻辑就可以由我们自己来实现了。

那大模型在这个过程中扮演什么角色呢？它负责理解用户的自然语言输入，并将其转换为中间层可以处理的格式。通过这种方式，我们可以充分利用大模型的语言理解能力，同时又不失去对硬件操作的控制权。我们只需要告诉大模型我们的 MCP 的 API 有哪些，功能是什么，需要什么参数，大模型就可以根据这些信息来生成相应的 API 调用。

幸运的是，xiaozhi 现在也完全支持 MCP 服务，因此我们只需要经过简单的配置即可使用。不过 MCP 具体的技术原理不在本文档的讨论范围内，感兴趣的同学可以参考 [Model Context Protocol](https://modelcontextprotocol.io/introduction)

## 使用内置 MCP 服务

在 xiaozhi-sf32 项目中，MCP 默认被开启并且内置了一些常用的硬件控制接口：

- 音量
- 屏幕亮度
- LED 开关（仅限黄山派）

同时，我们除了设置以外也可以获取到他们的状态。

我们只需要对着小智说：

```text
小智，现在音量是多少？
```

即可获取到当前音量。同样的，如果我们对小智说：

```text
小智，把音量调到 10
```

小智就会将音量调到 10，可以通过前后声音的变化以及再次询问小智音量来确认是否成功。

## 自定义 MCP 服务

显然，这些内置的 MCP 服务并不能满足所有的需求。我们可能需要控制其他硬件，或者实现一些更复杂的逻辑。这时候，我们就需要自定义 MCP 服务。MCP 服务其实分为两种，一种是内置的，用于控制板上硬件的，我们先暂且称之为内部 MCP；另一种则是由小智的后台进行调用添加，因此我们称之为外部 MCP。

### 内部 MCP

内部 MCP 的添加很简单，我们只需要往`app/src/mcp/mcp_server.cc`中的`McpServer::AddCommonTools()`函数中调用`AddTool`方法即可添加新的 MCP 服务即可。

#### AddTool

AddTool 有两个重载，一个是

```cpp
void AddTool(const std::string& name, const std::string& description, const PropertyList& properties, std::function<ReturnValue(const PropertyList&)> callback);
```

另一个是

```cpp
void AddTool(McpTool* tool);
```

##### void AddTool(const std::string& name, const std::string& description, const PropertyList& properties, std::function<ReturnValue(const PropertyList&)> callback)

- `name`：MCP 服务的名称，用于大模型调用时的识别。类型是`std::string`。
- `description`：MCP 服务的描述，用于大模型了解这个服务的功能。类型是`std::string`。
- `properties`：MCP 服务的属性列表，用于定义这个服务的输入参数。类型是`PropertyList`。
- `callback`：MCP 服务的回调函数，用于处理服务的具体逻辑。类型是`std::function<ReturnValue(const PropertyList&)>`。

不过要理解每一个参数的类型定义可能需要一些 C++的基础知识。简单来说，`PropertyList`是一个类，用于定义输入的参数列表。如果我们这个工具不需要任何输入参数，例如用于获取音量的 MCP 服务，那么我们可以传入一个空的`PropertyList`，也就是`PropertyList()`。而如果我们希望这个工具需要一个整数类型的输入参数，那么我们可以这样定义：

```cpp
PropertyList(
    {
        Property("volume", kPropertyTypeInteger, 0, 15)
    }
)
```

`Property`拥有如下几种构造函数：

```cpp
Property(const std::string& name, PropertyType type);
```

```cpp
template<typename T>
Property(const std::string& name, PropertyType type, const T& default_value);
```

```cpp
Property(const std::string& name, PropertyType type, int min_value, int max_value)
```

```cpp
Property(const std::string& name, PropertyType type, int default_value, int min_value, int max_value)
```

其中的`template<typename T>`表示这个构造函数可以接受任意类型的默认值。`PropertyType`是一个枚举类型，可以是`kValueTypeBoolean` `kValueTypeNumber` `kValueTypeString`三种中的其中一种。

不同的重载用于定义不同的场景下的属性。比如如果我们需要一个布尔类型的输入参数，那么可以使用第一个构造函数；如果需要一个整数类型的输入参数，并且有默认值，那么可以使用第二个构造函数；如果需要一个整数类型的输入参数，并且有最小值和最大值，那么可以使用第三个或第四个构造函数。

`ReturnValue`是一个`std::variant<bool, int, std::string>`的类型别名，表示返回值可以是布尔值、整数或字符串。而`std::function<ReturnValue(const PropertyList&)>`则是一个函数类型，表示这个函数接受一个`PropertyList`类型的参数，并返回一个`ReturnValue`类型的结果，如果你学过 C 语言的话，可以简单地理解为这是一个 C++的高级版的函数指针。

现在我们终于可以来添加一个 MCP 服务了。我们现在创建一个计算器，用于计算两个整数的和。我们可以写出如下代码

```cpp
AddTool("self.mcp.calculate",
    "Calculate the sum of two numbers.",
    PropertyList({
        Property("a", kPropertyTypeInteger, 0, 100),
        Property("b", kPropertyTypeInteger, 0, 100)
    }),
    [](const PropertyList& properties) -> ReturnValue {
        int a = properties["a"].value<int>();
        int b = properties["b"].value<int>();
        return a + b;
    });
```

看到这里你可能又要问了，什么是`[]()`，这又是啥？这其实是 C++的 Lambda 表达式，是一种匿名函数的写法。它可以让我们在定义函数时直接写出函数体，而不需要单独定义一个函数，减少我们的代码量。

当然我们也可以用传统写法，单独定义一个函数

```cpp
ReturnValue CalculateSum(const PropertyList& properties) {
    int a = properties["a"].value<int>();
    int b = properties["b"].value<int>();
    return a + b;
}

AddTool("self.mcp.calculate",
    "Calculate the sum of two numbers.",
    PropertyList({
        Property("a", kPropertyTypeInteger, 0, 100),
        Property("b", kPropertyTypeInteger, 0, 100)
    }),
    CalculateSum);
```

这样也可以达到同样的效果。

接下来你就可以问小智

```text
小智小智，一加一的和是多少啊
```

然后就可以获得小智`等于二`的回答了（

当然，这只是一个非常简单的例子，实际应用中你可以增加其他的硬件控制，例如尝试控制 IO 或者 PWM 甚至是 IIC 之类的硬件外设。

##### void AddTool(McpTool* tool)

除了用上面的方法之外，我们也可以直接创建一个`McpTool`对象，然后将其传入`AddTool`方法中。这样可以让我们更灵活地定义 MCP 服务。

```cpp
McpTool tool("self.mcp.calculate",
    "Calculate the sum of two numbers.",
    PropertyList({
        Property("a", kPropertyTypeInteger, 0, 100),
        Property("b", kPropertyTypeInteger, 0, 100)
    }),
    [](const PropertyList& properties) -> ReturnValue {
        int a = properties["a"].value<int>();
        int b = properties["b"].value<int>();
        return a + b;
    });
AddTool(&tool);
```

### 外部 MCP

与内部 MCP 不同的是，外部 MCP 是由小智的后台进行调用添加的，因此我们不需要修改任何代码。我们只需要在小智的后台添加一个 MCP 服务即可，但是这也就意味着我们需要一个 mcp client 来与小智的后台进行通信。

在这里我们提供了两种创建 client 的示例

#### mcp-calculator

mcp-calculator 是虾哥基于 python 语言编写的一个 MCP 客户端示例。

##### 拉取仓库

我们首先将 mcp-calculator 仓库克隆到本地：

```bash
git clone https://github.com/78/mcp-calculator.git
```

##### 安装依赖

假设我们现在已经有了一个 python 环境，我们可以使用 pip 来安装依赖：

```bash
pip install -r requirements.txt
```

::: tip
必要情况下，可以创建一个虚拟环境来隔离依赖。
:::

##### 获取小智接入点

现在我们需要获取小智的接入点。进入 [小智后台](https://xiaozhi.me)，点击`控制台`进入设备后台。选择我们想要添加 MCP 服务的设备，然后点击`配置角色`

![](image/2025-07-14-14-48-42.png)

然后拉到页面底部，点击右下角的`MCP 接入点`按钮

![](image/2025-07-14-14-49-23.png)

复制`接入点地址`，留作备用

![](image/2025-07-14-14-50-15.png)

##### 设置环境变量

现在我们需要设置环境变量，让 mcp-calculator 知道小智的接入点地址。我们可以在终端中输入以下命令：

::: tabs

@tab Linux/macOS

```bash
export MCP_ENDPOINT="<接入点地址>"
```

@tab Windows

```bash
$env:MCP_ENDPOINT="<接入点地址>"
```

:::

接下来，我们就可以启动 mcp-calculator 了。在终端中输入以下命令：

```bash
python mcp_pipe.py calculator.py
```

如果没有意外的话，你可以观察到有类似如下的输出：

```log
(.venv) ➜  mcp-calculator git:(main) ✗ python mcp_pipe.py calculator.py
2025-07-14 15:03:18,684 - MCP_PIPE - INFO - Connecting to WebSocket server...
2025-07-14 15:03:18,913 - MCP_PIPE - INFO - Successfully connected to WebSocket server
2025-07-14 15:03:18,918 - MCP_PIPE - INFO - Started calculator.py process
Processing request of type ListToolsRequest
Processing request of type PingRequest
```

这个时候我们返回刚才的小智后台，点击刷新按钮，可以观察到接入点状态变成了在线，并且多了一个`calculator`的 MCP 服务。

![](image/2025-07-14-15-05-06.png)

这个时候，我们重启小智设备，询问

```text
小智，114 乘以 514 是多少
```

如果一切正常的话，小智会回答你`58676`。

::: note
电脑上运行 mcp-calculator 时需要保证一直运行，否则小智无法使用 MCP 服务。
:::

#### xiaozhi-client

::: details
下列章节中所暴露的所有 Token 等均为演示使用，均以全部失效化处理。请勿使用这些 Token 进行任何实际操作。
:::

上面的 mcp-calculator 对于想独立开发 MCP 客户端的用户来说，提供了一个很好的参考。然而，如果你只是想有更多的 MCP 服务，而不想自己编写客户端，那么我们可以尝试使用 `xiaozhi-client`。

xiaozhi-client 是一个基于 Node.js 的 MCP 客户端，它的一大优势就是可以调用其他现成的 MCP 服务，并通过小智的后台进行调用。这样一来，我们不需要编写任何代码即可享受到 MCP 的便利了。

##### 安装 Node.js

因为 xiaozhi-client 是基于 Node.js 的，所以我们需要先安装 Node.js。可以从 [Node.js 官网](https://nodejs.org/) 下载并安装。一般来说，我们建议安装 LTS 版本。

![](image/2025-07-14-15-10-50.png)

安装好之后，我们可以在终端中输入

```bash
node -v
```

检查 Node.js 是否安装成功。如果输出了版本号，那么恭喜你，安装成功了。

```log
v22.17.0
```

##### 安装 pnpm

pnpm 是一个速度快、节省磁盘空间的软件包管理器。我们可以使用以下命令来安装 pnpm：

```bash
npm install -g pnpm@latest-10
```

也可以参考 [pnpm 官网](https://pnpm.io/zh/installation) 的描述自行选择其他安装方式。

##### 创建工程

现在我们可以创建一个新的工程来使用 xiaozhi-client。我们可以在终端中输入以下命令：

```bash
npm i -g xiaozhi-client

## 创建项目
xiaozhi create my-app --template hello-world

## 进入项目
cd my-app

## 安装依赖（主要是示例代码中 mcp 服务所使用的依赖）
pnpm install
```

##### 获取小智接入点

现在我们需要获取小智的接入点。进入 [小智后台](https://xiaozhi.me)，点击`控制台`进入设备后台。选择我们想要添加 MCP 服务的设备，然后点击`配置角色`

![](image/2025-07-14-14-48-42.png)

然后拉到页面底部，点击右下角的`MCP 接入点`按钮

![](image/2025-07-14-14-49-23.png)

复制`接入点地址`，留作备用

![](image/2025-07-14-14-50-15.png)

##### 修改配置文件

将项目根目录下的`xiaozhi.config.json`中的`mcpEndpoint`字段修改为我们刚才复制的接入点地址。

##### 启动 MCP 服务

现在我们可以启动 MCP 服务了。在终端中输入以下命令：

```bash
xiaozhi start
```

然后返回小智后台，点击刷新按钮，可以观察到接入点状态变成了在线，并且多了若干服务

![](image/2025-07-14-15-17-43.png)

现在我们重启小智设备，询问对应的 MCP 服务即可。

##### 使用 ModelScope 服务

现在的工具依然是在工程中自行编写代码，但是 xiaozhi-client 可以使用 ModelScope MCP 服务集成来调用其他现成的 MCP 服务。

###### 获取 ModelScope Token

首先，我们需要去 [ModelScope 官网](https://www.modelscope.cn/) 注册一个账号，这一步不多赘述。

接下来，我们需要生成一个 Token。登录 ModelScope 后，点击右上角的头像，选择`账号设置`，然后在左侧菜单中选择`访问令牌`，新建一个或者使用默认的 Token。

![](image/2025-07-14-15-23-22.png)

::: warning
请妥善保管好这个 Token，因为它可以用于访问你的 ModelScope 账号，请不要分享给他人。
:::

接下来，我们在配置文件的`mcpServers`字段中添加一个`modelscope`字段，用于存储我们的 ModelScope Token：

```json
{
  "modelscope": {
    "apiKey": "你的 API Token"
  }
}
```

###### 选择服务

我们可以去<https://www.modelscope.cn/mcp>查找有哪些现成的 MCP 服务可以使用。在这里我们使用 [12306 查询](https://www.modelscope.cn/mcp/servers/@Joooook/12306-mcp) 进行演示。

我们点击网页上右侧的`通过 SSE URL 连接服务`里面的`连接`按钮，可以选择有效期是 24 小时还是永久。

![](image/2025-07-14-15-38-53.png)

这样会自动生成一个包含`mcpServers`字段的 json 文件，我们将里面的内容复制到配置文件中。

```json
{
  "mcpServers": {
    "calculator": {
      "command": "node",
      "args": [
        "./mcpServers/calculator.js"
      ]
    },
    "datetime": {
      "command": "node",
      "args": [
        "./mcpServers/datetime.js"
      ]
    },
    "12306-mcp": {
      "type": "sse",
      "url": "https://mcp.api-inference.modelscope.net/8ec984b2344641/sse"
    }
  }
}
```

现在，整个`xiaozhi.config.json`中的内容看起来是这样的：

```json
{
  "mcpEndpoint": "wss://api.xiaozhi.me/mcp/?token=eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjM5NjU1OCwiYWdlbnRJZCI6NDcxNTE2LCJlbmRwb2ludElkIjoiYWdlbnRfNDcxNTE2IiwicHVycG9zZSI6Im1jcC1lbmRwb2ludCIsImlhdCI6MTc1MjQ3NTc4NX0.zpT8WOhYUEgzwkDgZEse0ZbOt6h_gx7i6Bk2PlttW3W268jqTzIqDUOeMzU4hi7XPaW3sPbi-COP2MItiRD-8A",
  "mcpServers": {
    "calculator": {
      "command": "node",
      "args": [
        "./mcpServers/calculator.js"
      ]
    },
    "datetime": {
      "command": "node",
      "args": [
        "./mcpServers/datetime.js"
      ]
    },
    "12306-mcp": {
      "type": "sse",
      "url": "https://mcp.api-inference.modelscope.net/9c316b7cc7c340/sse"
    }
  },
  "modelscope": {
    "apiKey": "748b5011-6faf-4973-b9e3-019b1b66588c"
  },
  "connection": {
    "heartbeatInterval": 30000,
    "heartbeatTimeout": 10000,
    "reconnectInterval": 5000
  },
  "webUI": {
    "port": 9999
  },
  "mcpServerConfig": {
    "calculator": {
      "tools": {
        "calculator": {
          "description": "For mathematical calculation, always use this tool to calculate the result of a JavaScript expression. Math object and basic operations are available.",
          "enable": true
        }
      }
    },
    "datetime": {
      "tools": {
        "get_current_time": {
          "description": "Get the current time in various formats",
          "enable": true
        },
        "get_current_date": {
          "description": "Get the current date in various formats",
          "enable": true
        },
        "format_datetime": {
          "description": "Format a given date/time string or timestamp into specified format",
          "enable": true
        },
        "add_time": {
          "description": "Add or subtract time from a given date/time",
          "enable": true
        }
      }
    }
  }
}
```

重新运行`xiaozhi start`命令即可。回到小智后台，点击刷新按钮，可以观察到接入点状态变成了在线，并且多了若干`12306-mcp`的 MCP 服务。

![](image/2025-07-14-15-41-58.png)

---

其余关于`xiaozhi-client`的使用方法可以参考 [xiaozhi-client 仓库](https://github.com/shenjingnan/xiaozhi-client)。

