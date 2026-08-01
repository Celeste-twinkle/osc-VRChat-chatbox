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
        call    build_heartbeat
        invoke  send,[client_socket],heartbeat_resp,[heartbeat_resp_len],0
        jmp     close_client

close_client:
        invoke  closesocket,[client_socket]
        jmp     accept_loop

exit:
        invoke  ExitProcess,0

open_existing_and_exit:
        ; We're a second instance: an old one is already running (mutex) or
        ; we couldn't bind. Tell the old instance to re-read settings.json
        ; (it may have a new port configured), wait a moment for the
        ; migration, then open the configured port in the browser and exit.
        call    notify_old_instance
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
