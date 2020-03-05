bits 16									; Permite instrucciones de 16 bits
org 0x7c00

boot:
	mov si,hello					; ah=0x0e int 0x10 means 'Write Character in TTY mode'
	mov ah, 0x0e
.loop:
	lodsb									; loads byte at address 'ds:si' into 'al'
	or al,al
	jz halt
	int 0x10
	jmp .loop
halt:
	cli										; clear interrupts
	hlt										; halt cpu
hello: db "Hello world!",0

times 510 - ($-$$) db 0	; pad reamining 510 bytes with 0
dw 0xaa55
