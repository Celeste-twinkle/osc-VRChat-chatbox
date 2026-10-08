# json.inc

由 `settings.inc` 在原 JSON 校验代码的位置引入，提供有界的 JSON 语法与 UTF-8 校验，不新增 DLL 导入。校验对象、数组、字符串与转义、数字、布尔值和 null，并拒绝尾随内容、错误分隔符、非法 UTF-8 与超过 64 层的嵌套。

`quicktext_schema_ok` 在完整语法校验后要求根级 items 数组最多 100 条，每条含字符串 id/text；允许附加字段。存储校验入口保留 ebx/edi/ebp，内部解析函数消费 esi/ecx。
