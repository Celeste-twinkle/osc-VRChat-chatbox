is_post_send:
        mov     esi,recv_buf
        mov     edi,post_send
        mov     ecx,post_send_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_get_settings:
        mov     esi,recv_buf
        mov     edi,get_settings
        mov     ecx,get_settings_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_post_settings:
        mov     esi,recv_buf
        mov     edi,post_settings
        mov     ecx,post_settings_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_post_reload_settings:
        mov     esi,recv_buf
        mov     edi,post_reload_settings
        mov     ecx,post_reload_settings_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_get_history:
        mov     esi,recv_buf
        mov     edi,get_history
        mov     ecx,get_history_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_post_history:
        mov     esi,recv_buf
        mov     edi,post_history
        mov     ecx,post_history_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_get_prompts:
        mov     esi,recv_buf
        mov     edi,get_prompts
        mov     ecx,get_prompts_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_post_prompts:
        mov     esi,recv_buf
        mov     edi,post_prompts
        mov     ecx,post_prompts_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_post_session_open:
        mov     esi,recv_buf
        mov     edi,post_session_open
        mov     ecx,post_session_open_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_post_session_close:
        mov     esi,recv_buf
        mov     edi,post_session_close
        mov     ecx,post_session_close_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_post_heartbeat:
        mov     esi,recv_buf
        mov     edi,post_heartbeat
        mov     ecx,post_heartbeat_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_post_lan_enable:
        mov     esi,recv_buf
        mov     edi,post_lan_enable
        mov     ecx,post_lan_enable_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_get_lan_ip:
        mov     esi,recv_buf
        mov     edi,get_lan_ip
        mov     ecx,get_lan_ip_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_post_typing:
        mov     esi,recv_buf
        mov     edi,post_typing
        mov     ecx,post_typing_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret

is_post_notify_sfx:
        mov     esi,recv_buf
        mov     edi,post_notify_sfx
        mov     ecx,post_notify_sfx_len
        repe    cmpsb
        sete    al
        movzx   eax,al
        ret
