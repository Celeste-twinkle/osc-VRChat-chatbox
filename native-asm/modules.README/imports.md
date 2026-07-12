# imports.inc

声明本程序实际使用的最小 Win32 DLL 导入：Kernel32、Advapi32、User32、Shell32、IP Helper 与 Winsock。FASM 的 API include 文件和手写导入在这里集中维护。

关联：由 server.asm 的 .idata 段 include。新增 Windows API 前先确认能否复用现有导入，以避免不必要地增大可执行文件。
