bits 32
org 0x7c00

boot2:
  mov esi, hello
  mov ebx, 0xb8000
.loop:
  lodsb
  or al,al
  jz halt
  or eax, 0x0100
  mov word [ebx], ax
  add ebx, 2
  jmp .loop
halt:
  cli
  hlt
hello: db "Hello world!",0

times 510 - ($-$$) db 0
dw 0xaa55
