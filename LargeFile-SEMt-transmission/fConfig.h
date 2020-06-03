#pragma once
#ifndef _FCONFIG_H
#define _FCONFIG_H
#include <zmq.h>
#include <pthread.h>
#define FILE_FRAME_SIZE 10240
#define NAMELENTH 20
#define MAXTHREAD 8
#define MAX_FILENAME 255
#define MARK_SIZE 5 //д���ļ�λ����ռ�ֽ�
#define CRYPT_MODES 5 //AES/DES/3DES/RC2/RC4
#pragma comment(lib,"pthreadVC2.lib")
typedef struct sFile {
	char mark;
	short threadNum;		//�߳���
	int spliteSize;			//��Ƭ��С��λBytes
	unsigned int filesize;	//�ļ���С
	short useCrypt;			//���ܷ�ʽ
	char sha256[65];
	char filename[128];		//�ļ���
	char filepath[128];
}sFile;
typedef struct packinfo {
	short isAllend;
	long index;
	int size;
}packInfo;
typedef struct filebuf {
	long index;
	short mark;
	int size;
	char splitEnd;
	char buff[FILE_FRAME_SIZE + 16];
}filebuf;
typedef struct argv_info {
	char work_mode;		//-r
	char ip[16];		//-l
	char port[6];		//-p
	int crypt_mode;		//-c
	int threadnum;		//-t
	int SplitSize;		//-z
	char passwd[33];	//-k
	char filename[128];	//-f
	char filepath[128];
}argv_info;
typedef struct crypt_buffer {
	int size;
	char buff[FILE_FRAME_SIZE + 16];
}crypt_buffer;
int getMem();
DWORD GetProcessorCoreCount();
int Str2Int(char * buff);
void split_ip_port(char *p);

void phex(uint8_t* str);
#endif