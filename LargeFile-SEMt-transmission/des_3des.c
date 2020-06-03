#pragma once
#include"des_3des.h"
int des_encrypt(const unsigned char* Key, const unsigned char KeyLen, const unsigned char* PlainContent, unsigned char* CipherContent, const int BlockCount){
	des_ctx dc;
	unsigned char  keyEn[8];
	unsigned char  keyDe[8];
	if ((BlockCount == 0) || (Key == NULL) || (CipherContent == NULL) || (PlainContent == NULL)){
		return -1;
	}
	if ((KeyLen != 8) && (KeyLen != 16)){
		return -1;
	}
	memcpy(CipherContent, PlainContent, BlockCount * 8);
	switch (KeyLen){
	case 8:    // DES
		memcpy(keyEn, Key, 8);               //keyEn
		des_key(&dc, keyEn);
		des_enc(&dc, CipherContent, BlockCount);
		break;
	case 16:    // 3DES
		memcpy(keyEn, Key, 8);               //keyEn
		memcpy(keyDe, Key + 8, 8);      //keyDe
		des_key(&dc, keyEn);
		des_enc(&dc, CipherContent, BlockCount);
		des_key(&dc, keyDe);
		des_dec(&dc, CipherContent, BlockCount);
		des_key(&dc, keyEn);
		des_enc(&dc, CipherContent, BlockCount);
		break;
	default:
		return -1;
	}
	return 0;
}

int des_decrypt(const unsigned char* Key, const unsigned char KeyLen, const unsigned char* CipherContent, unsigned char* PlainContent, const int BlockCount){
	des_ctx dc;
	unsigned char keyEn[8];
	unsigned char keyDe[8];
	if ((BlockCount == 0) || (Key == NULL) || (CipherContent == NULL) || (PlainContent == NULL)){
		return -1;
	}
	if ((KeyLen != 8) && (KeyLen != 16)){
		return -1;
	}
	memcpy(PlainContent, CipherContent, BlockCount * 8);
	switch (KeyLen){
	case 8:    // DES
		memcpy(keyEn, Key, 8);               //keyEn
		des_key(&dc, keyEn);
		des_dec(&dc, PlainContent, BlockCount);
		break;
	case 16:
		memcpy(keyEn, Key, 8);               //keyEn
		memcpy(keyDe, Key + 8, 8);        //keyDe
		des_key(&dc, keyEn);
		des_dec(&dc, PlainContent, BlockCount);
		des_key(&dc, keyDe);
		des_enc(&dc, PlainContent, BlockCount);
		des_key(&dc, keyEn);
		des_dec(&dc, PlainContent, BlockCount);
		break;
	default:
		return -1;
	}
	return 0;
}