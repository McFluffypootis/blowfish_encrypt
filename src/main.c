#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "blfsh.h"

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
    fprintf(stderr,
            "Usage %s -o [output file...] -[ed] [input file] [-k] [key]\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  int opt;
  while ((opt = getopt(argc, argv, "oedk:")) != -1) {
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
    case 'o':
      break;
    default:
      fprintf(stderr,
              "Usage %s -o [output file...] -[ed] [input file] [-k] [key]\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  // open file
  if (optind < argc) {

    if ((fp = fopen(argv[optind + 1], "rb")) == NULL) {
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
  if ((fp_out = fopen(argv[optind], "w")) == NULL) {
    fprintf(stderr, "%s: failed to create output file %s (%d %s)\n", argv[0],
            argv[optind], errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (mode == MODE_ENCRYPT) {
    blowfish_encrypt(file_buffer, file_len);

  } else if (mode == MODE_DECRYPT) {

    blowfish_decrypt(file_buffer, file_len);
  }

  fwrite(file_buffer, sizeof(char), file_len, fp_out);
  fclose(fp_out);
  free(file_buffer);

  return 0;
}
