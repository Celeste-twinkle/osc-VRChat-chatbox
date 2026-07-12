command_line_has_lan:
.scan:
        mov     al,[esi]
        test    al,al
        jz      .no
        cmp     al,'-'
        je      .check_dash
        cmp     al,'/'
        je      .check_slash
        inc     esi
        jmp     .scan
.check_dash:
        cmp     byte [esi+1],'-'
        jne     .next
        cmp     byte [esi+2],'l'
        jne     .next
        cmp     byte [esi+3],'a'
        jne     .next
        cmp     byte [esi+4],'n'
        jne     .next
        jmp     .yes
.check_slash:
        cmp     byte [esi+1],'l'
        jne     .next
        cmp     byte [esi+2],'a'
        jne     .next
        cmp     byte [esi+3],'n'
        jne     .next
        jmp     .yes
.next:
        inc     esi
        jmp     .scan
.yes:
        mov     eax,1
        ret
.no:
        xor     eax,eax
        ret

command_line_has_minimized:
.scan:
        mov     al,[esi]
        test    al,al
        jz      .no
        cmp     al,'-'
        je      .check_dash
        cmp     al,'/'
        je      .check_slash
        inc     esi
        jmp     .scan
.check_dash:
        cmp     byte [esi+1],'-'
        jne     .next
        cmp     byte [esi+2],'m'
        jne     .next
        cmp     byte [esi+3],'i'
        jne     .next
        cmp     byte [esi+4],'n'
        jne     .next
        cmp     byte [esi+5],'i'
        jne     .next
        cmp     byte [esi+6],'m'
        jne     .next
        cmp     byte [esi+7],'i'
        jne     .next
        cmp     byte [esi+8],'z'
        jne     .next
        cmp     byte [esi+9],'e'
        jne     .next
        cmp     byte [esi+10],'d'
        jne     .next
        jmp     .yes
.check_slash:
        cmp     byte [esi+1],'m'
        jne     .next
        cmp     byte [esi+2],'i'
        jne     .next
        cmp     byte [esi+3],'n'
        jne     .next
        cmp     byte [esi+4],'i'
        jne     .next
        cmp     byte [esi+5],'m'
        jne     .next
        cmp     byte [esi+6],'i'
        jne     .next
        cmp     byte [esi+7],'z'
        jne     .next
        cmp     byte [esi+8],'e'
        jne     .next
        cmp     byte [esi+9],'d'
        jne     .next
        jmp     .yes
.next:
        inc     esi
        jmp     .scan
.yes:
        mov     eax,1
        ret
.no:
        xor     eax,eax
        ret
