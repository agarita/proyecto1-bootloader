# proyecto1-bootloader
Proyecto 1 del curso de Principios de Sistemas Operativos

## Boot 1
Programa que imprime un Hola Mundo! en 16 bits (http://3zanders.co.uk/2017/10/13/writing-a-bootloader/). Solo puede correr programas de hasta 1 MB de memoria.

## Boot 2
Programa que imprime un Hola Mundo! en modo protegido de 32 bits (http://3zanders.co.uk/2017/10/16/writing-a-bootloader2/). Para usar más de 1 MB de memoria, se debe activar la línea A20 con la función 'A20-Gate activate'(https://wiki.osdev.org/A20_Line). Para usar las instrucciones de 32 bits y los registros completos se debe setear una 'Global Descriptor Table', que define los segmentos (https://en.wikipedia.org/wiki/Global_Descriptor_Table).

## Como correr el programa en QEMU
Para compilar el programa en NASM.
```
nasm -f bin boot1.asm -o boot1.bin
```

Para ejecutar el programa en arquitectura x86 en QEMU.
```
qemu-system-x86_64 -fda boot1.bin
```
