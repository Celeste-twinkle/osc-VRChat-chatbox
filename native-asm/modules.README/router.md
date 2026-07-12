# router.inc

拥有 TCP accept_loop 与 /send、/typing、/notify-sfx 等端点处理。它把请求交给 request-matchers.inc 判定，再分发到 HTTP、会话、LAN 或 OSC 模块；所有路径最终经 lifecycle.inc 关闭客户端。

关联：/send 和 /typing 调用 osc.inc；设置/历史请求交给 http.inc；会话与 LAN 请求分别交给 lifecycle.inc。新增端点应同时更新本文件、request-matchers.inc 和 data-core.inc 的请求前缀。
