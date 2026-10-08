; Bounded JSON grammar/UTF-8 validation, without extra DLL imports.
; Parsing routines consume esi/ecx, return eax=1/0, and preserve edi/ebp.
; ebx is the remaining nesting budget. Store validators preserve ebx/edi/ebp.
json_object_ok:
        push    ebx
        push    edi
        push    ebp
        mov     esi,[body_ptr]
        mov     ecx,[settings_write_len]
        mov     ebx,64
        call    json_skip_space
        test    ecx,ecx
        jz      .no
        cmp     byte [esi],'{'
        jne     .no
        call    json_parse_object
        test    eax,eax
        jz      .done
        call    json_skip_space
        test    ecx,ecx
        jnz     .no
        mov     eax,1
        jmp     .done
.no:
        xor     eax,eax
.done:
        pop     ebp
        pop     edi
        pop     ebx
        ret

json_skip_space:
        test    ecx,ecx
        jz      .done
.next:
        mov     al,[esi]
        cmp     al,' '
        je      .consume
        cmp     al,9
        je      .consume
        cmp     al,10
        je      .consume
        cmp     al,13
        jne     .done
.consume:
        inc     esi
        dec     ecx
        jnz     .next
.done:
        ret

json_parse_value:
        call    json_skip_space
        test    ecx,ecx
        jz      json_invalid
        mov     al,[esi]
        cmp     al,'"'
        je      json_parse_string
        cmp     al,'{'
        je      json_parse_object
        cmp     al,'['
        je      json_parse_array
        cmp     al,'t'
        je      .true
        cmp     al,'f'
        je      .false
        cmp     al,'n'
        je      .null
        jmp     json_parse_number
.true:
        cmp     ecx,4
        jb      json_invalid
        cmp     dword [esi],'true'
        jne     json_invalid
        mov     edx,4
        jmp     .literal
.false:
        cmp     ecx,5
        jb      json_invalid
        cmp     dword [esi],'fals'
        jne     json_invalid
        cmp     byte [esi+4],'e'
        jne     json_invalid
        mov     edx,5
        jmp     .literal
.null:
        cmp     ecx,4
        jb      json_invalid
        cmp     dword [esi],'null'
        jne     json_invalid
        mov     edx,4
.literal:
        add     esi,edx
        sub     ecx,edx
        mov     eax,1
        ret

json_parse_object:
        dec     ebx
        js      .depth_fail
        inc     esi
        dec     ecx
        call    json_skip_space
        test    ecx,ecx
        jz      .no
        cmp     byte [esi],'}'
        je      .close
.key:
        test    ecx,ecx
        jz      .no
        cmp     byte [esi],'"'
        jne     .no
        call    json_parse_string
        test    eax,eax
        jz      .no
        call    json_skip_space
        test    ecx,ecx
        jz      .no
        cmp     byte [esi],':'
        jne     .no
        inc     esi
        dec     ecx
        call    json_parse_value
        test    eax,eax
        jz      .no
        call    json_skip_space
        test    ecx,ecx
        jz      .no
        cmp     byte [esi],'}'
        je      .close
        cmp     byte [esi],','
        jne     .no
        inc     esi
        dec     ecx
        call    json_skip_space
        jmp     .key
.close:
        inc     esi
        dec     ecx
        inc     ebx
        mov     eax,1
        ret
.no:
        inc     ebx
        jmp     json_invalid
.depth_fail:
        inc     ebx
        jmp     json_invalid

json_parse_array:
        dec     ebx
        js      .depth_fail
        inc     esi
        dec     ecx
        call    json_skip_space
        test    ecx,ecx
        jz      .no
        cmp     byte [esi],']'
        je      .close
.value:
        call    json_parse_value
        test    eax,eax
        jz      .no
        call    json_skip_space
        test    ecx,ecx
        jz      .no
        cmp     byte [esi],']'
        je      .close
        cmp     byte [esi],','
        jne     .no
        inc     esi
        dec     ecx
        jmp     .value
.close:
        inc     esi
        dec     ecx
        inc     ebx
        mov     eax,1
        ret
.no:
        inc     ebx
        jmp     json_invalid
.depth_fail:
        inc     ebx
        jmp     json_invalid

json_parse_string:
        inc     esi
        dec     ecx
.next:
        test    ecx,ecx
        jz      json_invalid
        mov     al,[esi]
        cmp     al,'"'
        je      .close
        cmp     al,20h
        jb      json_invalid
        cmp     al,'\'
        je      .escape
        cmp     al,80h
        jae     .utf8
