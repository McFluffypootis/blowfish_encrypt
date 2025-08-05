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

void blowfish_init(char key[]) {
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

int main(int argc, char *argv[]) {

  enum { MODE_ENCRYPT, MODE_DECRYPT } mode = MODE_ENCRYPT;

  char key[576];

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

  if (optind < argc) {
    printf("optind %s \n", argv[optind]);

    if ((fp = fopen(argv[optind], "r")) == NULL) {
      fprintf(stderr, "%s: failed to open %s (%d %s)\n", argv[0], argv[optind],
              errno, strerror(errno));
    }
  }

  blowfish_init(key);

  char chunk[10];

  uint32_t L;
  uint32_t R;

  if (mode == MODE_ENCRYPT) {
    while (fgets(chunk, 10, fp)) {
      L = *(uint32_t *)chunk;
      R = *(uint32_t *)(chunk + 4);
      printf("L: %x R: %x \n", L, R);
      blowfish_encrypt(&L, &R);
    }
  } else if (mode == MODE_DECRYPT) {
    while (fgets(chunk, 10, fp)) {
      L = *(uint32_t *)chunk;
      R = *(uint32_t *)(chunk + 4);
      printf("L: %x R: %x \n", L, R);
      blowfish_decrypt(&L, &R);
    }
  }

  return 0;
}
