# settings.inc

处理设置文件的读取、默认值写入、JSON 有效性检查，以及启动、最小化和 LAN 标志的提取。还提供将进程工作目录切换到 exe 所在目录的函数。

关联：bootstrap.inc 在启动阶段调用它；startup.inc 复用设置标志并持久化变更；http.inc 负责把浏览器提交的完整设置 JSON 写入同一文件。常量和缓冲区在 data-core.inc、data-runtime.inc。
