bits 16       ; Le dice a NASM que es código de 16 bits
org 0x7c00    ; Le dice a NASM que empiece a trabajar en el offset 0x7c00

;≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡
; Inicializamos nuestro ambiente de trabajo.
;≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡
boot:
	mov ax, 0x2401 ; Activa el bit A20. Que permite acceder toda la memoria.
	int 0x15

  mov ax, 0x3    ; Setea el BIOS en modo de texto 3 en vga.
	int 0x10

	cli            ; limpia la bandera de interrupciones

;===============================================================================
; Se activan las instrucciones de 32 bits y el acceso a los registros completos.
;===============================================================================
	lgdt [gdt_pointer]   ; Carga la tabla GlobalDescriptorTable.
	mov eax, cr0
	or eax,0x1           ; Activa el bit de modo protegido
	mov cr0, eax
	jmp CODE_SEG:boot2   ; Salto largo al código de segmento

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

;-------------------------------------------------------------------------------
; Se definen dos variables que son offsets en el gdt para poder usarlas.
;-------------------------------------------------------------------------------
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

;≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡
; Ahora ya se puede usar el modo de 32 bits.
;≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡
bits 32

;===============================================================================
; Se setea los segmentos libres para que apunten al segmento de datos.
;===============================================================================
boot2:
	mov ax, DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

;-------------------------------------------------------------------------------
; Ahora ya no podemos llamar al BIOS pero podemos escribir en el buffer de texto
; VGA. Este está en la memoria asignada a la ubicación 0xB8000.
;-------------------------------------------------------------------------------
	mov esi,hello
	mov ebx,0xb8000

;-------------------------------------------------------------------------------
; Cualquier carácter en pantalla tiene la siguiente estructura:
;   backgroud_color  (0-3)  0xX---
;   foreground_color (4-7)  0x-X--
;   ascii_character  (8-15) 0x--XX
; Entonces se debe agregar 0x0100 para hacer azul todos los caracteres.
;-------------------------------------------------------------------------------
.loop:
	lodsb              ; Carga un byte en la dirección 'ds:si' a 'al'
	or al,al           ; al = 0 ?
	jz halt            ; Si no hay más caracteres, salga
	or eax,0x0100      ; Setea el color de texto a azul
	mov word [ebx], ax ; Movemos los 2 bytes carácter para que quede pegado al eax
	add ebx, 2         ; Se mueve 2 posiciones para caer sobre el próximo carácter
	jmp .loop          ; Siguiente carácter

halt:
	cli                ; limpia la bandera de interrupciones
	hlt						   	 ; detiene la ejecución

hello: db "Hello world!",0

times 510 - ($-$$) db 0 ; rellena los siguientes 510 bytes con 0
dw 0xaa55               ; IMPORTANTE - Esto es lo que lo hace bootloader, marca
												; 						 que estos 512 bytes son booteables.