.ascii:
        inc     esi
        dec     ecx
        jmp     .next
.escape:
        inc     esi
        dec     ecx
        jz      json_invalid
        mov     al,[esi]
        cmp     al,'"'
        je      .ascii
        cmp     al,'\'
        je      .ascii
        cmp     al,'/'
        je      .ascii
        cmp     al,'b'
        je      .ascii
        cmp     al,'f'
        je      .ascii
        cmp     al,'n'
        je      .ascii
        cmp     al,'r'
        je      .ascii
        cmp     al,'t'
        je      .ascii
        cmp     al,'u'
        jne     json_invalid
        cmp     ecx,5
        jb      json_invalid
        mov     edx,4
.hex:
        inc     esi
        dec     ecx
        mov     al,[esi]
        cmp     al,'0'
        jb      json_invalid
        cmp     al,'9'
        jbe     .hex_ok
        or      al,20h
        cmp     al,'a'
        jb      json_invalid
        cmp     al,'f'
        ja      json_invalid
.hex_ok:
        dec     edx
        jnz     .hex
        jmp     .ascii
.utf8:
        cmp     al,0C2h
        jb      json_invalid
        cmp     al,0DFh
        jbe     .two
        cmp     al,0EFh
        jbe     .three
        cmp     al,0F4h
        ja      json_invalid
        cmp     ecx,4
        jb      json_invalid
        cmp     al,0F0h
        jne     .not_f0
        cmp     byte [esi+1],90h
        jb      json_invalid
.not_f0:
        cmp     al,0F4h
        jne     .four_tail
        cmp     byte [esi+1],8Fh
        ja      json_invalid
.four_tail:
        mov     al,[esi+3]
        and     al,0C0h
        cmp     al,80h
        jne     json_invalid
        mov     edx,4
        jmp     .three_tail
.three:
        cmp     ecx,3
        jb      json_invalid
        cmp     al,0E0h
        jne     .not_e0
        cmp     byte [esi+1],0A0h
        jb      json_invalid
.not_e0:
        cmp     al,0EDh
        jne     .three_ready
        cmp     byte [esi+1],9Fh
        ja      json_invalid
.three_ready:
        mov     edx,3
.three_tail:
        mov     al,[esi+2]
        and     al,0C0h
        cmp     al,80h
        jne     json_invalid
        jmp     .continuation
.two:
        cmp     ecx,2
        jb      json_invalid
        mov     edx,2
.continuation:
        mov     al,[esi+1]
        and     al,0C0h
        cmp     al,80h
        jne     json_invalid
        add     esi,edx
        sub     ecx,edx
        jmp     .next
.close:
        inc     esi
        dec     ecx
        mov     eax,1
        ret

json_parse_number:
        cmp     byte [esi],'-'
        jne     .integer
        inc     esi
        dec     ecx
        jz      json_invalid
.integer:
        cmp     byte [esi],'0'
        je      .zero
        cmp     byte [esi],'1'
        jb      json_invalid
        cmp     byte [esi],'9'
        ja      json_invalid
        call    json_digits
        jmp     .fraction
.zero:
        inc     esi
        dec     ecx
.fraction:
        test    ecx,ecx
        jz      .yes
        cmp     byte [esi],'.'
        jne     .exponent
        inc     esi
        dec     ecx
        call    json_digits
        test    eax,eax
        jz      json_invalid
.exponent:
        test    ecx,ecx
        jz      .yes
        mov     al,[esi]
        or      al,20h
        cmp     al,'e'
        jne     .yes
        inc     esi
        dec     ecx
        jz      json_invalid
        cmp     byte [esi],'+'
        je      .sign
        cmp     byte [esi],'-'
        jne     .exp_digits
.sign:
        inc     esi
        dec     ecx
.exp_digits:
        call    json_digits
        ret
.yes:
        mov     eax,1
        ret

json_digits:
        xor     edx,edx
.next:
        test    ecx,ecx
        jz      .done
        cmp     byte [esi],'0'
        jb      .done
        cmp     byte [esi],'9'
        ja      .done
        inc     edx
        inc     esi
        dec     ecx
        jmp     .next
.done:
        xor     eax,eax
        test    edx,edx
        setnz   al
        ret

json_invalid:
        xor     eax,eax
        ret

; Compare a validated JSON key with an ASCII field name, decoding \u escapes.
; edi points at the opening quote, esi just after the closing quote, edx at
; the expected zero-terminated name. Preserve the surrounding parser state.
json_key_equals:
        push    ebx
        push    edi
        push    ecx
        push    esi
        mov     ebx,esi
        dec     ebx
        mov     esi,edx
        inc     edi
