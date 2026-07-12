start:
        call    set_cwd_to_exe_dir
        call    load_settings_flags

        invoke  WSAStartup,0202h,wsa_data

        invoke  socket,AF_INET,SOCK_DGRAM,IPPROTO_UDP
        mov     [osc_socket],eax
        invoke  htons,9000
        mov     word [osc_addr+2],ax
        invoke  inet_addr,localhost
        mov     dword [osc_addr+4],eax

        invoke  socket,AF_INET,SOCK_STREAM,IPPROTO_TCP
        mov     [server_socket],eax
        cmp     eax,INVALID_SOCKET
        je      exit

        invoke  htons,19001
        mov     word [server_addr+2],ax
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
        invoke  bind,[server_socket],server_addr,16
        cmp     eax,0
        jne     open_existing_and_exit
        invoke  setsockopt,[server_socket],0FFFFh,1006h,accept_timeout,4
        invoke  listen,[server_socket],8

        call    setup_tray
        invoke  GetCommandLine
        mov     esi,eax
        call    command_line_has_minimized
        cmp     eax,1
        je      accept_loop
        invoke  ShellExecute,0,open_action,url,0,0,1
