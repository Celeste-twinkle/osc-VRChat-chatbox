; HTML UI embedded from the standalone ui/ project build output.
; html_len is defined here (html comes from html.inc via the `file` directive).
include '..\ui\dist\html.inc'
html_len = $ - html
