accept_loop:
        call    process_messages
        cmp     dword [quit_requested],1
        je      shutdown_now
        call    cleanup_sessions
        mov     dword [readfds],1
        mov     eax,[server_socket]
        mov     [readfds+4],eax
        mov     dword [select_timeout],1
        mov     dword [select_timeout+4],0
        invoke  select,0,readfds,0,0,select_timeout
        cmp     eax,0
        jle     accept_loop
        invoke  accept,[server_socket],0,0
        cmp     eax,INVALID_SOCKET
        je      accept_loop

        mov     [client_socket],eax
        mov     dword [shutdown_pending],0
        invoke  setsockopt,[client_socket],0FFFFh,1006h,recv_timeout,4
        invoke  recv,[client_socket],recv_buf,recv_buf_size-1,0
        cmp     eax,0
        jle     close_client

        mov     [recv_len],eax
        mov     byte [recv_buf+eax],0

        call    is_post_send
        cmp     eax,1
        je      handle_send

        call    is_get_settings
        cmp     eax,1
        je      handle_get_settings

        call    is_post_settings
        cmp     eax,1
        je      handle_post_settings

        call    is_get_history
        cmp     eax,1
        je      handle_get_history

        call    is_post_history
        cmp     eax,1
        je      handle_post_history

        call    is_post_session_open
        cmp     eax,1
        je      handle_session_open

        call    is_post_session_close
        cmp     eax,1
        je      handle_session_close

        call    is_post_heartbeat
        cmp     eax,1
        je      handle_heartbeat

        call    is_post_lan_enable
        cmp     eax,1
        je      handle_lan_enable

        call    is_get_lan_ip
        cmp     eax,1
        je      handle_lan_ip

        call    is_post_typing
        cmp     eax,1
        je      handle_typing

        call    is_post_notify_sfx
        cmp     eax,1
        je      handle_notify_sfx

        call    serve_index
        jmp     close_client

handle_typing:
        call    find_body
        test    eax,eax
        jz      send_no_content
        mov     esi,eax
        cmp     byte [esi],'t'
        jne     .off
        push    1
        call    send_typing_osc
        jmp     send_no_content
.off:
        push    0
        call    send_typing_osc
        jmp     send_no_content

handle_notify_sfx:
        call    find_body
        test    eax,eax
        jz      send_no_content
        mov     esi,eax
        cmp     byte [esi],'t'
        jne     .off
        mov     dword [notify_sfx],1
        jmp     send_no_content
.off:
        mov     dword [notify_sfx],0
        jmp     send_no_content

handle_send:
        call    find_body
        test    eax,eax
        jz      send_no_content

        mov     esi,eax
        mov     ecx,[recv_len]
        sub     ecx,esi
        add     ecx,recv_buf
        cmp     ecx,7900
        jle     .len_ok
        mov     ecx,7900

.len_ok:
        push    ecx
        push    esi
        call    send_osc

send_no_content:
        invoke  send,[client_socket],http_204,http_204_len,0
        jmp     close_client

handle_get_settings:
        call    serve_settings
        jmp     close_client

handle_get_history:
        call    serve_history
        jmp     close_client

handle_post_history:
        call    write_history
        jmp     close_client

handle_post_settings:
        call    find_body
        test    eax,eax
        jz      send_no_content

        mov     esi,eax
        mov     [body_ptr],esi
        call    parse_content_length
        mov     [content_len],eax
        mov     ecx,[recv_len]
        sub     ecx,[body_ptr]
        add     ecx,recv_buf
        mov     [body_have],ecx

.read_more_settings:
        mov     eax,[body_have]
        cmp     eax,[content_len]
        jge     .settings_len_ready
        mov     eax,recv_buf
        add     eax,[recv_len]
        mov     edx,recv_buf_size-1
        sub     edx,[recv_len]
        jle     .settings_len_ready
        invoke  recv,[client_socket],eax,edx,0
        cmp     eax,0
        jle     .settings_len_ready
        add     [recv_len],eax
        add     [body_have],eax
        jmp     .read_more_settings

.settings_len_ready:
        mov     ecx,[content_len]
        cmp     ecx,[body_have]
        jle     .settings_have_ok
        mov     ecx,[body_have]
.settings_have_ok:
        cmp     ecx,settings_buf_size
        jle     .settings_len_ok
        mov     ecx,settings_buf_size

.settings_len_ok:
        mov     [settings_write_len],ecx
        invoke  CreateFile,settings_file,40000000h,0,0,2,80h,0
        cmp     eax,-1
        je      send_no_content
        mov     [settings_handle],eax
        invoke  WriteFile,[settings_handle],[body_ptr],[settings_write_len],bytes_done,0
        invoke  CloseHandle,[settings_handle]
        call    settings_json_valid
        cmp     eax,1
        je      .settings_valid
        call    use_default_settings_flags
        call    update_startup_setting
        invoke  send,[client_socket],http_204,http_204_len,0
        jmp     close_client
.settings_valid:
        call    parse_settings_flags
        call    update_startup_setting
        invoke  send,[client_socket],http_204,http_204_len,0
        jmp     close_client
