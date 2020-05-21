#include"Cryptions.h"
#include"fConfig.h"
#include"aes.h"
//aes mode: 1, 128  2,192  3,256
void Encrypt(crypt_buffer *cb , char *pwd, int mode) {
	//printf("en pwd %s\n", pwd);
	int size = cb->size;
	char *tmp = (char *)malloc(size);
	memcpy(tmp, cb->buff,size);
	int len = 0;
	if (mode >= 1&&mode<=3) {
		switch (mode){
		case 1:
			len = 16;
			break;
		case 2:
			len = 24;
			break;
		case 3:
			len = 32;
			break;
		default:
			break;
		}
		DWORD outlen;
		unsigned char *out_data= (unsigned char*)malloc((size/16+1)*16);
		UCHAR *key = (char*)malloc(len + 1);
		memcpy(key, pwd, len+1);
		key[len + 1] = 0;
		InitializePrivateKey(len, key);
		//printf("en key len:%d  key: %s\nkey hex: ", len, key);
		//phex(key);
		outlen = AesEncrypt(tmp, size, out_data);
		cb->size = outlen;
		memcpy(cb->buff, out_data, outlen);
	}
}
void Decrypt(crypt_buffer *cb, char *pwd, int mode) {
	//printf("de pwd %s\n", pwd);
	
	int size = cb->size;
	char *tmp = (char *)malloc(size);
	memcpy(tmp, cb->buff, size);
	int len = 0;
	if (mode >= 1 && mode <= 3) {
		switch (mode) {
		case 1:
			len = 16;
			break;
		case 2:
			len = 24;
			break;
		case 3:
			len = 32;
			break;
		default:
			break;
		}
		DWORD outlen;
		unsigned char *out_data = (unsigned char*)malloc((size / 16 + 1) * 16);
		UCHAR *key = (char*)malloc(len + 1);
		memcpy(key, pwd, len + 1);
		key[len + 1] = 0;
		InitializePrivateKey(len, key);
		//printf("de key len:%d  key: %s\nkey hex: ", len,key);
		//phex(key);
		outlen = AesDecrypt(tmp, size, out_data);
		//phex(out_data);
		cb->size = outlen;
		memcpy(cb->buff, out_data, outlen);
	}
}