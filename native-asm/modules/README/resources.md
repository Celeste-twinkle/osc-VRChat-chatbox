# resources.inc

定义 PE 图标、版本信息与 application manifest 资源。图标文件位于 native-asm/assets/，manifest 保持 asInvoker，不要求管理员权限。

关联：由 server.asm 的 .rsrc 段 include。修改图标或版本文本会改变资源节大小，应在发布前重新检查最终 exe 体积。
