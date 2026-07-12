find_host_lan_ip:
        mov     dword [best_ip_score],-1
        mov     byte [best_ip],0
        call    scan_interfaces_best
        cmp     dword [best_ip_score],-1
        jg      .found
        xor     eax,eax
        ret
.found:
        mov     eax,1
        ret

score_ip:
        cmp     byte [esi],'1'
        jne     .not_1
        cmp     byte [esi+1],'9'
        jne     .check_10
        cmp     byte [esi+2],'2'
        jne     .zero
        cmp     byte [esi+3],'.'
        jne     .zero
        cmp     byte [esi+4],'1'
        jne     .zero
        cmp     byte [esi+5],'6'
        jne     .zero
        cmp     byte [esi+6],'8'
        jne     .zero
        cmp     byte [esi+7],'.'
        jne     .zero
        mov     eax,3
        ret
.check_10:
        cmp     byte [esi+1],'0'
        jne     .check_172
        cmp     byte [esi+2],'.'
        jne     .check_172
        mov     eax,2
        ret
.check_172:
        cmp     byte [esi+1],'7'
        jne     .zero
        cmp     byte [esi+2],'2'
        jne     .zero
        cmp     byte [esi+3],'.'
        jne     .zero
        lea     edi,[esi+4]
        call    parse_octet
        cmp     eax,16
        jb      .zero
        cmp     eax,31
        ja      .zero
        mov     eax,1
        ret
.not_1:
        jmp     .zero
.zero:
        xor     eax,eax
        ret

parse_octet:
        xor     eax,eax
        xor     ecx,ecx
.loop:
        mov     bl,[edi]
        cmp     bl,'0'
        jb      .done
        cmp     bl,'9'
        ja      .done
        imul    eax,eax,10
        sub     bl,'0'
        movzx   ebx,bl
        add     eax,ebx
        inc     edi
        inc     ecx
        cmp     ecx,3
        jb      .loop
.done:
        ret

send_lan_json:
        cmp     byte [best_ip],0
        je      .selected_ready
        mov     esi,best_ip
.selected_ready:
        mov     edi,selected_ip
        mov     ecx,15
.copy_selected:
        lodsb
        stosb
        test    al,al
        jz      .selected_done
        loop    .copy_selected
        mov     byte [edi],0
.selected_done:
        mov     edi,lan_json
        mov     esi,lan_json_head
        mov     ecx,lan_json_head_len
        rep     movsb
        mov     esi,selected_ip
        call    append_raw_ip
        mov     esi,lan_json_mid
        mov     ecx,lan_json_mid_len
        rep     movsb
        mov     dword [lan_first],1
        mov     esi,selected_ip
        call    append_json_ip
        mov     [lan_json_ptr],edi

        call    append_interfaces_to_lan_json
.list_done:
        mov     edi,[lan_json_ptr]
        mov     esi,lan_json_tail
        mov     ecx,lan_json_tail_len
        rep     movsb
        mov     eax,edi
        sub     eax,lan_json
        mov     [lan_json_len],eax
        invoke  send,[client_socket],http_200_json,http_200_json_len,0
        invoke  send,[client_socket],lan_json,[lan_json_len],0
        ret

append_raw_ip:
        mov     ecx,15
.raw_loop:
        lodsb
        test    al,al
        jz      .raw_done
        stosb
        loop    .raw_loop
.raw_done:
        ret

append_json_ip:
        cmp     dword [lan_first],1
        je      .first
        mov     al,','
        stosb
        jmp     .quote
.first:
        mov     dword [lan_first],0
.quote:
        mov     al,'"'
        stosb
        call    append_raw_ip
        mov     al,'"'
        stosb
        ret

ip_equals_selected:
        mov     edi,selected_ip
.compare:
        mov     al,[esi]
        cmp     al,[edi]
        jne     .no
        test    al,al
        jz      .yes
        inc     esi
        inc     edi
        jmp     .compare
.yes:
        mov     eax,1
        ret
.no:
        xor     eax,eax
        ret

scan_interfaces_best:
        call    load_interface_list
        cmp     eax,1
        jne     .done
        mov     edi,ip_table+4
        mov     ecx,[if_count]
.loop:
        test    ecx,ecx
        jz      .done
        mov     [if_ptr],edi
        push    ecx
        invoke  inet_ntoa,dword [edi]
        test    eax,eax
        jz      .next
        mov     esi,eax
        call    usable_ip_string
        cmp     eax,1
        jne     .next
        mov     esi,if_text
        call    score_ip
        cmp     eax,[best_ip_score]
        jle     .next
        mov     [best_ip_score],eax
        mov     esi,if_text
        mov     edi,best_ip
        mov     ecx,15
.copy_best:
        lodsb
        stosb
        test    al,al
        jz      .copied
        loop    .copy_best
        mov     byte [edi],0
.copied:
        cmp     dword [best_ip_score],3
        je      .finish_pop
.next:
        pop     ecx
        mov     edi,[if_ptr]
        add     edi,24
        dec     ecx
        jmp     .loop
.finish_pop:
        pop     ecx
.done:
        ret

append_interfaces_to_lan_json:
        call    load_interface_list
        cmp     eax,1
        jne     .done
        mov     edi,ip_table+4
        mov     ecx,[if_count]
.loop:
        test    ecx,ecx
        jz      .done
        mov     [if_ptr],edi
        push    ecx
        invoke  inet_ntoa,dword [edi]
        test    eax,eax
        jz      .next
        mov     esi,eax
        call    usable_ip_string
        cmp     eax,1
        jne     .next
        mov     esi,if_text
        call    ip_equals_selected
        cmp     eax,1
        je      .next
        mov     edi,[lan_json_ptr]
        mov     esi,if_text
        call    append_json_ip
        mov     [lan_json_ptr],edi
.next:
        pop     ecx
        mov     edi,[if_ptr]
        add     edi,24
        dec     ecx
        jmp     .loop
.done:
        ret

load_interface_list:
        mov     dword [ip_table_len],ip_table_size
        invoke  GetIpAddrTable,ip_table,ip_table_len,0
        test    eax,eax
        jnz     .fail
        mov     eax,dword [ip_table]
        mov     [if_count],eax
        mov     eax,1
        ret
.fail:
        xor     eax,eax
        ret

usable_ip_string:
        mov     edi,if_text
        mov     ecx,15
.copy:
        lodsb
        stosb
        test    al,al
        jz      .copied
        loop    .copy
        mov     byte [edi],0
.copied:
        cmp     byte [if_text],0
        je      .no
        cmp     dword [if_text],'0.0.'
        jne     .not_zero
        cmp     dword [if_text+4],'0.0'
        je      .no
.not_zero:
        cmp     byte [if_text],'1'
        jne     .yes
        cmp     byte [if_text+1],'2'
        jne     .yes
        cmp     byte [if_text+2],'7'
        jne     .yes
        cmp     byte [if_text+3],'.'
        je      .no
.yes:
        mov     eax,1
        ret
.no:
        xor     eax,eax
        ret
