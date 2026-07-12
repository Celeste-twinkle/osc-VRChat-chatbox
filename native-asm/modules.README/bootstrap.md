# bootstrap.inc

负责 start、WinSock 初始化、初始监听地址选择和 OSC UDP 套接字创建。成功后进入 router.inc 的 accept_loop；检测到已有实例时调用 lifecycle.inc 的退出路径。

依赖：settings.inc 加载启动标志，command-line.inc 解析 --lan/--minimized，lan-listener.inc 建立 LAN 监听，tray.inc 初始化托盘，data-runtime.inc 保存套接字和状态。
