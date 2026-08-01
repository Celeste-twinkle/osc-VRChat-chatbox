start:
        call    set_cwd_to_exe_dir
        call    load_settings_flags

        invoke  WSAStartup,0202h,wsa_data

        invoke  socket,AF_INET,SOCK_DGRAM,IPPROTO_UDP
        mov     [osc_socket],eax
        invoke  htons,[osc_port]
        mov     word [osc_addr+2],ax
        invoke  inet_addr,localhost
        mov     dword [osc_addr+4],eax

        invoke  socket,AF_INET,SOCK_STREAM,IPPROTO_TCP
        mov     [server_socket],eax
        cmp     eax,INVALID_SOCKET
        je      exit

        ; Single-instance check: a named mutex is system-wide, so a second
        ; process (even a different exe path) reliably detects the first.
        invoke  CreateMutex,0,1,app_mutex
        invoke  GetLastError
        cmp     eax,ERROR_ALREADY_EXISTS
        je      open_existing_and_exit

        invoke  GetCommandLine
        mov     esi,eax
        call    command_line_has_lan
        cmp     eax,1
        je      bind_lan
        cmp     dword [lan_allowed],1
        jne     bind_localhost
bind_lan:
        mov     dword [lan_enabled],1
        mov     dword [server_addr+4],0
        jmp     bind_ready

bind_localhost:
        invoke  inet_addr,localhost
        mov     dword [server_addr+4],eax

bind_ready:
        mov     dword [bind_tries],100
.bind_retry:
        invoke  htons,[server_port]
        mov     word [server_addr+2],ax
        invoke  bind,[server_socket],server_addr,16
        cmp     eax,0
        je      .bound
        inc     dword [server_port]
        invoke  closesocket,[server_socket]
        invoke  socket,AF_INET,SOCK_STREAM,IPPROTO_TCP
        mov     [server_socket],eax
        cmp     eax,INVALID_SOCKET
        je      exit
        dec     dword [bind_tries]
        jnz     .bind_retry
        jmp     open_existing_and_exit
.bound:
        call    build_url
        invoke  setsockopt,[server_socket],0FFFFh,1006h,accept_timeout,4
        invoke  listen,[server_socket],8
        call    persist_running_port

        call    setup_tray
        invoke  GetCommandLine
        mov     esi,eax
        call    command_line_has_minimized
        cmp     eax,1
        je      accept_loop
        invoke  ShellExecute,0,open_action,url,0,0,1
