#include <stdint.h>

static uint32_t P[18];
static uint32_t S[4][256];

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

void blowfish_encrypt_chunk(uint32_t *L, uint32_t *R) {
  for (int round = 0; round < 16; round++) {
    *L = *L ^ P[round];
    *R = f(*L) ^ *R;
    swap(L, R);
  }
  swap(L, R);
  *R = *R ^ P[16];
  *L = *L ^ P[17];
}

void blowfish_decrypt_chunk(uint32_t *L, uint32_t *R) {
  for (int round = 17; round > 1; round--) {
    *L = *L ^ P[round];
    *R = f(*L) ^ *R;
    swap(L, R);
  }
  swap(L, R);
  *R = *R ^ P[1];
  *L = *L ^ P[0];
}

void blowfish_init(char key[]) {

  // initalize S & P with some defaut values (ideally with digits of PI)
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
    blowfish_encrypt_chunk(&L, &R);
    P[i] = L;
    P[i + 1] = R;
  }

  for (i = 0; i < 4; i++) {
    for (int j = 0; j < 256; j += 2) {
      blowfish_encrypt_chunk(&L, &R);
      S[i][j] = L;
      S[i][j + 1] = R;
    }
  }
}

void blowfish_encrypt(char* file_buffer, long file_len) {
  uint32_t L = 0;
  uint32_t R = 0;
  long i = 0;
  while (i < file_len) {
    L = *(uint32_t *)(file_buffer + i);
    R = *(uint32_t *)(file_buffer + 4 + i);
    blowfish_encrypt_chunk(&L, &R);
    *(uint32_t *)(file_buffer + i) = L;
    *(uint32_t *)(file_buffer + i + 4) = R;
    i += 8;
  }
}

void blowfish_decrypt(char* file_buffer, long file_len) {
  uint32_t L = 0;
  uint32_t R = 0;
  long i = 0;
  while (i < file_len) {
    L = *(uint32_t *)(file_buffer + i);
    R = *(uint32_t *)(file_buffer + 4 + i);

    blowfish_decrypt_chunk(&L, &R);

    *(uint32_t *)(file_buffer + i) = L;
    *(uint32_t *)(file_buffer + i + 4) = R;
    i += 8;
  }
}
