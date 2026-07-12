# request-matchers.inc

提供轻量的 HTTP 请求行前缀匹配函数，例如 is_post_send 和 is_get_settings。这些函数只识别路由，不解析请求体。

关联：仅由 router.inc 调用；请求字面量位于 data-core.inc。新路由需要在这里增加匹配器，避免在接收循环中内联字符串比较。
