setup_tray:
        invoke  GetModuleHandle,0
        mov     [app_instance],eax
        mov     [tray_wc.hInstance],eax
        mov     [tray_wc.lpfnWndProc],tray_wnd_proc
        mov     [tray_wc.lpszClassName],tray_class
        invoke  RegisterClass,tray_wc
        invoke  CreateWindowEx,0,tray_class,tray_title,0,0,0,0,0,0,0,[app_instance],0
        mov     [tray_hwnd],eax
        test    eax,eax
        jz      .done
        invoke  GetSystemMetrics,SM_CXSMICON
        mov     ebx,eax
        invoke  GetSystemMetrics,SM_CYSMICON
        invoke  LoadImage,[app_instance],1,IMAGE_ICON,ebx,eax,LR_DEFAULTCOLOR
        test    eax,eax
        jnz     .icon_ready
        invoke  LoadIcon,[app_instance],1
        test    eax,eax
        jnz     .icon_ready
        invoke  LoadIcon,0,IDI_APPLICATION
.icon_ready:
        mov     [tray_icon],eax
        mov     [tray_nid.cbSize],sizeof.NOTIFYICONDATAA
        mov     eax,[tray_hwnd]
        mov     [tray_nid.hWnd],eax
        mov     [tray_nid.uID],1
        mov     [tray_nid.uFlags],NIF_MESSAGE+NIF_ICON+NIF_TIP
        mov     [tray_nid.uCallbackMessage],WM_TRAY
        mov     eax,[tray_icon]
        mov     [tray_nid.hIcon],eax
        mov     esi,tray_tip
        mov     edi,tray_nid.szTip
        mov     ecx,tray_tip_len
        rep     movsb
        invoke  Shell_NotifyIcon,NIM_ADD,tray_nid
.done:
        ret

remove_tray:
        cmp     dword [tray_hwnd],0
        je      .done
        invoke  Shell_NotifyIcon,NIM_DELETE,tray_nid
        invoke  DestroyWindow,[tray_hwnd]
        mov     dword [tray_hwnd],0
.done:
        ret

process_messages:
.loop:
        invoke  PeekMessage,msg,0,0,0,PM_REMOVE
        test    eax,eax
        jz      .done
        cmp     dword [msg+4],0012h
        jne     .dispatch
        mov     dword [quit_requested],1
        jmp     .loop
.dispatch:
        invoke  TranslateMessage,msg
        invoke  DispatchMessage,msg
        jmp     .loop
.done:
        ret

show_tray_menu:
        push    ebx
        call    select_tray_menu_texts
        invoke  CreatePopupMenu
        mov     [tray_menu],eax
        test    eax,eax
        jz      .done
        invoke  AppendMenuW,[tray_menu],0,ID_TRAY_OPEN,[tray_open_text]
        invoke  AppendMenuW,[tray_menu],MF_SEPARATOR,0,0
        xor     eax,eax
        cmp     dword [lan_enabled],1
        jne     .lan_flag_ready
        or      eax,MF_CHECKED
.lan_flag_ready:
        invoke  AppendMenuW,[tray_menu],eax,ID_TRAY_LAN,[tray_lan_text]
        xor     eax,eax
        cmp     dword [startup_mode],1
        jne     .startup_flag_ready
        or      eax,MF_CHECKED
.startup_flag_ready:
        invoke  AppendMenuW,[tray_menu],eax,ID_TRAY_STARTUP,[tray_startup_text]
        xor     eax,eax
        cmp     dword [startup_mode],1
        je      .min_check
        or      eax,MF_GRAYED
        jmp     .min_flag_ready
.min_check:
        cmp     dword [startup_minimized],1
        jne     .min_flag_ready
        or      eax,MF_CHECKED
