# command-line.inc

扫描进程命令行中的 LAN 与最小化启动开关。扫描逻辑不分配内存，适合在早期启动阶段使用。

关联：bootstrap.inc 使用它决定首次绑定和是否打开浏览器；lifecycle.inc 在唤醒已有实例时复用最小化判断；参数字符串由 data-core.inc 定义。
