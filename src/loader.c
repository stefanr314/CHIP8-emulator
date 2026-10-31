#include "../include/loader.h"
#include <stdlib.h>

typedef enum {
  SUCCESS = 0,
  FILE_TOO_LARGE = 1,
  FILE_TOO_SMALL = 2,
  FILE_CORRUPTED = 3

} RomValidationStatus;

static inline int is_regular_file(mode_t mode) { return S_ISREG(mode); }

static inline int chip8_file_validation(off_t fileSize) {
  RomValidationStatus status = SUCCESS;

  if (fileSize > MAX_ROM_SIZE) {
    status = FILE_TOO_LARGE;
  }

  if (fileSize < 2) {
    status = FILE_TOO_SMALL;
  }

  if (fileSize % 2 != 0) {
    status = FILE_CORRUPTED;
  }

  return status;
}

static void print_hex_dump(void *memory, int bytes);

int load_rom_file(char *restrict path, uint8_t *restrict memory) {
  int return_value = EXIT_SUCCESS;
  STAT_STRUCT stat;
  FILE *fp = fopen(path, "rb");

  if (!fp) {
    (void)fprintf(stderr, "File could not be opened. Check the location of file"
                          " and try again.\n");
    return EXIT_FAILURE;
  }

  int fd = FILENO(fp);
  if (FSTAT(fd, &stat) != 0) {
    // error occured
    return_value = -1;
    goto FILE_ERROR;
  }

  if (!is_regular_file(stat.st_mode)) {
    (void)fprintf(stderr, "The opened file is not the regular file.\n");
    return_value = -1;
    goto FILE_ERROR;
  }

  RomValidationStatus status = chip8_file_validation(stat.st_size);

  switch (status) {
  case SUCCESS:
    printf("U have opened game file with name: %s.\n", path);
    break;
  case FILE_TOO_LARGE:

    fprintf(stderr, "Size of loaded file expands the ROM capacity of original "
                    "CHIP8 program.\n");
    return_value = -1;
    goto FILE_ERROR;
    break;
  case FILE_TOO_SMALL:
    fprintf(stderr, "Loaded file is too small.\n");
    return_value = -1;
    goto FILE_ERROR;
    break;
  case FILE_CORRUPTED:
    fprintf(stderr, "Loaded file is corrupted.\n");
    return_value = -1;
    goto FILE_ERROR;
    break;
  default:
    fprintf(stderr, "Unexpected value when loading the rom file.\n");
  }

  (void)rewind(fp);
  size_t bytes_read =
      fread((void *)memory, (size_t)stat.st_size, (size_t)1, fp);

  if (bytes_read < (size_t)1) {
    if (feof(fp)) {
      fprintf(stderr, "EOF reached before reading the hole ROM file.\n");
    } else if (ferror(fp)) {
      fprintf(stderr, "Error upon reding the ROM file.\n");
    }

    return_value = -1;
    goto FILE_ERROR;
  }

  print_hex_dump((void *)memory, (int)stat.st_size);

FILE_ERROR:
  fclose(fp);
  return return_value;
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
