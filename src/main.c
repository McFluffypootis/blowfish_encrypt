#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint32_t P[18];
uint32_t S[4][256];

void swap(uint32_t *a, uint32_t *b) {
  uint32_t t = *a;
  *a = *b;
  *b = t;
}

uint32_t f(uint32_t x) {
  uint8_t hb = (x >> 24);
  uint8_t sb = (x >> 16);
  uint8_t tb = (x >> 8);
  uint8_t lb = x & 0xff;

  uint32_t h = S[0][hb] + S[1][sb];
  return (h ^ S[2][tb]) + S[3][lb];
}

void blowfish_encrypt(uint32_t *L, uint32_t *R) {
  for (int round = 0; round < 16; round++) {
    *L = *L ^ P[round];
    *R = f(*L) ^ *R;
    swap(L, R);
  }
  swap(L, R);
  *R = *R ^ P[16];
  *L = *L ^ P[17];
}

void blowfish_decrypt(uint32_t *L, uint32_t *R) {
  for (int round = 17; round > 1; round--) {
    *L = *L ^ P[round];
    *R = f(*L) ^ *R;
    swap(L, R);
  }
  swap(L, R);
  *R = *R ^ P[1];
  *L = *L ^ P[0];
}

FILE *fp;
FILE *fp_out;

char *file_buffer;
long file_len = 0;

const char outfmt[] = ".blfsh";

void blowfish_init(char key[]) {
  
  // initalize S & P with some defaut values (idealy with digits of PI)
  for (int i = 0; i < 18; i++) {
    P[i] = i;
  }

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 256; j++) {
      S[i][j] = j;
    }
  }

  int key_position = 0;
  int key_length = 576;

  int i = 0;
  for (i = 0; i < 18; i++) {
    int k = 0;
    for (int j = 0; j < 18; j++) {
      k = (k << 8) | key[key_position];
      key_position = (key_position + 1) % key_length;
    }
    P[i] = P[i] ^ k;
  }

  uint32_t L = 0;
  uint32_t R = 0;
  for (i = 0; i < 17; i += 2) {
    blowfish_encrypt(&L, &R);
    P[i] = L;
    P[i + 1] = R;
  }

  for (i = 0; i < 4; i++) {
    for (int j = 0; j < 256; j += 2) {
      blowfish_encrypt(&L, &R);
      S[i][j] = L;
      S[i][j + 1] = R;
    }
  }
}

void get_chunk(char buff[], uint32_t *L, uint32_t *R) {}

int main(int argc, char *argv[]) {

  enum { MODE_ENCRYPT, MODE_DECRYPT } mode = MODE_ENCRYPT;

  char key[576];
  for(int i = 0; i < 576; i++) {
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
  printf("\n P %d", P[0]);
  printf("S %d \n", S[0][0]);

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
      printf("L: %x, R: %x \n", L, R);
      blowfish_encrypt(&L, &R);
      printf("ENC L: %x, R: %x \n", L, R);

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
      printf("L: %x, R: %x \n", L, R);
      blowfish_decrypt(&L, &R);
      printf("DEC L: %x, R: %x \n", L, R);
      *(uint32_t *)(file_buffer + i) = L;
      *(uint32_t *)(file_buffer + i + 4) = R;
      i += 8;
    }
  }

  long i;
  for (i = 0; i < file_len; i += 8) {
    printf("%hhx ", file_buffer[i]);
    printf("%hhx ", file_buffer[i + 1]);
    printf("%hhx ", file_buffer[i + 2]);
    printf("%hhx ", file_buffer[i + 3]);
    printf("%hhx ", file_buffer[i + 4]);
    printf("%hhx ", file_buffer[i + 5]);
    printf("%hhx ", file_buffer[i + 6]);
    printf("%hhx  \n", file_buffer[i + 7]);
  }
  fwrite(file_buffer, sizeof(char), file_len, fp_out);
  fclose(fp_out);
  free(file_buffer);

  return 0;
}
