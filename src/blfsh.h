#ifndef BLFSH_H
#define BLFSH_H

#include <stdint.h>
#include <stdio.h>

void blowfish_init(char key[]);

void blowfish_encrypt_chunk(uint32_t *L, uint32_t *R);

void blowfish_decrypt_chunk(uint32_t *L, uint32_t *R);

void blowfish_encrypt(char *file_buffer, long file_len);

void blowfish_decrypt(char *file_buffer, long file_len);

#endif
