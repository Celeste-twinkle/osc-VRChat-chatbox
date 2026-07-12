# tray.inc

管理隐藏窗口、通知区图标、语言化菜单和菜单命令。菜单可打开 UI、退出、切换 LAN、开机启动与最小化启动。

关联：bootstrap.inc 初始化托盘；lifecycle.inc 负责退出；startup.inc 实现持久化开关；lan-listener.inc 执行监听切换。菜单文本和窗口句柄在 data-core.inc 与 data-runtime.inc。
