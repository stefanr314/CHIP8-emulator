#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

int load_rom_file(char *restrict filename, uint8_t *restrict memory);

static inline int is_regular_file(mode_t mode) { return S_ISREG(mode); }

/* fprintf statements should be moved from inline functions*/
static inline int is_valid_chip8_file(off_t fileSize) {
  if (fileSize > MAX_ROM_SIZE) {
    fprintf(stderr, "Size of loaded file expands the ROM capacity of original "
                    "CHIP8 program.\n");
    return 0;
  }

  if (fileSize < 2) {
    fprintf(stderr, "Loaded file is too small.\n");
    return 0;
  }

  if (fileSize % 2 != 0) {
    fprintf(stderr, "Loaded file is corrupted.\n");
    return 0;
  }

  return 1;
}

void print_hex_dump(void *memory, int bytes);

int main(int argc, char *argv[]) {
  uint8_t memory[MEM_CAP]; // to be refactored

  if (argc < 2) {
    (void)fprintf(stderr,
                  "Please provide the name of the rom file to be loaded. "
                  "The format is as follows: <exec_file> <rom_filename>.\n");
    return EXIT_FAILURE;
  }

  if (argc != 2) {
    (void)fprintf(stderr,
                  "Please provide only single rom file to be loaded.\n");
    return EXIT_FAILURE;
  }

  char *filename = argv[1];
  int notLoaded = load_rom_file(filename, &memory[0x200]);
  if (notLoaded) {
    return EXIT_FAILURE;
  }

  printf("\nLets play some games, shall we...\n");

  return EXIT_SUCCESS;
}

int load_rom_file(char *restrict filename, uint8_t *restrict memory) {
  int returnValue = EXIT_SUCCESS;
  STAT_STRUCT stat;
  FILE *fp = fopen(filename, "rb");

  if (!fp) {
    (void)fprintf(stderr, "File could not be opened. Check the location of file"
                          " and try again.\n");
    return EXIT_FAILURE;
  }

  int fd = FILENO(fp);
  if (fstat(fd, &stat) != 0) {
    // error occured
    returnValue = -1;
    goto FILE_ERROR;
  }

  if (!is_regular_file(stat.st_mode)) {
    (void)fprintf(stderr, "The opened file is not the regular file.\n");
    returnValue = -1;
    goto FILE_ERROR;
  }

  if (!is_valid_chip8_file(stat.st_size)) {
    returnValue = -1;
    goto FILE_ERROR;
  }

  printf("U have opened game file with name: %s.\n", filename);

  (void)rewind(fp);
  size_t bytesRead = fread((void *)memory, (size_t)stat.st_size, (size_t)1, fp);

  if (bytesRead < (size_t)1) {
    if (feof(fp)) {
      fprintf(stderr, "EOF reached before reading the hole ROM file.\n");
    } else if (ferror(fp)) {
      fprintf(stderr, "Error upon reding the ROM file.\n");
    }

    returnValue = -1;
    goto FILE_ERROR;
  }

  print_hex_dump((void *)memory, (int)stat.st_size);

FILE_ERROR:
  fclose(fp);
  return returnValue;
}

void print_hex_dump(void *memory, int bytes) {
  unsigned char *mem = (unsigned char *)memory;
  for (int i = 0; i < bytes; i += 2) {
    if (i != 0 && i % 16 == 0) {
      printf("\n");
    }
    printf("%.2x%.2x ", mem[i], mem[i + 1]);
  }
}
