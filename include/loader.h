#ifndef LOADER_H
#define LOADER_H 1
#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef CHIP8_RAM_SIZE

#define CHIP8_RAM_SIZE 4096

#endif /* ifndef CHIP8_RAM_SIZE                                                       \
#define CHIP8_RAM_SIZE 4096                                                           \
#endif */
#ifndef MAX_ROM_SIZE

#define MAX_ROM_SIZE (CHIP8_RAM_SIZE - 512)

#endif /* ifndef MAX_ROM_SIZE                                                    \
                                                                               \ \
#define MAX_ROM_SIZE (CHIP8_RAM_SIZE - 512) */

#if defined(_WIN32)
#include <io.h>
#define FSTAT _fstat
#define FILENO _fileno
#define STAT_STRUCT struct _stat
#ifndef S_ISREG

#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif /* ifndef S_ISREG                                                       \
        */
#elif defined(__unix__)
#include <unistd.h>
#define FSTAT fstat
#define FILENO fileno
#define STAT_STRUCT struct stat
#else
#error No operating system specified
#endif /* if defined (_WIN32)*/

/* Function for loading a rom file from location on path into the memory pointed
 * by pointer declared as second argument */
extern int load_rom_file(const char *__restrict path, uint8_t *__restrict memory,
                         size_t *__restrict read_size);

#endif
