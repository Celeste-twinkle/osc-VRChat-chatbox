localhost db '127.0.0.1',0
open_action db 'open',0
url_prefix db 'http://127.0.0.1:',0
url rb 32
num_tmp rb 16
heartbeat_resp rb 128
heartbeat_resp_len dd 0
num_ptr dd 0

post_send db 'POST /send '
post_send_len = $ - post_send
get_settings db 'GET /settings '
get_settings_len = $ - get_settings
post_settings db 'POST /settings '
post_settings_len = $ - post_settings
post_reload_settings db 'POST /reload-settings '
post_reload_settings_len = $ - post_reload_settings
reload_request db 'POST /reload-settings HTTP/1.1',13,10
               db 'Host: 127.0.0.1',13,10
               db 'Content-Length: 0',13,10,13,10
reload_request_len = $ - reload_request
get_history db 'GET /history '
get_history_len = $ - get_history
post_history db 'POST /history '
post_history_len = $ - post_history
post_session_open db 'POST /session/open '
post_session_open_len = $ - post_session_open
post_session_close db 'POST /session/close '
post_session_close_len = $ - post_session_close
post_heartbeat db 'POST /heartbeat'
post_heartbeat_len = $ - post_heartbeat
post_lan_enable db 'POST /lan-enable '
post_lan_enable_len = $ - post_lan_enable
get_lan_ip db 'GET /lan-ip '
get_lan_ip_len = $ - get_lan_ip
post_typing db 'POST /typing '
post_typing_len = $ - post_typing
post_notify_sfx db 'POST /notify-sfx '
post_notify_sfx_len = $ - post_notify_sfx
content_length_header db 'CONTENT-LENGTH:'
settings_file db 'settings.json',0
history_file db 'history.json',0
dns_probe_ip db '8.8.8.8',0
startup_key db 'Software\Microsoft\Windows\CurrentVersion\Run',0
startup_value db 'VRC Chatbox OSC',0
startup_arg db ' --startup',0
startup_minimized_arg db ' --minimized',0
json_startup_true db '"startup":true'
json_startup_true_len = $ - json_startup_true
json_start_minimized_true db '"startMinimized":true'
json_start_minimized_true_len = $ - json_start_minimized_true
json_lan_true db '"lan":true'
json_lan_true_len = $ - json_lan_true
json_startup_key db '"startup":'
json_startup_key_len = $ - json_startup_key
json_start_minimized_key db '"startMinimized":'
json_start_minimized_key_len = $ - json_start_minimized_key
json_lan_key db '"lan":'
json_lan_key_len = $ - json_lan_key
json_translate_key db '"translate"'
json_translate_key_len = $ - json_translate_key
json_provider_key db '"provider"'
json_provider_key_len = $ - json_provider_key
json_port_key db '"port":'
json_port_key_len = $ - json_port_key
json_osc_port_key db '"oscPort":'
json_osc_port_key_len = $ - json_osc_port_key
json_running_key db '"runningPort":'
json_running_key_len = $ - json_running_key
tray_class db 'VRCChatboxOSCTrayWindow',0
app_mutex db 'VRCChatboxOSCSingleInstance',0
tray_title db 'VRC Chatbox OSC',0
tray_tip db 'VRC Chatbox OSC',0
tray_tip_len = $ - tray_tip
tray_menu_open_en_w du 'Open UI',0
tray_menu_exit_en_w du 'Exit',0
tray_menu_lan_en_w du 'Allow LAN access',0
tray_menu_startup_en_w du 'Start with Windows',0
tray_menu_start_minimized_en_w du 'Start minimized',0
tray_menu_open_zh_w dw 06253h,05f00h,0055h,0049h,0
tray_menu_exit_zh_w dw 09000h,051fah,0
tray_menu_lan_zh_w dw 5141h,8bb8h,5c40h,57dfh,7f51h,8fdeh,63a5h,0
tray_menu_startup_zh_w dw 5f00h,673ah,81eah,542fh,52a8h,0
tray_menu_start_minimized_zh_w dw 6700h,5c0fh,5316h,5f00h,673ah,81eah,542fh,52a8h,0
tray_menu_open_ja_w dw 0055h,0049h,03092h,0958bh,0304fh,0
tray_menu_exit_ja_w dw 07d42h,04e86h,0
tray_menu_lan_ja_w dw 004ch,0041h,004eh,63a5h,7d9ah,3092h,8a31h,53efh,0
tray_menu_startup_ja_w dw 0057h,0069h,006eh,0064h,006fh,0077h,0073h,8d77h,52d5h,6642h,306bh,958bh,59cbh,0
tray_menu_start_minimized_ja_w dw 6700h,5c0fh,5316h,8d77h,52d5h,0
tray_menu_open_ko_w dw 0055h,0049h,0020h,0c5f4h,0ae30h,0
tray_menu_exit_ko_w dw 0c885h,0b8cch,0
tray_menu_lan_ko_w dw 004ch,0041h,004eh,0020h,0c811h,0c18dh,0020h,0d5c8h,0c6a9h,0
tray_menu_startup_ko_w dw 0057h,0069h,006eh,0064h,006fh,0077h,0073h,0020h,0c2dch,0c791h,0020h,0c2dch,0020h,0c2e4h,0d589h,0
tray_menu_start_minimized_ko_w dw 0cd5ch,0c18ch,0d654h,0020h,0c2dch,0c791h,0

