# http.inc

处理 HTTP 请求体与 Content-Length，并服务嵌入式首页、设置、历史记录、AI 提示词模板、共享术语词库、常用文本和 LAN 地址 JSON。历史、提示词、常用文本与设置文件的实际读写也在此模块。提示词与术语词库共用 `prompts.json`，常用文本单独存在 `quicktext.json`，两者写入限制均为 32 KiB；服务会校验完整 JSON 语法，再写入临时文件并原子替换，遇到无效、超长或写入失败时返回对应的非 2xx 状态。

`/quicktext` 的 GET/POST 响应携带基于文件内容的 ETag。浏览器 POST 携带该 ETag 的 If-Match；文件已改变时返回 412，由前端合并后重试。仅缺失文件返回空列表和 `"missing00"` 标签；已保存的空列表具有内容标签，损坏或不可读的文件返回 500，以免旧缓存被误当升级数据上传。

常用文本放在 exe 旁边而不是只放浏览器 localStorage，是因为 localStorage 按 origin 隔离：换端口、localhost↔127.0.0.1、局域网地址或换浏览器都会各存一份，用户会以为数据丢了。

关联：router.inc 进入本模块；首页内容位于 data-ui.inc，响应和文件名位于 data-core.inc，请求/文件缓冲区位于 data-runtime.inc；LAN JSON 由 lan-discovery.inc 填充。
