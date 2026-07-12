# data-core.inc

集中保存不可变或初始化后只读的字符串与常量数据：HTTP 路由前缀和响应头、文件名、默认设置、注册表键、托盘文案、LAN JSON 模板及 OSC 地址/类型标签。

关联：几乎所有代码模块引用本文件。新增端点通常需要同时修改 router.inc、request-matchers.inc 和这里的请求前缀。
