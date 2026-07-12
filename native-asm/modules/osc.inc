send_osc:
        push    ebp
        mov     ebp,esp
        mov     esi,[ebp+8]
        mov     ebx,[ebp+12]
        mov     edi,osc_packet

        mov     esi,osc_addr_text
        mov     ecx,osc_addr_text_len
        call    write_osc_string

        cmp     dword [notify_sfx],1
        je      .types_ready
        mov     byte [osc_types+3],'F'
.types_ready:
        mov     esi,osc_types
        mov     ecx,osc_types_len
        call    write_osc_string
        mov     byte [osc_types+3],'T'

        mov     esi,[ebp+8]
        mov     ecx,ebx
        call    write_osc_string

        mov     eax,edi
        sub     eax,osc_packet
        invoke  sendto,[osc_socket],osc_packet,eax,0,osc_addr,16
        pop     ebp
        ret     8

send_typing_osc:
        push    ebp
        mov     ebp,esp
        mov     edi,osc_packet
        mov     esi,osc_typing_addr
        mov     ecx,osc_typing_addr_len
        call    write_osc_string
        mov     eax,[ebp+8]
        test    eax,eax
        jz      .do_false
        mov     esi,osc_typing_true
        mov     ecx,osc_typing_true_len
        jmp     .do_write
.do_false:
        mov     esi,osc_typing_false
        mov     ecx,osc_typing_false_len
.do_write:
        call    write_osc_string
        mov     eax,edi
        sub     eax,osc_packet
        invoke  sendto,[osc_socket],osc_packet,eax,0,osc_addr,16
        pop     ebp
        ret     4

write_osc_string:
        push    ecx
        rep     movsb
        mov     byte [edi],0
        inc     edi
        pop     ecx
        inc     ecx

.pad:
        test    ecx,3
        jz      .done
        mov     byte [edi],0
        inc     edi
        inc     ecx
        jmp     .pad

.done:
        ret