.min_flag_ready:
        invoke  AppendMenuW,[tray_menu],eax,ID_TRAY_START_MINIMIZED,[tray_start_minimized_text]
        invoke  AppendMenuW,[tray_menu],MF_SEPARATOR,0,0
        invoke  AppendMenuW,[tray_menu],0,ID_TRAY_EXIT,[tray_exit_text]
        invoke  GetCursorPos,pt
        invoke  SetForegroundWindow,[tray_hwnd]
        invoke  TrackPopupMenu,[tray_menu],TPM_RIGHTBUTTON+TPM_RETURNCMD+TPM_BOTTOMALIGN,[pt.x],[pt.y],0,[tray_hwnd],0
        mov     ebx,eax
        invoke  DestroyMenu,[tray_menu]
        cmp     ebx,ID_TRAY_OPEN
        je      .open
        cmp     ebx,ID_TRAY_LAN
        je      .toggle_lan
        cmp     ebx,ID_TRAY_STARTUP
        je      .toggle_startup
        cmp     ebx,ID_TRAY_START_MINIMIZED
        je      .toggle_start_minimized
        cmp     ebx,ID_TRAY_EXIT
        je      .exit
        jmp     .done
.open:
        invoke  ShellExecute,0,open_action,url,0,0,1
        jmp     .done
.toggle_lan:
        call    toggle_lan_from_tray
        jmp     .done
.toggle_startup:
        call    toggle_startup_from_tray
        jmp     .done
.toggle_start_minimized:
        call    toggle_start_minimized_from_tray
        jmp     .done
.exit:
        mov     dword [quit_requested],1
.done:
        pop     ebx
        ret

select_tray_menu_texts:
        mov     dword [tray_open_text],tray_menu_open_en_w
        mov     dword [tray_exit_text],tray_menu_exit_en_w
        mov     dword [tray_lan_text],tray_menu_lan_en_w
        mov     dword [tray_startup_text],tray_menu_startup_en_w
        mov     dword [tray_start_minimized_text],tray_menu_start_minimized_en_w
        invoke  GetUserDefaultLangID
        and     eax,03ffh
        cmp     eax,LANG_CHINESE
        je      .zh
        cmp     eax,LANG_JAPANESE
        je      .ja
        cmp     eax,LANG_KOREAN
        je      .ko
        ret
.zh:
        mov     dword [tray_open_text],tray_menu_open_zh_w
        mov     dword [tray_exit_text],tray_menu_exit_zh_w
        mov     dword [tray_lan_text],tray_menu_lan_zh_w
        mov     dword [tray_startup_text],tray_menu_startup_zh_w
        mov     dword [tray_start_minimized_text],tray_menu_start_minimized_zh_w
        ret
.ja:
        mov     dword [tray_open_text],tray_menu_open_ja_w
        mov     dword [tray_exit_text],tray_menu_exit_ja_w
        mov     dword [tray_lan_text],tray_menu_lan_ja_w
        mov     dword [tray_startup_text],tray_menu_startup_ja_w
        mov     dword [tray_start_minimized_text],tray_menu_start_minimized_ja_w
        ret
.ko:
        mov     dword [tray_open_text],tray_menu_open_ko_w
        mov     dword [tray_exit_text],tray_menu_exit_ko_w
        mov     dword [tray_lan_text],tray_menu_lan_ko_w
        mov     dword [tray_startup_text],tray_menu_startup_ko_w
        mov     dword [tray_start_minimized_text],tray_menu_start_minimized_ko_w
        ret

tray_wnd_proc:
        push    ebp
        mov     ebp,esp
        mov     eax,[ebp+12]
        cmp     eax,WM_TRAY
        je      .tray
        cmp     eax,0002h
        je      .destroy
        invoke  DefWindowProc,dword [ebp+8],dword [ebp+12],dword [ebp+16],dword [ebp+20]
        jmp     .ret
.tray:
        mov     eax,[ebp+20]
        cmp     eax,WM_RBUTTONUP
        je      .menu
        cmp     eax,WM_LBUTTONDBLCLK
        je      .open
        xor     eax,eax
        jmp     .ret
.menu:
        call    show_tray_menu
        xor     eax,eax
        jmp     .ret
.open:
        invoke  ShellExecute,0,open_action,url,0,0,1
        xor     eax,eax
        jmp     .ret
.destroy:
        invoke  PostQuitMessage,0
        xor     eax,eax
.ret:
        mov     esp,ebp
        pop     ebp
        ret     16
