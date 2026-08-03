; Port management: configurable port, auto-increment on conflict, hot
; migration with dual-port transition, runningPort persistence, and
; second-instance notification. Included by server.asm after router.inc.
;
; Owned symbols: init_server_port, update_osc_port, json_get_number,
; json_find_key, persist_running_port, notify_old_instance, build_url,
; build_heartbeat, try_port_migration, start_migration_to,
; tick_port_migration, cancel_port_migration.

init_server_port:
        ; Startup-only: read the configured port from settings.json into
        ; server_port. Runtime port changes go through try_port_migration
        ; (which compares against the CURRENT actual port), never by
        ; overwriting server_port directly.
        mov     edi,json_port_key
        mov     edx,json_port_key_len
        call    json_get_number
        test    eax,eax
        jz      .port_ok
        cmp     eax,65535
        ja      .port_ok
        cmp     eax,1024
        jb      .port_ok
        mov     [server_port],eax
.port_ok:
        mov     edi,json_osc_port_key
        mov     edx,json_osc_port_key_len
        call    json_get_number
        test    eax,eax
        jz      .done
        cmp     eax,65535
        ja      .done
        cmp     eax,1024
        jb      .done
        mov     [osc_port],eax
.done:
        ret

update_osc_port:
        ; Runtime: refresh osc_port from settings.json after a settings save.
        ; Unlike init_server_port this must NOT touch server_port (that would
        ; defeat try_port_migration's comparison).
        mov     edi,json_osc_port_key
        mov     edx,json_osc_port_key_len
        call    json_get_number
        test    eax,eax
        jz      .done
        cmp     eax,65535
        ja      .done
        cmp     eax,1024
        jb      .done
        mov     [osc_port],eax
.done:
        ret

json_get_number:
        ; edi = key, edx = key_len. Returns the number after the key in eax
        ; (0 if not found). Skips whitespace after the key (external editors
        ; may write '"port": 19020').
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
        je      .found
        inc     esi
        loop    .scan
.no:
        xor     eax,eax
        ret
.found:
        add     esi,edx
        ; Skip whitespace (external editors may write '"port": 19020')
.skip_ws:
        mov     bl,[esi]
        cmp     bl,' '
        je      .ws_next
        cmp     bl,9
        je      .ws_next
        jmp     .digits
.ws_next:
        inc     esi
        jmp     .skip_ws
.digits:
        xor     eax,eax
        xor     ebx,ebx
.digits_loop:
        mov     bl,[esi]
        cmp     bl,'0'
        jb      .done
        cmp     bl,'9'
        ja      .done
        imul    eax,eax,10
        sub     bl,'0'
        add     eax,ebx
        inc     esi
        jmp     .digits_loop
.done:
        ret

json_find_key:
        ; Like json_has_true but returns the absolute position of the key in
        ; eax (0 if not found). edi=key, edx=key_len.
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
        mov     eax,esi
        ret

persist_running_port:
        ; Keep the internal locator synchronized with the socket that is
        ; currently authoritative.
        mov     eax,[server_port]
        mov     edi,json_running_key
        mov     edx,json_running_key_len
        jmp     persist_port_value

persist_configured_port:
        ; Runtime conflict fallback may choose a different port from the one
        ; requested. Keep the user-visible setting synchronized with it.
        mov     eax,[new_port]
        mov     edi,json_port_key
        mov     edx,json_port_key_len

persist_port_value:
        ; eax = value, edi = JSON key including colon, edx = key length.
        ; Rewrites only that numeric field and leaves the rest of the compact
        ; settings object untouched.
        mov     [persist_value],eax
        mov     [persist_key],edi
        mov     [persist_key_len],edx
        invoke  CreateFile,settings_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .done
        mov     [settings_handle],eax
        invoke  ReadFile,[settings_handle],settings_buf,settings_buf_size,bytes_done,0
        invoke  CloseHandle,[settings_handle]
        mov     eax,[bytes_done]
        test    eax,eax
        jz      .done
        mov     [settings_write_len],eax
        mov     dword [body_ptr],settings_buf

        ; digits of the selected value -> num_tmp (forward order)
        mov     eax,[persist_value]
        xor     ecx,ecx
.digits:
        xor     edx,edx
        mov     ebx,10
        div     ebx
        add     dl,'0'
        push    edx
        inc     ecx
        test    eax,eax
        jnz     .digits
        mov     edi,num_tmp
.wr:
        pop     eax
        stosb
        loop    .wr
        mov     byte [edi],0
        mov     eax,edi
        sub     eax,num_tmp
        mov     [num_len],eax

        mov     edi,[persist_key]
        mov     edx,[persist_key_len]
        call    json_find_key
        test    eax,eax
        jnz     .replace

        ; ---- not present: insert before the final '}' ----
        mov     esi,[body_ptr]
        add     esi,[settings_write_len]
        dec     esi
.scan_end:
        cmp     byte [esi],'}'
        je      .found_end
        dec     esi
        cmp     esi,[body_ptr]
        jb      .done
        jmp     .scan_end
.found_end:
        mov     [num_ptr],esi           ; '}' position
        mov     eax,esi
        dec     eax
        cmp     byte [eax],'{'
        jne     .need_comma
        xor     ebx,ebx
        jmp     .have_comma
.need_comma:
        mov     ebx,1
.have_comma:
        mov     eax,[num_len]
        add     eax,ebx
        add     eax,[persist_key_len]
        mov     [tmp_len],eax           ; insert length
        mov     eax,[settings_write_len]
        add     eax,[tmp_len]
        cmp     eax,settings_buf_size
        ja      .done
        ; shift [ '}' .. end ] right by insert length
        mov     esi,[num_ptr]
        mov     ecx,[body_ptr]
        add     ecx,[settings_write_len]
        sub     ecx,esi
        mov     edi,esi
        add     edi,[tmp_len]
        add     esi,ecx
        add     edi,ecx
        dec     esi
        dec     edi
        std
        rep     movsb
        cld
        ; write the inserted field
        mov     edi,[num_ptr]
        test    ebx,ebx
        jz      .no_comma
        mov     al,','
        stosb
.no_comma:
        mov     esi,[persist_key]
        mov     ecx,[persist_key_len]
        rep     movsb
        mov     esi,num_tmp
        mov     ecx,[num_len]
        rep     movsb
        mov     eax,[settings_write_len]
        add     eax,[tmp_len]
        mov     [settings_write_len],eax
        jmp     .write_back

.replace:
        ; key exists: eax = its position; replace the number after it.
        mov     esi,eax
        add     esi,[persist_key_len]   ; esi = right after the key
        ; Skip whitespace: external editors may write '"key": 123'
.skip_ws:
        mov     al,[esi]
        cmp     al,' '
        je      .ws_next
        cmp     al,9
        je      .ws_next
        jmp     .ws_done
.ws_next:
        inc     esi
        jmp     .skip_ws
.ws_done:
        mov     [num_ptr],esi           ; esi = start of digits
        mov     edi,esi
.scan_old:
        mov     al,[edi]
        cmp     al,'0'
        jb      .old_end
        cmp     al,'9'
        ja      .old_end
        inc     edi
        jmp     .scan_old
.old_end:
        ; edi = end of old digits, old_len = edi-esi
        mov     eax,edi
        sub     eax,esi
        mov     [tmp_len],eax           ; old_len
        ; tail = [ edi .. body+len )
        mov     ebx,[body_ptr]
        add     ebx,[settings_write_len]
        sub     ebx,edi                 ; tail length
        mov     eax,[tmp_len]
        mov     ecx,[num_len]
        sub     eax,ecx                 ; delta = old_len - new_len
        jz      .copy_new
        jg      .shrink
        ; grow: shift tail right by -delta
        neg     eax
        mov     edx,eax
        add     eax,[settings_write_len]
        cmp     eax,settings_buf_size
        ja      .done
        mov     esi,edi
        mov     edi,esi
        add     edi,edx
        mov     ecx,ebx
        add     esi,ecx
        add     edi,ecx
        dec     esi
        dec     edi
        std
        rep     movsb
        cld
        jmp     .copy_new
.shrink:
        ; shrink: shift tail left by delta (dst = old_end - delta, src = old_end)
        mov     edx,eax
        mov     esi,edi                 ; src = old_end
        mov     edi,esi
        sub     edi,edx                 ; dst = old_end - delta
        mov     ecx,ebx
        rep     movsb
.copy_new:
        mov     edi,[num_ptr]
        mov     esi,num_tmp
        mov     ecx,[num_len]
        rep     movsb
        mov     eax,[settings_write_len]
        add     eax,[num_len]
        sub     eax,[tmp_len]
        mov     [settings_write_len],eax

.write_back:
        invoke  CreateFile,settings_file,40000000h,0,0,2,80h,0
        cmp     eax,-1
        je      .done
        mov     [settings_handle],eax
        invoke  WriteFile,[settings_handle],[body_ptr],[settings_write_len],bytes_done,0
        invoke  CloseHandle,[settings_handle]
.done:
        ret

notify_old_instance:
        ; Read the old instance's actual listener from runningPort, then POST
        ; /reload-settings so it can apply a newly configured target. The
        ; caller opens this known-good listener; its heartbeat will redirect
        ; local pages if a hot migration is in progress.
        invoke  CreateFile,settings_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .build_running_url
        mov     [settings_handle],eax
        invoke  ReadFile,[settings_handle],settings_buf,settings_buf_size,bytes_done,0
        invoke  CloseHandle,[settings_handle]
        mov     eax,[bytes_done]
        test    eax,eax
        jz      .build_running_url
        mov     [settings_write_len],eax
        mov     dword [body_ptr],settings_buf

        ; runningPort = old instance's actual port
        mov     edi,json_running_key
        mov     edx,json_running_key_len
        call    json_get_number
        test    eax,eax
        jz      .build_running_url
        mov     [notify_port],eax

        ; connect to the old instance
        invoke  socket,AF_INET,SOCK_STREAM,IPPROTO_TCP
        mov     [notify_sock],eax
        cmp     eax,INVALID_SOCKET
        je      .build_running_url
        invoke  htons,[notify_port]
        mov     word [notify_addr+2],ax
        invoke  inet_addr,localhost
        mov     dword [notify_addr+4],eax
        invoke  connect,[notify_sock],notify_addr,16
        cmp     eax,0
        jne     .close_sock
        ; POST /reload-settings
        invoke  send,[notify_sock],reload_request,reload_request_len,0
        invoke  closesocket,[notify_sock]
        jmp     .build_running_url
.close_sock:
        invoke  closesocket,[notify_sock]

.build_running_url:
        mov     eax,[notify_port]
        test    eax,eax
        jz      .use_server
        jmp     .have_port
.use_server:
        mov     eax,[server_port]
.have_port:
        mov     [server_port],eax
        call    build_url
        ret

build_url:
        mov     edi,url
        mov     esi,url_prefix
.copy_prefix:
        mov     al,[esi]
        test    al,al
        jz      .prefix_done
        stosb
        inc     esi
        jmp     .copy_prefix
.prefix_done:
        mov     eax,[server_port]
        mov     ecx,num_tmp+16
        mov     byte [ecx],0
        dec     ecx
.digits:
        xor     edx,edx
        mov     ebx,10
        div     ebx
        add     dl,'0'
        mov     [ecx],dl
        dec     ecx
        test    eax,eax
        jnz     .digits
        inc     ecx
.copy_num:
        mov     al,[ecx]
        test    al,al
        jz      .done
        stosb
        inc     ecx
        jmp     .copy_num
.done:
        mov     byte [edi],0
        ret

build_heartbeat:
        ; X-Port tells the page which port is authoritative. If a migration
        ; (port_pending) is in progress, report the new port so the page can
        ; jump; otherwise report the current listening port.
        ; Digits are pushed onto the stack in reverse, then popped out in
        ; order -- no scratch buffers shared with build_url, no rep movsb
        ; clobbering the digit pointer.
        mov     edi,heartbeat_resp
        mov     esi,http_204_head
        mov     ecx,http_204_head_len
        rep     movsb
        mov     esi,xport_prefix
.copy_prefix:
        mov     al,[esi]
        test    al,al
        jz      .prefix_done
        stosb
        inc     esi
        jmp     .copy_prefix
.prefix_done:
        ; NOTE: the copy loop above clobbered eax (al was used as scratch and
        ; ends up 0 on the NUL). Re-load the port here.
        cmp     dword [port_pending],0
        je      .use_current
        mov     eax,[port_pending]
        jmp     .have_num
.use_current:
        mov     eax,[server_port]
.have_num:
        xor     ecx,ecx
.digits:
        xor     edx,edx
        mov     ebx,10
        div     ebx
        add     dl,'0'
        push    edx
        inc     ecx
        test    eax,eax
        jnz     .digits
.write_digits:
        pop     eax
        stosb
        loop    .write_digits
        mov     esi,http_204_tail
        mov     ecx,http_204_tail_len
        rep     movsb
        mov     eax,edi
        sub     eax,heartbeat_resp
        mov     [heartbeat_resp_len],eax
        ret

try_port_migration:
        ; Event-driven: called after POST /settings or /reload-settings.
        ; Re-read settings.json; if the configured port differs from the
        ; current listening port, open an alt listener on the new port while
        ; keeping the old one (page migrates via heartbeat X-Port redirect).
        ;
        ; If a migration is already running (port_pending != 0), we do NOT
        ; kill the in-flight alt socket: clients may already be on it, and
        ; closing it would strand them. Instead we remember the newest
        ; target in pending_next and apply it once the current migration
        ; completes (queue-tail semantics). Repeated changes only keep the
        ; latest target.
        cmp     dword [port_pending],0
        jne     .note_pending
        jmp     .read_settings

.note_pending:
        ; Migration in progress. Read the configured port; if it differs
        ; from the in-flight target, stash it as the next target.
        invoke  CreateFile,settings_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .done
        mov     [settings_handle],eax
        invoke  ReadFile,[settings_handle],settings_buf,settings_buf_size,bytes_done,0
        invoke  CloseHandle,[settings_handle]
        mov     eax,[bytes_done]
        test    eax,eax
        jz      .done
        mov     [settings_write_len],eax
        mov     dword [body_ptr],settings_buf
        mov     edi,json_port_key
        mov     edx,json_port_key_len
        call    json_get_number
        test    eax,eax
        jz      .done
        cmp     eax,[port_pending]
        je      .done                    ; same target, nothing to note
        cmp     eax,1024
        jb      .done
        cmp     eax,65535
        ja      .done
        mov     [pending_next],eax       ; remember newest, overwrite older
        ret

.read_settings:
        invoke  CreateFile,settings_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .done
        mov     [settings_handle],eax
        invoke  ReadFile,[settings_handle],settings_buf,settings_buf_size,bytes_done,0
        invoke  CloseHandle,[settings_handle]
        mov     eax,[bytes_done]
        test    eax,eax
        jz      .done
        mov     [settings_write_len],eax
        mov     dword [body_ptr],settings_buf
        mov     edi,json_port_key
        mov     edx,json_port_key_len
        call    json_get_number
        test    eax,eax
        jz      .done
        cmp     eax,[server_port]
        je      .done
        cmp     eax,1024
        jb      .done
        cmp     eax,65535
        ja      .done
        ; New port differs -> open alt listener (shared helper)
        call    start_migration_to
.done:
        ret

start_migration_to:
        ; Start a migration to the port in eax. If that port is occupied,
        ; scan upward for at most 100 candidates and persist the selected
        ; fallback. Opens the alt listener and sets port_pending/activity.
        ; Returns eax=0 only when no candidate can be opened.
        mov     [new_port],eax
        mov     dword [bind_tries],100
.retry:
        invoke  socket,AF_INET,SOCK_STREAM,IPPROTO_TCP
        mov     [alt_socket],eax
        cmp     eax,INVALID_SOCKET
        je      .give_up
        invoke  setsockopt,[alt_socket],0FFFFh,SO_EXCLUSIVEADDRUSE,reuse_opt,4
        invoke  htons,[new_port]
        mov     word [alt_addr+2],ax
        mov     eax,[lan_enabled]
        test    eax,eax
        jnz     .lan
        invoke  inet_addr,localhost
        mov     dword [alt_addr+4],eax
        jmp     .bind
.lan:
        mov     dword [alt_addr+4],0
.bind:
        invoke  bind,[alt_socket],alt_addr,16
        cmp     eax,0
        jne     .try_next
        invoke  setsockopt,[alt_socket],0FFFFh,1006h,accept_timeout,4
        invoke  listen,[alt_socket],8
        cmp     eax,0
        jne     .try_next
        call    persist_configured_port
        mov     eax,[new_port]
        mov     [port_pending],eax
        invoke  GetTickCount
        mov     [old_last_activity],eax
        mov     eax,1
        ret
.try_next:
        invoke  closesocket,[alt_socket]
        mov     dword [alt_socket],0
        cmp     dword [new_port],65535
        jae     .give_up
        inc     dword [new_port]
        dec     dword [bind_tries]
        jnz     .retry
.give_up:
        ; Keep settings truthful if every candidate failed.
        mov     eax,[server_port]
        mov     [new_port],eax
        call    persist_configured_port
        xor     eax,eax
        ret

tick_port_migration:
        ; Called from accept_loop while port_pending != 0. The old port is
        ; dropped only once it has been idle for a while -- every connection
        ; on it bumps old_last_activity -- so clients still polling the old
        ; port are never cut off. This includes tunnel/proxy clients, which
        ; cannot follow the X-Port redirect and keep heartbeating the old
        ; port; as long as anything connects, the old port stays alive, and
        ; it is closed 15 s after the last connection.
        invoke  GetTickCount
        mov     [now_tick],eax
        mov     eax,[now_tick]
        sub     eax,[old_last_activity]
        cmp     eax,15000               ; 15 s idle on the old port
        jae     .switch
        ret
.switch:
        mov     eax,[alt_socket]
        test    eax,eax
        jz      .clear
        invoke  closesocket,[server_socket]
        mov     eax,[alt_socket]
        mov     [server_socket],eax
        mov     dword [alt_socket],0
        mov     eax,[port_pending]
        mov     [server_port],eax
        mov     dword [port_pending],0
        call    build_url
        call    persist_running_port
        ; If newer targets were requested during this migration, start the
        ; next leg now (pending_next holds the newest one).
        cmp     dword [pending_next],0
        je      .done
        mov     eax,[pending_next]
        mov     dword [pending_next],0
        cmp     eax,[server_port]
        je      .done
        call    start_migration_to
.done:
        ret
.clear:
        mov     dword [port_pending],0
        jmp     .done

cancel_port_migration:
        cmp     dword [alt_socket],0
        je      .reset
        invoke  closesocket,[alt_socket]
        mov     dword [alt_socket],0
.reset:
        mov     dword [port_pending],0
        mov     dword [pending_next],0
        ret
