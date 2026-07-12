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
