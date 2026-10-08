# settings.inc

处理设置文件的读取、默认值写入、设置/提示词/常用文本 JSON 的轻量有效性检查，以及启动、最小化和 LAN 标志的提取。默认设置包含双语输出顺序；浏览器提交的完整 JSON 会保留该值。还提供将进程工作目录切换到 exe 所在目录的函数。

`json_object_ok` 是共享的快照检查：非空、首尾是 `{}`、花括号与字符串配对；`settings_json_valid`、`prompts_json_valid`（额外要求 activeId/items 键）和 `quicktext_json_valid`（额外要求 ≤ 32 KiB）都建立在它之上。它用 ebx/ebp/edi 做计数器，调用方需自行保存还要用的寄存器。

关联：bootstrap.inc 在启动阶段调用它；startup.inc 复用设置标志并持久化变更；http.inc 负责把浏览器提交的完整设置 JSON 写入同一文件。常量和缓冲区在 data-core.inc、data-runtime.inc。
