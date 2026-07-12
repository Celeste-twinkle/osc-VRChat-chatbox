read_heartbeat_id:
        mov     esi,recv_buf
        mov     ecx,[recv_len]
.scan:
        cmp     ecx,3
        jb      .not_found
        cmp     byte [esi],'i'
        jne     .next
        cmp     byte [esi+1],'d'
        jne     .next
        cmp     byte [esi+2],'='
        jne     .next
        add     esi,3
        mov     edi,current_id
        mov     ecx,31
.copy:
        mov     al,[esi]
        cmp     al,' '
        je      .done
        cmp     al,'&'
        je      .done
        cmp     al,13
        je      .done
        cmp     al,10
        je      .done
        test    al,al
        jz      .done
        stosb
        inc     esi
        loop    .copy
.done:
        mov     byte [edi],0
        mov     eax,current_id
        ret
.next:
        inc     esi
        dec     ecx
        jmp     .scan
.not_found:
        xor     eax,eax
        ret

upsert_session:
        invoke  GetTickCount
        mov     [now_tick],eax
        xor     ebx,ebx
.find_loop:
        cmp     ebx,session_slots
        jge     .insert
        mov     edi,sessions
        mov     eax,ebx
        shl     eax,5
        add     edi,eax
        cmp     byte [edi],0
        je      .next_find
        push    ebx
        push    edi
        mov     esi,current_id
        mov     ecx,32
        repe    cmpsb
        pop     edi
        pop     ebx
        je      .update
.next_find:
        inc     ebx
        jmp     .find_loop
.update:
        mov     eax,ebx
        shl     eax,2
        mov     edx,last_seen
        add     edx,eax
        mov     eax,[now_tick]
        mov     [edx],eax
        ret
.insert:
        xor     ebx,ebx
.insert_loop:
        cmp     ebx,session_slots
        jge     .done
        mov     edi,sessions
        mov     eax,ebx
        shl     eax,5
        add     edi,eax
        cmp     byte [edi],0
        je      .write
        inc     ebx
        jmp     .insert_loop
.write:
        mov     esi,current_id
        mov     ecx,32
        rep     movsb
        mov     eax,ebx
        shl     eax,2
        mov     edx,last_seen
        add     edx,eax
        mov     eax,[now_tick]
        mov     [edx],eax
.done:
        ret

cleanup_sessions:
        invoke  GetTickCount
        mov     [now_tick],eax
        xor     ebx,ebx
        xor     esi,esi
.loop:
        cmp     ebx,session_slots
        jge     .done
        mov     edi,sessions
        mov     eax,ebx
        shl     eax,5
        add     edi,eax
        cmp     byte [edi],0
        je      .next
        mov     eax,ebx
        shl     eax,2
        mov     edx,last_seen
        add     edx,eax
        mov     eax,[now_tick]
        sub     eax,[edx]
        cmp     eax,[idle_timeout]
        jbe     .active
        mov     byte [edi],0
        jmp     .next
.active:
        inc     esi
.next:
        inc     ebx
        jmp     .loop
.done:
        ret
