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
        invoke  ReadFile,[settings_handle],recv_buf,prompts_max_size+1,bytes_done,0
        invoke  CloseHandle,[settings_handle]
        mov     eax,[bytes_done]
        mov     [settings_write_len],eax
        mov     dword [body_ptr],recv_buf
        call    prompts_json_valid
        cmp     eax,1
        jne     .empty
        invoke  send,[client_socket],http_200_json,http_200_json_len,0
        invoke  send,[client_socket],recv_buf,[bytes_done],0
        ret
.empty:
        invoke  send,[client_socket],http_200_json,http_200_json_len,0
        invoke  send,[client_socket],empty_prompts_json,empty_prompts_json_len,0
        ret

write_prompts:
        call    find_body
        test    eax,eax
        jz      .bad_request
        mov     esi,eax
        mov     [body_ptr],esi
        call    parse_content_length
        mov     [content_len],eax
        test    eax,eax
        jz      .bad_request
        cmp     eax,prompts_max_size
        ja      .too_large
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
        mov     eax,[body_have]
        cmp     eax,[content_len]
        jb      .bad_request
        mov     ecx,[content_len]
        mov     [settings_write_len],ecx
        call    prompts_json_valid
        cmp     eax,1
        jne     .bad_request
        ; Write a validated snapshot beside the destination, flush it, then
        ; atomically replace prompts.json. A failed write never truncates the
        ; last known-good prompt collection.
        invoke  CreateFile,prompts_temp_file,40000000h,0,0,2,80h,0
        cmp     eax,-1
        je      .write_error
        mov     [settings_handle],eax
        invoke  WriteFile,[settings_handle],[body_ptr],[settings_write_len],bytes_done,0
        mov     ebx,eax
        test    ebx,ebx
        jz      .mark_write_failed
        mov     eax,[bytes_done]
        cmp     eax,[settings_write_len]
        jne     .mark_write_failed
        invoke  FlushFileBuffers,[settings_handle]
        mov     ebx,eax
        jmp     .close_write
.mark_write_failed:
        xor     ebx,ebx
.close_write:
        invoke  CloseHandle,[settings_handle]
        test    ebx,ebx
        jz      .write_error
        invoke  MoveFileEx,prompts_temp_file,prompts_file,MOVEFILE_REPLACE_EXISTING+MOVEFILE_WRITE_THROUGH
        test    eax,eax
        jz      .write_error
        invoke  send,[client_socket],http_204,http_204_len,0
        ret
.bad_request:
        invoke  send,[client_socket],http_400,http_400_len,0
        ret
.too_large:
        jmp     send_too_large
.write_error:
        invoke  DeleteFile,prompts_temp_file
        invoke  send,[client_socket],http_500,http_500_len,0
        ret

; Common text lives beside the other user data files instead of only in the
; browser profile: localStorage is keyed by origin, so a changed port, a
; localhost/127.0.0.1 switch, or a LAN address would each look like data loss.
serve_quicktext:
        invoke  CreateFile,quicktext_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .open_failed
        mov     [settings_handle],eax
        invoke  ReadFile,[settings_handle],recv_buf,quicktext_max_size+1,bytes_done,0
        mov     ebx,eax
        invoke  CloseHandle,[settings_handle]
        test    ebx,ebx
        jz      .error
        mov     eax,[bytes_done]
        mov     [settings_write_len],eax
        mov     dword [body_ptr],recv_buf
        call    quicktext_json_valid
        cmp     eax,1
        jne     .error
        mov     esi,recv_buf
        mov     ecx,[bytes_done]
        call    quicktext_set_etag
        call    send_quicktext_json_header
        invoke  send,[client_socket],recv_buf,[bytes_done],0
        ret
.open_failed:
        invoke  GetLastError
        cmp     eax,2                 ; only a genuinely missing file migrates
        jne     .error
.empty:
        call    quicktext_missing_etag
        call    send_quicktext_json_header
        invoke  send,[client_socket],empty_quicktext_json,empty_quicktext_json_len,0
        ret
