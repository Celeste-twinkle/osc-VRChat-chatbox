# osc.inc

构造并通过 UDP 发送 /chatbox/input 和 /chatbox/typing OSC 包。notify_sfx 决定 /chatbox/input 的第三个布尔标记是 T 还是 F。

关联：router.inc 的发送、输入状态和提示音端点调用本模块；OSC 地址、类型标签和包缓冲区分别来自 data-core.inc、data-runtime.inc。修改参数顺序时必须同步 VRChat OSC 规范。
