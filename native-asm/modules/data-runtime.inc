server_socket dd 0
client_socket dd 0
osc_socket dd 0
server_port dd 19001
osc_port dd 9000
port_pending dd 0
new_port dd 0
tmp_len dd 0
old_last_activity dd 0
pending_next dd 0
bind_tries dd 100
notify_port dd 0
notify_sock dd 0
alt_socket dd 0
persist_key dd 0
persist_key_len dd 0
persist_value dd 0
num_len dd 0
app_instance dd 0
tray_hwnd dd 0
tray_icon dd 0
tray_menu dd 0
tray_open_text dd 0
tray_exit_text dd 0
tray_lan_text dd 0
tray_startup_text dd 0
tray_start_minimized_text dd 0
quit_requested dd 0
recv_len dd 0
session_count dd 0
shutdown_pending dd 0
lan_enabled dd 0
lan_allowed dd 0
notify_sfx dd 1
startup_mode dd 0
startup_minimized dd 0
json_bool_value dd 0
idle_timeout dd 30000
now_tick dd 0
current_id rb 32
sessions rb session_slots * 32
last_seen rd session_slots
content_len dd 0
body_have dd 0
body_ptr dd 0
settings_handle dd 0
startup_key_handle dd 0
bytes_done dd 0
recv_timeout dd 2000
settings_write_len dd 0
startup_cmd_len dd 0
tmp_socket dd 0
sockaddr_len dd 16
accept_timeout dd 1000
readfds dd 0,0,0
select_timeout dd 1,0
if_ptr dd 0
if_count dd 0
ip_table_len dd 4096
best_ip_score dd 0
best_ip rb 16
if_text rb 16
selected_ip rb 16
lan_first dd 0
lan_json_ptr dd 0
lan_json rb 512
tray_wc WNDCLASS
tray_nid NOTIFYICONDATAA
msg MSG
pt POINT

server_addr dw AF_INET
            dw 0
            dd 0
            rb 8

alt_addr    dw AF_INET
            dw 0
            dd 0
            rb 8

notify_addr dw AF_INET
            dw 0
            dd 0
            rb 8

osc_addr    dw AF_INET
            dw 0
            dd 0
            rb 8

tmp_addr    dw AF_INET
            dw 0
            dd 0
            rb 8

local_addr  dw AF_INET
            dw 0
            dd 0
            rb 8

wsa_data rb 400
recv_buf_size = 65536
recv_buf rb recv_buf_size
settings_buf_size = 4096
settings_buf rb settings_buf_size
exe_path rb 260
startup_cmd rb 320
osc_packet rb 8192
ip_table_size = 4096
ip_table rb ip_table_size
