# P2P VOD WebServer
在Linux环境下用C++实现的点对点视频点播服务

## 功能
### 高效并发和连接管理：
* 利用Epoll技术与线程池实现多线程的Reactor模式，提升服务器并发处理能力。
* 采用基于小根堆的定时器自动关闭超时的非活动连接，优化资源利用。
* 利用RAII机制实现了MySQL和Redis的连接池，减少数据库连接的开销。
### 用户认证与会话管理：
* 用户注册和登录通过MySQL数据库管理，保障数据安全性。
* 结合Redis生成5分钟有效的SessionID用于登录状态验证，减少重复验证操作。
* 引入布隆过滤器快速检测非注册用户，减轻数据库查询负担。
### 请求处理与资源响应：
* 利用正则表达式和状态机精确解析HTTP请求，高效处理静态和多媒体资源。
* 支持HTTP 206部分内容请求，实现视频文件的分段传输和点播功能。
### 数据传输与网络同步：
* 通过UDP进行节点间的视频数据传输，结合重传机制和滑动窗口控制传输的可靠性与流量。
* 使用Socket编程实现链路状态通告（LSA），同步多节点间的信息并进行节点探活。
### 系统日志与资源管理：
* 实现异步日志系统，通过单例模式和阻塞队列记录服务器运行状态。

## 环境要求
* Linux
* C++20
* MySql

## 目录树
```
.
├── code           源代码
│   ├── buffer
│   ├── config
│   ├── http
│   ├── log
│   ├── timer
│   ├── pool
│   ├── server
│   └── main.cpp
├── test           单元测试
│   ├── Makefile
│   └── test.cpp
├── resources      静态资源
│   ├── index.html
│   ├── image
│   ├── video
│   ├── js
│   └── css
├── bin            可执行文件
│   └── server
├── log            日志文件
├── webbench-1.5   压力测试
├── build          
│   └── Makefile
├── Makefile
├── LICENSE
└── readme.md
```


## 项目启动
需要先配置好对应的数据库
```bash
// 建立yourdb库
create database yourdb;

// 创建user表
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, password) VALUES('name', 'password');
```

```bash
make
./bin/server
```


## 致谢
Linux高性能服务器编程，游双著.

[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)
