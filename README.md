# Proyecto1: Bootloader
Proyecto 1 del curso de Principios de Sistemas Operativos

## Boot 1
***
Programa que imprime un Hola Mundo! en 16 bits.

Solo puede correr programas de 512 bytes de memoria. Escribe directamente sobre el buffer de texto que haya actualmente en el BIOS. Lee carácter por carácter, hasta que no hayan más, y los va imprimiendo en pantalla por medio de la interrupción del  BIOS 0x10. Finalmente se rellena el resto de los 512 bytes con 0's, ya que los bootloader simples tienen definido como *booteables* los 512 bytes que vea.

Este ejemplo es tomado de: http://3zanders.co.uk/2017/10/13/writing-a-bootloader/

## Boot 2
***
Programa que imprime un Hola Mundo! en modo protegido de 32 bits.

Para poder usar el modo protegido de 32 bits se deben cumplir dos condiciones:
1. Activar las instrucciones de 32 bits.
2. Dar acceso a los registros completos de 32 bits.

Un programa de 512 bytes de memoria no es suficiente para hacer programas muy complejos. Para poder usar más de 1 MB de memoria, se le debe pedir permiso al BIOS para acceder a toda la memoria, para lo cuál debe activar la [línea A20](https://wiki.osdev.org/A20_Line) con la función *A20-Gate activate*.
Para activar las instrucciones de 32 bits y dar acceso a los registros completos se debe activar el bit de modo de la CPU y setear una [*Global Descriptor Table*](https://en.wikipedia.org/wiki/Global_Descriptor_Table), que define un segmento de 32 bits.
En el caso de este ejemplo se van a tener 3 GDT, un segmento nulo, un segmento de código y otro segmento de datos. La estructura de cada uno de los GDT es:
* **Base**: 32 bits que definen donde empieza el segmento.
* **Límite**: 20 bits que describen donde termina el segmento. Si la granularidad es 1 entonces se multiplica por 4096.
* **Acceso**: 8 bits sobre los permisos de este segmento.
  * **Presente**: 1 bit que debe ser uno para que sea válido.
  * **Privilegio**: 2 bits que asignan el anillo de seguridad, 0 para el más alto (kernel) y 3 para el más bajo (aplicaciones de usuario).
  * **Tipo de Descriptor**: 1 bit que debe estar activo para los segmentos de código o datos y debería estar limpio para segmentos del sistema.
  * **Ejecutable**: 1 bit que si está activo el segmento puede ser ejecutado, si está limpio es un segmento de datos.
  * **Dirección**: 1 bit el cuál indica si está activo de que el segmento puede ser ejecutado desde niveles inferiores de privilegio. Si es 0 solo puede ser ejecutado en el nivel asignado en el bit de **Privilegio**.
  * **Lectura/Escritura**: 1 bit que define si puede o no leer y escribir en este segmento.
  * **Accedido**: 1 bit que dice si el CPU a accedido el segmento. Siempre se setea en 0, cuando el CPU lo visita se encarga de activarlo.
* **Bandera**: 4 bits que activa banderas para informar de algo al CPU. Originalmente solo tenía 2 banderas, pero en la arquitectura **x86-64** se agrega 1 bandera más.
  * **Granularidad**: 1 bit que si está desactivado marca que el límite está en bloques de 1 byte, si está prendido, el límite es en múltiplos de bloques de 4 KB.
  * **Tamaño**: 1 bit que define el modo del segmento, cuando está apagado está en modo de 16 bits y cuando está prendido está en modo de 32 bits protegido.
  * **L**: 1 bit que indica el descriptor de código de **x86-64**. Está reservado para los segmentos de datos.
La estructura del GDT está organizada de la siguiente manera:
![Diseño de un GDT](/Ayudas/GDT_Entry_Layout.png)

En este ejemplo se realiza una pequeña variación al hola mundo de boot1, en este caso se cambia el color del texto en el modo de texto de VGA. Para esto es importante conocer la estructura del [buffer de texto de VGA](https://en.wikipedia.org/wiki/VGA-compatible_text_mode). Estos tienen la siguiente estructura:
* **Color de fondo**: 4 bits que determinan el color del fondo del carácter. Son 16 colores posibles.
* **Color de texto**: 4 bits que determinan el color del carácter. Son 16 colores posibles.
* **Código del carácter**: 8 bits del código ASCII del carácter.

Este ejemplo es tomado de: http://3zanders.co.uk/2017/10/16/writing-a-bootloader2/

## Como correr el programa en QEMU
***
Para compilar el programa en NASM.
```
nasm -f bin bootX.asm -o bootX.bin
```

Para ejecutar el programa en arquitectura x86 en QEMU.
```
qemu-system-x86_64 -fda bootX.bin
```
