find_body:
        mov     esi,recv_buf
        mov     ecx,[recv_len]
        sub     ecx,3
        jle     .not_found

.scan:
        cmp     dword [esi],0A0D0A0Dh
        je      .found
        inc     esi
        loop    .scan

.not_found:
        xor     eax,eax
        ret

.found:
        lea     eax,[esi+4]
        ret

parse_content_length:
        mov     esi,recv_buf
        mov     ecx,[recv_len]

.next:
        cmp     ecx,15
        jb      .zero
        push    esi
        mov     edi,content_length_header
        mov     edx,15

.cmp:
        mov     al,[esi]
        cmp     al,'a'
        jb      .case_ok
        cmp     al,'z'
        ja      .case_ok
        sub     al,32
.case_ok:
        cmp     al,[edi]
        jne     .no
        inc     esi
        inc     edi
        dec     edx
        jnz     .cmp
        pop     esi
        add     esi,15
        xor     eax,eax
.digits:
        mov     bl,[esi]
        cmp     bl,' '
        je      .skip
        cmp     bl,'0'
        jb      .done
        cmp     bl,'9'
        ja      .done
        imul    eax,eax,10
        sub     bl,'0'
        movzx   ebx,bl
        add     eax,ebx
.skip:
        inc     esi
        jmp     .digits
.done:
        ret

.no:
        pop     esi
        inc     esi
        dec     ecx
        jmp     .next

.zero:
        xor     eax,eax
        ret

serve_index:
        invoke  send,[client_socket],http_200_html,http_200_html_len,0
        invoke  send,[client_socket],html,html_len,0
        ret

serve_history:
        invoke  CreateFile,history_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .empty
        mov     [settings_handle],eax
        invoke  ReadFile,[settings_handle],recv_buf,recv_buf_size-1,bytes_done,0
        invoke  CloseHandle,[settings_handle]
        invoke  send,[client_socket],http_200_json,http_200_json_len,0
        mov     eax,[bytes_done]
        test    eax,eax
        jz      .empty_body
        invoke  send,[client_socket],recv_buf,[bytes_done],0
        ret
.empty:
        invoke  send,[client_socket],http_200_json,http_200_json_len,0
.empty_body:
        invoke  send,[client_socket],empty_history_json,empty_history_json_len,0
        ret

write_history:
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
.read_more:
        mov     eax,[body_have]
        cmp     eax,[content_len]
        jge     .len_ready
        mov     eax,recv_buf
        add     eax,[recv_len]
        mov     edx,recv_buf_size-1
        sub     edx,[recv_len]
        jle     .len_ready
        invoke  recv,[client_socket],eax,edx,0
        cmp     eax,0
        jle     .len_ready
        add     [recv_len],eax
        add     [body_have],eax
        jmp     .read_more
.len_ready:
        mov     ecx,[content_len]
        cmp     ecx,[body_have]
        jle     .have_ok
        mov     ecx,[body_have]
.have_ok:
        cmp     ecx,recv_buf_size-1
        jle     .size_ok
        mov     ecx,recv_buf_size-1
.size_ok:
        mov     [settings_write_len],ecx
        invoke  CreateFile,history_file,40000000h,0,0,2,80h,0
        cmp     eax,-1
        je      send_no_content
        mov     [settings_handle],eax
        invoke  WriteFile,[settings_handle],[body_ptr],[settings_write_len],bytes_done,0
        invoke  CloseHandle,[settings_handle]
        jmp     send_no_content

serve_prompts:
        invoke  CreateFile,prompts_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .empty
        mov     [settings_handle],eax
        invoke  ReadFile,[settings_handle],recv_buf,recv_buf_size-1,bytes_done,0
        invoke  CloseHandle,[settings_handle]
        invoke  send,[client_socket],http_200_json,http_200_json_len,0
        mov     eax,[bytes_done]
        test    eax,eax
        jz      .empty_body
        invoke  send,[client_socket],recv_buf,[bytes_done],0
        ret
.empty:
        invoke  send,[client_socket],http_200_json,http_200_json_len,0
.empty_body:
        invoke  send,[client_socket],empty_prompts_json,empty_prompts_json_len,0
        ret

write_prompts:
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
.read_more:
        mov     eax,[body_have]
        cmp     eax,[content_len]
        jge     .len_ready
        mov     eax,recv_buf
        add     eax,[recv_len]
        mov     edx,recv_buf_size-1
        sub     edx,[recv_len]
        jle     .len_ready
        invoke  recv,[client_socket],eax,edx,0
        cmp     eax,0
        jle     .len_ready
        add     [recv_len],eax
        add     [body_have],eax
        jmp     .read_more
.len_ready:
        mov     ecx,[content_len]
        cmp     ecx,[body_have]
        jle     .have_ok
        mov     ecx,[body_have]
.have_ok:
        cmp     ecx,recv_buf_size-1
        jle     .size_ok
        mov     ecx,recv_buf_size-1
.size_ok:
        mov     [settings_write_len],ecx
        invoke  CreateFile,prompts_file,40000000h,0,0,2,80h,0
        cmp     eax,-1
        je      send_no_content
        mov     [settings_handle],eax
        invoke  WriteFile,[settings_handle],[body_ptr],[settings_write_len],bytes_done,0
        invoke  CloseHandle,[settings_handle]
        jmp     send_no_content

serve_settings:
        invoke  CreateFile,settings_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .empty
        mov     [settings_handle],eax
        invoke  ReadFile,[settings_handle],settings_buf,settings_buf_size,bytes_done,0
        invoke  CloseHandle,[settings_handle]
        mov     eax,[bytes_done]
        mov     [settings_write_len],eax
        mov     dword [body_ptr],settings_buf
        call    settings_json_valid
        cmp     eax,1
        jne     .empty
        invoke  send,[client_socket],http_200_json,http_200_json_len,0
        invoke  send,[client_socket],settings_buf,[bytes_done],0
        ret

.empty:
        call    write_default_settings
        invoke  send,[client_socket],http_200_json,http_200_json_len,0
        invoke  send,[client_socket],default_settings,default_settings_len,0
        ret

serve_lan_ip:
        cmp     dword [lan_enabled],1
        jne     .fallback
        call    find_host_lan_ip
        cmp     eax,1
        je      .send_best
        invoke  socket,AF_INET,SOCK_DGRAM,IPPROTO_UDP
        mov     [tmp_socket],eax
        cmp     eax,INVALID_SOCKET
        je      .fallback
        invoke  htons,80
        mov     word [tmp_addr+2],ax
        invoke  inet_addr,dns_probe_ip
        mov     dword [tmp_addr+4],eax
        invoke  connect,[tmp_socket],tmp_addr,16
        mov     dword [sockaddr_len],16
        invoke  getsockname,[tmp_socket],local_addr,sockaddr_len
        invoke  closesocket,[tmp_socket]
        cmp     eax,0
        jne     .fallback
        invoke  inet_ntoa,dword [local_addr+4]
        test    eax,eax
        jz      .fallback
        mov     esi,eax
        call    send_lan_json
        ret
.send_best:
        mov     esi,best_ip
        call    send_lan_json
        ret
.fallback:
        invoke  send,[client_socket],http_200_json,http_200_json_len,0
        invoke  send,[client_socket],lan_fallback_json,lan_fallback_json_len,0
        ret
