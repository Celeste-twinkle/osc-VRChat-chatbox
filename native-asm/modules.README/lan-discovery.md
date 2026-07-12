# lan-discovery.inc

使用 GetIpAddrTable 枚举 IPv4 地址，对地址评分，选择主地址，并生成可供 UI 使用的 LAN JSON 列表。

关联：http.inc 的 /lan-ip 响应调用它；lan-listener.inc 负责实际监听。IP 表、临时文本和 JSON 缓冲区均位于 data-runtime.inc。
