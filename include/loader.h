#ifndef LOADER_H
#define LOADER_H 1
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef MEM_CAP

#define MEM_CAP 4096

#endif /* ifndef MEM_CAP                                                       \
#define MEM_CAP 4096                                                           \
#endif */
#ifndef MAX_ROM_SIZE

#define MAX_ROM_SIZE (MEM_CAP - 512)

#endif /* ifndef MAX_ROM_SIZE                                                    \
                                                                               \ \
#define MAX_ROM_SIZE (MEM_CAP - 512) */

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
extern int load_rom_file(char *__restrict path, uint8_t *__restrict memory);

#endif