.error:
        invoke  send,[client_socket],http_500,http_500_len,0
        ret

send_quicktext_json_header:
        invoke  send,[client_socket],http_200_quicktext,http_200_quicktext_len,0
        jmp     send_quicktext_etag

send_quicktext_etag:
        invoke  send,[client_socket],quicktext_etag,quicktext_etag_len,0
        invoke  send,[client_socket],quicktext_header_end,quicktext_header_end_len,0
        ret

write_quicktext:
        call    find_body
        test    eax,eax
        jz      .bad_request
        mov     esi,eax
        mov     [body_ptr],esi
        call    parse_content_length
        mov     [content_len],eax
        test    eax,eax
        jz      .bad_request
        cmp     eax,quicktext_max_size
        ja      .too_large
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
        mov     eax,[body_have]
        cmp     eax,[content_len]
        jb      .bad_request
        mov     ecx,[content_len]
        mov     [settings_write_len],ecx
        call    quicktext_json_valid
        cmp     eax,1
        jne     .bad_request
        call    quicktext_precondition_ok
        cmp     eax,1
        je      .write
        test    eax,eax
        jz      .conflict
        jmp     .write_error
.write:
        ; Same atomic pattern as prompts: a failed write must never truncate
        ; the last known-good collection.
        invoke  CreateFile,quicktext_temp_file,40000000h,0,0,2,80h,0
        cmp     eax,-1
        je      .write_error
        mov     [settings_handle],eax
        invoke  WriteFile,[settings_handle],[body_ptr],[settings_write_len],bytes_done,0
        mov     ebx,eax
        test    ebx,ebx
        jz      .mark_write_failed
        mov     eax,[bytes_done]
        cmp     eax,[settings_write_len]
        jne     .mark_write_failed
        invoke  FlushFileBuffers,[settings_handle]
        mov     ebx,eax
        jmp     .close_write
.mark_write_failed:
        xor     ebx,ebx
.close_write:
        invoke  CloseHandle,[settings_handle]
        test    ebx,ebx
        jz      .write_error
        invoke  MoveFileEx,quicktext_temp_file,quicktext_file,MOVEFILE_REPLACE_EXISTING+MOVEFILE_WRITE_THROUGH
        test    eax,eax
        jz      .write_error
        mov     esi,[body_ptr]
        mov     ecx,[settings_write_len]
        call    quicktext_set_etag
        invoke  send,[client_socket],http_204_quicktext,http_204_quicktext_len,0
        jmp     send_quicktext_etag
.conflict:
        invoke  send,[client_socket],http_412,http_412_len,0
        ret
.bad_request:
        invoke  send,[client_socket],http_400,http_400_len,0
        ret
.too_large:
        jmp     send_too_large
.write_error:
        invoke  DeleteFile,quicktext_temp_file
        invoke  send,[client_socket],http_500,http_500_len,0
        ret

; Finish the error response before closing a socket with an unread body.
; Otherwise Winsock can reset the connection and hide the HTTP 413. Bound
; both the receive wait and the discarded bytes for untrusted large requests.
send_too_large:
        invoke  send,[client_socket],http_413,http_413_len,0
        invoke  shutdown,[client_socket],1
        invoke  setsockopt,[client_socket],0FFFFh,1006h,drain_timeout,4
        mov     ebp,[content_len]
        mov     eax,[recv_len]
        sub     eax,[body_ptr]
        add     eax,recv_buf
        sub     ebp,eax
        jle     .done
        cmp     ebp,recv_buf_size
        jbe     .read
        mov     ebp,recv_buf_size
.read:
        mov     eax,settings_buf_size
        cmp     ebp,eax
        jae     .receive
        mov     eax,ebp
.receive:
        invoke  recv,[client_socket],settings_buf,eax,0
        test    eax,eax
        jle     .done
        sub     ebp,eax
        jg      .read
.done:
        ret

