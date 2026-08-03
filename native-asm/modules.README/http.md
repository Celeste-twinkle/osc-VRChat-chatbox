# http.inc

处理 HTTP 请求体与 Content-Length，并服务嵌入式首页、设置、历史记录、AI 提示词模板和 LAN 地址 JSON。历史、提示词与设置文件的实际读写也在此模块。

关联：router.inc 进入本模块；首页内容位于 data-ui.inc，响应和文件名位于 data-core.inc，请求/文件缓冲区位于 data-runtime.inc；LAN JSON 由 lan-discovery.inc 填充。
