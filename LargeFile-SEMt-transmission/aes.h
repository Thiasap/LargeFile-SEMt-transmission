
#ifndef __AES_H
#define __AES_H	 

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include<memory.h>
#define AESKEY	"71412E2299B0EEA5"
#define AESIV	"71412E2299B0EEA5"

typedef unsigned long DWORD;
typedef unsigned char UCHAR, *PUCHAR;
typedef void *PVOID, *LPVOID;
typedef unsigned char byte;
typedef DWORD *PDWORD, *LPDWORD;

#ifndef VOID
#define VOID void
#endif

//#pragma once

//enum KeySize { Bits128, Bits192, Bits256 };  // key size, in bits, for construtor
#define Bits128	16
#define Bits192	24
#define Bits256	32


void Aes_init(int keySize, unsigned char* keyBytes);

void Cipher(unsigned char* input, unsigned char* output);  // encipher 16-bit input
void InvCipher(unsigned char* input, unsigned char* output);  // decipher 16-bit input


void SetNbNkNr(int keySize);
void AddRoundKey(int round);      //轮密钥加
void SubBytes(void);                  //S盒字节代换
void InvSubBytes(void);               //逆S盒字节代换
void ShiftRows(void);                 //行移位
void InvShiftRows(void);
void MixColumns(void);                //列混淆
void InvMixColumns(void);
unsigned char gfmultby01(unsigned char b);
unsigned char gfmultby02(unsigned char b);
unsigned char gfmultby03(unsigned char b);
unsigned char gfmultby09(unsigned char b);
unsigned char gfmultby0b(unsigned char b);
unsigned char gfmultby0d(unsigned char b);
unsigned char gfmultby0e(unsigned char b);
void KeyExpansion(void);              //密钥扩展
unsigned char* SubWord(unsigned char* word);         //密钥S盒字代换
unsigned char* RotWord(unsigned char* word);         //密钥移位


void test_func(void);
void InitializePrivateKey(DWORD KeySize, UCHAR *KeyBytes);                       //AES 密钥初始化
DWORD AesEncrypt(LPVOID InBuffer, DWORD InLength, LPVOID OutBuffer);			//AES 加密数据
DWORD AesDecrypt(LPVOID InBuffer, DWORD InLength, LPVOID OutBuffer);			//AES 解密数据

#endif