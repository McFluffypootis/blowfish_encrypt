#ifndef BLFSH_H
#define BLFSH_H

#include <stdint.h>
#include <stdio.h>

void blowfish_init(char key[]);

void blowfish_encrypt(uint32_t *L, uint32_t *R);

void blowfish_decrypt(uint32_t *L, uint32_t *R);

#endif
