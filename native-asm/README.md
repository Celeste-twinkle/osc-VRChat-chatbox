# native-asm

This is the smallest local-build target.

The program does not package a browser or webview. It starts a tiny local HTTP
server, then asks Windows to open the system default browser with
`ShellExecute("open", "http://127.0.0.1:19001")`.

By default, the HTTP server listens on `127.0.0.1:19001` only. This keeps the
network surface small and helps avoid Windows reputation/heuristic warnings for
a tiny unsigned executable that opens a local server.

If you need access from devices on the same LAN, click the LAN button on the
page, or start the executable with `--lan` or `/lan`. In LAN mode it listens on
`0.0.0.0:19001`, so devices on the same network can open the page with:

```text
http://<this-pc-lan-ip>:19001
```

For example, if the PC address is `192.168.1.23`, use:

```text
http://192.168.1.23:19001
```

Windows Firewall may ask for network permission the first time the exe runs.

Build:

```bat
cd native-asm
build.bat
```

Output:

```text
native-asm\dist\vrc-chatbox-osc-asm.exe
```

The HTML UI is embedded into the executable as plain text, so the release can be
a single exe. That embedded HTML is only the page served to the user's existing
browser, not a browser runtime.

## Source layout

`server.asm` is now a thin FASM entry file. It declares the PE sections and
includes the implementation modules under `modules/` in emitted-order. Keep
that include order stable unless a deliberate binary layout change is wanted.

Every `.inc` module has a corresponding Markdown document in
`modules.README/`. Start with [the module map](modules.README/README.md) for
responsibilities, dependencies, and the HTTP/OSC call flow.

## UI 开发

### 目录结构

```text
native-asm/
├── ui/                # UI 工程（完整 npm 项目）
│   ├── package.json   # esbuild 构建期依赖
│   ├── build.js       # 一键构建：minify 合并 + 生成 html.inc
│   ├── src/           # 源码（人工编辑，提交到 git）
│   │   ├── template.html  # HTML 骨架，含 /*STYLE*/ 和 /*SCRIPT*/ 占位符
│   │   ├── style.css      # 所有样式，可带注释和换行
│   │   └── app.js         # 所有 JavaScript 逻辑，可带注释和换行
│   └── dist/          # 构建产物（自动生成，git 忽略）
│       ├── index.html # 合并压缩后的最终页面
│       └── html.inc       # FASM 嵌入指令（modules/data-ui.inc 引用）
└── tools/             # FASM 编译器（与 UI 无关）
    └── fasm/
```

### 构建流程

`build.bat` 自动执行：
1. `node ui\build.js` — minify 合并 CSS/JS → `ui/dist/index.html` → `ui/dist/html.inc`
2. FASM 编译 `server.asm` → `dist/vrc-chatbox-osc-asm.exe`

UI 构建产物通过 `modules/data-ui.inc` 嵌入 exe：

```asm
; modules/data-ui.inc
include '..\ui\dist\html.inc'
html_len = $ - html
```

**首次构建需安装 node 依赖**（构建期，不进产物）：

```bat
cd native-asm\ui
npm install
```

**编辑页面后只需**：

```bat
cd native-asm
build.bat
```

无需手动处理 `db` 字符串或转义引号。如果新增了 CSS 或 JS 依赖，只改 `template.html` 中的占位符即可。

### 黑暗模式与框架

- 页面支持**黑暗模式**：默认跟随系统（`prefers-color-scheme`），点页面右上角 🌙/☀️ 按钮切换，选择保存在 `localStorage`（键 `vrcChatboxTheme`）。
- 所有颜色通过 CSS 变量（`:root` 浅色 / `[data-theme="dark"]` 深色）定义。
- 构建时 `build.bat` 会先结束正在运行的旧 exe（否则 FASM 无法覆盖写入），再重新编译。
