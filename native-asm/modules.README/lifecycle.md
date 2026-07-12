# lifecycle.inc

实现会话打开/关闭、心跳、LAN 请求的路由处理，以及客户端关闭、退出和已运行实例的唤醒。它是浏览器会话状态与 Windows 生命周期之间的边界。

关联：调用 sessions.inc 更新心跳和数量，调用 lan-listener.inc 切换监听，调用 tray.inc 处理窗口消息；由 router.inc 和 bootstrap.inc 进入。
