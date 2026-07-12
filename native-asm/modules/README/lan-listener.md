# lan-listener.inc

在仅本机 127.0.0.1 与 LAN 0.0.0.0 两种 HTTP 监听方式之间切换，并在失败时回退到本机监听。

关联：bootstrap.inc 用它处理初始 LAN 选择，lifecycle.inc 和 tray.inc 用它响应运行时切换；可访问地址由 lan-discovery.inc 计算。
