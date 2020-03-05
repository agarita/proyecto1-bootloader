# proyecto1-bootloader
Proyecto 1 del curso de Principios de Sistemas Operativos

## Como correr el programa en QEMU
Para compilar el programa en NASM.
```
nasm -f bin boot1.asm -o boot1.bin
```

Para ejecutar el programa en arquitectura x86 en QEMU.
```
qemu-system-x86_64 -fda boot1.bin
```
