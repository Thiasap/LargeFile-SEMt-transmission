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
	if (mode >= 1&&mode<=5) {
		switch (mode){
		case 1:			//AES-128-cbc
			len = 16;
			break;
		case 2:			//AES-192-cbc
			len = 24;
			break;
		case 3:			//AES-256-cbc
			len = 32;
			break;
		case 4:			//DES
			len = 8;
			break;
		case 5:			//3DES
			len = 16;
			break;
		default:
			break;
		}
		unsigned char *out_data;
		UCHAR *key = (char*)malloc(len);
		memcpy(key, pwd, len);
		if (mode >= 1 && mode <= 3) {
			DWORD outlen;
			out_data = (unsigned char*)malloc((size / 16 + 1) * 16);
			InitializePrivateKey(len, key);
			outlen = AesEncrypt(tmp, size, out_data);
			cb->size = outlen;
			memcpy(cb->buff, out_data, outlen);
		}
		else if (mode <= 5) {
			out_data = (unsigned char*)malloc(size);
			des_encrypt(key, len, tmp, out_data, (size + 8 - 1) / 8);
			memcpy(cb->buff, out_data, size);
		}
		free(tmp);
		free(out_data);
		free(key);
		return;
	}

}
void Decrypt(crypt_buffer *cb, char *pwd, int mode) {
	int size = cb->size;
	char *tmp = (char *)malloc(size);
	memcpy(tmp, cb->buff, size);
	int len = 0;
	if (mode >= 1 && mode <= 5) {
		switch (mode) {
		case 1:			//AES-128-cbc
			len = 16;
			break;
		case 2:			//AES-192-cbc
			len = 24;
			break;
		case 3:			//AES-256-cbc
			len = 32;
			break;
		case 4:			//DES
			len = 8;
			break;
		case 5:			//3DES
			len = 16;
			break;
		default:
			break;
		}
		unsigned char *out_data;
		UCHAR *key = (char*)malloc(len);
		memcpy(key, pwd, len);
		if (mode >= 1 && mode <= 3) {
			DWORD outlen;
			out_data = (unsigned char*)malloc((size / 16 + 1) * 16);
			InitializePrivateKey(len, key);
			outlen = AesDecrypt(tmp, size, out_data);
			cb->size = outlen;
			memcpy(cb->buff, out_data, outlen);
		}else if (mode <= 5) {
			out_data = (unsigned char*)malloc(size);
			des_decrypt(key, len, tmp, out_data, (size+8-1) / 8);
			memcpy(cb->buff, out_data, size);
		}
		free(tmp);
		free(out_data);
		free(key);
		return;
	}
}