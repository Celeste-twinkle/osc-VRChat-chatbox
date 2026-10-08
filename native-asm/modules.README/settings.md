# settings.inc

处理设置文件的读取、默认值写入、设置/提示词/常用文本 JSON 的轻量有效性检查，以及启动、最小化和 LAN 标志的提取。默认设置包含双语输出顺序；浏览器提交的完整 JSON 会保留该值。还提供将进程工作目录切换到 exe 所在目录的函数。

`json_object_ok` 由本模块引入的 `json.inc` 提供，检查完整 JSON 对象语法、字符串转义、UTF-8 和最多 64 层嵌套，并保存 ebx/ebp/edi。`settings_json_valid` 额外要求 translate/provider 键，`prompts_json_valid` 要求 activeId/items 键，`quicktext_json_valid` 要求 ≤ 32 KiB、根级 items 数组最多 100 条、每条包含字符串 id/text。非法提示词或常用文本快照不会替换已有文件。

关联：bootstrap.inc 在启动阶段调用它；startup.inc 复用设置标志并持久化变更；http.inc 负责把浏览器提交的完整设置 JSON 写入同一文件。常量和缓冲区在 data-core.inc、data-runtime.inc。