.next:
        cmp     edi,ebx
        je      .end_key
        movzx   eax,byte [edi]
        inc     edi
        cmp     al,'\'
        jne     .compare
        cmp     byte [edi],'u'
        jne     .no
        inc     edi
        xor     edx,edx
        mov     ecx,4
.hex:
        movzx   eax,byte [edi]
        cmp     al,'9'
        jbe     .digit
        or      al,20h
        sub     eax,'a'-10
        jmp     .nibble
.digit:
        sub     eax,'0'
.nibble:
        shl     edx,4
        or      edx,eax
        inc     edi
        loop    .hex
        mov     eax,edx
        cmp     eax,127
        ja      .no
.compare:
        cmp     byte [esi],0
        je      .no
        cmp     al,[esi]
        jne     .no
        inc     esi
        jmp     .next
.end_key:
        xor     eax,eax
        cmp     byte [esi],0
        sete    al
        jmp     .done
.no:
        xor     eax,eax
.done:
        pop     esi
        pop     ecx
        pop     edi
        pop     ebx
        ret

; Require a root items array with at most 100 objects, each carrying string
; id/text fields. Extra properties are allowed and checked by the JSON parser.
quicktext_schema_ok:
        push    ebx
        push    edi
        push    ebp
        mov     esi,[body_ptr]
        mov     ecx,[settings_write_len]
        mov     ebx,64
        xor     ebp,ebp
        call    json_skip_space
        inc     esi
        dec     ecx
        call    json_skip_space
        cmp     byte [esi],'}'
        je      .no
.key:
        mov     edi,esi
        call    json_parse_string
        mov     edx,json_quick_items_name
        call    json_key_equals
        mov     edx,eax
.colon:
        call    json_skip_space
        inc     esi
        dec     ecx
        call    json_skip_space
        test    edx,edx
        jz      .other
        test    ebp,ebp
        jnz     .no
        mov     ebp,1
        cmp     byte [esi],'['
        jne     .no
        call    quicktext_array_ok
        jmp     .value_done
.other:
        call    json_parse_value
.value_done:
        test    eax,eax
        jz      .no
        call    json_skip_space
        cmp     byte [esi],'}'
        je      .finish_value
        inc     esi
        dec     ecx
        call    json_skip_space
        jmp     .key
.finish_value:
        mov     eax,ebp
        jmp     .done
.no:
        xor     eax,eax
.done:
        pop     ebp
        pop     edi
        pop     ebx
        ret

quicktext_array_ok:
        push    edi
        xor     edi,edi
        inc     esi
        dec     ecx
        call    json_skip_space
        cmp     byte [esi],']'
        je      .yes
.item:
        cmp     byte [esi],'{'
        jne     .no
        inc     edi
        cmp     edi,100
        ja      .no
        call    quicktext_item_ok
        test    eax,eax
        jz      .no
        call    json_skip_space
        cmp     byte [esi],']'
        je      .yes
        inc     esi
        dec     ecx
        call    json_skip_space
        jmp     .item
.yes:
        inc     esi
        dec     ecx
        mov     eax,1
        jmp     .done
.no:
        xor     eax,eax
.done:
        pop     edi
        ret

quicktext_item_ok:
        push    edi
        push    ebp
        xor     ebp,ebp
        inc     esi
        dec     ecx
        call    json_skip_space
        cmp     byte [esi],'}'
        je      .no
.key:
        mov     edi,esi
        call    json_parse_string
        mov     edx,json_quick_id_name
        call    json_key_equals
        test    eax,eax
        jz      .text
        mov     edx,1
        jmp     .colon
.text:
        mov     edx,json_quick_text_name
        call    json_key_equals
        lea     edx,[eax+eax]
.colon:
        call    json_skip_space
        inc     esi
        dec     ecx
        call    json_skip_space
        test    edx,edx
        jz      .other
        test    ebp,edx
        jnz     .no
        or      ebp,edx
        cmp     byte [esi],'"'
        jne     .no
        call    json_parse_string
        jmp     .value_done
.other:
        call    json_parse_value
.value_done:
        test    eax,eax
        jz      .no
        call    json_skip_space
        cmp     byte [esi],'}'
        je      .finish_value
        inc     esi
        dec     ecx
        call    json_skip_space
        jmp     .key
.finish_value:
        cmp     ebp,3
        jne     .no
        inc     esi
        dec     ecx
        mov     eax,1
        jmp     .done
.no:
        xor     eax,eax
.done:
        pop     ebp
        pop     edi
        ret
