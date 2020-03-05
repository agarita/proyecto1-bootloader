# proyecto1-bootloader
Proyecto 1 del curso de Principios de Sistemas Operativos

## Boot 1
Programa que imprime un Hola Mundo! en 16 bits. Solo puede correr programas de hasta 1 MB de memoria

## Boot 2
Programa que imprime un Hola Mundo! en modo protegido de 32 bits. Para usar más de 1 MB de memoria, se debe activar la línea A20 con la función 'A20-Gate activate'. https://wiki.osdev.org/A20_Line .

## Como correr el programa en QEMU
Para compilar el programa en NASM.
```
nasm -f bin boot1.asm -o boot1.bin
```

Para ejecutar el programa en arquitectura x86 en QEMU.
```
qemu-system-x86_64 -fda boot1.bin
```
