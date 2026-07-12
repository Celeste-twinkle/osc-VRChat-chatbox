handle_session_open:
        invoke  send,[client_socket],http_204,http_204_len,0
        jmp     close_client

handle_session_close:
        invoke  send,[client_socket],http_204,http_204_len,0
        jmp     close_client

handle_lan_enable:
        call    enable_lan_listener
        call    serve_lan_ip
        jmp     close_client

handle_lan_ip:
        call    serve_lan_ip
        jmp     close_client

handle_heartbeat:
        call    read_heartbeat_id
        test    eax,eax
        jz      .done
        call    upsert_session
.done:
        invoke  send,[client_socket],http_204,http_204_len,0
        jmp     close_client

close_client:
        invoke  closesocket,[client_socket]
        jmp     accept_loop

exit:
        invoke  ExitProcess,0

open_existing_and_exit:
        invoke  GetCommandLine
        mov     esi,eax
        call    command_line_has_minimized
        cmp     eax,1
        je      .skip_open
        invoke  ShellExecute,0,open_action,url,0,0,1
.skip_open:
        invoke  closesocket,[server_socket]
        invoke  closesocket,[osc_socket]
        invoke  WSACleanup
        invoke  ExitProcess,0

shutdown_now:
        call    remove_tray
        invoke  closesocket,[server_socket]
        invoke  closesocket,[osc_socket]
        invoke  WSACleanup
        invoke  ExitProcess,0
