build/bin/chip8 : build/obj/chip8.o build/obj/loader.o
	gcc -Wall -Wextra -pedantic -std=c11 -o build/bin/chip8 build/obj/chip8.o build/obj/loader.o

build/obj/chip8.o : src/chip8.c include/loader.h
	gcc -Wall -Wextra -pedantic -std=c11 -c src/chip8.c -o build/obj/chip8.o

build/obj/loader.o : src/loader.c include/loader.h
	gcc -Wall -Wextra -pedantic -std=c11 -c src/loader.c -o build/obj/loader.o

build/debug/chip8 : src/chip8.c
	gcc -Wall -pedantic -std=c11 -g3 -O0 -DDEBUG src/chip8.c -o build/debug/chip8
	chmod +x build/debug/chip8