http_200_html db 'HTTP/1.1 200 OK',13,10
              db 'Connection: close',13,10
              db 'Content-Type: text/html; charset=utf-8',13,10,13,10
http_200_html_len = $ - http_200_html

http_200_json db 'HTTP/1.1 200 OK',13,10
              db 'Connection: close',13,10
              db 'Content-Type: application/json; charset=utf-8',13,10,13,10
http_200_json_len = $ - http_200_json

http_204 db 'HTTP/1.1 204 No Content',13,10
         db 'Connection: close',13,10
         db 'Content-Length: 0',13,10,13,10
http_204_len = $ - http_204

http_204_head db 'HTTP/1.1 204 No Content',13,10
              db 'Connection: close',13,10
http_204_head_len = $ - http_204_head
xport_prefix db 'X-Port: ',0
http_204_tail db 13,10
              db 'Content-Length: 0',13,10,13,10
http_204_tail_len = $ - http_204_tail

empty_json db '{}'
empty_json_len = $ - empty_json
empty_history_json db '[]'
empty_history_json_len = $ - empty_history_json
default_settings db '{"translate":true,"src":"zh-CN","dst":"en","provider":"mymemory","endpoint":"","model":"","key":"","mmEmail":"","mmKey":"","format":"both","uiLang":"auto","startup":false,"startMinimized":false,"lan":false,"port":19001,"oscPort":9000}'
default_settings_len = $ - default_settings
reuse_opt dd 1
lan_fallback_json db '{"ip":"127.0.0.1","ips":["127.0.0.1"]}'
lan_fallback_json_len = $ - lan_fallback_json
lan_json_head db '{"ip":"'
lan_json_head_len = $ - lan_json_head
lan_json_mid db '","ips":['
lan_json_mid_len = $ - lan_json_mid
lan_json_tail db ']}'
lan_json_tail_len = $ - lan_json_tail
lan_json_len dd 0

osc_addr_text db '/chatbox/input'
osc_addr_text_len = $ - osc_addr_text
osc_types db ',sTT'
osc_types_len = $ - osc_types
osc_typing_addr db '/chatbox/typing'
osc_typing_addr_len = $ - osc_typing_addr
osc_typing_true db ',T'
osc_typing_true_len = $ - osc_typing_true
osc_typing_false db ',F'
osc_typing_false_len = $ - osc_typing_false
