bits 16				; Le dice a NASM que es código de 16 bits.
org 0x7c00    ; Le dice a NASM que empiece a trabajar en el offset 0x7c00.

;≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡
; Bootloader.
;≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡
boot:
	mov ax, 0x2401	; Activa el bit A20. Que permite acceder toda la memoria.
	int 0x15

	mov ax, 0x3			; Setea el BIOS en modo de texto 3 en vga.
	int 0x10

;===============================================================================
; Cargar más espacios de memoria usando los servicios de disco.
;===============================================================================
	mov [disk],dl					; Guardamos el disco en memoria

	mov ah, 0x2    				; read sectors
	mov al, 1      				; sectors to read
	mov ch, 0      				; cylinder idx
	mov dh, 0      				; head idx
	mov cl, 2      				; sector idx
	mov dl, [disk] 				; disk idx
	mov bx, copy_target		; target pointer
	int 0x13

	cli										; Limpia la bandera de interrupciones.

;===============================================================================
; Se activan las instrucciones de 32 bits y el acceso a los registros completos.
;===============================================================================
	lgdt [gdt_pointer]		; Carga la tabla GlobalDescriptorTable.
	mov eax, cr0
	or eax,0x1						; Activa el bit de modo protegido.
	mov cr0, eax

;-------------------------------------------------------------------------------
; Se setean los segmentos libres para que apunten al segmento de datos.
;-------------------------------------------------------------------------------
	mov ax, DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	jmp CODE_SEG:boot2		; Salto largo al segmento de código.

;-------------------------------------------------------------------------------
; Definimos los GDT que vamos a usar.
;-------------------------------------------------------------------------------
gdt_start:
	dq 0x0         ; Es la misma estructura de code y data pero está todo en cero.
gdt_code:
	dw 0xFFFF      ; limit  (0-15)
	dw 0x0         ; base   (16-31)
	db 0x0         ; base   (32-39)
	db 10011010b   ; access (40-47)
	db 11001111b   ; limit  (48-51) / flags (52-55)
	db 0x0         ; base   (56-63)
gdt_data:
  dw 0xFFFF      ; limit  (0-15)
  dw 0x0         ; base   (16-31)
  db 0x0         ; base   (32-39)
	db 10010010b   ; access (40-47)
	db 11001111b   ; limit  (48-51) / flags (52-55)
	db 0x0         ; base   (56-63)
gdt_end:

;-------------------------------------------------------------------------------
; Para cargar el gdt se ocupa usar una estructura de puntero.
;-------------------------------------------------------------------------------
gdt_pointer:
	dw gdt_end - gdt_start
	dd gdt_start

disk:
	db 0x0

;-------------------------------------------------------------------------------
; Se definen dos variables que son offsets en el gdt para poder usarlas.
;-------------------------------------------------------------------------------
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510 - ($-$$) db 0	; Rellena los siguientes 510 bytes con 0
dw 0xaa55								; IMPORTANTE - Esto es lo que lo hace bootloader, marca
												; 						 que estos 512 bytes son booteables.

;≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡
; Programa.
;≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡
copy_target:
	bits 32
	hello: db "Hello from outside the 512 bytes world!!",0
boot2:
	mov esi,hello
	mov ebx,0xb8000
.loop:
	lodsb
	or al,al
	jz halt
	or eax,0x0F00
	mov word [ebx], ax
	add ebx,2
	jmp .loop
halt:
	cli
	hlt

times 1024 - ($-$$) db 0	; Rellena los siguientes 1024 bytes con 0, para no
													; copiar los bytes sin inicializar.
