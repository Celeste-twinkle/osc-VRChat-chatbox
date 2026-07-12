# sessions.inc

维护浏览器 session id、心跳时间戳和空闲回收。会话数量用于决定是否在空闲后退出或保持服务。

关联：lifecycle.inc 的 /session/* 和 /heartbeat 处理器调用本模块；data-runtime.inc 提供固定容量的会话表和计时字段。
