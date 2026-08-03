load_settings_flags:
        invoke  CreateFile,settings_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .default
        mov     [settings_handle],eax
        invoke  ReadFile,[settings_handle],settings_buf,settings_buf_size,bytes_done,0
        invoke  CloseHandle,[settings_handle]
        mov     eax,[bytes_done]
        mov     [settings_write_len],eax
        mov     dword [body_ptr],settings_buf
        call    settings_json_valid
        cmp     eax,1
        jne     .default
        call    parse_settings_flags
        call    init_server_port
.done:
        ret
.default:
        call    use_default_settings_flags
        ret

use_default_settings_flags:
        call    write_default_settings
        mov     dword [body_ptr],default_settings
        mov     dword [settings_write_len],default_settings_len
        call    parse_settings_flags
        call    init_server_port
        ret

write_default_settings:
        invoke  CreateFile,settings_file,40000000h,0,0,2,80h,0
        cmp     eax,-1
        je      .done
        mov     [settings_handle],eax
        invoke  WriteFile,[settings_handle],default_settings,default_settings_len,bytes_done,0
        invoke  CloseHandle,[settings_handle]
.done:
        ret

settings_json_valid:
        mov     ecx,[settings_write_len]
        test    ecx,ecx
        jz      .no
        mov     esi,[body_ptr]
.scan_start:
        mov     al,[esi]
        cmp     al,' '
        je      .next_start
        cmp     al,9
        je      .next_start
        cmp     al,13
        je      .next_start
        cmp     al,10
        je      .next_start
        cmp     al,'{'
        jne     .no
        jmp     .scan_end_init
.next_start:
        inc     esi
        dec     ecx
        jnz     .scan_start
        jmp     .no
.scan_end_init:
        mov     ecx,[settings_write_len]
        mov     esi,[body_ptr]
        add     esi,ecx
        dec     esi
.scan_end:
        mov     al,[esi]
        cmp     al,' '
        je      .prev_end
        cmp     al,9
        je      .prev_end
        cmp     al,13
        je      .prev_end
        cmp     al,10
        je      .prev_end
        cmp     al,'}'
        jne     .no
        mov     edi,json_translate_key
        mov     edx,json_translate_key_len
        call    json_has_true
        cmp     eax,1
        jne     .no
        mov     edi,json_provider_key
        mov     edx,json_provider_key_len
        call    json_has_true
        cmp     eax,1
        jne     .no
        mov     eax,1
        ret
.prev_end:
        dec     esi
        dec     ecx
        jnz     .scan_end
.no:
        xor     eax,eax
        ret

parse_settings_flags:
        mov     edi,json_startup_true
        mov     edx,json_startup_true_len
        call    json_has_true
        mov     [startup_mode],eax
        mov     edi,json_start_minimized_true
        mov     edx,json_start_minimized_true_len
        call    json_has_true
        mov     [startup_minimized],eax
        cmp     dword [startup_mode],1
        je      .startup_ok
        mov     dword [startup_minimized],0
.startup_ok:
        mov     edi,json_lan_true
        mov     edx,json_lan_true_len
        call    json_has_true
        mov     [lan_allowed],eax
        ret

set_cwd_to_exe_dir:
        invoke  GetModuleFileName,0,exe_path,260
        mov     esi,exe_path
        xor     edi,edi
.scan:
        mov     al,[esi]
        test    al,al
        jz      .found_end
        cmp     al,'\'
        jne     .next
        mov     edi,esi
.next:
        inc     esi
        jmp     .scan
.found_end:
        test    edi,edi
        jz      .done
        mov     byte [edi],0
        invoke  SetCurrentDirectory,exe_path
.done:
        ret

json_has_true:
        mov     esi,[body_ptr]
        mov     ecx,[settings_write_len]
        sub     ecx,edx
        jl      .no
        inc     ecx
.scan:
        push    esi
        push    edi
        push    ecx
        mov     ecx,edx
        repe    cmpsb
        sete    al
        pop     ecx
        pop     edi
        pop     esi
        cmp     al,1
        je      .yes
        inc     esi
        loop    .scan
.no:
        xor     eax,eax
        ret
.yes:
        mov     eax,1
        ret
