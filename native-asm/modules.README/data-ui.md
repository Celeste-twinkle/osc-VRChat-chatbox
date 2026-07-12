# data-ui.inc

包含完整的 UTF-8 HTML、CSS 和 JavaScript 页面，以及 html_len。该数据作为单一 HTTP 页面由 native 服务直接输出，不依赖 WebView 或额外静态文件。

关联：仅 http.inc 的 serve_index 直接读取；前端通过 router.inc 暴露的端点管理设置、历史、LAN、提示音、typing 和消息发送。编辑后必须保持 UTF-8 并运行嵌入式 JavaScript 语法检查。
