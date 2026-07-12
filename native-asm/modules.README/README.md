# native-asm 模块地图

server.asm 只定义 FASM 段并按原有输出顺序 include 模块。模块的 include 顺序同时决定代码和数据的布局；除非有意改变二进制布局，不要调换顺序。

    browser / HTTP
      -> router -> request-matchers -> http / sessions / lan-listener / osc
      -> settings <-> startup
    Windows lifecycle -> bootstrap -> lifecycle -> tray
    http -> lan-discovery
    all code -> data-core / data-ui / data-runtime

每个 .inc 都有同名 Markdown 文件。server.asm 的说明位于 [server.md](server.md)。

| 模块 | 职责 |
| --- | --- |
| bootstrap | WinSock、监听套接字和应用启动 |
| router | HTTP 接收循环与端点处理分发 |
| settings / startup | 设置文件、启动项和 JSON 布尔字段 |
| lifecycle / tray | 退出、会话端点和系统托盘 |
| request-matchers / command-line | 请求与命令行前缀识别 |
| lan-listener / lan-discovery | LAN 监听与网卡地址发现 |
| sessions | 浏览器会话心跳与空闲清理 |
| http | HTTP 解析、静态页面、设置和历史存取 |
| osc | VRChat Chatbox 与 typing OSC 包编码 |
| data-* | 常量、嵌入式 UI 和可写运行时状态 |
| imports / resources | 最小 Win32 导入与 PE 资源 |
