#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "blfsh.h"

const char outfmt[] = ".blfsh";

FILE *fp;
FILE *fp_out;

char *file_buffer;
long file_len = 0;

int main(int argc, char *argv[]) {

  enum { MODE_ENCRYPT, MODE_DECRYPT } mode = MODE_ENCRYPT;

  char key[576];
  for (int i = 0; i < 576; i++) {
    key[i] = i;
  }

  if (argc <= 1) {
    fprintf(stderr, "Usage %s [-de] [file...] [-k] [key...]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int opt;
  while ((opt = getopt(argc, argv, "edk:")) != -1) {
    switch (opt) {
    case 'd':
      mode = MODE_DECRYPT;
      break;
    case 'e':
      mode = MODE_ENCRYPT;
      break;
    case 'k':
      strcpy(key, optarg);
      break;
    case 'p':
      break;
    default:
      fprintf(stderr, "Usage %s [-de] [file...] [-k] [key...]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  // open file
  if (optind < argc) {
    printf("optind %s \n", argv[optind]);

    if ((fp = fopen(argv[optind], "rb")) == NULL) {
      fprintf(stderr, "%s: failed to open %s (%d %s)\n", argv[0], argv[optind],
              errno, strerror(errno));
      exit(EXIT_FAILURE);
    }

    // get file length
    fseek(fp, 0, SEEK_END);
    file_len = ftell(fp);
    rewind(fp);

    // ensure even chunkiness
    if (file_len % 16 != 0) {
      file_len = file_len + (16 - file_len % 16);
    }

    // create file buffer
    file_buffer = (char *)malloc(file_len * sizeof(char));
    fread(file_buffer, 1, file_len, fp);
    fclose(fp);
  }

  blowfish_init(key);

  uint32_t L = 0;
  uint32_t R = 0;

  if (mode == MODE_ENCRYPT) {
    if ((fp_out = fopen(strcat(argv[optind], outfmt), "a")) == NULL) {
      fprintf(stderr, "%s: failed to create output file %s (%d %s)\n", argv[0],
              argv[optind], errno, strerror(errno));
      exit(EXIT_FAILURE);
    }

    long i = 0;
    while (i < file_len) {
      L = *(uint32_t *)(file_buffer + i);
      R = *(uint32_t *)(file_buffer + 4 + i);

      blowfish_encrypt(&L, &R);

      *(uint32_t *)(file_buffer + i) = L;
      *(uint32_t *)(file_buffer + i + 4) = R;
      i += 8;
    }
  } else if (mode == MODE_DECRYPT) {
    long i = 0;
    if ((fp_out = fopen(strcat(argv[optind], outfmt), "a")) == NULL) {
      fprintf(stderr, "%s: failed to create output file %s (%d %s)\n", argv[0],
              argv[optind], errno, strerror(errno));
      exit(EXIT_FAILURE);
    }
    while (i < file_len) {
      L = *(uint32_t *)(file_buffer + i);
      R = *(uint32_t *)(file_buffer + 4 + i);
      blowfish_decrypt(&L, &R);
      *(uint32_t *)(file_buffer + i) = L;
      *(uint32_t *)(file_buffer + i + 4) = R;
      i += 8;
    }
  }

  fwrite(file_buffer, sizeof(char), file_len, fp_out);
  fclose(fp_out);
  free(file_buffer);

  return 0;
}
