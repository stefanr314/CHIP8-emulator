build/bin/chip8 : build/obj/chip8.o
	gcc -Wall -Wextra -pedantic -std=c11 -o build/bin/chip8 build/obj/chip8.o

build/obj/chip8.o : src/chip8.c
	gcc -Wall -pedantic -c src/chip8.c -o build/obj/chip8.o