; A compact content tag prevents stale browser snapshots from replacing a
; newer file. The listener handles requests serially, so the check/write pair
; cannot interleave with another HTTP save. No additional imports or buffers.
quicktext_set_etag:
        mov     eax,811C9DC5h
        call    quicktext_hash_bytes
        jmp     quicktext_format_etag

quicktext_hash_bytes:
        test    ecx,ecx
        jz      .done
.next:
        movzx   edx,byte [esi]
        xor     eax,edx
        imul    eax,eax,01000193h
        inc     esi
        loop    .next
.done:
        ret

quicktext_format_etag:
        mov     edi,quicktext_etag
        mov     byte [edi],'1'
        inc     edi
        mov     ecx,8
.next:
        rol     eax,4
        mov     edx,eax
        and     edx,15
        mov     dl,[quicktext_hex+edx]
        mov     [edi],dl
        inc     edi
        loop    .next
        ret

quicktext_missing_etag:
        mov     dword [quicktext_etag],'miss'
        mov     dword [quicktext_etag+4],'ing0'
        mov     byte [quicktext_etag+8],'0'
        ret

quicktext_file_etag:
        invoke  CreateFile,quicktext_file,80000000h,1,0,3,80h,0
        cmp     eax,-1
        je      .open_failed
        mov     [settings_handle],eax
        mov     ebx,811C9DC5h
        xor     ebp,ebp
.read:
        invoke  ReadFile,[settings_handle],settings_buf,settings_buf_size,bytes_done,0
        test    eax,eax
        jz      .read_failed
        mov     ecx,[bytes_done]
        test    ecx,ecx
        jz      .finish
        add     ebp,ecx
        cmp     ebp,quicktext_max_size
        ja      .read_failed
        mov     esi,settings_buf
        mov     eax,ebx
        call    quicktext_hash_bytes
        mov     ebx,eax
        jmp     .read
.finish:
        invoke  CloseHandle,[settings_handle]
        mov     eax,ebx
        call    quicktext_format_etag
        mov     eax,1
        ret
.read_failed:
        invoke  CloseHandle,[settings_handle]
        xor     eax,eax
        ret
.open_failed:
        invoke  GetLastError
        cmp     eax,2
        jne     .no
        call    quicktext_missing_etag
        mov     eax,1
        ret
.no:
        xor     eax,eax
        ret

quicktext_precondition_ok:
        mov     esi,recv_buf
        mov     ecx,[body_ptr]
        sub     ecx,esi
.scan:
        cmp     ecx,if_match_header_len
        jb      .yes                  ; old API callers may omit If-Match
        cmp     esi,recv_buf
        je      .next
        cmp     byte [esi-1],10
        jne     .next
        push    esi
        push    ecx
        mov     edi,if_match_header
        mov     edx,if_match_header_len
.key:
        mov     al,[esi]
        or      al,20h
        cmp     al,[edi]
        jne     .not_key
        inc     esi
        inc     edi
        dec     edx
        jnz     .key
        pop     ecx
        pop     esi
        add     esi,if_match_header_len
        sub     ecx,if_match_header_len
.space:
        test    ecx,ecx
        jz      .no
        cmp     byte [esi],' '
        je      .consume
        cmp     byte [esi],9
        jne     .value
.consume:
        inc     esi
        dec     ecx
        jmp     .space
.value:
        cmp     ecx,quicktext_etag_len+2
        jb      .no
        cmp     byte [esi],'"'
        jne     .no
        cmp     byte [esi+quicktext_etag_len+1],'"'
        jne     .no
        inc     esi
        mov     [quicktext_if_match],esi
        call    quicktext_file_etag
        test    eax,eax
        jz      .file_error
        mov     esi,[quicktext_if_match]
        mov     edi,quicktext_etag
        mov     ecx,quicktext_etag_len
        repe    cmpsb
        jne     .no
.yes:
        mov     eax,1
        ret
.not_key:
        pop     ecx
        pop     esi
.next:
        inc     esi
        dec     ecx
        jmp     .scan
.no:
        xor     eax,eax
        ret
.file_error:
        mov     eax,2
        ret

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
