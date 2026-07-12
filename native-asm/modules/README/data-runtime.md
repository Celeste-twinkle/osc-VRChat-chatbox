# data-runtime.inc

保存可写全局状态与缓冲区：套接字、托盘句柄、设置和 LAN 标志、会话表、HTTP 请求缓冲区、文件缓冲区、地址结构和 OSC 数据包。

关联：所有有状态模块共享本文件。更改缓冲区大小或字段布局时，应检查 http.inc、sessions.inc、lan-discovery.inc 和 osc.inc 的边界计算。
