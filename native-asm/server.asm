format PE GUI 4.0
entry start

include '..\tools\fasm\include\win32ax.inc'

AF_INET      = 2
SOCK_STREAM  = 1
SOCK_DGRAM   = 2
IPPROTO_TCP  = 6
IPPROTO_UDP  = 17
INVALID_SOCKET = -1
session_slots = 16
WM_COMMAND = 0111h
WM_USER = 0400h
WM_TRAY = WM_USER + 1
WM_RBUTTONUP = 0205h
WM_LBUTTONDBLCLK = 0203h
ID_TRAY_OPEN = 1001
ID_TRAY_EXIT = 1002
ID_TRAY_LAN = 1003
ID_TRAY_STARTUP = 1004
ID_TRAY_START_MINIMIZED = 1005
MF_GRAYED = 0001h
MF_CHECKED = 0008h
MF_SEPARATOR = 0800h
RT_ICON = 3
RT_GROUP_ICON = 14
RT_VERSION = 16
RT_MANIFEST = 24
LANG_NEUTRAL = 0

section '.text' code readable executable

; Keep this include order stable: it preserves the emitted code layout.
; Module ownership and dependencies are documented in modules\README\.
include 'modules\bootstrap.inc'
include 'modules\router.inc'
include 'modules\settings.inc'
include 'modules\lifecycle.inc'
include 'modules\tray.inc'
include 'modules\request-matchers.inc'
include 'modules\command-line.inc'
include 'modules\startup.inc'
include 'modules\lan-listener.inc'
include 'modules\sessions.inc'
include 'modules\http.inc'
include 'modules\lan-discovery.inc'
include 'modules\osc.inc'

section '.data' data readable writeable
; Data remains in its original order so code and resource offsets stay stable.
include 'modules\data-core.inc'
include 'modules\data-ui.inc'
include 'modules\data-runtime.inc'

section '.idata' import data readable writeable
include 'modules\imports.inc'

section '.rsrc' resource data readable
include 'modules\resources.inc'
