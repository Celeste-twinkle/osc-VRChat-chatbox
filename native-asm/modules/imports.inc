library kernel32,'KERNEL32.DLL',\
        advapi32,'ADVAPI32.DLL',\
        user32,'USER32.DLL',\
        shell32,'SHELL32.DLL',\
        iphlpapi,'IPHLPAPI.DLL',\
        wsock32,'WSOCK32.DLL'

include '..\tools\fasm\include\api\kernel32.inc'
include '..\tools\fasm\include\api\user32.inc'
include '..\tools\fasm\include\api\shell32.inc'
include '..\tools\fasm\include\api\wsock32.inc'

import advapi32,\
       RegCloseKey,'RegCloseKey',\
       RegCreateKeyEx,'RegCreateKeyExA',\
       RegDeleteValue,'RegDeleteValueA',\
       RegOpenKeyEx,'RegOpenKeyExA',\
       RegSetValueEx,'RegSetValueExA'

import iphlpapi,\
       GetIpAddrTable,'GetIpAddrTable'
