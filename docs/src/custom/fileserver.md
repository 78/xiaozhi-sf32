---
title: FileServer Docker部署
icon: fa-brands fa-docker
---

# FileServer Docker 部署教程

## 一、简介

本教程将指导您使用 Docker 部署 FileServer 讲解如何使用提供的 Docker 镜像和配置文件，完成小智OTA服务的部署、配置和管理。

## 二、所需文件

在开始之前，请确保您已获得以下文件：

| 文件名 | 作用 |
|--------|------|
| `fileserver.tar` | FileServer 主服务的 Docker 镜像文件 |
| `fileserver.yml` | FileServer 服务的 Docker Compose 配置文件 |


## 三、环境准备

### 1. 下载并安装 Docker

Docker 是用于部署容器化应用的工具，我们需要先安装它。

#### Windows 系统
1. 访问 [Docker 官方下载页面](https://www.docker.com/products/docker-desktop)
2. 下载并安装 Docker Desktop for Windows
3. 安装时勾选 "Install required Windows components for WSL 2"
4. 安装完成后，启动 Docker Desktop

#### macOS 系统
1. 访问 [Docker 官方下载页面](https://www.docker.com/products/docker-desktop)
2. 下载并安装 Docker Desktop for Mac
3. 安装完成后，启动 Docker Desktop

#### Ubuntu 系统
1. 打开终端，执行以下命令：
   ```bash
   sudo apt update
   sudo apt install apt-transport-https ca-certificates curl software-properties-common
   curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
   echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
   sudo apt update
   sudo apt install docker-ce docker-ce-cli containerd.io docker-compose-plugin
   sudo usermod -aG docker $USER
   ```
2. 重启终端或注销后重新登录

### 2. 验证 Docker 安装

打开终端（Windows 系统打开命令提示符或 PowerShell），执行以下命令：

```bash
docker --version
docker compose version
```

如果能看到版本信息，说明 Docker 安装成功。

## 四、文件说明

### 1. Docker 镜像文件

- **`fileserver.tar`**：这是 FileServer 主服务的 Docker 镜像，包含了 FileServer 运行所需的所有组件和依赖。

### 2. Docker Compose 配置文件

Docker Compose 是用于定义和运行多容器 Docker 应用的工具。配置文件使用 YAML 格式，定义了服务、网络、卷等配置。

#### `fileserver.yml` 结构说明

```yaml
services:  # 定义服务列表
  postgres:  # 数据库服务
    image: postgres:16-alpine  # 使用的镜像
    container_name: fs_postgres  # 容器名称
    ports:  # 端口映射（宿主机端口:容器端口）
        - "8432:5432"
    environment:  # 环境变量
        POSTGRES_USER: fileserver  # 数据库用户名
        POSTGRES_PASSWORD: fsAbc@123_  # 数据库密码
        POSTGRES_DB: fileserver  # 数据库名称
    volumes:  # 数据持久化（宿主机路径:容器路径）
        - ./database/postgresql:/var/lib/postgresql/data
    healthcheck:  # 健康检查配置
      test: ["CMD-SHELL", "pg_isready -U fileserver -d fileserver"]
      interval: 3s
      retries: 10
      start_period: 10s

  fileserver:  # FileServer 服务
    image: fileserver:latest  # 使用的镜像
    container_name: fileserver  # 容器名称
    ports:  # 端口映射
      - "8217:8000"
    volumes:  # 数据持久化
      - ./files:/app/files
    depends_on:  # 依赖关系
      postgres:
        condition: service_healthy  # 等 postgres 健康后再启动
    environment:  # 环境变量
        CLIENT_ID: Mac_Local  # 客户端标识
        DOMAIN: http://10.23.30.174  # 部署的 IP 地址
        PORT: 8217  # 部署的端口号
        # 其他环境变量...
```


## 五、部署步骤

### 1. 准备部署目录

创建一个用于部署的目录，将所有提供的文件放入该目录：

```bash
# 创建部署目录
mkdir -p fileserver-deploy
cd fileserver-deploy

# 将所有文件复制到该目录
# 假设文件在当前目录下
cp ../fileserver.tar ../fileserver.yml 
```

### 2. 修改配置文件

修改 `fileserver.yml` 配置文件：

1. 数据库服务配置主要需要修改：
```yaml
postgres: # 数据库
  ...
  ports:
        - "8432:5432" # 将宿主机的 8432 端口映射到容器的 5432 端口
  volumes: #在当前目录创建 database/postgresql 目录作为数据持久化目录（左边 = 宿主机路径，右边 = 容器内路径（固定格式））
    - ./database/postgresql:/var/lib/postgresql/data
```

2. 文件服务配置主要需要修改：
```yaml
fileserver: # 文件服务
  ...
  ports:
      - "8217:8000"
  volumes: # 在当前目录创建 files 目录作为数据持久化目录（左边 = 宿主机路径，右边 = 容器内路径（固定格式））
    - ./files:/app/files
# 修改客户端ID和域名，例如：
  ...
environment:
    CLIENT_ID: Mac_Local # 修改自己的clientID
    DOMAIN: http://10.23.30.174 # 修改为部署的ip地址
    PORT: 8217 # 修改为部署的端口

# **注意**：您可以通过 ifconfig en0/en1（Mac/Linux）或 ipconfig （Windows）查看本地IP地址。

# 将数据库地址改为 postgres （容器名称），因为在Docker Compose中，服务间可以通过容器名访问数据库服务

    DATABASE_URL: postgresql+asyncpg://fileserver:fsAbc%40123_@postgres:5432/fileserver
   # 即将10.23.30.174 改为 postgres，8432 改为 5432 （这是 PostgreSQL 容器内部的默认端口，而非宿主机映射的端口）
```

### 3. 导入 Docker 镜像

执行以下命令导入 Docker 镜像：

```bash
# 导入 FileServer 镜像
docker load -i fileserver.tar
```

### 4. 部署 FileServer 服务

执行以下命令部署 FileServer 服务：

```bash
# 启动 FileServer 服务
docker compose -f fileserver.yml up -d
```

### 5. 验证部署

执行以下命令检查服务状态：

```bash
# 检查 FileServer 服务状态
docker compose -f fileserver.yml ps
```

如果服务状态显示为 `Up`，说明部署成功。

**重要提示**：FileServer 部署后默认没有用户，您必须按照下一章节的步骤创建管理员用户，否则将无法登录文件浏览器。


## 六、用户管理

### 1. 检查是否有默认用户

FileServer 部署后默认没有用户，需要手动创建。您可以按照以下步骤检查数据库中是否已有用户：

```bash
# 1. 连接到数据库容器
docker exec -it fs_postgres psql -U fileserver -d fileserver

# 2. 在数据库命令行中执行以下命令，查看用户表
SELECT * FROM users;

# 3. 如果输出为空（显示 (0 rows)），说明没有用户，需要创建
# 4. 输入 \q 退出数据库命令行
\q
```

### 2. 创建管理员用户

如果没有默认用户，您需要手动创建一个管理员用户。以下是详细步骤：

#### 步骤 1：生成 Argon2 哈希密码

1. 创建一个 Python 脚本，用于生成安全的密码哈希：

   ```bash
   cat > generate_password.py << 'EOF'
   import argon2
   
   # 生成 argon2 哈希密码
   def generate_hash(password):
       hasher = argon2.PasswordHasher(
           time_cost=2,
           memory_cost=64 * 1024,
           parallelism=1,
           hash_len=32,
           salt_len=16
       )
       return hasher.hash(password)
   
   # 生成密码为 "abc@123_" 的哈希
   hashed_password = generate_hash("abc@123_")
   print(f"Hashed password: {hashed_password}")
   EOF
   ```

2. 运行 Python 脚本，生成密码哈希：

   ```bash
   docker run --rm -v $(pwd):/app python:3.11-slim bash -c "pip install argon2-cffi && python /app/generate_password.py"
   ```

3. 记录输出的哈希值，例如：
   ```
   $argon2id$v=19$m=65536,t=2,p=1$IE3pKKLnrJBplSHCeXMSkw$fA9VWzJNMaDeiptZdAwzWeBTI9fGskp+lfd+k5XvTAM
   ```

#### 步骤 2：创建SQL文件并插入用户

为了避免命令行中的复杂引号问题，我们使用SQL文件来创建用户，这是更可靠的方式：

1. 创建一个SQL文件，用于管理用户：

   ```bash
   cat > insert_user.sql << 'EOF'
   -- 删除旧用户（如果存在）
   DELETE FROM users WHERE username = 'admin';
   
   -- 插入新用户
   INSERT INTO users (username, hashed_password, role, email) 
   VALUES ('admin', '{hashed_password}', 'admin', 'admin@example.com');
   EOF
   ```

2. 编辑SQL文件，将生成的哈希值填入 `{hashed_password}` 位置：

   - 使用文本编辑器打开 `insert_user.sql` 文件
   - 将 `{hashed_password}` 替换为之前生成的哈希值
   - 例如：
     ```sql
     INSERT INTO users (username, hashed_password, role, email) 
     VALUES ('admin', '$argon2id$v=19$m=65536,t=2,p=1$IE3pKKLnrJBplSHCeXMSkw$fA9VWzJNMaDeiptZdAwzWeBTI9fGskp+lfd+k5XvTAM', 'admin', 'admin@example.com');
     ```

3. 执行SQL文件，创建用户：

   ```bash
   # 将SQL文件复制到数据库容器中
   docker cp insert_user.sql fs_postgres:/tmp/insert_user.sql
   
   # 在数据库容器中执行SQL文件
   docker exec -it fs_postgres psql -U fileserver -d fileserver -f /tmp/insert_user.sql
   ```
   
   执行成功后，会显示：
   ```
   DELETE 1
   INSERT 0 1
   ```
   
   或
   ```
   INSERT 0 1
   ```
   
   这表示用户已成功创建。

#### 步骤 3：验证用户可以登录

1. 使用 curl 命令测试登录：

   ```bash
   curl -X POST -H "Content-Type: application/json" -d '{"username":"admin","password":"abc@123_"}' http://localhost:8217/login
   ```

2. 如果返回以下响应，说明登录成功：
   ```json
   {"result":200,"message":"登录成功","data":null}
   ```

### 3. 使用创建的用户登录

现在您可以使用创建的用户名和密码登录文件浏览器：
- 用户名：admin
- 密码：abc@123_

## 七、访问服务

### 1. 访问 API 文档

在浏览器中输入以下地址访问 FileServer 的 API 文档：

```
http://localhost:8217/docs
```

### 2. 访问文件浏览器

在浏览器中输入以下地址访问文件浏览器：

```
http://localhost:8217/browser/
```
具体固件文件部署格式参考小智OTA部署

## 八、停止和更新服务

### 1. 停止服务

```bash
# 停止 FileServer 服务
docker compose -f fileserver.yml down
```

### 2. 更新服务

如果您获得了新版本的 Docker 镜像，执行以下步骤更新服务：

```bash
# 停止当前服务
docker compose -f fileserver.yml down

# 删除旧镜像
docker rmi fileserver:latest

# 导入新镜像
docker load -i fileserver-new.tar

# 启动新服务
docker compose -f fileserver.yml up -d
```

## 九、常见问题及解决方法

### 1. 端口被占用

**问题**：启动服务时提示端口被占用，如 `Error starting userland proxy: listen tcp 0.0.0.0:8217: bind: address already in use`

**解决方法**：

1. 检查端口占用情况：
   ```bash
   # Windows 系统
   netstat -ano | findstr :8217
   
   # macOS 或 Ubuntu 系统
   lsof -i :8217
   ```

2. 停止占用端口的进程，或修改配置文件中的端口映射：
   ```yaml
   # 修改 fileserver.yml 中的端口映射
   ports:
     - "8218:8000"  # 将 8217 改为其他未被占用的端口
   ```

3. 重启服务：
   ```bash
   docker compose -f fileserver.yml down
   docker compose -f fileserver.yml up -d
   ```

### 2. 无法访问服务

**问题**：在浏览器中无法访问 API 文档或文件浏览器

**解决方法**：

1. 检查服务是否正常运行：
   ```bash
   docker compose -f fileserver.yml ps
   ```

2. 检查防火墙设置，确保端口已开放：
   - Windows：检查 Windows Defender 防火墙设置
   - macOS：检查系统偏好设置中的安全性与隐私
   - Ubuntu：执行 `sudo ufw status` 检查防火墙状态

3. 检查 IP 地址和端口是否正确：
   - 确认您在浏览器中输入的 IP 地址和端口与配置文件中的 `DOMAIN` 和 `PORT` 一致

### 3. 登录失败

**问题**：访问文件浏览器时，登录失败

**解决方法**：

1. 检查数据库中是否存在用户：
   ```bash
   # 连接到数据库容器
   docker exec -it fs_postgres psql -U fileserver -d fileserver
   
   # 查看用户表
   SELECT * FROM users;
   ```

2. 如果用户表为空，按照本教程 用户管理 章节的步骤创建管理员用户


