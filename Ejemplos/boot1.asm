bits 16				; Le dice a NASM que es código de 16 bits
org 0x7c00		; Le dice a NASM que empiece a trabajar en el offset 0x7c00

boot:
	mov si,hello		; Apunta el registro si a la locación de memoria de la etiqueta hello
	mov ah, 0x0e		; 0x0e significa 'Write Character in TTY mode'

.loop:
	lodsb						; carga un byte de 'ds:si' a 'al'
	or al,al				; al = 0 ?
	jz halt					; si no hay más caracteres, salga
	int 0x10				; corre la interrupción del BIOS 0x10 - Video Services
	jmp .loop				; siguiente carácter

halt:
	cli							; limpia la bandera de interrupciones
	hlt							; detiene la ejecución

hello: db "Hello world!",0

times 510 - ($-$$) db 0	; rellena los siguientes 510 bytes con 0
dw 0xaa55								; IMPORTANTE - Esto es lo que lo hace bootloader, marca
												; 						 que estos 512 bytes son booteables.