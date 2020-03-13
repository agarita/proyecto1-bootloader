cli_exec:
	nasm -f elf32 boot.asm -o boot.o
	gcc -m32 -ffreestanding -fno-PIE -c kmain.c -o kmain.o
	ld -melf_i386 -T linker.ld kmain.o boot.o -o kernel.bin.
	qemu-system-i386 -fda kernel.bin

clean:
	rm -rf *.o *.bin *.elf
