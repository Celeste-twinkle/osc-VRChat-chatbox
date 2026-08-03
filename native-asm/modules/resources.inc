directory RT_ICON,icons,\
          RT_GROUP_ICON,group_icons,\
          RT_VERSION,versions,\
          RT_MANIFEST,manifests

resource icons,\
         1,LANG_NEUTRAL,app_icon_16,\
         2,LANG_NEUTRAL,app_icon_32,\
         3,LANG_NEUTRAL,app_icon_48

resource group_icons,\
         1,LANG_NEUTRAL,app_icon_group

resource versions,\
         1,LANG_NEUTRAL,version_info

resource manifests,\
         1,LANG_NEUTRAL,app_manifest

icon app_icon_group,\
     app_icon_16,'assets\icon-gpt-16.ico',\
     app_icon_32,'assets\icon-gpt-q8-32.ico',\
     app_icon_48,'assets\icon-gpt-q8-48.ico'

versioninfo version_info,VOS_NT_WINDOWS32,VFT_APP,VFT2_UNKNOWN,0409h,04E4h,\
            'CompanyName','osc-VRChat-chatbox',\
            'FileDescription','VRChat Chatbox OSC local helper',\
            'FileVersion','2.3.0.0',\
            'InternalName','vrc-chatbox-osc.exe',\
            'OriginalFilename','vrc-chatbox-osc.exe',\
            'ProductName','VRC Chatbox OSC',\
            'ProductVersion','2.3.0.0'

resdata app_manifest
  db '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>',13,10
  db '<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">',13,10
  db '  <assemblyIdentity version="2.3.0.0" processorArchitecture="x86" name="VRCChatboxOSC" type="win32"/>',13,10
  db '  <description>VRChat Chatbox OSC local helper</description>',13,10
  db '  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">',13,10
  db '    <security>',13,10
  db '      <requestedPrivileges>',13,10
  db '        <requestedExecutionLevel level="asInvoker" uiAccess="false"/>',13,10
  db '      </requestedPrivileges>',13,10
  db '    </security>',13,10
  db '  </trustInfo>',13,10
  db '</assembly>'
endres
