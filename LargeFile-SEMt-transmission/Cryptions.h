#pragma once
#ifndef _CRYPTIONS_H
#define _CRYPTIONS_H
#include"fConfig.h"
#include"sha256.h"
void Encrypt(crypt_buffer *cb, char *pwd, int mode);
void Decrypt(crypt_buffer *cb, char *pwd, int mode);
#endif