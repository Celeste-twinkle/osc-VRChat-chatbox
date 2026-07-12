# startup.inc

负责 Windows Run 注册表项、设置 JSON 中布尔字段的修改，以及自启动命令行构造。它保证 --startup 和 --minimized 与 UI 设置一致。

关联：tray.inc 的菜单命令调用切换函数；settings.inc 提供当前设置与文件准备；data-core.inc 保存注册表路径、键名和 JSON 键，data-runtime.inc 保存句柄与命令缓冲区。
