enable_lan_listener:
        cmp     dword [lan_enabled],1
        je      .done
        invoke  closesocket,[server_socket]
        invoke  socket,AF_INET,SOCK_STREAM,IPPROTO_TCP
        mov     [server_socket],eax
        cmp     eax,INVALID_SOCKET
        je      .fallback
        invoke  setsockopt,[server_socket],0FFFFh,4,reuse_opt,4
        invoke  htons,19001
        mov     word [server_addr+2],ax
        mov     dword [server_addr+4],0
        invoke  bind,[server_socket],server_addr,16
        cmp     eax,0
        jne     .fallback
        invoke  setsockopt,[server_socket],0FFFFh,1006h,accept_timeout,4
        invoke  listen,[server_socket],8
        mov     dword [lan_enabled],1
        ret
.fallback:
        call    restore_localhost_listener
.done:
        ret

restore_localhost_listener:
        mov     dword [lan_enabled],0
        invoke  closesocket,[server_socket]
        invoke  socket,AF_INET,SOCK_STREAM,IPPROTO_TCP
        mov     [server_socket],eax
        cmp     eax,INVALID_SOCKET
        je      .done
        invoke  setsockopt,[server_socket],0FFFFh,4,reuse_opt,4
        invoke  htons,19001
        mov     word [server_addr+2],ax
        invoke  inet_addr,localhost
        mov     dword [server_addr+4],eax
        invoke  bind,[server_socket],server_addr,16
        cmp     eax,0
        jne     .done
        invoke  setsockopt,[server_socket],0FFFFh,1006h,accept_timeout,4
        invoke  listen,[server_socket],8
.done:
        ret
