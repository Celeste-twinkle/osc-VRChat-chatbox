update_startup_setting:
        cmp     dword [startup_mode],1
        jne     .delete
        invoke  RegCreateKeyEx,HKEY_CURRENT_USER,startup_key,0,0,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,0,startup_key_handle,0
        cmp     eax,0
        jne     .done
        call    build_startup_command
        invoke  RegSetValueEx,[startup_key_handle],startup_value,0,REG_SZ,startup_cmd,[startup_cmd_len]
        invoke  RegCloseKey,[startup_key_handle]
        jmp     .done
.delete:
        invoke  RegOpenKeyEx,HKEY_CURRENT_USER,startup_key,0,KEY_SET_VALUE,startup_key_handle
        cmp     eax,0
        jne     .done
        invoke  RegDeleteValue,[startup_key_handle],startup_value
        invoke  RegCloseKey,[startup_key_handle]
.done:
        ret

toggle_lan_from_tray:
        cmp     dword [lan_enabled],1
        je      .disable
        call    enable_lan_listener
        cmp     dword [lan_enabled],1
        jne     .persist
        mov     dword [lan_allowed],1
        jmp     .persist
.disable:
        call    restore_localhost_listener
        mov     dword [lan_allowed],0
.persist:
        call    persist_settings_flags
        ret

toggle_startup_from_tray:
        cmp     dword [startup_mode],1
        je      .disable
        mov     dword [startup_mode],1
        jmp     .apply
.disable:
        mov     dword [startup_mode],0
        mov     dword [startup_minimized],0
.apply:
        call    update_startup_setting
        call    persist_settings_flags
        ret

toggle_start_minimized_from_tray:
        cmp     dword [startup_mode],1
        jne     .done
        xor     dword [startup_minimized],1
        call    update_startup_setting
        call    persist_settings_flags
.done:
        ret

persist_settings_flags:
        invoke  CreateFile,settings_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .use_default
        mov     [settings_handle],eax
        invoke  ReadFile,[settings_handle],settings_buf,settings_buf_size,bytes_done,0
        invoke  CloseHandle,[settings_handle]
        mov     eax,[bytes_done]
        mov     [settings_write_len],eax
        mov     dword [body_ptr],settings_buf
        call    settings_json_valid
        cmp     eax,1
        je      .patch
.use_default:
        mov     esi,default_settings
        mov     edi,settings_buf
        mov     ecx,default_settings_len
        rep     movsb
        mov     dword [settings_write_len],default_settings_len
        mov     dword [body_ptr],settings_buf
.patch:
        mov     edi,json_startup_key
        mov     edx,json_startup_key_len
        mov     eax,[startup_mode]
        call    set_json_bool
        mov     edi,json_start_minimized_key
        mov     edx,json_start_minimized_key_len
        mov     eax,[startup_minimized]
        call    set_json_bool
        mov     edi,json_lan_key
        mov     edx,json_lan_key_len
        mov     eax,[lan_allowed]
        call    set_json_bool
        invoke  CreateFile,settings_file,40000000h,0,0,2,80h,0
        cmp     eax,-1
        je      .done
        mov     [settings_handle],eax
        invoke  WriteFile,[settings_handle],settings_buf,[settings_write_len],bytes_done,0
        invoke  CloseHandle,[settings_handle]
.done:
        ret

set_json_bool:
        push    ebx
        mov     [json_bool_value],eax
        mov     esi,[body_ptr]
        mov     ecx,[settings_write_len]
.scan:
        cmp     ecx,edx
        jb      .done
        push    esi
        push    edi
        push    ecx
        mov     ecx,edx
        repe    cmpsb
        pop     ecx
        pop     edi
        pop     esi
        je      .found
        inc     esi
        dec     ecx
        jmp     .scan
.found:
        add     esi,edx
        cmp     dword [json_bool_value],1
        je      .want_true
        cmp     dword [esi],65757274h
        jne     .done
        mov     eax,[settings_write_len]
        cmp     eax,settings_buf_size
        jae     .done
        mov     ebx,esi
        mov     edx,[body_ptr]
        add     edx,eax
        mov     esi,edx
        dec     esi
        mov     edi,edx
        mov     ecx,edx
        sub     ecx,ebx
        sub     ecx,4
        std
        rep     movsb
        cld
        mov     esi,ebx
        mov     dword [esi],736c6166h
        mov     byte [esi+4],'e'
        inc     dword [settings_write_len]
        jmp     .done
.want_true:
        cmp     dword [esi],65757274h
        je      .done
        cmp     dword [esi],736c6166h
        jne     .done
        cmp     byte [esi+4],'e'
        jne     .done
        mov     dword [esi],65757274h
        mov     edi,esi
        add     edi,4
        mov     esi,edi
        inc     esi
        mov     eax,[body_ptr]
        add     eax,[settings_write_len]
        sub     eax,esi
        mov     ecx,eax
        cld
        rep     movsb
        dec     dword [settings_write_len]
.done:
        pop     ebx
        ret

build_startup_command:
        invoke  GetModuleFileName,0,exe_path,260
        mov     esi,exe_path
        mov     edi,startup_cmd
        mov     al,'"'
        stosb
.copy_path:
        lodsb
        test    al,al
        jz      .path_done
        stosb
        jmp     .copy_path
.path_done:
        mov     al,'"'
        stosb
        mov     esi,startup_arg
.copy_arg:
        lodsb
        stosb
        test    al,al
        jnz     .copy_arg
        dec     edi
        cmp     dword [startup_minimized],1
        jne     .no_min_arg
        mov     esi,startup_minimized_arg
.copy_min_arg:
        lodsb
        stosb
        test    al,al
        jnz     .copy_min_arg
        jmp     .finish
.no_min_arg:
        mov     byte [edi],0
        inc     edi
.finish:
        mov     eax,edi
        sub     eax,startup_cmd
        mov     [startup_cmd_len],eax
        ret
