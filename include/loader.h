#ifndef LOADER_H
#define LOADER_H 1
#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32)
#include <io.h>
#define FSTAT _fstat
#define FILENO _fileno
#define STAT_STRUCT struct _stat
#ifndef S_ISREG

#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif /* ifndef S_ISREG                                                       \
        */
#elif defined(__unix__) || defined(__linux__) ||                               \
    (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#define FSTAT fstat
#define FILENO fileno
#define STAT_STRUCT struct stat
#else
#error No operating system specified
#endif /* if defined (_WIN32)*/

/* Function for loading a rom file from location on path into the memory pointed
 * by pointer declared as second argument. Size read by the successful load is
 * stored in read_size pointer argument. Maximal allowed value for rom size is
 * written as 4th argument; since this value is much significant than int type
 * can store size_t is not used.
 * Returns the status of perfoming load. */
extern int load_rom_file(const char *__restrict path,
                         uint8_t *__restrict memory,
                         size_t *__restrict read_size, int max_rom_size);

#endif
